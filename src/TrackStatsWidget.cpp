#include "TrackStatsWidget.h"

TrackStatsWidget::TrackStatsWidget(QWidget *parent) : 
    QWidget(parent)
{
    // Set fixed width
    setMinimumWidth(250);
    setMaximumWidth(250);
    
    // Set background color and style
    setStyleSheet("background-color: white; border-radius: 4px;");
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);
    
    // Title
    m_titleLabel = new QLabel("Track Statistics", this);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2196F3;");
    mainLayout->addWidget(m_titleLabel);
    
    // Current position section
    QStringList posLabels = {"Distance:", "Elevation:", "Elevation Gain:", "Latitude:", "Longitude:"};
    QLabel* posLabelsArr[5]; // Updated to 5 elements
    QWidget* positionSection = createStatsSection("Current Position", posLabelsArr, posLabels);
    m_positionTitle = posLabelsArr[0]->parentWidget()->findChild<QLabel*>("sectionTitle");
    m_distanceLabel = posLabelsArr[0];
    m_elevationLabel = posLabelsArr[1];
    m_elevGainLabel = posLabelsArr[2];
    m_latitudeLabel = posLabelsArr[3]; // New label for latitude
    m_longitudeLabel = posLabelsArr[4]; // New label for longitude
    mainLayout->addWidget(positionSection);
    
    // Track information section
    QStringList trackLabels = {"Total Distance:", "Max Elevation:", "Min Elevation:", "Total Gain:", "% Uphill:", "% Downhill:", "% Flat:"};
    QLabel* trackLabelsArr[7];
    QWidget* trackSection = createStatsSection("Track Information", trackLabelsArr, trackLabels);
    m_trackTitle = trackLabelsArr[0]->parentWidget()->findChild<QLabel*>("sectionTitle");
    m_totalDistanceLabel = trackLabelsArr[0];
    m_maxElevationLabel = trackLabelsArr[1];
    m_minElevationLabel = trackLabelsArr[2];
    m_totalElevGainLabel = trackLabelsArr[3];
    m_uphillPercentLabel = trackLabelsArr[4];
    m_downhillPercentLabel = trackLabelsArr[5];
    m_flatPercentLabel = trackLabelsArr[6];
    mainLayout->addWidget(trackSection);
    
    // Add stretch at bottom to push everything to the top
    mainLayout->addStretch();
    
    // Initialize with proper placeholders
    m_distanceLabel->setText("0.00 mi");
    m_elevationLabel->setText("0.0 ft");
    m_elevGainLabel->setText("0.0 ft");
    m_latitudeLabel->setText("0° 00' 00\"N");
    m_longitudeLabel->setText("0° 00' 00\"E");
    
    m_totalDistanceLabel->setText("0.00 mi");
    m_maxElevationLabel->setText("0.0 ft");
    m_minElevationLabel->setText("0.0 ft");
    m_totalElevGainLabel->setText("0.0 ft");
    m_uphillPercentLabel->setText("0.0%");
    m_downhillPercentLabel->setText("0.0%");
    m_flatPercentLabel->setText("0.0%");
}

QWidget* TrackStatsWidget::createStatsSection(const QString& title, QLabel** labelArray, const QStringList& labelTexts)
{
    QWidget* container = new QWidget(this);
    container->setObjectName("statsSection");
    
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    
    // Section title
    QLabel* titleLabel = new QLabel(title, container);
    titleLabel->setObjectName("sectionTitle");
    titleLabel->setStyleSheet("font-weight: bold; color: #424242;");
    layout->addWidget(titleLabel);
    
    // Stats grid
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(8, 4, 8, 4);
    gridLayout->setHorizontalSpacing(8);
    gridLayout->setVerticalSpacing(4);
    
    for (int i = 0; i < labelTexts.size(); ++i) {
        QLabel* keyLabel = new QLabel(labelTexts[i], container);
        keyLabel->setStyleSheet("color: #616161;");
        
        QLabel* valueLabel = new QLabel("", container);
        valueLabel->setStyleSheet("color: #212121; font-weight: bold;");
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        
        gridLayout->addWidget(keyLabel, i, 0);
        gridLayout->addWidget(valueLabel, i, 1);
        
        labelArray[i] = valueLabel;
    }
    
    layout->addLayout(gridLayout);
    
    return container;
}

void TrackStatsWidget::updateStats(const TrackPoint& point, int pointIndex, const GPXParser& parser) {
    // Update position-specific stats
    updatePosition(point, pointIndex, parser);
    
    // Update track-specific stats (though these don't change with position)
    setTrackInfo(parser);
}

