# Set the project name for the tests
project(gpx_viewer_tests)

# Find necessary packages
find_package(Qt5 COMPONENTS Test REQUIRED)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Create the test executable
add_executable(run_tests
    test_main.cpp
    gpxparser_test.cpp
)

# Link the test executable against gtest, our library, and Qt Test
target_link_libraries(run_tests
    gtest_main
    gpx_viewer_lib
    Qt5::Test
)

# Add the test to ctest
include(GoogleTest)
gtest_discover_tests(run_tests)

# Add a custom target for code coverage
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_custom_target(coverage
        COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details -o coverage.html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report..."
    )
endif()
