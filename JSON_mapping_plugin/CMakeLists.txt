set( CONFIGS
    config/json_mapping.cfg
)

set( JSON_LIBNAME JSON_mapping_plugin )

# Group source files by functionality for better organization
set( UTIL_SOURCES
    src/utils/scale_offset.cpp
    src/utils/uda_plugin_helpers.cpp
    src/utils/ram_cache.cpp
    src/utils/subset.cpp
    src/utils/syntax_parser.cpp
    src/utils/indices.cpp
)

set( MAP_TYPE_SOURCES
    src/map_types/custom_mapping.cpp
    src/map_types/dim_mapping.cpp
    src/map_types/expr_mapping.cpp
    src/map_types/plugin_mapping.cpp
    src/map_types/value_mapping.cpp
)

set( HANDLER_SOURCES
    src/handlers/mapping_handler.cpp
)

set( SOURCES
    JSON_mapping_plugin.cpp
    ${UTIL_SOURCES}
    ${MAP_TYPE_SOURCES}
    ${HANDLER_SOURCES}
)

set( UTIL_HEADERS
    src/utils/scale_offset.hpp
    src/utils/uda_plugin_helpers.hpp
    src/utils/ram_cache.hpp
    src/utils/subset.hpp
    src/utils/uda_type_sizes.hpp
    src/utils/print_uda_structs.hpp
)

set( MAP_TYPE_HEADERS
    src/map_types/base_mapping.hpp
    src/map_types/custom_mapping.hpp
    src/map_types/dim_mapping.hpp
    src/map_types/expr_mapping.hpp
    src/map_types/map_arguments.hpp
    src/map_types/plugin_mapping.hpp
    src/map_types/value_mapping.hpp
)

set( HANDLER_HEADERS
    src/handlers/mapping_handler.hpp
)

set( HEADERS
    JSON_mapping_plugin.h
    ${UTIL_HEADERS}
    ${MAP_TYPE_HEADERS}
    ${HANDLER_HEADERS}
)

# Create a list of all external dependencies
set( EXTERNAL_DEPS
    ${UDA_CLIENT_LIBRARIES}
    ${Boost_LIBRARIES}
    uda_cpp
)

include( plugins )
uda_plugin(
    NAME IMAS_JSON_MAP
    ENTRY_FUNC jsonMappingPlugin
    DESCRIPTION "MAP IMAS element name to TOKAMAK data using JSON mapping files"
    EXAMPLE "IMAS_JSON_MAP::read()"
    LIBNAME ${JSON_LIBNAME}
    SOURCES ${SOURCES}
    CONFIG_FILE ${CONFIGS}
    EXTRA_INCLUDE_DIRS
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/ext_include
        ${UDA_CLIENT_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
    EXTRA_LINK_DIRS
        ${UDA_CLIENT_LIBRARY_DIRS}
        ${Boost_LIBRARY_DIRS}
    EXTRA_LINK_LIBS
        ${EXTERNAL_DEPS}
)

# Unit testing setup
if( ${PROJECT_NAME}_ENABLE_TESTING )
    message( STATUS "TESTS: Building tests for the project." )
    add_subdirectory( test )
endif()

list( TRANSFORM SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )
list( TRANSFORM HEADERS PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )

set_property( TARGET clang-format APPEND PROPERTY SOURCES ${SOURCES} ${HEADERS} )
