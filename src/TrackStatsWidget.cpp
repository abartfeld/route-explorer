#include "TrackStatsWidget.h"
#include <QPushButton>
#include <QScrollArea>
#include <deque>

TrackStatsWidget::TrackStatsWidget(QWidget *parent) : 
    QWidget(parent),
    m_useMetricUnits(false) // Default to imperial units
{
    // Set fixed width
    setMinimumWidth(280);
    setMaximumWidth(280);
    
    // Set overall background and modern card-like appearance
    setStyleSheet("background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; box-shadow: 0 2px 4px rgba(0,0,0,0.1);");
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);
    
    // Title with more modern styling
    m_titleLabel = new QLabel("Track Statistics", this);
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1976D2;");
    mainLayout->addWidget(m_titleLabel);
    
    // Current position section
    QStringList posLabels = {"Distance:", "Elevation:", "Elevation Gain:", "Gradient:", "Latitude:", "Longitude:"};
    QLabel* posLabelsArr[6];
    QWidget* positionSection = createStatsSection("Current Position", posLabelsArr, posLabels);
    m_positionTitle = posLabelsArr[0]->parentWidget()->findChild<QLabel*>("sectionTitle");
    m_distanceLabel = posLabelsArr[0];
    m_elevationLabel = posLabelsArr[1];
    m_elevGainLabel = posLabelsArr[2];
    m_gradientLabel = posLabelsArr[3];
    m_latitudeLabel = posLabelsArr[4];
    m_longitudeLabel = posLabelsArr[5];
    mainLayout->addWidget(positionSection);
    
    // Track information section
    QStringList trackLabels = {"Total Distance:", "Max Elevation:", "Min Elevation:", "Total Gain:", 
                              "% Uphill:", "% Downhill:", "% Flat:", "Steepest Uphill:", "Steepest Downhill:"};
    QLabel* trackLabelsArr[9]; // Updated to 9 elements
    QWidget* trackSection = createStatsSection("Track Information", trackLabelsArr, trackLabels);
    m_trackTitle = trackLabelsArr[0]->parentWidget()->findChild<QLabel*>("sectionTitle");
    m_totalDistanceLabel = trackLabelsArr[0];
    m_maxElevationLabel = trackLabelsArr[1];
    m_minElevationLabel = trackLabelsArr[2];
    m_totalElevGainLabel = trackLabelsArr[3];
    m_uphillPercentLabel = trackLabelsArr[4];
    m_downhillPercentLabel = trackLabelsArr[5];
    m_flatPercentLabel = trackLabelsArr[6];
    m_steepestUphillLabel = trackLabelsArr[7];
    m_steepestDownhillLabel = trackLabelsArr[8];
    mainLayout->addWidget(trackSection);
    
    // Create mini elevation profile
    m_miniProfile = new QCustomPlot(this);
    m_miniProfile->setMinimumHeight(100);
    m_miniProfile->setBackground(QColor("#f8f8f8"));
    m_miniProfile->xAxis->setVisible(false);
    m_miniProfile->yAxis->setVisible(false);
    m_miniProfile->setInteraction(QCP::iRangeDrag, false);
    m_miniProfile->setInteraction(QCP::iRangeZoom, false);
    
    // Add graph for elevation profile
    m_miniProfile->addGraph();
    m_miniProfile->graph(0)->setPen(QPen(QColor("#2196F3"), 2));
    m_miniProfile->graph(0)->setBrush(QBrush(QColor(33, 150, 243, 50)));
    
    // Add current position marker
    m_miniProfile->addGraph();
    m_miniProfile->graph(1)->setLineStyle(QCPGraph::lsNone);
    m_miniProfile->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, QColor("#F44336"), 6));
    
    // Segments section
    QWidget* segmentContainer = new QWidget(this);
    QVBoxLayout* segmentLayout = new QVBoxLayout(segmentContainer);
    segmentLayout->setContentsMargins(0, 0, 0, 0);
    segmentLayout->setSpacing(8);
    
    m_segmentTitle = new QLabel("Segments", segmentContainer);
    m_segmentTitle->setObjectName("sectionTitle");
    m_segmentTitle->setStyleSheet("font-weight: bold; color: #424242;");
    segmentLayout->addWidget(m_segmentTitle);
    
    // Add mini profile to segments section
    segmentLayout->addWidget(m_miniProfile);
    
    // Create scrollable segment list
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMaximumHeight(150);
    
    m_segmentListWidget = new QWidget(scrollArea);
    m_segmentListWidget->setLayout(new QVBoxLayout());
    m_segmentListWidget->layout()->setContentsMargins(0, 0, 0, 0);
    m_segmentListWidget->layout()->setSpacing(4);
    
    scrollArea->setWidget(m_segmentListWidget);
    segmentLayout->addWidget(scrollArea);
    
    // Segment details section
    m_segmentDetailsWidget = new QWidget(this);
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_segmentDetailsWidget);
    detailsLayout->setContentsMargins(8, 8, 8, 8);
    detailsLayout->setSpacing(4);
    
    m_segmentDetailsTitle = new QLabel("Segment Details", m_segmentDetailsWidget);
    m_segmentDetailsTitle->setStyleSheet("font-weight: bold; color: #424242;");
    detailsLayout->addWidget(m_segmentDetailsTitle);
    
    QGridLayout* detailsGrid = new QGridLayout();
    detailsGrid->setContentsMargins(0, 4, 0, 0);
    detailsGrid->setHorizontalSpacing(8);
    detailsGrid->setVerticalSpacing(4);
    
    QLabel* typeLabel = new QLabel("Type:", m_segmentDetailsWidget);
    typeLabel->setStyleSheet("color: #616161;");
    m_segmentTypeLabel = new QLabel("-", m_segmentDetailsWidget);
    m_segmentTypeLabel->setStyleSheet("color: #212121; font-weight: bold;");
    
    QLabel* distLabel = new QLabel("Distance:", m_segmentDetailsWidget);
    distLabel->setStyleSheet("color: #616161;");
    m_segmentDistanceLabel = new QLabel("-", m_segmentDetailsWidget);
    m_segmentDistanceLabel->setStyleSheet("color: #212121; font-weight: bold;");
    
    QLabel* elevLabel = new QLabel("Elevation:", m_segmentDetailsWidget);
    elevLabel->setStyleSheet("color: #616161;");
    m_segmentElevationLabel = new QLabel("-", m_segmentDetailsWidget);
    m_segmentElevationLabel->setStyleSheet("color: #212121; font-weight: bold;");
    
    QLabel* gradLabel = new QLabel("Gradient:", m_segmentDetailsWidget);
    gradLabel->setStyleSheet("color: #616161;");
    m_segmentGradientLabel = new QLabel("-", m_segmentDetailsWidget);
    m_segmentGradientLabel->setStyleSheet("color: #212121; font-weight: bold;");
    
    detailsGrid->addWidget(typeLabel, 0, 0);
    detailsGrid->addWidget(m_segmentTypeLabel, 0, 1);
    detailsGrid->addWidget(distLabel, 1, 0);
    detailsGrid->addWidget(m_segmentDistanceLabel, 1, 1);
    detailsGrid->addWidget(elevLabel, 2, 0);
    detailsGrid->addWidget(m_segmentElevationLabel, 2, 1);
    detailsGrid->addWidget(gradLabel, 3, 0);
    detailsGrid->addWidget(m_segmentGradientLabel, 3, 1);
    
    detailsLayout->addLayout(detailsGrid);
    m_segmentDetailsWidget->setVisible(false);
    segmentLayout->addWidget(m_segmentDetailsWidget);
    
    mainLayout->addWidget(segmentContainer);
    
    // Units toggle button with more modern styling
    m_unitsToggleButton = new QPushButton("Switch to Metric", this);
    m_unitsToggleButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  font-weight: bold;"
        "  border: none;"
        "  font-size: 12px;"
        "  box-shadow: 0 2px 2px rgba(0,0,0,0.1);"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0D47A1;"
        "  box-shadow: 0 1px 1px rgba(0,0,0,0.1);"
        "}"
    );
    connect(m_unitsToggleButton, &QPushButton::clicked, this, &TrackStatsWidget::toggleUnits);
    mainLayout->addWidget(m_unitsToggleButton);
    
    // Add stretch at bottom to push everything to the top
    mainLayout->addStretch();
    
    // Initialize with proper placeholders
    m_distanceLabel->setText("0.00 mi");
    m_elevationLabel->setText("0.0 ft");
    m_elevGainLabel->setText("0.0 ft");
    m_gradientLabel->setText("0.0%");
    m_latitudeLabel->setText("0° 00' 00\"N");
    m_longitudeLabel->setText("0° 00' 00\"E");
    
    m_totalDistanceLabel->setText("0.00 mi");
    m_maxElevationLabel->setText("0.0 ft");
    m_minElevationLabel->setText("0.0 ft");
    m_totalElevGainLabel->setText("0.0 ft");
    m_uphillPercentLabel->setText("0.0%");
    m_downhillPercentLabel->setText("0.0%");
    m_flatPercentLabel->setText("0.0%");
    m_steepestUphillLabel->setText("0.0%");
    m_steepestDownhillLabel->setText("0.0%");
}

