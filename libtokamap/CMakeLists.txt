# Project details
# project(
#     LibTokaMap
#     VERSION 0.1.0
#     LANGUAGES CXX
# )

find_package( Boost REQUIRED )

# Specify C++ standard for all targets
#set( CMAKE_CXX_STANDARD 17 )
#set( CMAKE_CXX_STANDARD_REQUIRED ON )
#set( CMAKE_CXX_EXTENSIONS OFF )

# Group source files by functionality for better organization
set( UTIL_SOURCES
    src/utils/indices.cpp
    src/utils/ram_cache.cpp
    src/utils/scale_offset.cpp
    src/utils/subset.cpp
    src/utils/syntax_parser.cpp
)

set( MAP_TYPE_SOURCES
    src/map_types/custom_mapping.cpp
    src/map_types/data_source_mapping.cpp
    src/map_types/dim_mapping.cpp
    src/map_types/expr_mapping.cpp
    src/map_types/map_arguments.cpp
    src/map_types/value_mapping.cpp
)

set( HANDLER_SOURCES
    src/handlers/mapping_handler.cpp
)

set( SOURCES
    ${UTIL_SOURCES}
    ${MAP_TYPE_SOURCES}
    ${HANDLER_SOURCES}
)

set( UTIL_HEADERS
    src/utils/ram_cache.hpp
    src/utils/scale_offset.hpp
    src/utils/subset.hpp
)

set( MAP_TYPE_HEADERS
    src/map_types/base_mapping.hpp
    src/map_types/custom_mapping.hpp
    src/map_types/data_source_mapping.hpp
    src/map_types/dim_mapping.hpp
    src/map_types/expr_mapping.hpp
    src/map_types/map_arguments.hpp
    src/map_types/value_mapping.hpp
)

set( HANDLER_HEADERS
    src/handlers/mapping_handler.hpp
)

set( HEADERS
    ${UTIL_HEADERS}
    ${MAP_TYPE_HEADERS}
    ${HANDLER_HEADERS}
)

add_library( tokamap ${SOURCES} )
target_link_libraries( tokamap PRIVATE Boost::boost )
target_include_directories( tokamap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ext_include
)

# Unit testing setup
if( ENABLE_TESTING )
    add_subdirectory( test )
endif()

list( TRANSFORM SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )
list( TRANSFORM HEADERS PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )

set_property( TARGET clang-format APPEND PROPERTY SOURCES ${SOURCES} ${HEADERS} )
