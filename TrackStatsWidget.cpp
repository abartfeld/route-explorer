#include "TrackStatsWidget.h"
#include <QDateTime>

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
    QStringList posLabels = {"Distance:", "Elevation:", "Elevation Gain:", "Coordinates:"};
    QLabel* posLabelsArr[4];
    QWidget* positionSection = createStatsSection("Current Position", posLabelsArr, posLabels);
    m_positionTitle = posLabelsArr[0]->parentWidget()->findChild<QLabel*>("sectionTitle");
    m_distanceLabel = posLabelsArr[0];
    m_elevationLabel = posLabelsArr[1];
    m_elevGainLabel = posLabelsArr[2];
    m_coordinatesLabel = posLabelsArr[3];
    mainLayout->addWidget(positionSection);
    
    // Track information section
    QStringList trackLabels = {"Total Distance:", "Max Elevation:", "Min Elevation:", "Total Gain:", "Avg Speed:", "Start Time:", "Duration:"};
    QLabel* trackLabelsArr[7];
    QWidget* trackSection = createStatsSection("Track Information", trackLabelsArr, trackLabels);
    m_trackTitle = trackLabelsArr[0]->parentWidget()->findChild<QLabel*>("sectionTitle");
    m_totalDistanceLabel = trackLabelsArr[0];
    m_maxElevationLabel = trackLabelsArr[1];
    m_minElevationLabel = trackLabelsArr[2];
    m_totalElevGainLabel = trackLabelsArr[3];
    m_avgSpeedLabel = trackLabelsArr[4];
    m_startTimeLabel = trackLabelsArr[5];
    m_durationLabel = trackLabelsArr[6];
    mainLayout->addWidget(trackSection);
    
    // Add stretch at bottom to push everything to the top
    mainLayout->addStretch();
    
    // Initialize with empty data
    m_distanceLabel->setText("-- mi");
    m_elevationLabel->setText("-- ft");
    m_elevGainLabel->setText("-- ft");
    m_coordinatesLabel->setText("--°, --°");
    
    m_totalDistanceLabel->setText("-- mi");
    m_maxElevationLabel->setText("-- ft");
    m_minElevationLabel->setText("-- ft");
    m_totalElevGainLabel->setText("-- ft");
    m_avgSpeedLabel->setText("-- mph");
    m_startTimeLabel->setText("--");
    m_durationLabel->setText("--");
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
    double totalDistance = parser.getTotalDistance();
    double currentDistance = point.distance;
    double elevationGain = parser.getCumulativeElevationGain(pointIndex);
    
    // Update current position information
    m_distanceLabel->setText(QString("%1 / %2 mi")
                            .arg(metersToMiles(currentDistance), 0, 'f', 2)
                            .arg(metersToMiles(totalDistance), 0, 'f', 2));
    
    m_elevationLabel->setText(QString("%1 ft")
                             .arg(metersToFeet(point.elevation), 0, 'f', 1));
    
    m_elevGainLabel->setText(QString("%1 ft")
                            .arg(metersToFeet(elevationGain), 0, 'f', 1));
    
    m_coordinatesLabel->setText(QString("%1°, %2°")
                               .arg(point.coord.latitude(), 0, 'f', 6)
                               .arg(point.coord.longitude(), 0, 'f', 6));
}

void TrackStatsWidget::setTrackInfo(const GPXParser& parser) {
    const std::vector<TrackPoint>& points = parser.getPoints();
    
    if (points.empty()) {
        return;
    }
    
    double totalDistance = parser.getTotalDistance();
    double totalGain = parser.getTotalElevationGain();
    double maxElev = parser.getMaxElevation();
    double minElev = parser.getMinElevation();
    
    // Calculate average speed (assuming time data available)
    double avgSpeed = 0.0;
    QString startTime = "--";
    QString duration = "--";
    
    // Add additional statistics
    m_totalDistanceLabel->setText(QString("%1 mi").arg(metersToMiles(totalDistance), 0, 'f', 2));
    m_maxElevationLabel->setText(QString("%1 ft").arg(metersToFeet(maxElev), 0, 'f', 1));
    m_minElevationLabel->setText(QString("%1 ft").arg(metersToFeet(minElev), 0, 'f', 1));
    m_totalElevGainLabel->setText(QString("%1 ft").arg(metersToFeet(totalGain), 0, 'f', 1));
    
    // If we had timestamp information in the TrackPoint structure:
    // (placeholder for now, would need to extend GPXParser)
    m_avgSpeedLabel->setText(QString("%1 mph").arg(avgSpeed, 0, 'f', 1));
    m_startTimeLabel->setText(startTime);
    m_durationLabel->setText(duration);
}