QWidget* TrackStatsWidget::createStatsSection(const QString& title, QLabel** labelArray, const QStringList& labelTexts) 
{
    QWidget* container = new QWidget(this);
    container->setObjectName("statsSection");
    
    // Add a subtle background gradient for modern look
    container->setStyleSheet("background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #f8f9fa); border-radius: 8px;");
    
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 12, 12, 8);
    layout->setSpacing(10);
    
    // Section title with modern styling
    QLabel* titleLabel = new QLabel(title, container);
    titleLabel->setObjectName("sectionTitle");
    titleLabel->setStyleSheet("font-weight: bold; color: #1976D2; font-size: 14px;");
    layout->addWidget(titleLabel);
    
    // Add separator line
    QFrame* line = new QFrame(container);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("border: none; background-color: #e0e0e0; max-height: 1px;");
    layout->addWidget(line);
    
    // Stats grid
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 8, 0, 4);
    gridLayout->setHorizontalSpacing(12);
    gridLayout->setVerticalSpacing(8);
    
    for (int i = 0; i < labelTexts.size(); ++i) {
        QLabel* keyLabel = new QLabel(labelTexts[i], container);
        keyLabel->setStyleSheet("color: #616161; font-size: 11px;");
        
        QLabel* valueLabel = new QLabel("", container);
        valueLabel->setStyleSheet("color: #212121; font-weight: bold; font-size: 12px;");
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
    
    // Update mini profile marker position
    if (m_miniProfile->graph(0)->dataCount() > 0) {
        QVector<double> x, y;
        x.append(m_useMetricUnits ? metersToKilometers(point.distance) : metersToMiles(point.distance));
        y.append(m_useMetricUnits ? point.elevation : metersToFeet(point.elevation));
        m_miniProfile->graph(1)->setData(x, y);
        m_miniProfile->replot(QCustomPlot::rpQueuedReplot);
    }
}

