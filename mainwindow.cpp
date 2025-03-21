#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QIcon>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QFrame>
#include <QPalette>
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Set window properties
    setWindowTitle("GPX Track Viewer");
    setMinimumSize(900, 600);
    
    // Initialize UI elements with Material Design styling
    setupUi();
    
    // Connect signals/slots
    connect(m_positionSlider, &QSlider::valueChanged, this, &MainWindow::updatePosition);
    
    // Initialize timer for smooth updates
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(50); // 50ms delay
    connect(&m_updateTimer, &QTimer::timeout, this, &MainWindow::updateDisplay);
    
    // Status bar message
    statusBar()->showMessage("Ready. Open a GPX file to begin.");
}

void MainWindow::setupUi() {
    // Set application font to Roboto for Material Design look
    QFont robotoFont("Roboto", 9);
    QApplication::setFont(robotoFont);
    
    // Set application style sheet with Material Design elements
    QString materialStyle = R"(
        QMainWindow, QDialog {
            background-color: #f5f5f5;
        }
        QMenuBar, QStatusBar {
            background-color: #ffffff;
            border: none;
        }
        QMenuBar::item {
            padding: 6px 10px;
            background-color: transparent;
        }
        QMenuBar::item:selected {
            background-color: #e0e0e0;
            border-radius: 4px;
        }
        QToolBar {
            background-color: #ffffff;
            border-bottom: 1px solid #e0e0e0;
            spacing: 8px;
            padding: 4px;
        }
        QToolButton {
            border: none;
            border-radius: 4px;
            padding: 4px;
            background-color: transparent;
        }
        QToolButton:hover {
            background-color: #f0f0f0;
        }
        QToolButton:pressed {
            background-color: #e0e0e0;
        }
        QSplitter::handle {
            background-color: #e0e0e0;
        }
        QGroupBox {
            font-weight: bold;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            margin-top: 1ex;
            padding-top: 10px;
            background-color: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 5px;
            color: #424242;
        }
    )";
    qApp->setStyleSheet(materialStyle);
    
    // Set up the main window with material design look
    resize(1024, 768);
    setWindowTitle("GPX Viewer");
    setWindowIcon(QIcon(":/icons/map-marker.svg"));
    setUnifiedTitleAndToolBarOnMac(true);
    
    // Create the central widget and main layout
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);
    
    // Create left main panel for map and elevation
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);
    
    // Create the map widget
    m_mapView = new MapWidget(this);
    m_mapView->setMinimumSize(400, 300);
    QFrame *mapFrame = new QFrame();
    mapFrame->setFrameShape(QFrame::StyledPanel);
    mapFrame->setFrameShadow(QFrame::Plain);
    QVBoxLayout *mapLayout = new QVBoxLayout(mapFrame);
    mapLayout->setContentsMargins(0, 0, 0, 0);
    mapLayout->addWidget(m_mapView);
    leftLayout->addWidget(mapFrame, 3); // Set stretch to 3
    
    // Create the bottom panel containing the elevation profile
    QWidget *bottomPanel = new QWidget();
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(12, 12, 12, 12);
    bottomLayout->setSpacing(8);
    leftLayout->addWidget(bottomPanel, 1); // Set stretch to 1
    
    // Create the elevation profile plot with themed colors
    m_elevationPlot = new QCustomPlot();
    m_elevationPlot->setMinimumHeight(170);
    m_elevationPlot->setBackground(QColor("#ffffff"));
    m_elevationPlot->xAxis->setBasePen(QPen(QColor("#9e9e9e"), 1));
    m_elevationPlot->yAxis->setBasePen(QPen(QColor("#9e9e9e"), 1));
    m_elevationPlot->xAxis->setTickPen(QPen(QColor("#9e9e9e"), 1));
    m_elevationPlot->yAxis->setTickPen(QPen(QColor("#9e9e9e"), 1));
    m_elevationPlot->xAxis->setSubTickPen(QPen(QColor("#bdbdbd"), 1));
    m_elevationPlot->yAxis->setSubTickPen(QPen(QColor("#bdbdbd"), 1));
    m_elevationPlot->xAxis->setTickLabelColor(QColor("#424242"));
    m_elevationPlot->yAxis->setTickLabelColor(QColor("#424242"));
    m_elevationPlot->xAxis->setLabelColor(QColor("#424242"));
    m_elevationPlot->yAxis->setLabelColor(QColor("#424242"));
    m_elevationPlot->xAxis->grid()->setPen(QPen(QColor("#e0e0e0"), 1, Qt::DotLine));
    m_elevationPlot->yAxis->grid()->setPen(QPen(QColor("#e0e0e0"), 1, Qt::DotLine));
    m_elevationPlot->xAxis->grid()->setSubGridPen(QPen(QColor("#f5f5f5"), 1, Qt::DotLine));
    m_elevationPlot->yAxis->grid()->setSubGridPen(QPen(QColor("#f5f5f5"), 1, Qt::DotLine));
    m_elevationPlot->xAxis->setLabel("Distance (miles)");
    m_elevationPlot->yAxis->setLabel("Elevation (feet)");
    
    // Create graph for elevation profile
    m_elevationPlot->addGraph();
    m_elevationPlot->graph(0)->setPen(QPen(QColor("#2196F3"), 2));
    m_elevationPlot->graph(0)->setBrush(QBrush(QColor(33, 150, 243, 50)));
    
    // Create graph for current position marker
    m_elevationPlot->addGraph();
    m_elevationPlot->graph(1)->setLineStyle(QCPGraph::lsNone);
    m_elevationPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, QColor("#F44336"), 8));
    
    // Fix axis ranges to prevent auto-scaling
    m_elevationPlot->setInteraction(QCP::iRangeDrag, false);
    m_elevationPlot->setInteraction(QCP::iRangeZoom, false);
    
    bottomLayout->addWidget(m_elevationPlot);
    
    // Get the axis rect margins so we can match the slider precisely with the plot area
    int leftAxisMargin = m_elevationPlot->axisRect()->margins().left();
    int rightAxisMargin = m_elevationPlot->axisRect()->margins().right();
    
    // Create slider for position with Material Design styling and proper margins to align with Y-axis
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setEnabled(false);
    m_positionSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: #e0e0e0;"
        "  border-radius: 2px;"
        "  margin-left: " + QString::number(leftAxisMargin) + "px;" // Match Y axis exactly
        "  margin-right: " + QString::number(rightAxisMargin) + "px;" // Match right edge exactly
        "}"
        "QSlider::handle:horizontal {"
        "  background: #2196F3;"
        "  border: none;"
        "  width: 16px;"
        "  height: 16px;"
        "  margin: -6px 0px;" // Center it exactly on the track
        "  border-radius: 8px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #2196F3;"
        "  border-radius: 2px;"
        "  margin-left: 0px;" // Make the colored bar start exactly at axis
        "  margin-right: 0px;"
        "}"
    );
    bottomLayout->addWidget(m_positionSlider);
    
    // Add left panel to main layout
    mainLayout->addWidget(leftPanel, 3); // Set stretch to 3 for left panel
    
    // Create the right panel for statistics with Material Design appearance
    QFrame *rightPanel = new QFrame();
    rightPanel->setFrameShape(QFrame::StyledPanel);
    rightPanel->setFrameShadow(QFrame::Plain);
    rightPanel->setStyleSheet("background-color: #ffffff; border-radius: 4px;");
    rightPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    rightPanel->setMinimumWidth(250); // Set minimum width for the stats panel
    
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    
    // Create the statistics title
    QLabel *statsTitle = new QLabel("Track Statistics");
    statsTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2196F3; margin-bottom: 12px;");
    rightLayout->addWidget(statsTitle);
    
    // Create the elevation label with Material Design typography
    m_elevationLabel = new QLabel();
    m_elevationLabel->setTextFormat(Qt::RichText);
    m_elevationLabel->setWordWrap(true);
    m_elevationLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_elevationLabel->setStyleSheet("color: #424242; padding: 8px 0px;");
    rightLayout->addWidget(m_elevationLabel);
    
    // Add spacer to push stats to the top
    rightLayout->addStretch();
    
    // Add right panel to main layout
    mainLayout->addWidget(rightPanel, 1); // Set stretch to 1 for right panel
    
    // Create toolbar with Material Design-inspired styling
    QToolBar *toolBar = addToolBar("Main Toolbar");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24));
    toolBar->setStyleSheet("QToolBar { background: #ffffff; border-bottom: 1px solid #e0e0e0; spacing: 8px; }");
    
    // Add actions
    QAction *openAction = toolBar->addAction(QIcon(":/icons/open-file.svg"), "Open GPX File");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    
    toolBar->addSeparator();
    
    QAction *zoomInAction = toolBar->addAction(QIcon(":/icons/zoom-in.svg"), "Zoom In");
    QAction *zoomOutAction = toolBar->addAction(QIcon(":/icons/zoom-out.svg"), "Zoom Out");
    
    // Connect zoom buttons to map
    connect(zoomInAction, &QAction::triggered, [this]() {
        // Create wheel event with correct parameters for this Qt version
        QPointF pos(m_mapView->width()/2, m_mapView->height()/2);
        QPointF globalPos = m_mapView->mapToGlobal(pos.toPoint());
        QPoint pixelDelta(0, 0);
        QPoint angleDelta(0, 120);
        Qt::MouseButtons buttons = Qt::NoButton;
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        QWheelEvent wheelEvent(pos, globalPos, pixelDelta, angleDelta, 
                               buttons, modifiers, Qt::NoScrollPhase, false,
                               Qt::MouseEventNotSynthesized); // Add source parameter
        QCoreApplication::sendEvent(m_mapView, &wheelEvent);
    });
    
    connect(zoomOutAction, &QAction::triggered, [this]() {
        // Create wheel event with correct parameters for this Qt version
        QPointF pos(m_mapView->width()/2, m_mapView->height()/2);
        QPointF globalPos = m_mapView->mapToGlobal(pos.toPoint());
        QPoint pixelDelta(0, 0);
        QPoint angleDelta(0, -120);
        Qt::MouseButtons buttons = Qt::NoButton;
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        QWheelEvent wheelEvent(pos, globalPos, pixelDelta, angleDelta,
                               buttons, modifiers, Qt::NoScrollPhase, false,
                               Qt::MouseEventNotSynthesized); // Add source parameter
        QCoreApplication::sendEvent(m_mapView, &wheelEvent);
    });
    
    // Create a status bar with Material Design styling
    statusBar()->setStyleSheet("QStatusBar { background: #ffffff; color: #757575; }");
    
    // Set up position slider
    connect(m_positionSlider, &QSlider::valueChanged, this, &MainWindow::updatePosition);
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(100); // 100ms delay for smooth interaction
    connect(&m_updateTimer, &QTimer::timeout, this, &MainWindow::updateDisplay);
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open GPX File", 
                                                  QString(), "GPX Files (*.gpx);;All Files (*)");
    if (fileName.isEmpty()) {
        return;
    }
    
    // Parse the GPX file
    QElapsedTimer timer;
    timer.start();
    bool success = m_gpxParser.parse(fileName);
    qDebug() << "GPX parsing took" << timer.elapsed() << "ms";
    
    if (!success || m_gpxParser.getPoints().empty()) {
        QMessageBox::warning(this, "Error", "Failed to load valid GPX data from the file.");
        return;
    }
    
    // Set the route on the map
    std::vector<QGeoCoordinate> coordinates;
    coordinates.reserve(m_gpxParser.getPoints().size());
    
    for (const auto& point : m_gpxParser.getPoints()) {
        coordinates.push_back(point.coord);
    }
    
    m_mapView->setRoute(coordinates);
    
    // Plot elevation profile
    plotElevationProfile();
    
    // Set up position slider
    m_positionSlider->setRange(0, 1000); // Use 0-1000 for smooth percentage representation
    m_positionSlider->setValue(0);
    m_positionSlider->setEnabled(true);
    
    // Update display
    m_currentPointIndex = 0;
    updatePosition(0); // Initialize with first point
    
    // Update status bar
    QFileInfo fileInfo(fileName);
    statusBar()->showMessage(QString("Loaded %1 data points from %2")
                           .arg(m_gpxParser.getPoints().size())
                           .arg(fileInfo.fileName()));
}

