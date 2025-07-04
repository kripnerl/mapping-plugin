#include "test_helpers.hpp"
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>
#include <fstream>

// Initialize test JSON data
const nlohmann::json TEST_MAPPINGS_CFG = {
    {"mappingLocations", {
        {"machine", "test_machine"},
        {"shot", "12345"},
        {"path", "test_ids"}
    }}
};

const nlohmann::json TEST_GLOBALS = {
    {"globalVar1", 42.0},
    {"globalVar2", "global_string"},
    {"globalArray", {1, 2, 3, 4, 5}}
};

const nlohmann::json TEST_MAPPINGS = {
    {"valueMappings", {
        {"simple_float", {"type", "VALUE", "value", 3.14}},
        {"simple_int", {"type", "VALUE", "value", 42}},
        {"simple_string", {"type", "VALUE", "value", "test_string"}},
        {"simple_array", {"type", "VALUE", "value", {1, 2, 3, 4, 5}}},
        {"simple_object", {"type", "VALUE", "value", {{"key1", "value1"}, {"key2", 2}}}}
    }},
    {"pluginMappings", {
        {"test_plugin", {
            {"type", "PLUGIN"},
            {"plugin", "mockPlugin"},
            {"function", "getData"},
            {"arguments", {
                {"argName", "argValue"}
            }}
        }}
    }},
    {"exprMappings", {
        {"test_expr", {
            {"type", "EXPR"},
            {"expr", "globalVar1 * 2 + 10"}
        }}
    }},
    {"dimMappings", {
        {"test_dim", {
            {"type", "DIMENSION"},
            {"dimensions", {0, 1, 2, 3, 4}}
        }}
    }},
    {"customMappings", {
        {"test_custom", {
            {"type", "CUSTOM"},
            {"template", "This is a {{ value }} with {{ globalVar2 }}"},
            {"variables", {
                {"value", "test template"}
            }}
        }}
    }}
};

const nlohmann::json TEST_SHOT_GLOBALS = {
    {"globalVar1", 100.0},
    {"shotSpecificVar", "shot_specific_value"}
};

const nlohmann::json TEST_SHOT_MAPPINGS = {
    {"valueMappings", {
        {"shot_specific_float", {"type", "VALUE", "value", 9.87}}
    }}
};

IDAM_PLUGIN_INTERFACE* PluginInterfaceFixture::Create(
    const std::string& function,
    const std::string& machine,
    const std::string& path,
    int shot
) {
    std::string request = function + "::" + machine + "/" + path + "?shot=" + std::to_string(shot);

    IDAM_PLUGIN_INTERFACE* interface = udaCreatePluginInterface(request.c_str());

    std::ofstream my_log_file;
    my_log_file.open("/Users/aparker/out.log", std::ios_base::app);
    my_log_file << "CREATED INTERFACE" << std::endl;

    return interface;
}

