cmake_minimum_required( VERSION 3.15 )

# Project details
project(
    JSONMappingPlugin
    VERSION 0.2.0.0
    LANGUAGES CXX
)

# Extra cmake modules to find libraries
list(
    APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)

# Specify C++ standard for all targets
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# Export compile commands for tools like clang-tidy
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Set default build type if not specified
if( NOT CMAKE_BUILD_TYPE)
    set( CMAKE_BUILD_TYPE "Debug" )
endif()

message( STATUS "Started CMake for ${PROJECT_NAME} v${PROJECT_VERSION}...\n" )

# Prevent building in the source directory
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR )
    message( FATAL_ERROR "In-source builds not allowed.
            Please make a new directory (called a build directory)
            and run CMake from there.\n" )
endif()

# Find dependencies using modern approach
if( POLICY CMP0167 )
    cmake_policy( SET CMP0167 NEW )
endif()

# Set project options
include( StaticAnalyzers )
include( Utils )

option( ENABLE_TESTING "Enable unit tests for the project (from the `test` subfolder)." OFF )
option( ENABLE_PROFILING "Enable profiling for the project." OFF )
option( FETCH_LIBTOKAMAP "Fetch libtokamap as a dependency." OFF )

# add_clang_format_target()

if( ENABLE_TESTING )
    enable_testing()
endif()

if( FETCH_LIBTOKAMAP )
    include( FetchContent )
    FetchContent_Declare(
        libtokamap
        GIT_REPOSITORY https://github.com/ukaea/libtokamap.git
        GIT_TAG develop
        CMAKE_ARGS -DENABLE_TESTS=OFF -DENABLE_PROFILING=${ENABLE_PROFILING}
    )

    FetchContent_MakeAvailable( libtokamap )
endif()

find_package( UDA REQUIRED )
add_subdirectory( mapping_plugin )
# add_subdirectory( geometry_map_plugin )
# add_subdirectory( CUSTOM_MASTU_plugin )
