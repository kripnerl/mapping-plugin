# Project details
# project(
#     MappingPlugin
#     VERSION 0.2.0
#     LANGUAGES CXX
# )

find_package( UDA 2.7.0 REQUIRED )
find_package( Boost REQUIRED )
find_package( libtokamap REQUIRED )

set( SOURCES
    src/uda_data_source.cpp
    src/uda_ram_cache.cpp
    src/uda_plugin_helpers.cpp
)

set( HEADERS
    src/uda_type_sizes.hpp
    src/print_uda_structs.hpp
    src/uda_ram_cache.hpp
    src/uda_data_source.hpp
    src/uda_plugin_helpers.hpp
)

add_library( libtokamap_uda_extension ${SOURCES} )
target_include_directories( libtokamap_uda_extension PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/ext_include
)
target_link_libraries( libtokamap_uda_extension PRIVATE Boost::boost LibTokaMap::libtokamap )

set( EXTRA_DEFINITIONS )
if( ENABLE_PROFILING )
    list( APPEND EXTRA_DEFINITIONS -DENABLE_PROFILING )
endif()

include( plugins )
uda_plugin(
    NAME IMAS_JSON_MAP
    ENTRY_FUNC jsonMappingPlugin
    DESCRIPTION "MAP IMAS element name to TOKAMAK data using JSON mapping files"
    EXAMPLE "IMAS_JSON_MAP::read()"
    LIBNAME mapping_plugin
    SOURCES mapping_plugin.cpp
    CONFIG_FILE config/json_mapping.cfg config/libtokamap_config.json
    EXTRA_INCLUDE_DIRS
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/ext_include
        ${CMAKE_SOURCE_DIR}/libtokamap/src
        ${UDA_CLIENT_INCLUDE_DIRS}
    EXTRA_LINK_DIRS
        ${UDA_CLIENT_LIBRARY_DIRS}
    EXTRA_LINK_LIBS
        Boost::boost
        LibTokaMap::libtokamap
        libtokamap_uda_extension
        uda_client
    EXTRA_DEFINITIONS
        ${EXTRA_DEFINITIONS}
)

# Unit testing setup
if( ENABLE_TESTING )
    add_subdirectory( test )
endif()

list( TRANSFORM SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )
list( TRANSFORM HEADERS PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )

set_property( TARGET clang-format APPEND PROPERTY SOURCES
    ${SOURCES}
    ${HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/mapping_plugin.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mapping_plugin.h
)

# Create module
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/etc/module.in
    ${CMAKE_BINARY_DIR}/etc/mapping_plugin/${PROJECT_VERSION}
    @ONLY
)

set( MODULE_PATH "modulefiles/mapping_plugin" CACHE PATH "location to install module files to" )
set( MODULE_NAME "${PROJECT_VERSION}" CACHE STRING "name of the module file" )
string( CONFIGURE "${MODULE_PATH}" MODULE_PATH_EXPANDED @ONLY )

get_filename_component(
    MODULE_PATH_EXPANDED ${MODULE_PATH_EXPANDED}
    REALPATH BASE_DIR "${CMAKE_INSTALL_PREFIX}"
)

install(
    FILES ${CMAKE_BINARY_DIR}/etc/mapping_plugin/${PROJECT_VERSION}
    DESTINATION ${MODULE_PATH_EXPANDED}
    RENAME ${MODULE_NAME}
)

# Generate scripts
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
