# cmake_minimum_required( VERSION 3.15 )

# Project details
# project(
#     libtokamap_tests
#     LANGUAGES CXX
# )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( TEST_SOURCES
    src/data_source_mapping_test.cpp
    src/indices_test.cpp
    src/parse_slices_test.cpp
    src/syntax_parser_test.cpp
    src/typed_data_array_test.cpp
    src/value_mapping_test.cpp
    src/mapping_locator_test.cpp
)

include( FetchContent )
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)
FetchContent_MakeAvailable( Catch2 )

# Create test executable
add_executable( libtokamap_tests ${TEST_SOURCES} )

# INCLUDE
target_include_directories( libtokamap_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# LIBS
target_link_libraries( libtokamap_tests
    PRIVATE
    Catch2::Catch2WithMain
    test_helpers
    LibTokaMap::libtokamap
)

# Test helpers library
add_library( test_helpers STATIC src/test_helpers.cpp )
target_include_directories( test_helpers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ext_include
)
target_link_libraries( test_helpers PUBLIC
    Catch2::Catch2
)

# CTest integration
list( APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras )

include( Catch )
catch_discover_tests( libtokamap_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