void MainWindow::updatePosition(int value) {
    // Convert slider position (0-1000) to percentage of total distance
    double percentage = value / 1000.0;
    
    const std::vector<TrackPoint>& points = m_gpxParser.getPoints();
    if (points.empty()) {
        return;
    }
    
    // Get total distance
    double totalDistance = points.back().distance;
    
    // Calculate target distance - with safety bounds check
    double targetDistance = totalDistance * percentage;
    
    // Use a faster method to find the closest point - binary search for performance
    size_t nearestIndex = findClosestPointByDistance(targetDistance);
    
    // Only update if position changed
    if (m_currentPointIndex != nearestIndex) {
        m_currentPointIndex = nearestIndex;
        
        // Update with the new point data
        const TrackPoint& point = points[m_currentPointIndex];
        m_mapView->updateMarker(point.coord);
        updatePlotPosition(point);
        
        // Update statistics display
        double currentDistance = point.distance * 0.000621371; // meters to miles
        double totalDistanceMiles = totalDistance * 0.000621371; // meters to miles
        double elevationGain = m_gpxParser.getCumulativeElevationGain(m_currentPointIndex); // already in feet
        double currentElevation = point.elevation * 3.28084; // meters to feet
        
        m_elevationLabel->setText(
            QString("<div style='font-size: 14px;'>"
                    "<b>Distance:</b> %1 / %2 mi<br>"
                    "<b>Elevation:</b> %3 ft<br>"
                    "<b>Elevation Gain:</b> %4 ft<br>"
                    "<b>Coordinates:</b><br>%5°, %6°"
                    "</div>")
            .arg(currentDistance, 0, 'f', 2)
            .arg(totalDistanceMiles, 0, 'f', 2)
            .arg(currentElevation, 0, 'f', 1)
            .arg(elevationGain, 0, 'f', 1)
            .arg(point.coord.latitude(), 0, 'f', 6)
            .arg(point.coord.longitude(), 0, 'f', 6)
        );
    }
}

