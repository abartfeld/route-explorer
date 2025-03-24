cmake_minimum_required(VERSION 3.5)

project(gpx_viewer LANGUAGES CXX)

# Add project source directory to include path for absolute paths
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# Update C++ standard from 11 to 14 for Google Test compatibility
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix working directory issues by using absolute paths
set(PROJECT_SOURCE_DIR_ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_BINARY_DIR_ABSOLUTE ${CMAKE_CURRENT_BINARY_DIR})

# Make sure all build files stay in the build directory
# Set output directories for the main build using absolute paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/lib)

# Fix the RCC options by removing the custom options that are causing errors
# Let CMake handle RCC automatically
set(CMAKE_AUTOMOC_MOC_OPTIONS "-o${PROJECT_BINARY_DIR_ABSOLUTE}/moc_%.cpp")
set(CMAKE_AUTOUIC_OPTIONS "-o${PROJECT_BINARY_DIR_ABSOLUTE}/ui_%.h")
# Remove this line causing the RCC issue:
# set(CMAKE_AUTORCC_OPTIONS "-o${PROJECT_BINARY_DIR_ABSOLUTE}/qrc_%.cpp")

# Ensure other CMake-generated files stay in build dir
set(CMAKE_AUTOGEN_BUILD_DIR ${PROJECT_BINARY_DIR_ABSOLUTE}/autogen)

# Enable testing with absolute paths
enable_testing()
include(CTest)

# Find required Qt packages
find_package(Qt5 COMPONENTS Core Widgets Network Positioning PrintSupport Test REQUIRED)

# Include directories with absolute paths
include_directories(
    ${PROJECT_SOURCE_DIR_ABSOLUTE}/include
    ${PROJECT_SOURCE_DIR_ABSOLUTE}/third_party
)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/MapWidget.cpp
    src/gpxparser.cpp
    src/TrackStatsWidget.cpp
    third_party/qcustomplot.cpp
)

# Header files
set(HEADERS
    include/mainwindow.h
    include/MapWidget.h
    include/gpxparser.h
    include/TrackStatsWidget.h
    third_party/qcustomplot.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(gpx_viewer
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Link libraries
target_link_libraries(gpx_viewer
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Positioning
    Qt5::PrintSupport
)

# Install rules
install(TARGETS gpx_viewer DESTINATION bin)

# Set compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gpx_viewer PRIVATE -Wall -Wextra -pedantic)
elseif(MSVC)
    target_compile_options(gpx_viewer PRIVATE /W4)
endif()
