// Lazy embedded-Python data-source support for the mapping plugin.
//
// Built only when MAPPING_PLUGIN_PYTHON is defined (requires Python3 + NumPy
// headers). See python_data_source.hpp for the rationale and lifetime model.

#include "python_data_source.hpp"

#ifdef MAPPING_PLUGIN_PYTHON

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>

// Pull libpython into the global symbol namespace BEFORE Py_Initialize:
// UDA dlopens this plugin RTLD_LOCAL, so without this NumPy's C extensions
// cannot resolve Python symbols at import time.
#include <dlfcn.h>

#include <numpy/arrayobject.h>

#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <toml.hpp>

namespace mapping_plugin
{
namespace
{

// ---------------------------------------------------------------------------
// Python error reporting
// ---------------------------------------------------------------------------

std::string fetch_python_error()
{
    if (PyErr_Occurred() == nullptr) {
        return {};
    }
    PyObject *ptype = nullptr, *pvalue = nullptr, *ptraceback = nullptr;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
    std::string message{"unknown Python error"};
    if (pvalue != nullptr) {
        PyObject* str = PyObject_Str(pvalue);
        if (str != nullptr) {
            const char* text = PyUnicode_AsUTF8(str);
            if (text != nullptr) {
                message = text;
            }
            Py_DECREF(str);
        }
    }
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
    return message;
}

// ---------------------------------------------------------------------------
// TypedDataArray -> NumPy (for custom-function inputs). Copying variant.
// ---------------------------------------------------------------------------

void free_wrapped_memory(void* data)
{
    std::free(data);
}

template <typename T, int NPY_TYPE>
PyObject* wrap_array_copy(const libtokamap::TypedDataArray& data)
{
    const auto& shape = data.shape();
    int ndim = static_cast<int>(shape.size());
    std::vector<npy_intp> dims(shape.begin(), shape.end());

    void* copied = std::malloc(data.size() * sizeof(T));
    std::memcpy(copied, data.data<T>(), data.size() * sizeof(T));

    PyArray_Descr* descr = PyArray_DescrFromType(NPY_TYPE);
    PyObject* array = PyArray_NewFromDescr(&PyArray_Type, descr, ndim, dims.data(), nullptr, copied,
                                           NPY_ARRAY_CARRAY, nullptr);
    if (array == nullptr) {
        std::free(copied);
        return nullptr;
    }
    PyObject* capsule = PyCapsule_New(copied, nullptr, [](PyObject* cap) { std::free(PyCapsule_GetPointer(cap, nullptr)); });
    if (capsule == nullptr) {
        Py_DECREF(array);
        return nullptr;
    }
    PyArray_SetBaseObject(reinterpret_cast<PyArrayObject*>(array), capsule);
    return array;
}

PyObject* typed_data_array_to_numpy(const libtokamap::TypedDataArray& data)
{
    using libtokamap::DataType;
    switch (data.data_type()) {
        case DataType::Double:
            return wrap_array_copy<double, NPY_FLOAT64>(data);
        case DataType::Float:
            return wrap_array_copy<float, NPY_FLOAT32>(data);
        case DataType::Int64:
            return wrap_array_copy<int64_t, NPY_INT64>(data);
        case DataType::Int32:
            return wrap_array_copy<int32_t, NPY_INT32>(data);
        case DataType::Int16:
            return wrap_array_copy<int16_t, NPY_INT16>(data);
        case DataType::Int8:
            return wrap_array_copy<int8_t, NPY_INT8>(data);
        case DataType::UInt64:
            return wrap_array_copy<uint64_t, NPY_UINT64>(data);
        case DataType::UInt32:
            return wrap_array_copy<uint32_t, NPY_UINT32>(data);
        case DataType::UInt16:
            return wrap_array_copy<uint16_t, NPY_UINT16>(data);
        case DataType::UInt8:
            return wrap_array_copy<uint8_t, NPY_UINT8>(data);
        default:
            Py_INCREF(Py_None);
            return Py_None;
    }
}

// ---------------------------------------------------------------------------
// Embedded interpreter lifecycle — lazy, once per uda_server process
// ---------------------------------------------------------------------------

std::once_flag g_python_once;

void ensure_python_interpreter()
{
    std::call_once(g_python_once, [] {
        if (Py_IsInitialized() == 0) {
            // Default the libpython path from the compile-time Python, but
            // allow the deployment to override it.
            const char* libpython = std::getenv("MAPPING_PLUGIN_LIBPYTHON");
            if (libpython == nullptr || *libpython == '\0') {
                libpython = MAPPING_PLUGIN_DEFAULT_LIBPYTHON;
            }
            if (libpython != nullptr && *libpython != '\0') {
                if (dlopen(libpython, RTLD_NOW | RTLD_GLOBAL) == nullptr) {
                    throw std::runtime_error{std::string{"failed to load libpython: "} + dlerror()};
                }
            }
            Py_Initialize();
            if (Py_IsInitialized() == 0) {
                throw std::runtime_error{"Py_Initialize failed"};
            }

            // An embedded interpreter sees neither the caller's PYTHONPATH
            // nor the venv paths, so fold both into sys.path explicitly.
            {
                PyObject* sys_path = PySys_GetObject("path"); // borrowed ref
                const char* pythonpath = std::getenv("PYTHONPATH");
                if (pythonpath != nullptr && *pythonpath != '\0') {
                    std::string paths{pythonpath};
                    size_t pos = 0;
                    while (pos <= paths.size()) {
                        size_t sep = paths.find(':', pos);
                        std::string entry = paths.substr(pos, sep == std::string::npos ? sep : sep - pos);
                        if (!entry.empty()) {
                            PyObject* pyentry = PyUnicode_FromString(entry.c_str());
                            PyList_Insert(sys_path, 0, pyentry);
                            Py_DECREF(pyentry);
                        }
                        if (sep == std::string::npos) {
                            break;
                        }
                        pos = sep + 1;
                    }
                }
                const char* extra = std::getenv("MAPPING_PLUGIN_PYTHONPATH");
                if (extra != nullptr && *extra != '\0') {
                    PyObject* pyextra = PyUnicode_FromString(extra);
                    PyList_Insert(sys_path, 0, pyextra);
                    Py_DECREF(pyextra);
                }
            }
        }
        if (_import_array() != 0) {
            throw std::runtime_error{"NumPy C API initialisation failed: " + fetch_python_error()};
        }
    });
}

// ---------------------------------------------------------------------------
// Python/C++ value conversions (mirrors clibtokamap)
// ---------------------------------------------------------------------------

PyObject* json_to_pyobject(const nlohmann::json& value)
{
    if (value.is_object()) {
        PyObject* dict = PyDict_New();
        for (auto& [key, sub] : value.items()) {
            PyObject* py_sub = json_to_pyobject(sub);
            PyDict_SetItemString(dict, key.c_str(), py_sub);
            Py_DECREF(py_sub);
        }
        return dict;
    }
    if (value.is_array()) {
        PyObject* list = PyList_New(static_cast<Py_ssize_t>(value.size()));
        for (size_t i = 0; i < value.size(); ++i) {
            PyList_SET_ITEM(list, static_cast<Py_ssize_t>(i), json_to_pyobject(value[i]));
        }
        return list;
    }
    if (value.is_string()) {
        return PyUnicode_FromString(value.get<std::string>().c_str());
    }
    if (value.is_boolean()) {
        return PyBool_FromLong(value.get<bool>() ? 1 : 0);
    }
    if (value.is_number_integer()) {
        return PyLong_FromLongLong(value.get<long long>());
    }
    if (value.is_number_float()) {
        return PyFloat_FromDouble(value.get<double>());
    }
    if (value.is_null()) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(value.dump().c_str());
}

bool numpy_to_typed_data_array(PyObject* object, libtokamap::TypedDataArray& out, std::string& error)
{
    if (PyUnicode_Check(object)) {
        const char* text = PyUnicode_AsUTF8(object);
        if (text == nullptr) {
            error = "failed to convert Python str to UTF-8";
            return false;
        }
        out = libtokamap::TypedDataArray{std::string{text}};
        return true;
    }
    if (PyArray_Check(object) == 0) {
        error = "Python data source must return a NumPy array or str";
        return false;
    }
    auto* array = reinterpret_cast<PyArrayObject*>(object);
    if (PyArray_ISCARRAY(array) == 0) {
        error = "Python data source returned a non-C-contiguous NumPy array";
        return false;
    }
    void* data = PyArray_DATA(array);
    auto size = static_cast<size_t>(PyArray_SIZE(array));
    int rank = PyArray_NDIM(array);
    npy_intp* shape = PyArray_DIMS(array);
    std::vector<size_t> shape_vec(shape, shape + rank);

    // The TypedDataArray(T*, size, shape) constructor copies the data, so the
    // NumPy array can be released as soon as this call returns.
    switch (PyArray_TYPE(array)) {
        case NPY_BOOL: {
            auto* bool_data = reinterpret_cast<npy_bool*>(data);
            std::vector<uint8_t> values(bool_data, bool_data + size);
            out = libtokamap::TypedDataArray{values, shape_vec};
            return true;
        }
        case NPY_INT8:
            out = libtokamap::TypedDataArray{reinterpret_cast<int8_t*>(data), size, shape_vec};
            return true;
        case NPY_INT16:
            out = libtokamap::TypedDataArray{reinterpret_cast<int16_t*>(data), size, shape_vec};
            return true;
        case NPY_INT32:
            out = libtokamap::TypedDataArray{reinterpret_cast<int32_t*>(data), size, shape_vec};
            return true;
        case NPY_INT64:
            out = libtokamap::TypedDataArray{reinterpret_cast<int64_t*>(data), size, shape_vec};
            return true;
        case NPY_UINT8:
            out = libtokamap::TypedDataArray{reinterpret_cast<uint8_t*>(data), size, shape_vec};
            return true;
        case NPY_UINT16:
            out = libtokamap::TypedDataArray{reinterpret_cast<uint16_t*>(data), size, shape_vec};
            return true;
        case NPY_UINT32:
            out = libtokamap::TypedDataArray{reinterpret_cast<uint32_t*>(data), size, shape_vec};
            return true;
        case NPY_UINT64:
            out = libtokamap::TypedDataArray{reinterpret_cast<uint64_t*>(data), size, shape_vec};
            return true;
        case NPY_FLOAT32:
            out = libtokamap::TypedDataArray{reinterpret_cast<float*>(data), size, shape_vec};
            return true;
        case NPY_FLOAT64:
            out = libtokamap::TypedDataArray{reinterpret_cast<double*>(data), size, shape_vec};
            return true;
        default:
            error = "Python data source returned data with unsupported dtype";
            return false;
    }
}

// ---------------------------------------------------------------------------
// PythonDataSource — a libtokamap::DataSource whose get() calls a Python
// object's .get(args) method. Mirrors libtokamap's own (clibtokamap) bridge.
// ---------------------------------------------------------------------------

class PythonDataSource final : public libtokamap::DataSource
{
  public:
    explicit PythonDataSource(PyObject* instance) : m_instance{instance} {}