void TrackStatsWidget::updatePosition(const TrackPoint& point, int pointIndex, const GPXParser& parser) {
    double totalDistance = parser.getTotalDistance(); // in meters
    double currentDistance = point.distance; // in meters
    double elevationGain = parser.getCumulativeElevationGain(pointIndex); // in meters
    
    // Calculate current gradient
    double currentGradient = 0.0;
    const std::vector<TrackPoint>& points = parser.getPoints();
    if (pointIndex > 0 && pointIndex < static_cast<int>(points.size())) {
        double prevDistance = points[pointIndex-1].distance;
        double prevElevation = points[pointIndex-1].elevation;
        double distDiff = point.distance - prevDistance;
        double elevDiff = point.elevation - prevElevation;
        
        if (distDiff > 0) {
            currentGradient = (elevDiff / distDiff) * 100.0;
        }
    }
    
    // Update current position information - show only current distance (not total)
    m_distanceLabel->setText(formatDistance(currentDistance));
    m_elevationLabel->setText(formatElevation(point.elevation));
    m_elevGainLabel->setText(formatElevation(elevationGain));
    
    // Set gradient with color coding based on steepness
    m_gradientLabel->setText(formatGradient(currentGradient));
    m_gradientLabel->setStyleSheet(getGradientColorStyle(currentGradient));
    
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
    
    // Highlight active segment in the list
    for (size_t i = 0; i < m_segments.size(); i++) {
        if (pointIndex >= static_cast<int>(m_segments[i].startIndex) && 
            pointIndex <= static_cast<int>(m_segments[i].endIndex)) {
            
            // Find the corresponding button and update its style
            QWidget* segmentList = m_segmentListWidget;
            for (int j = 0; j < segmentList->layout()->count(); j++) {
                QPushButton* btn = qobject_cast<QPushButton*>(segmentList->layout()->itemAt(j)->widget());
                if (btn) {
                    if (j == static_cast<int>(i)) {
                        btn->setStyleSheet("background-color: #e3f2fd; border: 1px solid #2196F3;");
                    } else {
                        btn->setStyleSheet("");
                    }
                }
            }
            
            // Show details for this segment
            showSegmentDetails(i);
            break;
        }
    }
}

