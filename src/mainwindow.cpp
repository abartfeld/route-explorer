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
#include <QSplitter>
#include <QApplication>
#include <QRandomGenerator>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent),
      m_currentPointIndex(0)
{
    setupUi();
}

MainWindow::~MainWindow()
{
    // Qt's parent-child mechanism handles cleanup
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
    
    // Create the central widget with horizontal layout
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);
    
    // Create left side layout that will contain map and elevation profile
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);
    
    // Create the map widget
    m_mapView = new MapWidget(this);
    m_mapView->setMinimumSize(400, 300);
    QFrame *mapFrame = new QFrame();
    mapFrame->setFrameShape(QFrame::StyledPanel);
    mapFrame->setFrameShadow(QFrame::Plain);
    QVBoxLayout *mapLayout = new QVBoxLayout(mapFrame);
    mapLayout->setContentsMargins(0, 0, 0, 0);
    mapLayout->addWidget(m_mapView);
    leftLayout->addWidget(mapFrame, 7); // Map gets 7/10 of vertical space
    
    // Create the bottom panel containing the elevation profile
    QWidget *bottomPanel = new QWidget();
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(8);
    leftLayout->addWidget(bottomPanel, 3); // Elevation gets 3/10 of vertical space
    
    // Create the elevation profile plot with themed colors
    m_elevationPlot = new QCustomPlot();
    m_elevationPlot->setMinimumHeight(120);
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
    
    // Disable interactions with the plot
    m_elevationPlot->setInteraction(QCP::iRangeDrag, false);
    m_elevationPlot->setInteraction(QCP::iRangeZoom, false);
    
    // Adjust the plot margins to match slider
    m_elevationPlot->axisRect()->setAutoMargins(QCP::msBottom|QCP::msTop);
    m_elevationPlot->axisRect()->setMargins(QMargins(50, 0, 50, 0));
    
    bottomLayout->addWidget(m_elevationPlot);
    
    // Create slider for position with Material Design styling
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setEnabled(false);
    
    bottomLayout->addWidget(m_positionSlider);
    
    // Add left panel to main layout, with stretch factor
    mainLayout->addWidget(leftPanel, 1);
    
    // Create the stats widget
    m_statsWidget = new TrackStatsWidget(this);
    mainLayout->addWidget(m_statsWidget, 0); // 0 stretch means it won't expand
    
    // Update slider style after plot is created
    int yAxisWidth = m_elevationPlot->yAxis->axisRect()->left();
    int rightMargin = m_elevationPlot->width() - m_elevationPlot->axisRect()->right();
    
    m_positionSlider->setStyleSheet(
        QString("QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: #e0e0e0;"
        "  border-radius: 2px;"
        "  margin-left: %1px;" // Match left edge of plot axis rect
        "  margin-right: %2px;" // Match right edge of plot axis rect
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
        "  margin-left: 0px;"
        "  margin-right: 0px;"
        "}").arg(yAxisWidth).arg(rightMargin));
    
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
        // Create wheel event with correct parameters for Qt
        QPointF pos(m_mapView->width()/2, m_mapView->height()/2);
        QPointF globalPos = m_mapView->mapToGlobal(pos.toPoint());
        QPoint pixelDelta(0, 0);
        QPoint angleDelta(0, 120);
        Qt::MouseButtons buttons = Qt::NoButton;
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        QWheelEvent wheelEvent(pos, globalPos, pixelDelta, angleDelta,
                           buttons, modifiers, Qt::NoScrollPhase, false,
                           Qt::MouseEventNotSynthesized);
        QCoreApplication::sendEvent(m_mapView, &wheelEvent);
    });
    
    connect(zoomOutAction, &QAction::triggered, [this]() {
        // Create wheel event with correct parameters for Qt
        QPointF pos(m_mapView->width()/2, m_mapView->height()/2);
        QPointF globalPos = m_mapView->mapToGlobal(pos.toPoint());
        QPoint pixelDelta(0, 0);
        QPoint angleDelta(0, -120);
        Qt::MouseButtons buttons = Qt::NoButton;
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        QWheelEvent wheelEvent(pos, globalPos, pixelDelta, angleDelta,
                           buttons, modifiers, Qt::NoScrollPhase, false,
                           Qt::MouseEventNotSynthesized);
        QCoreApplication::sendEvent(m_mapView, &wheelEvent);
    });
    
    // Create a status bar with Material Design styling
    statusBar()->setStyleSheet("QStatusBar { background: #ffffff; color: #757575; }");
    
    // Set up position slider
    connect(m_positionSlider, &QSlider::valueChanged, this, &MainWindow::updatePosition);
}

