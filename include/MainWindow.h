#pragma once

#include <QMainWindow>
#include <QSlider>
#include <QTimer>
#include <QTabWidget>
#include <QStackedWidget>
#include "../third_party/qcustomplot.h"
#include "GpxParser.h"
#include "MapWidget.h"
#include "TrackStatsWidget.h"
#include "ElevationView3D.h"
#include "LandingPage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void showLandingPage();

private slots:
    void openFile();
    void openFile(const QString& filePath);
    void updatePosition(int value);
    void handleRouteHover(int pointIndex);
    void handleFlythrough3DPositionChanged(int pointIndex);
    void switchToTab(int tabIndex);
    void showMainView();
    void createNewRoute();
    void showSettings();
    void show3DView();

private:
    void setupUi();
    void plotElevationProfile();
    void updatePlotPosition(const TrackPoint& point);
    size_t findClosestPointByDistance(double targetDistance);
    void addToRecentFiles(const QString& filePath);

    // UI Elements
    QStackedWidget *m_mainStack;
    LandingPage *m_landingPage;
    QWidget *m_mainView;
    QTabWidget *m_tabWidget;
    MapWidget *m_mapView;
    QCustomPlot *m_elevationPlot;
    QSlider *m_positionSlider;
    TrackStatsWidget *m_statsWidget;
    ElevationView3D *m_elevation3DView;

    // Data
    GPXParser m_gpxParser;
    size_t m_currentPointIndex;
    
    // Flag to prevent feedback loops when updating slider programmatically
    bool m_updatingFromHover;
    bool m_updatingFrom3D;
};