void MainWindow::updateDisplay() {
    const std::vector<TrackPoint>& points = m_gpxParser.getPoints();
    if (points.empty() || m_pendingSliderValue < 0 || 
        m_pendingSliderValue >= static_cast<int>(points.size())) {
        return;
    }
    
    // Update current point index
    m_currentPointIndex = m_pendingSliderValue;
    const TrackPoint& point = points[m_currentPointIndex];
    
    // Update marker position on map
    m_mapView->updateMarker(point.coord);
    
    // Update position on elevation graph
    updatePlotPosition(point);
    
    // Update statistics with imperial units
    double totalDistance = m_gpxParser.getTotalDistance(); // miles
    double currentDistance = point.distance * 0.000621371; // meters to miles
    double elevationGain = m_gpxParser.getCumulativeElevationGain(m_currentPointIndex); // feet
    double currentElevation = point.elevation * 3.28084; // meters to feet
    
    // Format statistics with Material Design typography
    m_elevationLabel->setText(
        QString("<div style='font-size: 14px;'>"
                "<b>Distance:</b> %1 / %2 mi<br>"
                "<b>Elevation:</b> %3 ft<br>"
                "<b>Elevation Gain:</b> %4 ft<br>"
                "<b>Coordinates:</b><br>%5°, %6°"
                "</div>")
        .arg(currentDistance, 0, 'f', 2)
        .arg(totalDistance, 0, 'f', 2)
        .arg(currentElevation, 0, 'f', 1)
        .arg(elevationGain, 0, 'f', 1)
        .arg(point.coord.latitude(), 0, 'f', 6)
        .arg(point.coord.longitude(), 0, 'f', 6)
    );
}

