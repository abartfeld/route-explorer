#pragma once

#include <QMainWindow>
#include <QSlider>
#include <QTimer>
#include "../third_party/qcustomplot.h"
#include "gpxparser.h"
#include "MapWidget.h"
#include "TrackStatsWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void updatePosition(int value);

private:
    void setupUi();
    void plotElevationProfile();
    void updatePlotPosition(const TrackPoint& point);
    size_t findClosestPointByDistance(double targetDistance);

    // UI Elements
    MapWidget *m_mapView;
    QCustomPlot *m_elevationPlot;
    QSlider *m_positionSlider;
    TrackStatsWidget *m_statsWidget;

    // Data
    GPXParser m_gpxParser;
    size_t m_currentPointIndex;
};
