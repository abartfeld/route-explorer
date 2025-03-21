#pragma once
#include <QMainWindow>
#include <QSlider>
#include <QTimer>
#include <QCustomPlot>
#include "gpxparser.h"
#include "MapWidget.h"
#include "TrackStatsWidget.h"

/**
 * @brief Main application window
 * 
 * Handles UI layout, file loading, and track visualization
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructs main window
     * @param parent Parent widget
     */
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * @brief Handle file open action
     */
    void openFile();

    /**
     * @brief Handle position slider changes
     * @param value New slider value
     */
    void updatePosition(int value);

private:
    /**
     * @brief Set up UI elements
     */
    void setupUi();

    /**
     * @brief Plot elevation profile graph
     */
    void plotElevationProfile();

    /**
     * @brief Update elevation plot marker position
     * @param point Current track point
     */
    void updatePlotPosition(const TrackPoint& point);

    /**
     * @brief Find closest point to target distance
     * @param targetDistance Distance in meters
     * @return Index of closest point
     */
    size_t findClosestPointByDistance(double targetDistance);

    // UI Elements
    MapWidget* m_mapView;
    QCustomPlot* m_elevationPlot;
    QSlider* m_positionSlider;
    TrackStatsWidget* m_statsWidget;

    // Data
    GPXParser m_gpxParser;
    size_t m_currentPointIndex{0};
};
