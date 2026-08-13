#ifndef MAPPING_PLUGIN_PYTHON_DATA_SOURCE_HPP
#define MAPPING_PLUGIN_PYTHON_DATA_SOURCE_HPP

// Optional embedded-Python data-source support for the mapping plugin.
//
// This is compiled ONLY when the plugin is built with MAPPING_PLUGIN_PYTHON=ON
// (requires Python3 + NumPy headers). When off, the plugin has no Python
// dependency at all and init_python_data_sources_if_configured() is a no-op,
// so the plugin keeps working in environments that do not require Python.
//
// The interpreter is started lazily — only when a config file declares Python
// data sources or custom functions — and lives for
// the rest of the uda_server process (one connection per process under
// xinetd), so all fields served in one connection share the interpreter,
// the registered data-source instances and their caches.

#include <filesystem>

#include <libtokamap.hpp>

namespace mapping_plugin
{

// Read the plugin's python-data-source config and, if it declares any
// [python_data_sources] / [python_custom_functions], start the embedded
// interpreter (once per process), instantiate the configured classes and
// register the custom functions on mapping_handler.
//
// The declarations live in a SEPARATE JSON file pointed to by the env var
// MAPPING_PLUGIN_PYTHON_CONFIG — they cannot sit in the libtokamap config
// because libtokamap validates it against a built-in schema with
// additionalProperties=false.
//
// Config shape (// comments are accepted):
//   {
//     "python_data_sources": {
//       "CDB": {
//         "module": "tokamap_compass.cdb_datasource",
//         "class_name": "CDBDataSource",  // optional, default <name>DataSource
//         "args": { }                     // optional constructor kwargs
//       }
//     },
//     "python_custom_functions": {
//       "compass": {                      // library name
//         "module": "tokamap_compass.custom_functions",
//         "functions": ["interp1d", "zero_as_nan"]
//       }
//     }
//   }
//
// Why JSON and not TOML: this config was TOML until the format was switched,
// which required vendoring a second, byte-identical copy of toml++ (17,880
// lines) into this repo alongside the one already inside libtokamap. libtokamap
// installs nlohmann/json.hpp, valijson, exprtk and inja out of its ext_include
// but NOT toml.hpp, and its own TOML->JSON loader (load_toml_file in
// src/handlers/mapping_handler.cpp) sits in an anonymous namespace, so there was
// no way to reach a TOML parser from here without duplicating one. Parsing this
// small config with the nlohmann::json that libtokamap already exports drops the
// duplicate entirely. libtokamap accepts JSON for its own config too, so this
// keeps both files in formats libtokamap itself supports.
//
// NOTE FOR THE LIBTOKAMAP MAINTAINER — the alternative fix is one line on your
// side, and would let this plugin take TOML configs again with no vendoring:
// add ext_include/toml.hpp to the EXT_HEADERS list in libtokamap's
// CMakeLists.txt (currently CMakeLists.txt:136-141, next to
// ext_include/nlohmann/json.hpp, ext_include/valijson_nlohmann_bundled.hpp,
// ext_include/exprtk/exprtk.hpp and ext_include/inja/inja.hpp) so it is
// installed into include/libtokamap like the other four. Promoting the existing
// load_toml_file() to public API would serve just as well, and would also give
// every consumer the same TOML->JSON semantics libtokamap uses internally.
//
// No-op (interpreter never started) when the env var is unset, the file is
// absent, or both tables are empty. Throws std::runtime_error if Python
// sources are configured but could not be initialised. Builds without
// MAPPING_PLUGIN_PYTHON compile to a stub that never starts Python and only
// fails loudly if such a config is present.
void init_python_data_sources_if_configured(libtokamap::MappingHandler& mapping_handler);

} // namespace mapping_plugin

#endif // MAPPING_PLUGIN_PYTHON_DATA_SOURCE_HPP

