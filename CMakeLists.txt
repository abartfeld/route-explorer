cmake_minimum_required(VERSION 3.10)
project(gpx_viewer VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt packages - ensure we include PrintSupport for QPrinter used by QCustomPlot
find_package(Qt5 COMPONENTS Core Gui Widgets Positioning Network PrintSupport REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Set source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    gpxparser.cpp
    MapWidget.cpp
    qcustomplot.cpp
)

set(HEADERS
    mainwindow.h
    gpxparser.h
    MapWidget.h
    qcustomplot.h
)

# Resources
set(RESOURCES
    resources.qrc
)

# Add Qt resources
qt5_add_resources(QRC_SOURCES ${RESOURCES})

# Create executable
add_executable(gpx_viewer ${SOURCES} ${HEADERS} ${QRC_SOURCES})

# Link Qt libraries - include PrintSupport for QPrinter class support
target_link_libraries(gpx_viewer
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Positioning
    Qt5::Network
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

# Enable automoc for Qt
set_target_properties(gpx_viewer PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)