void TrackStatsWidget::updatePosition(const TrackPoint& point, int pointIndex, const GPXParser& parser) {
    double totalDistance = parser.getTotalDistance(); // in meters
    double currentDistance = point.distance; // in meters
    double elevationGain = parser.getCumulativeElevationGain(pointIndex); // in meters
    
    // Update current position information
    m_distanceLabel->setText(QString("%1 / %2 mi")
                            .arg(metersToMiles(currentDistance), 0, 'f', 2)
                            .arg(metersToMiles(totalDistance), 0, 'f', 2));
    
    m_elevationLabel->setText(QString("%1 ft")
                             .arg(metersToFeet(point.elevation), 0, 'f', 1));
    
    m_elevGainLabel->setText(QString("%1 ft")
                            .arg(metersToFeet(elevationGain), 0, 'f', 1));
    
    // Convert decimal degrees to degrees, minutes, seconds format for latitude
    double lat = qAbs(point.coord.latitude());
    int latDeg = qFloor(lat);
    int latMin = qFloor((lat - latDeg) * 60.0);
    double latSec = (lat - latDeg - latMin / 60.0) * 3600.0;
    QString latDir = point.coord.latitude() >= 0 ? "N" : "S";
    
    // Convert decimal degrees to degrees, minutes, seconds format for longitude
    double lon = qAbs(point.coord.longitude());
    int lonDeg = qFloor(lon);
    int lonMin = qFloor((lon - lonDeg) * 60.0);
    double lonSec = (lon - lonDeg - lonMin / 60.0) * 3600.0;
    QString lonDir = point.coord.longitude() >= 0 ? "E" : "W";
    
    // Set latitude and longitude on separate lines
    m_latitudeLabel->setText(
        QString("%1° %2' %3\"%4")
        .arg(latDeg)
        .arg(latMin, 2, 10, QChar('0'))
        .arg(latSec, 4, 'f', 1, QChar('0'))
        .arg(latDir)
    );
    
    m_longitudeLabel->setText(
        QString("%1° %2' %3\"%4")
        .arg(lonDeg)
        .arg(lonMin, 2, 10, QChar('0'))
        .arg(lonSec, 4, 'f', 1, QChar('0'))
        .arg(lonDir)
    );
    
    // Set a slightly smaller font size to ensure the coordinates fit
    m_latitudeLabel->setStyleSheet("color: #212121; font-weight: bold; font-size: 8pt;");
    m_longitudeLabel->setStyleSheet("color: #212121; font-weight: bold; font-size: 8pt;");
}

// Optimization: Cache percentages to avoid recalculation when track hasn't changed
void TrackStatsWidget::setTrackInfo(const GPXParser& parser) {
    const std::vector<TrackPoint>& points = parser.getPoints();
    
    static size_t lastPointsSize = 0;
    static double cachedUphillPercent = 0.0;
    static double cachedDownhillPercent = 0.0;
    static double cachedFlatPercent = 0.0;
    
    if (points.empty()) {
        // Clear all values when no points
        m_totalDistanceLabel->setText("0.00 mi");
        m_maxElevationLabel->setText("0.0 ft");
        m_minElevationLabel->setText("0.0 ft");
        m_totalElevGainLabel->setText("0.0 ft");
        m_uphillPercentLabel->setText("0.0%");
        m_downhillPercentLabel->setText("0.0%");
        m_flatPercentLabel->setText("0.0%");
        
        // Reset cache
        lastPointsSize = 0;
        return;
    }
    
    // Get raw values in meters
    double totalDistance = parser.getTotalDistance(); // In meters
    double totalGain = parser.getTotalElevationGain(); // In meters
    double maxElev = parser.getMaxElevation(); // In meters
    double minElev = parser.getMinElevation(); // In meters
    
    // Check if we need to recalculate percentages
    bool recalculatePercentages = (lastPointsSize != points.size());
    
    double uphillPercent = cachedUphillPercent;
    double downhillPercent = cachedDownhillPercent;
    double flatPercent = cachedFlatPercent;
    
    if (recalculatePercentages) {
        // Calculate uphill, downhill, and flat percentages
        double uphillDistance = 0.0;
        double downhillDistance = 0.0;
        double flatDistance = 0.0;
        
        // Skip first point since we can't calculate grade yet
        for (size_t i = 1; i < points.size(); i++) {
            double segmentDistance = points[i].distance - points[i-1].distance;
            double elevationChange = points[i].elevation - points[i-1].elevation;
            
            // Calculate grade as a percentage (rise/run * 100)
            double grade = 0.0;
            if (segmentDistance > 0.0) {
                grade = (elevationChange / segmentDistance) * 100.0;
            }
            
            // Categorize the segment based on grade thresholds
            if (grade > 1.0) {
                uphillDistance += segmentDistance;
            } else if (grade < -1.0) {
                downhillDistance += segmentDistance;
            } else {
                flatDistance += segmentDistance;
            }
        }
        
        // Calculate percentages of total distance
        uphillPercent = (totalDistance > 0) ? (uphillDistance / totalDistance) * 100.0 : 0.0;
        downhillPercent = (totalDistance > 0) ? (downhillDistance / totalDistance) * 100.0 : 0.0;
        flatPercent = (totalDistance > 0) ? (flatDistance / totalDistance) * 100.0 : 0.0;
        
        // Update cache
        cachedUphillPercent = uphillPercent;
        cachedDownhillPercent = downhillPercent;
        cachedFlatPercent = flatPercent;
        lastPointsSize = points.size();
    }
    
    // Update statistics - convert metric to imperial units here
    m_totalDistanceLabel->setText(QString("%1 mi").arg(metersToMiles(totalDistance), 0, 'f', 2));
    m_maxElevationLabel->setText(QString("%1 ft").arg(metersToFeet(maxElev), 0, 'f', 1));
    m_minElevationLabel->setText(QString("%1 ft").arg(metersToFeet(minElev), 0, 'f', 1));
    m_totalElevGainLabel->setText(QString("%1 ft").arg(metersToFeet(totalGain), 0, 'f', 1));
    m_uphillPercentLabel->setText(QString("%1%").arg(uphillPercent, 0, 'f', 1));
    m_downhillPercentLabel->setText(QString("%1%").arg(downhillPercent, 0, 'f', 1));
    m_flatPercentLabel->setText(QString("%1%").arg(flatPercent, 0, 'f', 1));
}
