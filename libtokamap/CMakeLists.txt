# Project details
# project(
#     LibTokaMap
#     VERSION 0.1.0
#     LANGUAGES CXX
# )

# Specify C++ standard for all targets
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# Group source files by functionality for better organization
set( UTIL_SOURCES
    src/utils/indices.cpp
    src/utils/ram_cache.cpp
    src/utils/scale_offset.cpp
    src/utils/subset.cpp
    src/utils/syntax_parser.cpp
    src/utils/mapping_locator.cpp
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

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/include/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp
    @ONLY
)

add_library( libtokamap ${SOURCES} )
target_include_directories( libtokamap
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/ext_include
    INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext_include>
      $<INSTALL_INTERFACE:include/libtokamap>
)
target_compile_options( libtokamap PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Werror -Wpedantic>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror -Wpedantic>
)
set( PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/libtokamap.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp
)
set_target_properties( libtokamap PROPERTIES
    OUTPUT_NAME tokamap
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

add_library( LibTokaMap::libtokamap ALIAS libtokamap )

install( TARGETS libtokamap
    EXPORT tokamapTargets
    DESTINATION lib
    INCLUDES DESTINATION include
    PUBLIC_HEADER DESTINATION include
)

install( EXPORT tokamapTargets
    FILE LibTokaMapTargets.cmake
    NAMESPACE LibTokaMap::
    DESTINATION lib/cmake/libtokamap
)

include( CMakePackageConfigHelpers )

configure_package_config_file(
    cmake/LibTokaMapConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LibTokaMapConfig.cmake
    INSTALL_DESTINATION lib/cmake/libtokamap
)

install( FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LibTokaMapConfig.cmake
    DESTINATION lib/cmake/libtokamap
)

# Unit testing setup
if( ENABLE_TESTING )
    add_subdirectory( test )
endif()

if( ENABLE_EXAMPLES )
    add_subdirectory( examples/simple_mapper )
endif()

list( TRANSFORM SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )
list( TRANSFORM HEADERS PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )

set_property( TARGET clang-format APPEND PROPERTY SOURCES ${SOURCES} ${HEADERS} )
set_property( TARGET clang-format APPEND PROPERTY SOURCES ${SOURCES} ${HEADERS} )