    ~PythonDataSource() override
    {
        // uda_server exits after its connection so the interpreter is still up
        // in practice, but take the GIL defensively for the decref.
        if (Py_IsInitialized() != 0 && m_instance != nullptr) {
            PyGILState_STATE gil = PyGILState_Ensure();
            Py_DECREF(m_instance);
            PyGILState_Release(gil);
        }
    }

    PythonDataSource(PythonDataSource&&) = delete;
    PythonDataSource& operator=(PythonDataSource&&) = delete;
    PythonDataSource(const PythonDataSource&) = delete;
    PythonDataSource& operator=(const PythonDataSource&) = delete;

    libtokamap::TypedDataArray get(const libtokamap::DataSourceArgs& map_args,
                                   const libtokamap::MapArguments& /*arguments*/,
                                   libtokamap::RamCache* /*ram_cache*/) override
    {
        PyGILState_STATE gil = PyGILState_Ensure();

        PyObject* args_dict = PyDict_New();
        for (const auto& [key, value] : map_args) {
            PyObject* py_value = json_to_pyobject(value);
            PyDict_SetItemString(args_dict, key.c_str(), py_value);
            Py_DECREF(py_value);
        }

        PyObject* result = PyObject_CallMethod(m_instance, "get", "O", args_dict);
        Py_DECREF(args_dict);

        libtokamap::TypedDataArray array;
        std::string error;
        if (result == nullptr) {
            error = fetch_python_error();
        } else {
            numpy_to_typed_data_array(result, array, error);
            Py_DECREF(result);
        }
        PyGILState_Release(gil);

        if (!error.empty()) {
            throw libtokamap::DataSourceError{"python data source get failed: " + error};
        }
        return array;
    }