void MainWindow::plotElevationProfile() {
    const std::vector<TrackPoint>& points = m_gpxParser.getPoints();
    if (points.empty()) {
        return;
    }
    
    QVector<double> distances, elevations;
    distances.reserve(points.size());
    elevations.reserve(points.size());
    
    // Convert to miles and feet for display
    for (const auto& point : points) {
        distances.append(point.distance * 0.000621371); // meters to miles
        elevations.append(point.elevation * 3.28084); // meters to feet
    }
    
    // Set data for the elevation profile
    m_elevationPlot->graph(0)->setData(distances, elevations);
    
    // Set nice-looking axis ranges
    double minEle = m_gpxParser.getMinElevation() * 3.28084; // meters to feet
    double maxEle = m_gpxParser.getMaxElevation() * 3.28084; // meters to feet
    double elevRange = maxEle - minEle;
    
    // Add 10% padding to elevation range
    m_elevationPlot->yAxis->setRange(
        minEle - elevRange * 0.1, 
        maxEle + elevRange * 0.1
    );
    
    // X-axis range is from 0 to max distance
    m_elevationPlot->xAxis->setRange(0, distances.last());
    
    // Now update the slider margins to match the plot axis rect margins exactly
    int leftAxisMargin = m_elevationPlot->axisRect()->margins().left();
    int rightAxisMargin = m_elevationPlot->axisRect()->margins().right();
    
    m_positionSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: #e0e0e0;"
        "  border-radius: 2px;"
        "  margin-left: " + QString::number(leftAxisMargin) + "px;" // Match Y axis exactly
        "  margin-right: " + QString::number(rightAxisMargin) + "px;" // Match right edge exactly
        "}"
        "QSlider::handle:horizontal {"
        "  background: #2196F3;"
        "  border: none;"
        "  width: 16px;"
        "  height: 16px;"
        "  margin: -6px 0px;" // Center it exactly on the track
        "  border-radius: 8px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #2196F3;"
        "  border-radius: 2px;"
        "  margin-left: 0px;" // Make the colored bar start exactly at axis
        "  margin-right: 0px;"
        "}"
    );
    
    // Set up the position marker point (second graph)
    // Initialize with the first point
    QVector<double> x, y;
    if (!points.empty()) {
        x.append(points.front().distance * 0.000621371); // meters to miles
        y.append(points.front().elevation * 3.28084); // meters to feet
        m_elevationPlot->graph(1)->setData(x, y);
    }
    
    // Refresh the plot
    m_elevationPlot->replot();
}