// void JSONMappingTestFixture::SetUpTestEnvironment() {
//     // Setup any environment variables or global state needed for tests
// }
//
// void JSONMappingTestFixture::CleanUpTestEnvironment() {
//     // Clean up any resources created during tests
// }
//
// std::filesystem::path JSONMappingTestFixture::CreateTestMappingDir() {
//     // Gets the path to the test data directory (set by CMake)
//     const char* test_data_dir = std::getenv("UDA_JSON_MAPPING_DIR");
//     if (!test_data_dir) {
//         throw std::runtime_error("UDA_JSON_MAPPING_DIR environment variable not set");
//     }
//     return std::filesystem::path(test_data_dir);
// }
//
// std::unique_ptr<IDAM_PLUGIN_INTERFACE> JSONMappingTestFixture::CreateMockPluginInterface(
//     const std::string& function,
//     const std::string& machine, 
//     const std::string& path,
//     int shot,
//     int datatype,
//     int rank
// ) {
//     auto interface = std::make_unique<IDAM_PLUGIN_INTERFACE>();
//
//     // Initialize interface with default values
//     interface->pluginName = strdup("JSON_mapping_plugin");
//     interface->housekeeping = nullptr;
//     interface->data_block = nullptr;
//     interface->returnStatus = 0;
//
//     // Set request properties
//     interface->request = new REQUEST_DATA();
//     interface->request->function = strdup(function.c_str());
//     interface->request->path = strdup(path.c_str());
//     interface->request->source = strdup(machine.c_str());
//     interface->request->shot = shot;
//     interface->request->exp_number = 0;
//     interface->request->pass = 0;
//     interface->request->dataLevel = 0;
//
//     // Create an empty data_block structure
//     interface->data_block = new DATA_BLOCK();
//     interface->data_block->rank = rank;
//     interface->data_block->data_type = datatype;
//     interface->data_block->data = nullptr;
//     interface->data_block->dims = nullptr;
//     interface->data_block->error_code = 0;
//
//     if (rank > 0) {
//         interface->data_block->dims = new DIMS[rank];
//         for (int i = 0; i < rank; i++) {
//             interface->data_block->dims[i].dim = 0;
//             interface->data_block->dims[i].compressed = 0;
//             interface->data_block->dims[i].data_type = datatype;
//             interface->data_block->dims[i].dim_n = 0;
//             interface->data_block->dims[i].dim_units = nullptr;
//             interface->data_block->dims[i].data = nullptr;
//         }
//     }
//
//     return interface;
// }
//
// void JSONMappingTestFixture::FreePluginInterface(IDAM_PLUGIN_INTERFACE* interface) {
//     if (!interface) return;
//
//     if (interface->request) {
//         free(interface->request->function);
//         free(interface->request->path);
//         free(interface->request->source);
//         delete interface->request;
//     }
//
//     if (interface->data_block) {
//         free(interface->data_block->data);
//
//         if (interface->data_block->dims) {
//             for (int i = 0; i < interface->data_block->rank; i++) {
//                 free(interface->data_block->dims[i].data);
//                 free(interface->data_block->dims[i].dim_units);
//             }
//             delete[] interface->data_block->dims;
//         }
//
//         delete interface->data_block;
//     }
//
//     free(interface->pluginName);
// }
//
// void JSONMappingTestFixture::CreateMappingConfigFile(const std::filesystem::path& dir, const nlohmann::json& config) {
//     std::ofstream file(dir / "mappings.cfg.json");
//     file << config.dump(4);
// }
//
// void JSONMappingTestFixture::CreateGlobalsFile(const std::filesystem::path& dir, const nlohmann::json& globals) {
//     std::ofstream file(dir / "globals.json");
//     file << globals.dump(4);
// }
//
// void JSONMappingTestFixture::CreateMappingsFile(const std::filesystem::path& dir, const nlohmann::json& mappings) {
//     std::ofstream file(dir / "mappings.json");
//     file << mappings.dump(4);
// }
//
// // Helper for comparing DATA_BLOCK contents with a single float value
// void JSONMappingTestFixture::CheckDataBlockEquals(const DATA_BLOCK* actual, float expected_value) {
//     REQUIRE(actual != nullptr);
//     REQUIRE(actual->data_type == UDA_TYPE_FLOAT);
//     REQUIRE(actual->rank == 0);
//     REQUIRE(actual->data != nullptr);
//
//     float* data = static_cast<float*>(actual->data);
//     REQUIRE(*data == Approx(expected_value));
// }
//
// // Helper for comparing DATA_BLOCK contents with a vector of float values
// void JSONMappingTestFixture::CheckDataBlockEquals(const DATA_BLOCK* actual, const std::vector<float>& expected_values) {
//     REQUIRE(actual != nullptr);
//     REQUIRE(actual->data_type == UDA_TYPE_FLOAT);
//     REQUIRE(actual->rank == 1);
//     REQUIRE(actual->dims != nullptr);
//     REQUIRE(actual->dims[0].dim == static_cast<int>(expected_values.size()));
//     REQUIRE(actual->data != nullptr);
//
//     float* data = static_cast<float*>(actual->data);
//     for (size_t i = 0; i < expected_values.size(); i++) {
//         REQUIRE(data[i] == Approx(expected_values[i]));
//     }
// }
//
// // Helper for comparing DATA_BLOCK contents with a 2D vector of float values
// void JSONMappingTestFixture::CheckDataBlockEquals(const DATA_BLOCK* actual, const std::vector<std::vector<float>>& expected_values) {
//     REQUIRE(actual != nullptr);
//     REQUIRE(actual->data_type == UDA_TYPE_FLOAT);
//     REQUIRE(actual->rank == 2);
//     REQUIRE(actual->dims != nullptr);
//     REQUIRE(actual->dims[0].dim == static_cast<int>(expected_values.size()));
//
//     if (!expected_values.empty()) {
//         REQUIRE(actual->dims[1].dim == static_cast<int>(expected_values[0].size()));
//     }
//
//     REQUIRE(actual->data != nullptr);
//
//     float* data = static_cast<float*>(actual->data);
//     size_t rows = expected_values.size();
//     size_t cols = rows > 0 ? expected_values[0].size() : 0;
//
//     for (size_t i = 0; i < rows; i++) {
//         for (size_t j = 0; j < cols; j++) {
//             REQUIRE(data[i * cols + j] == Approx(expected_values[i][j]));
//         }
//     }
// }
//
// // Fill a DATA_BLOCK with a single float value
// void MockUDAPlugin::FillDataBlock(DATA_BLOCK* data_block, float value) {
//     if (!data_block) return;
//
//     data_block->data_type = UDA_TYPE_FLOAT;
//     data_block->rank = 0;
//     data_block->data = malloc(sizeof(float));
//     *static_cast<float*>(data_block->data) = value;
// }
//
// // Fill a DATA_BLOCK with a vector of float values
// void MockUDAPlugin::FillDataBlock(DATA_BLOCK* data_block, const std::vector<float>& values) {
//     if (!data_block) return;
//
//     data_block->data_type = UDA_TYPE_FLOAT;
//     data_block->rank = 1;
//     data_block->dims = new DIMS[1];
//     data_block->dims[0].dim = static_cast<int>(values.size());
//     data_block->dims[0].compressed = 0;
//     data_block->dims[0].data_type = UDA_TYPE_FLOAT;
//
//     data_block->data = malloc(sizeof(float) * values.size());
//     float* data = static_cast<float*>(data_block->data);
//
//     for (size_t i = 0; i < values.size(); i++) {
//         data[i] = values[i];
//     }
// }
//
// // Fill a DATA_BLOCK with a 2D vector of float values
// void MockUDAPlugin::FillDataBlock(DATA_BLOCK* data_block, const std::vector<std::vector<float>>& values) {
//     if (!data_block) return;
//
//     data_block->data_type = UDA_TYPE_FLOAT;
//     data_block->rank = 2;
//     data_block->dims = new DIMS[2];
//
//     size_t rows = values.size();
//     size_t cols = rows > 0 ? values[0].size() : 0;
//
//     data_block->dims[0].dim = static_cast<int>(rows);
//     data_block->dims[0].compressed = 0;
//     data_block->dims[0].data_type = UDA_TYPE_FLOAT;
//
//     data_block->dims[1].dim = static_cast<int>(cols);
//     data_block->dims[1].compressed = 0;
//     data_block->dims[1].data_type = UDA_TYPE_FLOAT;
//
//     data_block->data = malloc(sizeof(float) * rows * cols);
//     float* data = static_cast<float*>(data_block->data);
//
//     for (size_t i = 0; i < rows; i++) {
//         for (size_t j = 0; j < cols; j++) {
//             data[i * cols + j] = values[i][j];
//         }
//     }
// }
//
// // Mock plugin implementation for testing
// extern "C" int mockPlugin(IDAM_PLUGIN_INTERFACE* interface) {
//     // Default mock implementation that just returns a float value of 42.0
//     MockUDAPlugin::FillDataBlock(interface->data_block, 42.0f);
//     return 0;
// }