  private:
    PyObject* m_instance;
};

// ---------------------------------------------------------------------------
// PythonCustomFunction — wraps a Python callable as a libtokamap
// LibraryFunctionWrapper: call(inputs, params) -> function(inputs, params).
// ---------------------------------------------------------------------------

class PythonCustomFunction final : public libtokamap::LibraryFunctionWrapper
{
  public:
    explicit PythonCustomFunction(PyObject* function) : m_function{function} {}

    ~PythonCustomFunction() override
    {
        if (Py_IsInitialized() != 0 && m_function != nullptr) {
            PyGILState_STATE gil = PyGILState_Ensure();
            Py_DECREF(m_function);
            PyGILState_Release(gil);
        }
    }

    libtokamap::TypedDataArray operator()(libtokamap::CustomMappingInputs& inputs,
                                          const libtokamap::CustomMappingParams& params) const override
    {
        PyGILState_STATE gil = PyGILState_Ensure();

        PyObject* inputs_dict = PyDict_New();
        for (auto& [key, value] : inputs) {
            PyObject* py_array = typed_data_array_to_numpy(value);
            PyDict_SetItemString(inputs_dict, key.c_str(), py_array);
            Py_DECREF(py_array);
        }
        PyObject* params_dict = json_to_pyobject(params);
        PyObject* call_args = PyTuple_Pack(2, inputs_dict, params_dict);
        Py_DECREF(inputs_dict);
        Py_DECREF(params_dict);

        PyObject* result = PyObject_CallObject(m_function, call_args);
        Py_DECREF(call_args);

        libtokamap::TypedDataArray array;
        std::string error;
        if (result == nullptr) {
            error = fetch_python_error();
        } else {
            numpy_to_typed_data_array(result, array, error);
            Py_DECREF(result);
        }
        PyGILState_Release(gil);

        if (!error.empty()) {
            throw libtokamap::DataSourceError{"python custom function failed: " + error};
        }
        return array;
    }

