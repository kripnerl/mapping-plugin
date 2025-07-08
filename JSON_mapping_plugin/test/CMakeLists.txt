cmake_minimum_required( VERSION 3.15 )

# Project details
project(
    ${CMAKE_PROJECT_NAME}Tests
    LANGUAGES CXX
)

set( TEST_SOURCES
    src/main_test.cpp
    src/json_plugin_test.cpp
    src/value_mapping_test.cpp
)

include( FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)
FetchContent_MakeAvailable( Catch2 )

# Create test executable
add_executable( ${CMAKE_PROJECT_NAME}_tests ${TEST_SOURCES} )

# Set compiler standard
target_compile_features( ${CMAKE_PROJECT_NAME}_tests PUBLIC cxx_std_17 )

# INCLUDE
target_include_directories( ${CMAKE_PROJECT_NAME}_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/JSON_mapping_plugin
    ${CMAKE_SOURCE_DIR}/JSON_mapping_plugin/src
    ${UDA_CLIENT_INCLUDE_DIRS}
)

# LIBS
target_link_libraries( ${CMAKE_PROJECT_NAME}_tests
    PRIVATE
    Catch2::Catch2WithMain
    test_helpers
    ${JSON_LIBNAME}
    uda_plugins
)

# # Test data directory
# set( TEST_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_data" )
# file( MAKE_DIRECTORY ${TEST_DATA_DIR} )

# Test helpers library
add_library( test_helpers STATIC src/test_helpers.cpp )
target_include_directories( test_helpers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/JSON_mapping_plugin
    ${UDA_CLIENT_INCLUDE_DIRS}
)
target_link_libraries( test_helpers PUBLIC
    ${JSON_LIBNAME}
    Catch2::Catch2
)

# # Code coverage
# if( ${CMAKE_PROJECT_NAME}_ENABLE_CODE_COVERAGE )
#     target_compile_options( ${CMAKE_PROJECT_NAME}_tests PUBLIC -O0 -g -fprofile-arcs -ftest-coverage )
#     target_link_options( ${CMAKE_PROJECT_NAME}_tests PUBLIC -fprofile-arcs -ftest-coverage )
# endif()

# CTest integration
list( APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras )
include( Catch )
catch_discover_tests( ${CMAKE_PROJECT_NAME}_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
