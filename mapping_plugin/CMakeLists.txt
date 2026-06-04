# Project details
# project(
#     MappingPlugin
#     VERSION 0.2.0
#     LANGUAGES CXX
# )

find_package( UDA 2.7.0 REQUIRED )
find_package( LibTokaMap REQUIRED )

include( plugins )
uda_plugin(
    NAME IMAS_MAP
    ENTRY_FUNC mappingPlugin
    DESCRIPTION "MAP IMAS element name to TOKAMAK data using JSON mapping files"
    EXAMPLE "IMAS_MAP::read()"
    LIBNAME mapping_plugin
    SOURCES mapping_plugin.cpp src/uda_plugin_helpers.cpp
    CONFIG_FILE config/json_mapping.cfg config/libtokamap_config.toml
    EXTRA_INCLUDE_DIRS
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/ext_include
        ${UDA_CLIENT_INCLUDE_DIRS}
    EXTRA_LINK_DIRS
        ${UDA_CLIENT_LIBRARY_DIRS}
    EXTRA_LINK_LIBS
        LibTokaMap::libtokamap
)

# Generate scripts
list( GET UDA_DIR 0 UDA_HOME )
string( REGEX REPLACE ";" " " PLUGINS "${BUILT_PLUGINS}" )
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/activate-plugins.sh.in
    ${CMAKE_BINARY_DIR}/scripts/activate-plugins.sh
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/remove-plugins.sh.in
    ${CMAKE_BINARY_DIR}/scripts/remove-plugins.sh
    @ONLY
)