  private:
    PyObject* m_function;
};

// ---------------------------------------------------------------------------
// Config reading
// ---------------------------------------------------------------------------

struct PythonDataSourceSpec
{
    std::string name;
    std::string module;
    std::string class_name;
    std::unordered_map<std::string, nlohmann::json> args;
};

struct PythonCustomFunctionSpec
{
    std::string library;
    std::string module;
    std::vector<std::string> functions;
};

bool parse_python_data_sources(const toml::table& config, std::vector<PythonDataSourceSpec>& specs, std::string& error)
{
    auto* table = config["python_data_sources"].as_table();
    if (table == nullptr) {
        return true; // nothing configured
    }

    for (const auto& [key, node] : *table) {
        auto* sub = node.as_table();
        if (sub == nullptr) {
            error = "python_data_sources." + std::string{key.str()} + " must be a table";
            return false;
        }
        PythonDataSourceSpec spec;
        spec.name = std::string{key.str()};
        auto module = (*sub)["module"].value<std::string>();
        if (!module.has_value() || module->empty()) {
            error = "python_data_sources." + spec.name + " requires a 'module' key";
            return false;
        }
        spec.module = *module;
        spec.class_name = (*sub)["class_name"].value_or((*sub)["class"].value_or(spec.name + "DataSource"));
        if (auto* args = (*sub)["args"].as_table()) {
            for (const auto& [akey, anode] : *args) {
                const std::string arg_name{akey.str()};
                if (auto v = anode.value<std::string>()) {
                    spec.args[arg_name] = *v;
                } else if (auto b = anode.value<bool>()) {
                    spec.args[arg_name] = *b;
                } else if (auto i = anode.value<int64_t>()) {
                    spec.args[arg_name] = *i;
                } else if (auto f = anode.value<double>()) {
                    spec.args[arg_name] = *f;
                }
            }
        }
        specs.push_back(std::move(spec));
    }
    return true;
}

bool parse_python_custom_functions(const toml::table& config, std::vector<PythonCustomFunctionSpec>& specs,
                                   std::string& error)
{
    auto* table = config["python_custom_functions"].as_table();
    if (table == nullptr) {
        return true; // nothing configured
    }

    for (const auto& [key, node] : *table) {
        auto* sub = node.as_table();
        if (sub == nullptr) {
            error = "python_custom_functions." + std::string{key.str()} + " must be a table";
            return false;
        }
        PythonCustomFunctionSpec spec;
        spec.library = std::string{key.str()};
        auto module = (*sub)["module"].value<std::string>();
        if (!module.has_value() || module->empty()) {
            error = "python_custom_functions." + spec.library + " requires a 'module' key";
            return false;
        }
        spec.module = *module;
        auto* functions = (*sub)["functions"].as_array();
        if (functions == nullptr) {
            error = "python_custom_functions." + spec.library + " requires a 'functions' array";
            return false;
        }
        for (const auto& fname : *functions) {
            auto name = fname.value<std::string>();
            if (!name.has_value()) {
                error = "python_custom_functions." + spec.library + ".functions must be strings";
                return false;
            }
            spec.functions.push_back(*name);
        }
        specs.push_back(std::move(spec));
    }
    return true;
}

} // namespace

void init_python_data_sources_if_configured(libtokamap::MappingHandler& mapping_handler)
{
    const char* config_env = std::getenv("MAPPING_PLUGIN_PYTHON_CONFIG");
    if (config_env == nullptr || *config_env == '\0') {
        return; // no Python config declared anywhere — interpreter stays off
    }
    const std::filesystem::path config_path{config_env};
    if (!std::filesystem::exists(config_path)) {
        throw std::runtime_error{"MAPPING_PLUGIN_PYTHON_CONFIG points to a missing file: " + config_path.string()};
    }

    toml::table config;
    try {
        config = toml::parse_file(config_path.string());
    } catch (const toml::parse_error& e) {
        throw std::runtime_error{"failed to parse " + config_path.string() + ": " + e.what()};
    }

    std::vector<PythonDataSourceSpec> source_specs;
    std::vector<PythonCustomFunctionSpec> function_specs;
    std::string error;
    if (!parse_python_data_sources(config, source_specs, error) ||
        !parse_python_custom_functions(config, function_specs, error)) {
        throw std::runtime_error{error};
    }
    if (source_specs.empty() && function_specs.empty()) {
        return; // nothing Python-backed configured — interpreter stays off
    }

    ensure_python_interpreter();

    PyGILState_STATE gil = PyGILState_Ensure();
    for (const auto& spec : source_specs) {
        PyObject* module = PyImport_ImportModule(spec.module.c_str());
        if (module == nullptr) {
            std::string message = fetch_python_error();
            PyGILState_Release(gil);
            throw std::runtime_error{"failed to import '" + spec.module + "': " + message};
        }
        PyObject* cls = nullptr;
        if (module == nullptr) {
            std::string message = fetch_python_error();
            PyGILState_Release(gil);
            throw std::runtime_error{"failed to import '" + spec.module + "': " + message};
        }
        cls = PyObject_GetAttrString(module, spec.class_name.c_str());
        Py_DECREF(module);
        if (cls == nullptr) {
            std::string message = fetch_python_error();
            PyGILState_Release(gil);
            throw std::runtime_error{"failed to find class '" + spec.class_name + "' in " + spec.module + ": " +
                                     message};
        }
        PyObject* kwargs = PyDict_New();
        for (const auto& [key, value] : spec.args) {
            PyObject* py_value = json_to_pyobject(value);
            PyDict_SetItemString(kwargs, key.c_str(), py_value);
            Py_DECREF(py_value);
        }
        PyObject* empty_args = PyTuple_New(0);
        PyObject* instance = PyObject_Call(cls, empty_args, kwargs);
        Py_DECREF(empty_args);
        Py_DECREF(kwargs);
        Py_DECREF(cls);
        if (instance == nullptr) {
            std::string message = fetch_python_error();
            PyGILState_Release(gil);
            throw std::runtime_error{"failed to construct " + spec.module + "." + spec.class_name + ": " + message};
        }
        // The registry takes ownership; the Python object stays alive for the
        // process lifetime via the reference held in PythonDataSource.
        mapping_handler.register_data_source(spec.name, std::make_unique<PythonDataSource>(instance));
    }

    for (const auto& spec : function_specs) {
        PyObject* module = PyImport_ImportModule(spec.module.c_str());
        if (module == nullptr) {
            std::string message = fetch_python_error();
            PyGILState_Release(gil);
            throw std::runtime_error{"failed to import '" + spec.module + "': " + message};
        }
        for (const auto& function_name : spec.functions) {
            PyObject* function = PyObject_GetAttrString(module, function_name.c_str());
            if (function == nullptr) {
                std::string message = fetch_python_error();
                Py_DECREF(module);
                PyGILState_Release(gil);
                throw std::runtime_error{"failed to find function '" + function_name + "' in " + spec.module + ": " +
                                         message};
            }
            mapping_handler.register_custom_function(libtokamap::LibraryFunction{
                spec.library, function_name, std::make_unique<PythonCustomFunction>(function)});
        }
        Py_DECREF(module);
    }
    PyGILState_Release(gil);
}

} // namespace mapping_plugin

#else // !MAPPING_PLUGIN_PYTHON — stub: no Python dependency compiled in

#include <filesystem>
#include <stdexcept>

namespace mapping_plugin
{

void init_python_data_sources_if_configured(libtokamap::MappingHandler& /*mapping_handler*/)
{
    // Configurations that declare Python data sources cannot run on a build
    // without Python support — fail loudly rather than silently serve nothing.
    const char* config_env = std::getenv("MAPPING_PLUGIN_PYTHON_CONFIG");
    if (config_env == nullptr || *config_env == '\0') {
        return;
    }
    if (std::filesystem::exists(std::filesystem::path{config_env})) {
        throw std::runtime_error{std::string{"MAPPING_PLUGIN_PYTHON_CONFIG is set ("} + config_env +
                                 ") but this plugin was built without MAPPING_PLUGIN_PYTHON; "
                                 "rebuild with -DMAPPING_PLUGIN_PYTHON=ON"};
    }
}

} // namespace mapping_plugin

#endif // MAPPING_PLUGIN_PYTHON
