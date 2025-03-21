cmake_minimum_required(VERSION 3.5)

project(gpx_viewer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt packages
find_package(Qt5 COMPONENTS Core Widgets Network Positioning REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    MapWidget.cpp
    MapWidget.h
    gpxparser.cpp
    gpxparser.h
    qcustomplot.cpp
    qcustomplot.h
    TrackStatsWidget.cpp
    TrackStatsWidget.h
)

# Resources
set(RESOURCES
    resources.qrc
)

# Create executable
add_executable(gpx_viewer
    ${SOURCES}
    ${RESOURCES}
)

# Link libraries
target_link_libraries(gpx_viewer
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Positioning
)

# Install rules
install(TARGETS gpx_viewer DESTINATION bin)

# Set compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gpx_viewer PRIVATE -Wall -Wextra -pedantic)
elseif(MSVC)
    target_compile_options(gpx_viewer PRIVATE /W4)
endif()