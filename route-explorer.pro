QT += core gui network positioning 3dcore 3drender 3dextras svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/MapWidget.cpp \
    src/gpxparser.cpp \
    src/TrackStatsWidget.cpp \
    src/ElevationView3D.cpp \
    src/LandingPage.cpp \
    third_party/qcustomplot.cpp

HEADERS += \
    include/mainwindow.h \
    include/MapWidget.h \
    include/gpxparser.h \
    include/TrackStatsWidget.h \
    include/ElevationView3D.h \
    include/LandingPage.h \
    include/debug_helper.h \
    third_party/qcustomplot.h

RESOURCES += \
    resources/resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Include paths
INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/third_party
