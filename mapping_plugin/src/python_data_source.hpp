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
// The declarations live in a SEPARATE TOML file pointed to by the env var
// MAPPING_PLUGIN_PYTHON_CONFIG — they cannot sit in the libtokamap config
// because libtokamap validates it against a built-in schema with
// additionalProperties=false.
//
// Config shape:
//   [python_data_sources.CDB]
//   module = "tokamap_compass.cdb_datasource"
//   class  = "CDBDataSource"          # optional, defaults to <name>DataSource
//   [python_data_sources.CDB.args]    # optional kwargs for the constructor
//
//   [python_custom_functions.compass] # library name = "compass"
//   module = "tokamap_compass.custom_functions"
//   functions = ["interp1d", "zero_as_nan"]
//
// No-op (interpreter never started) when the env var is unset, the file is
// absent, or both tables are empty. Throws std::runtime_error if Python
// sources are configured but could not be initialised. Builds without
// MAPPING_PLUGIN_PYTHON compile to a stub that never starts Python and only
// fails loudly if such a config is present.
void init_python_data_sources_if_configured(libtokamap::MappingHandler& mapping_handler);

} // namespace mapping_plugin

#endif // MAPPING_PLUGIN_PYTHON_DATA_SOURCE_HPP

