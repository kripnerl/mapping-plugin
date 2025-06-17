cmake_minimum_required( VERSION 3.15 )

# Project details
project(
    ${CMAKE_PROJECT_NAME}Tests
    LANGUAGES CXX
)

message( STATUS "TESTS: Adding tests under ${CMAKE_PROJECT_NAME}Tests..." )

set( TEST_SOURCES
    src/main_test.cpp
)

# Create test fixture data directory
set( TEST_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_data" )
file( MAKE_DIRECTORY ${TEST_DATA_DIR} )

# Common test setup
add_library( test_common INTERFACE )
target_include_directories( test_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/JSON_mapping_plugin
    ${CMAKE_SOURCE_DIR}/JSON_mapping_plugin/src
    ${trompeloeil_SOURCE_DIR}/include
)

# Define test helper library
add_library( test_helpers STATIC
    src/test_helpers.cpp
)

target_include_directories (test_helpers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/JSON_mapping_plugin
    ${CMAKE_SOURCE_DIR}/JSON_mapping_plugin/src
    ${trompeloeil_SOURCE_DIR}/include
)

target_link_libraries( test_helpers PUBLIC
    ${JSON_LIBNAME}
    Catch2::Catch2
)

# Load Catch2 testing framework
include( FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)

# Fetch trompeloeil mocking library
FetchContent_Declare(
    trompeloeil
    GIT_REPOSITORY https://github.com/rollbear/trompeloeil.git
    GIT_TAG v49
)

FetchContent_MakeAvailable( Catch2 trompeloeil )

# Add Catch2's CMake module directory to allow use of Catch2 CTest integration
list( APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras )
include( Catch )

# Create main test executable that includes all tests
add_executable( ${CMAKE_PROJECT_NAME}_tests ${TEST_SOURCES} )

# Set compiler standard
target_compile_features( ${CMAKE_PROJECT_NAME}_tests PUBLIC cxx_std_17 )

# Add test data directory definition
# target_compile_definitions(${CMAKE_PROJECT_NAME}_tests PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")

# Setup code coverage if enabled
if( ${CMAKE_PROJECT_NAME}_ENABLE_CODE_COVERAGE )
    target_compile_options (${CMAKE_PROJECT_NAME}_tests PUBLIC -O0 -g -fprofile-arcs -ftest-coverage )
    target_link_options( ${CMAKE_PROJECT_NAME}_tests PUBLIC -fprofile-arcs -ftest-coverage )
    message( STATUS "TESTS: Code coverage is enabled and provided with GCC." )
endif()

# Link libraries
target_link_libraries( ${CMAKE_PROJECT_NAME}_tests
    PRIVATE
    test_common
    test_helpers
    Catch2::Catch2WithMain
    ${JSON_LIBNAME}
    ${Boost_LIBRARIES}
    ${UDA_CLIENT_LIBRARIES}
    trompeloeil::trompeloeil
)

# Make sure tests found with CTest
catch_discover_tests( ${CMAKE_PROJECT_NAME}_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    # PROPERTIES ENVIRONMENT "UDA_JSON_MAPPING_DIR=${TEST_DATA_DIR}"
)

message(STATUS "TESTS: Finished adding unit tests for ${CMAKE_PROJECT_NAME}." )
