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

# Add debug symbols
set(CMAKE_BUILD_TYPE Debug)

# Fix working directory issues by using absolute paths
set(PROJECT_SOURCE_DIR_ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_BINARY_DIR_ABSOLUTE ${CMAKE_CURRENT_BINARY_DIR})

# Make sure all build files stay in the build directory
# Set output directories for all build types
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR_ABSOLUTE}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR_ABSOLUTE}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR_ABSOLUTE}/lib)
endforeach()

# Always set main output directories (used for single-config generators like Makefiles)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/lib)

# Ensure autogen, moc, and ui files go to proper subdirectories
set(CMAKE_AUTOMOC_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/autogen/moc)
set(CMAKE_AUTORCC_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/autogen/rcc)
set(CMAKE_AUTOUIC_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR_ABSOLUTE}/autogen/ui)

# Fix the RCC options by removing the custom options that are causing errors
# Let CMake handle RCC automatically
set(CMAKE_AUTOMOC_MOC_OPTIONS "-o${PROJECT_BINARY_DIR_ABSOLUTE}/autogen/moc/moc_%.cpp")
set(CMAKE_AUTOUIC_OPTIONS "-o${PROJECT_BINARY_DIR_ABSOLUTE}/autogen/ui/ui_%.h")

# Ensure other CMake-generated files stay in build dir
set(CMAKE_AUTOGEN_BUILD_DIR ${PROJECT_BINARY_DIR_ABSOLUTE}/autogen)

# Keep CMake cache and files in build directory
set(CMAKE_CACHEFILE_DIR ${PROJECT_BINARY_DIR_ABSOLUTE})

# Find required Qt packages
find_package(Qt5 COMPONENTS Core Widgets Network Positioning PrintSupport Test 3DCore 3DRender 3DExtras Svg REQUIRED)

# Include directories with absolute paths
include_directories(
    ${PROJECT_SOURCE_DIR_ABSOLUTE}/include
    ${PROJECT_SOURCE_DIR_ABSOLUTE}/third_party
)

# Source files - ADD NEW FILES HERE
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MapWidget.cpp
    src/GpxParser.cpp
    src/TrackStatsWidget.cpp
    src/ElevationView3D.cpp
    src/LandingPage.cpp
    third_party/qcustomplot.cpp
)

# Header files - ADD NEW FILES HERE
set(HEADERS
    include/MainWindow.h
    include/MapWidget.h
    include/GpxParser.h
    include/TrackStatsWidget.h
    include/ElevationView3D.h
    include/LandingPage.h
    include/debug_helper.h
    include/build_info.h
    include/logging.h
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
    Qt5::3DCore
    Qt5::3DRender
    Qt5::3DExtras
    Qt5::Svg
)

# Install rules
install(TARGETS gpx_viewer 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Set compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gpx_viewer PRIVATE -Wall -Wextra -pedantic)
elseif(MSVC)
    target_compile_options(gpx_viewer PRIVATE /W4)
endif()

# Make sure each target's temporary files go into a separate directory
set_target_properties(gpx_viewer PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    PDB_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Message about build directory structure
message(STATUS "Build files will be generated in: ${PROJECT_BINARY_DIR_ABSOLUTE}")
message(STATUS "Binaries will be output to: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Libraries will be output to: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