void MainWindow::updatePlotPosition(const TrackPoint& point) {
    // The marker is the second graph (index 1)
    QVector<double> x, y;
    
    // Convert to miles and feet for display
    x.append(point.distance * 0.000621371); // meters to miles
    y.append(point.elevation * 3.28084); // meters to feet
    
    // Update marker position without changing elevation graph itself
    m_elevationPlot->graph(1)->setData(x, y);
    
    // Only replot what's necessary - don't adjust axis ranges
    m_elevationPlot->replot(QCustomPlot::rpQueuedReplot);
}

size_t MainWindow::findClosestPoint(double targetDistance) {
    const std::vector<TrackPoint>& points = m_gpxParser.getPoints();
    if (points.empty()) {
        return 0;
    }
    
    // Simple binary search to find the closest point to the target distance
    size_t low = 0;
    size_t high = points.size() - 1;
    
    while (low < high) {
        size_t mid = (low + high) / 2;
        double midDistance = points[mid].distance;
        
        if (std::abs(midDistance - targetDistance) < 0.1) {
            return mid; // Close enough
        }
        
        if (midDistance < targetDistance) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    
    // If we're still not sure, check which one is closer
    if (low > 0) {
        double distLow = std::abs(points[low-1].distance - targetDistance);
        double distHigh = std::abs(points[low].distance - targetDistance);
        if (distLow < distHigh) {
            return low-1;
        }
    }
    
    return low;
}

// Binary search implementation for faster point finding - much more efficient than linear search
size_t MainWindow::findClosestPointByDistance(double targetDistance) {
    const std::vector<TrackPoint>& points = m_gpxParser.getPoints();
    if (points.empty()) {
        return 0;
    }
    
    // Binary search to quickly find closest point
    size_t low = 0;
    size_t high = points.size() - 1;
    
    // Handle special cases for beginning and end of range
    if (targetDistance <= points[low].distance) return low;
    if (targetDistance >= points[high].distance) return high;
    
    // Binary search for finding closest point
    while (low <= high) {
        size_t mid = low + (high - low) / 2;
        
        if (points[mid].distance < targetDistance) {
            low = mid + 1;
        } else if (points[mid].distance > targetDistance) {
            // Make sure we don't underflow
            if (mid == 0) break;
            high = mid - 1;
        } else {
            // Exact match
            return mid;
        }
    }
    
    // After binary search, either low or high is closest to target
    if (low >= points.size()) low = points.size() - 1;
    if (high >= points.size()) high = points.size() - 1;
    
    double lowDiff = std::abs(points[low].distance - targetDistance);
    double highDiff = std::abs(points[high].distance - targetDistance);
    
    return (lowDiff < highDiff) ? low : high;
}
