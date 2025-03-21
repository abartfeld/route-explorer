#pragma once
#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QTimer>
#include <QtPositioning/QGeoCoordinate>
#include "qcustomplot.h"
#include "gpxparser.h"
#include "MapWidget.h"

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
     * @brief Update map view with new position
     * @param position New geographical position
     */
    void updateMap(const QGeoCoordinate& position);

    /**
     * @brief Plot elevation profile graph
     */
    void plotElevationProfile();

    /**
     * @brief Update display with current position
     */
    void updateDisplay();

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
    size_t findClosestPoint(double targetDistance);

    // Optimized method to find closest point efficiently with binary search
    size_t findClosestPointByDistance(double targetDistance);

    GPXParser m_gpxParser;
    QCustomPlot* m_elevationPlot;
    MapWidget* m_mapView;
    QSlider* m_positionSlider;
    QLabel* m_elevationLabel;
    size_t m_currentPointIndex{0};
    QTimer m_updateTimer;
    int m_pendingSliderValue{0};
};