void MainWindow::openFile() {
    QString filename = QFileDialog::getOpenFileName(this,
                                                   "Open GPX File",
                                                   QString(),
                                                   "GPX Files (*.gpx);;All Files (*)");
    if (filename.isEmpty()) {
        return;
    }
    
    if (m_gpxParser.parse(filename)) {
        const std::vector<TrackPoint>& points = m_gpxParser.getPoints();
        
        if (points.empty()) {
            statusBar()->showMessage("No track points found in GPX file", 3000);
            return;
        }
        
        // Create route on map
        std::vector<QGeoCoordinate> coordinates;
        coordinates.reserve(points.size());
        
        for (const auto& point : points) {
            coordinates.push_back(point.coord);
        }
        
        m_mapView->setRoute(coordinates);
        
        // Plot elevation profile
        plotElevationProfile();
        
        // Set up position slider
        m_positionSlider->setRange(0, 1000);
        m_positionSlider->setValue(0);
        m_positionSlider->setEnabled(true);
        
        // Update display
        m_currentPointIndex = 0;
        updatePosition(0);
        
        // Update track info in stats widget
        m_statsWidget->setTrackInfo(m_gpxParser);
        
        statusBar()->showMessage(QString("Loaded %1 with %2 points").arg(QFileInfo(filename).fileName()).arg(points.size()), 3000);
    } else {
        statusBar()->showMessage("Failed to load GPX file", 3000);
    }
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
    
    // Make sure plot takes full width
    m_elevationPlot->axisRect()->setAutoMargins(QCP::msBottom|QCP::msTop);
    m_elevationPlot->axisRect()->setMargins(QMargins(50, 0, 50, 0));
    
    // Get Y-axis width to match slider with plot area
    int yAxisWidth = m_elevationPlot->yAxis->axisRect()->left();
    int rightMargin = m_elevationPlot->width() - m_elevationPlot->axisRect()->right();
    
    // Update slider style to match plot axes exactly
    m_positionSlider->setStyleSheet(
        QString("QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: #e0e0e0;"
        "  border-radius: 2px;"
        "  margin-left: %1px;" // Match left edge of plot axis rect
        "  margin-right: %2px;" // Match right edge of plot axis rect
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
        "  margin-left: 0px;"
        "  margin-right: 0px;"
        "}").arg(yAxisWidth).arg(rightMargin));
    
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

void MainWindow::updatePosition(int value) {
    // Convert slider position (0-1000) to percentage of total distance
    double percentage = value / 1000.0;
    
    // Find nearest track point to this percentage of total distance
    const std::vector<TrackPoint>& points = m_gpxParser.getPoints();
    if (points.empty()) {
        return;
    }
    
    // Get total distance
    double totalDistance = points.back().distance;
    
    // Calculate target distance
    double targetDistance = totalDistance * percentage;
    
    // Find closest point using binary search
    size_t nearestIndex = findClosestPointByDistance(targetDistance);
    
    // Update if the point changed
    if (m_currentPointIndex != nearestIndex) {
        m_currentPointIndex = nearestIndex;
        
        const TrackPoint& point = points[m_currentPointIndex];
        m_mapView->updateMarker(point.coord);
        updatePlotPosition(point);
        
        // Update statistics display via the stats widget
        m_statsWidget->updatePosition(point, m_currentPointIndex, m_gpxParser);
    }
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
