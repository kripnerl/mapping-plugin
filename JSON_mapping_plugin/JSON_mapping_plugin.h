#pragma once

#include <clientserver/export.h>
#include <plugins/pluginStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API [[maybe_unused]] int jsonMappingPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif
