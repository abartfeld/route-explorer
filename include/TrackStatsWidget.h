#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QtMath>
#include <QDateTime>
#include "gpxparser.h"

class TrackStatsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TrackStatsWidget(QWidget *parent = nullptr);
    ~TrackStatsWidget() override = default;

    // Update all stats at once with a track point
    void updateStats(const TrackPoint& point, int pointIndex, const GPXParser& parser);
    
    // Update only the position (when slider moves)
    void updatePosition(const TrackPoint& point, int pointIndex, const GPXParser& parser);
    
    // Set track info when a new track is loaded
    void setTrackInfo(const GPXParser& parser);

private:
    // UI Elements
    QLabel* m_titleLabel;
    
    // Current position information
    QLabel* m_positionTitle;
    QLabel* m_distanceLabel;
    QLabel* m_elevationLabel;
    QLabel* m_elevGainLabel;
    QLabel* m_latitudeLabel;  // Separate label for latitude
    QLabel* m_longitudeLabel; // Separate label for longitude
    
    // Overall track information
    QLabel* m_trackTitle;
    QLabel* m_totalDistanceLabel;
    QLabel* m_maxElevationLabel;
    QLabel* m_minElevationLabel;
    QLabel* m_totalElevGainLabel;
    QLabel* m_uphillPercentLabel;
    QLabel* m_downhillPercentLabel;
    QLabel* m_flatPercentLabel;
    
    // Helper function to create stats sections
    QWidget* createStatsSection(const QString& title, QLabel** labelArray, const QStringList& labelTexts);
    
    // Convert meters to miles
    double metersToMiles(double meters) const { return meters * 0.000621371; }
    
    // Convert meters to feet
    double metersToFeet(double meters) const { return meters * 3.28084; }
};
