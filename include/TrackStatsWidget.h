#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QtMath>
#include <QDateTime>
#include <QPushButton>
#include <vector>
#include <utility>
#include "qcustomplot.h"
#include "GpxParser.h"

// Define a segment struct for track analysis
struct TrackSegment {
    enum Type { FLAT, CLIMB, DESCENT };
    Type type;
    size_t startIndex;
    size_t endIndex;
    double distance;       // in meters
    double elevationChange; // in meters
    double avgGradient;    // in percent
    double maxGradient;    // in percent
    double minGradient;    // in percent
};

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
    
    // Get analyzed segments for external use
    const std::vector<TrackSegment>& getSegments() const { return m_segments; }
    
    // Make toggleUnits public so tests can access it
    void toggleUnits();

private slots:
    void showSegmentDetails(int segmentIndex);

private:
    // UI Elements
    QLabel* m_titleLabel;
    
    // Current position information
    QLabel* m_positionTitle;
    QLabel* m_distanceLabel;
    QLabel* m_elevationLabel;
    QLabel* m_elevGainLabel;
    QLabel* m_latitudeLabel;
    QLabel* m_longitudeLabel;
    QLabel* m_gradientLabel; // New: Current gradient
    
    // Overall track information
    QLabel* m_trackTitle;
    QLabel* m_totalDistanceLabel;
    QLabel* m_maxElevationLabel;
    QLabel* m_minElevationLabel;
    QLabel* m_totalElevGainLabel;
    QLabel* m_uphillPercentLabel;
    QLabel* m_downhillPercentLabel;
    QLabel* m_flatPercentLabel;
    QLabel* m_steepestUphillLabel;   // New: Steepest uphill
    QLabel* m_steepestDownhillLabel; // New: Steepest downhill
    
    // Segment analysis section
    QLabel* m_segmentTitle;
    QCustomPlot* m_miniProfile;      // Mini elevation profile
    QWidget* m_segmentListWidget;    // Container for segment buttons
    
    // Units toggle
    QPushButton* m_unitsToggleButton;
    bool m_useMetricUnits;
    
    // Segment analysis data
    std::vector<TrackSegment> m_segments;
    QWidget* m_segmentDetailsWidget;
    QLabel* m_segmentDetailsTitle;
    QLabel* m_segmentTypeLabel;
    QLabel* m_segmentDistanceLabel;
    QLabel* m_segmentElevationLabel;
    QLabel* m_segmentGradientLabel;
    
    // Helper functions
    QWidget* createStatsSection(const QString& title, QLabel** labelArray, const QStringList& labelTexts);
    void createMiniProfile();
    void updateMiniProfile(const GPXParser& parser);
    void analyzeSegments(const GPXParser& parser);
    void createSegmentsList();
    void updateSegmentsList();
    QString getGradientColorStyle(double gradient) const;
    QString getDifficultyLabel(double gradient) const;
    
    // Segment analysis helper functions
    std::vector<double> calculateSmoothedGradients(const std::vector<TrackPoint>& points);
    std::vector<size_t> identifySegmentBoundaries(const std::vector<TrackPoint>& points, 
                                                 const std::vector<double>& smoothGradients);
    std::vector<TrackSegment> createRawSegments(const std::vector<TrackPoint>& points, 
                                              const std::vector<double>& smoothGradients,
                                              const std::vector<size_t>& boundaries);
    std::vector<TrackSegment> optimizeSegments(const std::vector<TrackSegment>& rawSegments,
                                             const std::vector<TrackPoint>& points);
    
    // Conversion functions
    double metersToMiles(double meters) const { return meters * 0.000621371; }
    double metersToFeet(double meters) const { return meters * 3.28084; }
    double metersToKilometers(double meters) const { return meters / 1000.0; }
    QString formatDistance(double meters) const;
    QString formatElevation(double meters) const;
    QString formatGradient(double gradient) const;

    // Modern UI styling helpers 
    QString getModernCardStyle() const {
        return "background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; padding: 12px;";
    }
    
    QString getModernButtonStyle(const QString& bgColor = "#2196F3", const QString& hoverColor = "#1976D2") const {
        return QString(
            "QPushButton {"
            "  background-color: %1;"
            "  color: white;"
            "  border-radius: 4px;"
            "  padding: 8px;"
            "  font-weight: bold;"
            "  border: none;"
            "  font-size: 12px;"
            "}"
            "QPushButton:hover {"
            "  background-color: %2;"
            "}"
        ).arg(bgColor, hoverColor);
    }
};
