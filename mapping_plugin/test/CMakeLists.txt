# cmake_minimum_required( VERSION 3.15 )

# Project details
# project(
#     mapping_plugin_tests
#     LANGUAGES CXX
# )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

find_package( Boost REQUIRED )

set( TEST_SOURCES
    src/uda_data_source_test.cpp
)

include( FetchContent )
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)
FetchContent_MakeAvailable( Catch2 )

# Create test executable
add_executable( mapping_plugin_tests ${TEST_SOURCES} )

# INCLUDE
target_include_directories( mapping_plugin_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# LIBS
target_link_libraries( mapping_plugin_tests
    PRIVATE
    Catch2::Catch2WithMain
    Boost::boost
    LibTokaMap::libtokamap
    libtokamap_uda_extension
    uda_plugins
    uda_client
)

# CTest integration
list( APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras )

include( Catch )
catch_discover_tests( mapping_plugin_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