void TrackStatsWidget::setTrackInfo(const GPXParser& parser) {
    const std::vector<TrackPoint>& points = parser.getPoints();
    
    if (points.empty()) {
        // Clear all values when no points
        m_totalDistanceLabel->setText(m_useMetricUnits ? "0.00 km" : "0.00 mi");
        m_maxElevationLabel->setText(m_useMetricUnits ? "0.0 m" : "0.0 ft");
        m_minElevationLabel->setText(m_useMetricUnits ? "0.0 m" : "0.0 ft");
        m_totalElevGainLabel->setText(m_useMetricUnits ? "0.0 m" : "0.0 ft");
        m_uphillPercentLabel->setText("0.0%");
        m_downhillPercentLabel->setText("0.0%");
        m_flatPercentLabel->setText("0.0%");
        m_steepestUphillLabel->setText("0.0%");
        m_steepestDownhillLabel->setText("0.0%");
        
        // Clear mini profile
        m_miniProfile->graph(0)->data()->clear();
        m_miniProfile->graph(1)->data()->clear();
        m_miniProfile->replot();
        
        // Clear segments
        m_segments.clear();
        QLayoutItem* child;
        while ((child = m_segmentListWidget->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        
        m_segmentDetailsWidget->setVisible(false);
        return;
    }
    
    // Analyze segments if this is new data
    static size_t lastPointsCount = 0;
    if (lastPointsCount != points.size()) {
        analyzeSegments(parser);
        updateMiniProfile(parser);
        updateSegmentsList();
        lastPointsCount = points.size();
    }
    
    // Get raw values in meters
    double totalDistance = parser.getTotalDistance(); // In meters
    double totalGain = parser.getTotalElevationGain(); // In meters
    double maxElev = parser.getMaxElevation(); // In meters
    double minElev = parser.getMinElevation(); // In meters
    
    // Calculate steepest sections
    double steepestUphill = 0.0;
    double steepestDownhill = 0.0;
    
    for (const auto& segment : m_segments) {
        if (segment.type == TrackSegment::CLIMB) {
            steepestUphill = std::max(steepestUphill, segment.maxGradient);
        } else if (segment.type == TrackSegment::DESCENT) {
            steepestDownhill = std::min(steepestDownhill, segment.minGradient);
        }
    }
    
    // Calculate uphill, downhill, and flat percentages
    double uphillDistance = 0.0;
    double downhillDistance = 0.0;
    double flatDistance = 0.0;
    
    // Use the segments for this calculation
    for (const auto& segment : m_segments) {
        switch (segment.type) {
            case TrackSegment::CLIMB:
                uphillDistance += segment.distance;
                break;
            case TrackSegment::DESCENT:
                downhillDistance += segment.distance;
                break;
            case TrackSegment::FLAT:
                flatDistance += segment.distance;
                break;
        }
    }
    
    // Calculate percentages of total distance
    double totalSegmentDistance = uphillDistance + downhillDistance + flatDistance;
    double uphillPercent = (totalSegmentDistance > 0) ? (uphillDistance / totalSegmentDistance) * 100.0 : 0.0;
    double downhillPercent = (totalSegmentDistance > 0) ? (downhillDistance / totalSegmentDistance) * 100.0 : 0.0;
    double flatPercent = (totalSegmentDistance > 0) ? (flatDistance / totalSegmentDistance) * 100.0 : 0.0;
    
    // Ensure percentages add up to 100% exactly
    double totalPercent = uphillPercent + downhillPercent + flatPercent;
    if (totalPercent > 0) {
        uphillPercent = (uphillPercent / totalPercent) * 100.0;
        downhillPercent = (downhillPercent / totalPercent) * 100.0;
        flatPercent = (flatPercent / totalPercent) * 100.0;
    }
    
    // Update statistics with proper units
    m_totalDistanceLabel->setText(formatDistance(totalDistance));
    m_maxElevationLabel->setText(formatElevation(maxElev));
    m_minElevationLabel->setText(formatElevation(minElev));
    m_totalElevGainLabel->setText(formatElevation(totalGain));
    m_uphillPercentLabel->setText(QString("%1%").arg(uphillPercent, 0, 'f', 1));
    m_downhillPercentLabel->setText(QString("%1%").arg(downhillPercent, 0, 'f', 1));
    m_flatPercentLabel->setText(QString("%1%").arg(flatPercent, 0, 'f', 1));
    
    // Update gradient info with color coding
    m_steepestUphillLabel->setText(formatGradient(steepestUphill));
    m_steepestUphillLabel->setStyleSheet(getGradientColorStyle(steepestUphill));
    
    m_steepestDownhillLabel->setText(formatGradient(steepestDownhill));
    m_steepestDownhillLabel->setStyleSheet(getGradientColorStyle(steepestDownhill));
}

void TrackStatsWidget::toggleUnits() {
    m_useMetricUnits = !m_useMetricUnits;
    m_unitsToggleButton->setText(m_useMetricUnits ? "Switch to Imperial" : "Switch to Metric");
    
    // Update all displays with new units
    const QMetaObject* meta = metaObject();
    int updateStatsIndex = meta->indexOfMethod("updateStats(const TrackPoint&,int,const GPXParser&)");
    if (updateStatsIndex != -1) {
        QMetaMethod method = meta->method(updateStatsIndex);
        method.invoke(this, Qt::DirectConnection);
    }
    
    // Update the mini profile
    updateMiniProfile(GPXParser());
}

void TrackStatsWidget::analyzeSegments(const GPXParser& parser) {
    const std::vector<TrackPoint>& points = parser.getPoints();
    if (points.size() < 2) {
        m_segments.clear();
        return;
    }
    
    m_segments.clear();
    
    // Calculate smoothed gradients for the entire track
    std::vector<double> smoothGradients = calculateSmoothedGradients(points);
    
    // Identify potential segment boundaries based on gradient changes
    std::vector<size_t> segmentBoundaries = identifySegmentBoundaries(points, smoothGradients);
    
    // Create initial segments from boundaries
    std::vector<TrackSegment> rawSegments = createRawSegments(points, smoothGradients, segmentBoundaries);
    
    // Post-process segments: merge similar adjacent segments, remove tiny segments
    m_segments = optimizeSegments(rawSegments, points);
}

std::vector<double> TrackStatsWidget::calculateSmoothedGradients(const std::vector<TrackPoint>& points) {
    // Parameters for gradient smoothing
    const int WINDOW_SIZE = 9; // Increased window size for smoother gradients
    
    // Step 1: Calculate raw point-to-point gradients
    std::vector<double> rawGradients(points.size(), 0.0);
    for (size_t i = 1; i < points.size(); i++) {
        double distDiff = points[i].distance - points[i-1].distance;
        double elevDiff = points[i].elevation - points[i-1].elevation;
        if (distDiff > 0) {
            rawGradients[i] = (elevDiff / distDiff) * 100.0;
        }
    }
    
    // Step 2: Apply weighted moving average smoothing to gradients
    std::vector<double> smoothGradients(points.size(), 0.0);
    int halfWindow = WINDOW_SIZE / 2;
    
    for (size_t i = 0; i < points.size(); i++) {
        double sum = 0.0;
        double totalWeight = 0.0;
        
        // Calculate weighted moving average with Gaussian-like weighting
        for (int j = -halfWindow; j <= halfWindow; j++) {
            size_t idx = i + j;
            if (idx >= 0 && idx < points.size()) {
                // Gaussian-like weight function - higher weight for closer points
                double weight = exp(-0.5 * pow(j / (halfWindow / 2.0), 2));
                sum += rawGradients[idx] * weight;
                totalWeight += weight;
            }
        }
        
        smoothGradients[i] = (totalWeight > 0) ? (sum / totalWeight) : 0.0;
    }
    
    return smoothGradients;
}

std::vector<size_t> TrackStatsWidget::identifySegmentBoundaries(
    const std::vector<TrackPoint>& points, 
    const std::vector<double>& smoothGradients)
{
    // Parameters for segment detection
    const double GRADIENT_THRESHOLD_FLAT = 1.0;  // % grade threshold
    const double SEGMENT_CHANGE_THRESHOLD = 2.0; // Threshold for segment type change
    const double MIN_SEGMENT_DISTANCE = 402.336; // 0.25 miles in meters
    
    std::vector<size_t> boundaries;
    boundaries.push_back(0); // Start with first point
    
    // Define segment type enumeration for local use
    enum class GradientType { FLAT, CLIMB, DESCENT };
    
    // Determine initial segment type
    GradientType currentType = GradientType::FLAT;
    if (points.size() > 1) {
        if (smoothGradients[1] > GRADIENT_THRESHOLD_FLAT) {
            currentType = GradientType::CLIMB;
        } else if (smoothGradients[1] < -GRADIENT_THRESHOLD_FLAT) {
            currentType = GradientType::DESCENT;
        }
    }
    
    // Track persistent gradient changes
    const int STABILITY_WINDOW = 5;
    std::vector<GradientType> recentTypes(STABILITY_WINDOW, currentType);
    
    // Process points to find major transitions (prefer longer segments)
    for (size_t i = 1; i < points.size(); i++) {
        // Determine current point's gradient type
        GradientType pointType = GradientType::FLAT;
        if (smoothGradients[i] > GRADIENT_THRESHOLD_FLAT) {
            pointType = GradientType::CLIMB;
        } else if (smoothGradients[i] < -GRADIENT_THRESHOLD_FLAT) {
            pointType = GradientType::DESCENT;
        }
        
        // Update rolling window of recent types
        for (int j = 0; j < STABILITY_WINDOW - 1; j++) {
            recentTypes[j] = recentTypes[j + 1];
        }
        recentTypes[STABILITY_WINDOW - 1] = pointType;
        
        // Check if there's a stable change in gradient type
        bool typeChange = false;
        if (pointType != currentType) {
            // Count occurrences of new type in the window
            int newTypeCount = 0;
            for (const auto& type : recentTypes) {
                if (type == pointType) newTypeCount++;
            }
            
            // Require majority of window to be the new type
            if (newTypeCount > STABILITY_WINDOW / 2) {
                typeChange = true;
            }
        }
        
        // If we detect a real segment transition and have enough distance
        if (typeChange && (i > 0) && 
            (points[i].distance - points[boundaries.back()].distance >= MIN_SEGMENT_DISTANCE)) {
            
            // Check if gradient change is significant enough
            double avgCurrentGradient = 0.0;
            double avgNewGradient = 0.0;
            
            // Calculate average gradient of current segment
            for (size_t j = boundaries.back(); j < i; j++) {
                avgCurrentGradient += smoothGradients[j];
            }
            avgCurrentGradient /= (i - boundaries.back());
            
            // Calculate average gradient looking ahead
            for (size_t j = i; j < std::min(i + STABILITY_WINDOW, points.size()); j++) {
                avgNewGradient += smoothGradients[j];
            }
            avgNewGradient /= std::min(STABILITY_WINDOW, static_cast<int>(points.size() - i));
            
            // Only add boundary if gradient change is significant
            if (std::abs(avgNewGradient - avgCurrentGradient) >= SEGMENT_CHANGE_THRESHOLD) {
                boundaries.push_back(i);
                currentType = pointType;
            }
        }
    }
    
    // Add final point if it's not already included
    if (boundaries.back() != points.size() - 1) {
        boundaries.push_back(points.size() - 1);
    }
    
    return boundaries;
}

std::vector<TrackSegment> TrackStatsWidget::createRawSegments(
    const std::vector<TrackPoint>& points,
    const std::vector<double>& smoothGradients,
    const std::vector<size_t>& boundaries)
{
    const double GRADIENT_THRESHOLD_FLAT = 1.0;  // % grade threshold
    const size_t MIN_SEGMENT_POINTS = 5;         // minimum points in a segment
    const double MIN_SEGMENT_DISTANCE = 402.336; // 0.25 miles in meters
    
    std::vector<TrackSegment> segments;
    
    // Create segments from adjacent boundary pairs
    for (size_t i = 0; i < boundaries.size() - 1; i++) {
        size_t startIdx = boundaries[i];
        size_t endIdx = boundaries[i+1];
        
        // Ensure segment has minimum length and points
        if (endIdx - startIdx < MIN_SEGMENT_POINTS) continue;
        
        double segmentDistance = points[endIdx].distance - points[startIdx].distance;
        if (segmentDistance < MIN_SEGMENT_DISTANCE) continue;
        
        // Calculate segment properties
        double segmentElevChange = points[endIdx].elevation - points[startIdx].elevation;
        
        // Calculate gradient statistics
        double sumGradient = 0.0;
        double maxGradient = -100.0;
        double minGradient = 100.0;
        
        for (size_t j = startIdx; j <= endIdx; j++) {
            sumGradient += smoothGradients[j];
            maxGradient = std::max(maxGradient, smoothGradients[j]);
            minGradient = std::min(minGradient, smoothGradients[j]);
        }
        
        double avgGradient = sumGradient / (endIdx - startIdx + 1);
        
        // Determine segment type based on average gradient
        TrackSegment::Type segmentType = TrackSegment::FLAT;
        if (avgGradient > GRADIENT_THRESHOLD_FLAT) {
            segmentType = TrackSegment::CLIMB;
        } else if (avgGradient < -GRADIENT_THRESHOLD_FLAT) {
            segmentType = TrackSegment::DESCENT;
        }
        
        // Create and add segment
        TrackSegment segment;
        segment.type = segmentType;
        segment.startIndex = startIdx;
        segment.endIndex = endIdx;
        segment.distance = segmentDistance;
        segment.elevationChange = segmentElevChange;
        segment.avgGradient = avgGradient;
        segment.maxGradient = maxGradient;
        segment.minGradient = minGradient;
        
        segments.push_back(segment);
    }
    
    return segments;
}

std::vector<TrackSegment> TrackStatsWidget::optimizeSegments(
    const std::vector<TrackSegment>& rawSegments,
    const std::vector<TrackPoint>& points)
{
    if (rawSegments.empty()) return rawSegments;
    
    // Parameters for segment merging
    const double SIMILAR_GRADIENT_THRESHOLD = 2.0; // Merge if gradients differ by less than this
    const double TINY_SEGMENT_THRESHOLD = 200.0;   // Meters - absorb segments smaller than this
    
    // First pass: merge adjacent segments of the same type with similar gradients
    std::vector<TrackSegment> mergedSegments;
    TrackSegment currentSegment = rawSegments[0];
    
    for (size_t i = 1; i < rawSegments.size(); i++) {
        const TrackSegment& nextSegment = rawSegments[i];
        bool shouldMerge = false;
        
        // Merge criteria:
        // 1. Same segment type
        // 2. Similar gradient OR very short next segment
        // 3. No type is "overwhelmed" by very different gradient
        if (nextSegment.type == currentSegment.type) {
            double gradientDiff = std::abs(nextSegment.avgGradient - currentSegment.avgGradient);
            
            // Always merge flat segments of the same type
            if (nextSegment.type == TrackSegment::FLAT) {
                shouldMerge = true;
            }
            // For climbs/descents, ensure gradients are similar 
            // OR next segment is tiny (and should be absorbed)
            else if (gradientDiff < SIMILAR_GRADIENT_THRESHOLD || 
                     nextSegment.distance < TINY_SEGMENT_THRESHOLD) {
                shouldMerge = true;
            }
        }
        // Consider absorbing tiny segments into surrounding ones
        else if (nextSegment.distance < TINY_SEGMENT_THRESHOLD) {
            shouldMerge = true;
        }
        
        if (shouldMerge) {
            // Merge by extending current segment
            currentSegment.endIndex = nextSegment.endIndex;
            currentSegment.distance += nextSegment.distance;
            currentSegment.elevationChange += nextSegment.elevationChange;
            
            // Update min/max gradients
            currentSegment.maxGradient = std::max(currentSegment.maxGradient, nextSegment.maxGradient);
            currentSegment.minGradient = std::min(currentSegment.minGradient, nextSegment.minGradient);
            
            // Recalculate average gradient for the whole segment
            currentSegment.avgGradient = currentSegment.elevationChange / currentSegment.distance * 100.0;
        } else {
            // Can't merge - add current segment and start a new one
            mergedSegments.push_back(currentSegment);
            currentSegment = nextSegment;
        }
    }
    
    // Add the last segment
    mergedSegments.push_back(currentSegment);
    
    // Second pass: re-classify segments based on their actual average gradient
    // (in case merging changed the character of the segment)
    const double GRADIENT_THRESHOLD_FLAT = 1.0;
    
    for (auto& segment : mergedSegments) {
        // Recalculate true average gradient
        double elevChange = points[segment.endIndex].elevation - points[segment.startIndex].elevation;
        double distance = points[segment.endIndex].distance - points[segment.startIndex].distance;
        
        if (distance > 0) {
            segment.avgGradient = (elevChange / distance) * 100.0;
            
            // Re-classify segment type based on average gradient
            if (segment.avgGradient > GRADIENT_THRESHOLD_FLAT) {
                segment.type = TrackSegment::CLIMB;
            } else if (segment.avgGradient < -GRADIENT_THRESHOLD_FLAT) {
                segment.type = TrackSegment::DESCENT;
            } else {
                segment.type = TrackSegment::FLAT;
            }
        }
    }
    
    return mergedSegments;
}

void TrackStatsWidget::updateMiniProfile(const GPXParser& parser) {
    const std::vector<TrackPoint>& points = parser.getPoints();
    
    // Clear existing data
    m_miniProfile->graph(0)->data()->clear();
    m_miniProfile->graph(1)->data()->clear();
    
    if (points.empty()) {
        m_miniProfile->replot();
        return;
    }
    
    // Prepare data for the plot
    QVector<double> x, y;
    x.reserve(points.size());
    y.reserve(points.size());
    
    // Fill in data points
    for (const auto& point : points) {
        double xVal = m_useMetricUnits ? metersToKilometers(point.distance) : metersToMiles(point.distance);
        double yVal = m_useMetricUnits ? point.elevation : metersToFeet(point.elevation);
        x.append(xVal);
        y.append(yVal);
    }
    
    // Set the data
    m_miniProfile->graph(0)->setData(x, y);
    
    // Set axis ranges with better styling
    double minElev = m_useMetricUnits ? parser.getMinElevation() : metersToFeet(parser.getMinElevation());
    double maxElev = m_useMetricUnits ? parser.getMaxElevation() : metersToFeet(parser.getMaxElevation());
    double elevRange = maxElev - minElev;
    double totalDist = m_useMetricUnits ? metersToKilometers(parser.getTotalDistance()) 
                                       : metersToMiles(parser.getTotalDistance());
    
    // Add padding to ranges for better visualization
    minElev -= elevRange * 0.08;
    maxElev += elevRange * 0.08;
    
    m_miniProfile->xAxis->setRange(0, totalDist);
    m_miniProfile->yAxis->setRange(minElev, maxElev);
    
    // Modern styling for the mini profile
    m_miniProfile->setBackground(QColor("#f8f9fa"));
    m_miniProfile->axisRect()->setBackground(QColor("#ffffff"));
    
    // Update coloring for segments
    if (!m_segments.empty()) {
        // Create a second graph layer to color segments
        QCPGraph* segGraph = nullptr;
        
        // Remove any existing segment graphs
        while (m_miniProfile->graphCount() > 2) {
            m_miniProfile->removeGraph(m_miniProfile->graphCount() - 1);
        }
        
        // Add colored segments
        for (size_t i = 0; i < m_segments.size(); i++) {
            const TrackSegment& segment = m_segments[i];
            
            // Create new graph for this segment
            segGraph = m_miniProfile->addGraph();
            
            // Set color based on segment type and gradient
            QColor segColor;
            switch (segment.type) {
                case TrackSegment::CLIMB:
                    if (segment.avgGradient > 10.0)
                        segColor = QColor(255, 0, 0, 180);  // Steep climb - red
                    else if (segment.avgGradient > 5.0)
                        segColor = QColor(255, 165, 0, 180); // Moderate climb - orange
                    else
                        segColor = QColor(255, 255, 0, 180); // Easy climb - yellow
                    break;
                case TrackSegment::DESCENT:
                    if (segment.avgGradient < -10.0)
                        segColor = QColor(128, 0, 128, 180); // Steep descent - purple
                    else if (segment.avgGradient < -5.0)
                        segColor = QColor(0, 0, 255, 180);   // Moderate descent - blue
                    else
                        segColor = QColor(173, 216, 230, 180); // Easy descent - light blue
                    break;
                default:
                    segColor = QColor(0, 128, 0, 180);       // Flat - green
                    break;
            }
            
            // Set pen and brush
            segGraph->setPen(QPen(segColor.darker(120), 2));
            segGraph->setBrush(QBrush(segColor.lighter(120)));
            
            // Extract segment data points
            QVector<double> segX, segY;
            for (size_t j = segment.startIndex; j <= segment.endIndex && j < points.size(); j++) {
                double xVal = m_useMetricUnits ? metersToKilometers(points[j].distance) 
                                             : metersToMiles(points[j].distance);
                double yVal = m_useMetricUnits ? points[j].elevation : metersToFeet(points[j].elevation);
                segX.append(xVal);
                segY.append(yVal);
            }
            
            // Set segment data
            segGraph->setData(segX, segY);
        }
        
        // Bring marker to front
        m_miniProfile->graph(1)->setLayer("overlay");
        
        // Add a grid line at current elevation for better readability
        if (!points.empty() && m_miniProfile->graphCount() > 1) {
            QCPGraph* gridGraph = m_miniProfile->addGraph();
            gridGraph->setPen(QPen(QColor(200, 200, 200, 70), 1, Qt::DashLine));
            
            double yVal = m_useMetricUnits ? points[0].elevation : metersToFeet(points[0].elevation);
            QVector<double> xData = {0, totalDist};
            QVector<double> yData = {yVal, yVal};
            gridGraph->setData(xData, yData);
            gridGraph->setLayer("background");
        }
    }
    
    // Replot
    m_miniProfile->replot();
}

void TrackStatsWidget::updateSegmentsList() {
    // Clear existing buttons
    QLayoutItem* child;
    while ((child = m_segmentListWidget->layout()->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    
    // Add a button for each segment
    for (size_t i = 0; i < m_segments.size(); i++) {
        const TrackSegment& segment = m_segments[i];
        
        // Create a button for this segment
        QString segmentTypeText;
        QString segmentIcon;
        
        switch (segment.type) {
            case TrackSegment::CLIMB:
                segmentTypeText = "Climb";
                segmentIcon = "▲";
                break;
            case TrackSegment::DESCENT:
                segmentTypeText = "Descent";
                segmentIcon = "▼";
                break;
            default:
                segmentTypeText = "Flat";
                segmentIcon = "→";
                break;
        }
        
        QString buttonText = QString("%1 %2 - %3 %4%")
                            .arg(segmentIcon)
                            .arg(segmentTypeText)
                            .arg(formatDistance(segment.distance))
                            .arg(segment.avgGradient, 0, 'f', 1);
        
        QPushButton* segmentButton = new QPushButton(buttonText);
        segmentButton->setStyleSheet(
            "text-align: left; padding: 8px; border-radius: 4px; border: 1px solid #e0e0e0; "
            "background-color: white; font-size: 11px; margin: 2px 0px;"
        );
        segmentButton->setFlat(true);
        
        // Store segment index in button's property
        segmentButton->setProperty("segmentIndex", static_cast<int>(i));
        
        // Connect button click to show segment details
        connect(segmentButton, &QPushButton::clicked, [this, i]() {
            showSegmentDetails(i);
        });
        
        // Add to layout
        m_segmentListWidget->layout()->addWidget(segmentButton);
    }
}

void TrackStatsWidget::showSegmentDetails(int segmentIndex) {
    if (segmentIndex < 0 || segmentIndex >= static_cast<int>(m_segments.size())) {
        m_segmentDetailsWidget->setVisible(false);
        return;
    }
    
    const TrackSegment& segment = m_segments[segmentIndex];
    
    // Update segment details
    QString segmentType;
    switch (segment.type) {
        case TrackSegment::CLIMB:
            segmentType = "Climb";
            break;
        case TrackSegment::DESCENT:
            segmentType = "Descent";
            break;
        default:
            segmentType = "Flat";
            break;
    }
    
    // Add difficulty rating
    QString difficulty = getDifficultyLabel(segment.avgGradient);
    segmentType += " - " + difficulty;
    
    // Set details
    m_segmentDetailsTitle->setText(QString("Segment %1").arg(segmentIndex + 1));
    m_segmentTypeLabel->setText(segmentType);
    m_segmentDistanceLabel->setText(formatDistance(segment.distance));
    m_segmentElevationLabel->setText(formatElevation(segment.elevationChange));
    
    // Set gradient with color coding
    m_segmentGradientLabel->setText(formatGradient(segment.avgGradient));
    m_segmentGradientLabel->setStyleSheet(getGradientColorStyle(segment.avgGradient));
    
    // Modern styling for the details widget
    m_segmentDetailsWidget->setStyleSheet(
        "background-color: #f8f9fa; border-radius: 6px; border: 1px solid #e0e0e0;"
    );
    
    // Show the details widget with a subtle animation
    m_segmentDetailsWidget->setVisible(true);
}

QString TrackStatsWidget::getGradientColorStyle(double gradient) const {
    QString color;
    
    // Color based on gradient severity
    if (gradient > 15.0) {
        color = "#d32f2f"; // Very steep climb - dark red
    } else if (gradient > 10.0) {
        color = "#f44336"; // Steep climb - red
    } else if (gradient > 5.0) {
        color = "#ff9800"; // Moderate climb - orange
    } else if (gradient > 2.0) {
        color = "#ffc107"; // Easy climb - amber
    } else if (gradient < -15.0) {
        color = "#9c27b0"; // Very steep descent - purple
    } else if (gradient < -10.0) {
        color = "#673ab7"; // Steep descent - deep purple
    } else if (gradient < -5.0) {
        color = "#3f51b5"; // Moderate descent - indigo
    } else if (gradient < -2.0) {
        color = "#2196f3"; // Easy descent - blue
    } else {
        color = "#4caf50"; // Flat - green
    }
    
    return QString("color: %1; font-weight: bold;").arg(color);
}

QString TrackStatsWidget::getDifficultyLabel(double gradient) const {
    // Return difficulty label based on gradient
    double absGradient = std::abs(gradient);
    
    if (absGradient > 15.0) {
        return "Very Hard";
    } else if (absGradient > 10.0) {
        return "Hard";
    } else if (absGradient > 5.0) {
        return "Moderate";
    } else if (absGradient > 2.0) {
        return "Easy";
    } else {
        return "Very Easy";
    }
}

QString TrackStatsWidget::formatDistance(double meters) const {
    if (m_useMetricUnits) {
        if (meters < 1000) {
            return QString("%1 m").arg(meters, 0, 'f', 0);
        } else {
            return QString("%1 km").arg(meters / 1000.0, 0, 'f', 2);
        }
    } else {
        return QString("%1 mi").arg(metersToMiles(meters), 0, 'f', 2);
    }
}

QString TrackStatsWidget::formatElevation(double meters) const {
    if (m_useMetricUnits) {
        return QString("%1 m").arg(meters, 0, 'f', 1);
    } else {
        return QString("%1 ft").arg(metersToFeet(meters), 0, 'f', 1);
    }
}

QString TrackStatsWidget::formatGradient(double gradient) const {
    return QString("%1%").arg(gradient, 0, 'f', 1);
}
