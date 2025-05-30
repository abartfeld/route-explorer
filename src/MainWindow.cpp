#include "MainWindow.h"
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
      m_currentPointIndex(0),
      m_updatingFromHover(false),
      m_updatingFrom3D(false)
{
    setupUi();
    
    // Show landing page by default on startup
    showLandingPage();
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
    
    // Create stacked widget to switch between landing page and main view
    m_mainStack = new QStackedWidget(this);
    setCentralWidget(m_mainStack);
    
    // Create landing page
    m_landingPage = new LandingPage(this);
    m_mainStack->addWidget(m_landingPage);
    
    // Create main view
    m_mainView = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_mainView);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create toolbar for main view
    QToolBar* toolBar = new QToolBar(this);
    toolBar->setIconSize(QSize(24, 24));
    toolBar->setMovable(false);
    
    QAction* homeAction = toolBar->addAction(QIcon(":/icons/home.svg"), "Home");
    connect(homeAction, &QAction::triggered, this, &MainWindow::showLandingPage);
    
    QAction* openAction = toolBar->addAction(QIcon(":/icons/open-file.svg"), "Open File");
    connect(openAction, &QAction::triggered, this, QOverload<>::of(&MainWindow::openFile));
    
    toolBar->addSeparator();
    
    QAction* settingsAction = toolBar->addAction(QIcon(":/icons/settings.svg"), "Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    mainLayout->addWidget(toolBar);
    
    // Create tab widget for the main view
    m_tabWidget = new QTabWidget(m_mainView);
    
    // Create the map tab
    QWidget* mapTab = new QWidget();
    QVBoxLayout* mapTabLayout = new QVBoxLayout(mapTab);
    mapTabLayout->setContentsMargins(4, 4, 4, 4);
    
    // Create a splitter for map and stats
    QSplitter* mapSplitter = new QSplitter(Qt::Horizontal);
    
    // Create the map widget
    m_mapView = new MapWidget();
    
    // Create the stats widget
    m_statsWidget = new TrackStatsWidget();
    
    // Add widgets to splitter
    mapSplitter->addWidget(m_mapView);
    mapSplitter->addWidget(m_statsWidget);
    
    // Set default sizes (70% map, 30% stats)
    mapSplitter->setSizes(QList<int>() << 700 << 300);
    
    // Add the splitter to the map tab
    mapTabLayout->addWidget(mapSplitter);
    
    // Create elevation profile
    QGroupBox* elevationGroup = new QGroupBox("Elevation Profile");
    QVBoxLayout* elevationLayout = new QVBoxLayout(elevationGroup);
    
    // Create the elevation plot
    m_elevationPlot = new QCustomPlot();
    m_elevationPlot->setMinimumHeight(150);
    m_elevationPlot->addGraph(); // Elevation profile graph
    m_elevationPlot->addGraph(); // Position marker graph (will be a single point)
    m_elevationPlot->graph(0)->setPen(QPen(QColor(64, 115, 244)));  // Blue line for elevation
    m_elevationPlot->graph(0)->setBrush(QBrush(QColor(200, 230, 255, 100)));  // Light blue fill
    m_elevationPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black), QBrush(Qt::red), 10));  // Red position marker
    m_elevationPlot->graph(1)->setLineStyle(QCPGraph::lsNone);  // No line, only points
    
    // Set axis labels
    m_elevationPlot->xAxis->setLabel("Distance (mi)");
    m_elevationPlot->yAxis->setLabel("Elevation (ft)");
    m_elevationPlot->xAxis->setTickLabelFont(QFont("Roboto", 8));
    m_elevationPlot->yAxis->setTickLabelFont(QFont("Roboto", 8));
    
    // Set background color
    m_elevationPlot->setBackground(QBrush(QColor(255, 255, 255)));
    m_elevationPlot->axisRect()->setBackground(QBrush(QColor(245, 245, 245)));
    
    // Interactive features
    m_elevationPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    
    // Add elevation plot to layout
    elevationLayout->addWidget(m_elevationPlot);
    
    // Create position slider
    m_positionSlider = new QSlider(Qt::Horizontal);
    m_positionSlider->setRange(0, 1000);
    m_positionSlider->setValue(0);
    m_positionSlider->setEnabled(false);
    elevationLayout->addWidget(m_positionSlider);
    
    // Add elevation group to map tab
    mapTabLayout->addWidget(elevationGroup);
    
    // Add map tab to tab widget
    m_tabWidget->addTab(mapTab, QIcon(":/icons/map-marker.svg"), "Map View");
    
    // Create the 3D view tab
    QWidget* view3DTab = new QWidget();
    QVBoxLayout* view3DLayout = new QVBoxLayout(view3DTab);
    
    // Create 3D elevation view
    m_elevation3DView = new ElevationView3D();
    view3DLayout->addWidget(m_elevation3DView);
    
    // Add 3D view tab to tab widget
    m_tabWidget->addTab(view3DTab, QIcon(":/icons/map-marker.svg"), "3D View");
    
    // Add tab widget to main layout
    mainLayout->addWidget(m_tabWidget);
    
    // Add main view to the stack widget
    m_mainStack->addWidget(m_mainView);
    
    // Connect signals
    connect(m_positionSlider, &QSlider::valueChanged, this, &MainWindow::updatePosition);
    connect(m_mapView, &MapWidget::routeHovered, this, &MainWindow::handleRouteHover);
    connect(m_elevation3DView, &ElevationView3D::positionChanged, this, &MainWindow::handleFlythrough3DPositionChanged);
    
    // Connect landing page signals
    connect(m_landingPage, &LandingPage::openFile, this, 
            QOverload<const QString&>::of(&MainWindow::openFile));
    connect(m_landingPage, &LandingPage::browse, this, 
            QOverload<>::of(&MainWindow::openFile));
    connect(m_landingPage, &LandingPage::createNewRoute, this, &MainWindow::createNewRoute);
    connect(m_landingPage, &LandingPage::showSettings, this, &MainWindow::showSettings);
    connect(m_landingPage, &LandingPage::show3DView, this, &MainWindow::show3DView);
}

void MainWindow::switchToTab(int tabIndex) {
    if (tabIndex >= 0 && tabIndex < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(tabIndex);
    }
}

void MainWindow::openFile() {
    QString filename = QFileDialog::getOpenFileName(this,
                                                   "Open GPX File",
                                                   QString(),
                                                   "GPX Files (*.gpx);;All Files (*)");
    if (filename.isEmpty()) {
        return;
    }
    
    openFile(filename);
}

void MainWindow::openFile(const QString& filePath) {
    qDebug() << "MainWindow::openFile - Opening file:" << filePath;
    
    if (m_gpxParser.parse(filePath)) {
        const std::vector<TrackPoint>& points = m_gpxParser.getPoints();
        
        if (points.empty()) {
            statusBar()->showMessage("No track points found in GPX file", 3000);
            return;
        }
        
        qDebug() << "MainWindow::openFile - Successfully parsed" << points.size() << "points";
        
        // Show the main view
        showMainView();
        
        // Create route on map
        std::vector<QGeoCoordinate> coordinates;
        coordinates.reserve(points.size());
        
        for (const auto& point : points) {
            coordinates.push_back(point.coord);
        }
        
        // First update stats widget to analyze segments
        m_statsWidget->setTrackInfo(m_gpxParser);
        
        // Get segments from stats widget
        const std::vector<TrackSegment>& segments = m_statsWidget->getSegments();
        
        // Provide track points to the map for hover information
        m_mapView->setTrackPoints(points);
        
        // Use segmented route if available
        if (!segments.empty()) {
            qDebug() << "MainWindow::openFile - Setting route with" << segments.size() << "segments";
            m_mapView->setRouteWithSegments(coordinates, segments, points);
        } else {
            qDebug() << "MainWindow::openFile - Setting route without segments";
            m_mapView->setRoute(coordinates);
        }
        
        // Plot elevation profile
        qDebug() << "MainWindow::openFile - Plotting elevation profile";
        plotElevationProfile();
        
        // Set up position slider
        m_positionSlider->setRange(0, 1000);
        m_positionSlider->setValue(0);
        m_positionSlider->setEnabled(true);
        
        // Update display
        m_currentPointIndex = 0;
        updatePosition(0);
        
        // Update 3D view
        try {
            qDebug() << "MainWindow::openFile - Updating 3D view with" << points.size() << "points";
            if (m_elevation3DView) {
                m_elevation3DView->setTrackData(points);
            } else {
                qWarning() << "MainWindow::openFile - 3D view is null";
            }
        } catch (const std::exception& e) {
            qCritical() << "MainWindow::openFile - Exception in 3D view update:" << e.what();
        } catch (...) {
            qCritical() << "MainWindow::openFile - Unknown exception in 3D view update";
        }
        
        // Add to recent files
        addToRecentFiles(filePath);
        
        // Show the main view
        showMainView();
        
        statusBar()->showMessage(QString("Loaded %1 with %2 points").arg(QFileInfo(filePath).fileName()).arg(points.size()), 3000);
    } else {
        statusBar()->showMessage("Failed to load GPX file", 3000);
    }
}

void MainWindow::addToRecentFiles(const QString& filePath) {
    QSettings settings;
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    
    // Remove if already exists
    recentFiles.removeAll(filePath);
    
    // Add to beginning of the list
    recentFiles.prepend(filePath);
    
    // Keep only MAX_RECENT_FILES
    while (recentFiles.size() > 10) {
        recentFiles.removeLast();
    }
    
    settings.setValue("recentFiles", recentFiles);
    
    // Update landing page if it exists
    if (m_landingPage) {
        m_landingPage->updateRecentFiles();
    }
}

void MainWindow::showLandingPage() {
    m_mainStack->setCurrentWidget(m_landingPage);
}

void MainWindow::showMainView() {
    m_mainStack->setCurrentWidget(m_mainView);
}

void MainWindow::createNewRoute() {
    QMessageBox::information(this, "Create New Route", 
                           "This feature is coming soon! You'll be able to create new routes by placing points on the map.");
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
    
    // Make sure plot takes full width - update the margins
    m_elevationPlot->setViewport(QRect(0, 0, m_elevationPlot->width(), m_elevationPlot->height()));
    m_elevationPlot->axisRect()->setAutoMargins(QCP::msBottom|QCP::msTop);
    m_elevationPlot->axisRect()->setMargins(QMargins(50, 10, 10, 20)); // Adjust left margin for Y-axis
    
    // Get Y-axis position to match slider with plot area
    // This gets the exact position of the y-axis line
    int yAxisPos = m_elevationPlot->yAxis->axisRect()->left();
    int rightMargin = 10; // Small right margin
    
    // Update slider style to align with Y-axis exactly
    m_positionSlider->setStyleSheet(
        QString("QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: #e0e0e0;"
        "  border-radius: 2px;"
        "  margin-left: %1px;" // Align with Y-axis exactly
        "  margin-right: %2px;" // Small right margin
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
        "}").arg(yAxisPos).arg(rightMargin));
    
    // Set up the position marker point (second graph)
    // Initialize with the first point
    QVector<double> x, y;
    if (!points.empty()) {
        x.append(points.front().distance * 0.000621371); // meters to miles
        y.append(points.front().elevation * 3.28084); // meters to feet
        m_elevationPlot->graph(1)->setData(x, y);
    }
    
    // Force the plot to update
    m_elevationPlot->replot();
}

void MainWindow::updatePosition(int value) {
    // If this update is coming from a hover event, skip to avoid feedback
    if (m_updatingFromHover) {
        return;
    }
    
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
    
    // Update 3D view position if not coming from 3D view
    if (!m_updatingFrom3D) {
        m_elevation3DView->updatePosition(m_currentPointIndex);
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

// New slot to handle hover events over the route on the map
void MainWindow::handleRouteHover(int pointIndex) {
    if (pointIndex < 0 || pointIndex >= static_cast<int>(m_gpxParser.getPoints().size())) {
        return;
    }
    
    // Prevent loop when slider position changes trigger hover events
    if (m_updatingFromHover) {
        return;
    }
    
    m_updatingFromHover = true;
    
    // Calculate the slider position based on point index
    // Convert from point index (0 to N) to slider range (0 to 1000)
    int totalPoints = m_gpxParser.getPoints().size();
    int sliderPos = pointIndex * 1000 / (totalPoints - 1);
    
    // Set the position slider without triggering its valueChanged signal
    m_positionSlider->blockSignals(true);
    m_positionSlider->setValue(sliderPos);
    m_positionSlider->blockSignals(false);
    
    // Update the current position
    m_currentPointIndex = pointIndex;
    
    // Update the marker on the map and in the plot
    const TrackPoint& point = m_gpxParser.getPoints()[pointIndex];
    m_mapView->updateMarker(point.coord);
    updatePlotPosition(point);
    
    // Update statistics display
    m_statsWidget->updatePosition(point, pointIndex, m_gpxParser);
    
    // Update 3D view position
    m_elevation3DView->updatePosition(pointIndex);
    
    m_updatingFromHover = false;
}

void MainWindow::handleFlythrough3DPositionChanged(int pointIndex) {
    qDebug() << "MainWindow::handleFlythrough3DPositionChanged - Position changed to" << pointIndex;
    
    if (pointIndex < 0 || pointIndex >= static_cast<int>(m_gpxParser.getPoints().size())) {
        qWarning() << "MainWindow::handleFlythrough3DPositionChanged - Invalid point index";
        return;
    }
    
    // Set flag to prevent feedback loops
    m_updatingFrom3D = true;
    
    // Calculate the slider position based on point index
    int totalPoints = m_gpxParser.getPoints().size();
    int sliderPos = pointIndex * 1000 / (totalPoints - 1);
    
    // Set the position slider
    m_positionSlider->blockSignals(true);
    m_positionSlider->setValue(sliderPos);
    m_positionSlider->blockSignals(false);
    
    // Update the current position
    m_currentPointIndex = pointIndex;
    
    // Update the marker on the map and in the plot
    const TrackPoint& point = m_gpxParser.getPoints()[pointIndex];
    m_mapView->updateMarker(point.coord);
    updatePlotPosition(point);
    
    // Update statistics display
    m_statsWidget->updatePosition(point, pointIndex, m_gpxParser);
    
    m_updatingFrom3D = false;
}

void MainWindow::showSettings() {
    // Create a dialog for application settings
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle("Route Explorer Settings");
    settingsDialog.setMinimumWidth(450);
    
    QVBoxLayout* layout = new QVBoxLayout(&settingsDialog);
    
    // Create tabs for different settings categories
    QTabWidget* tabWidget = new QTabWidget(&settingsDialog);
    
    // General settings tab
    QWidget* generalTab = new QWidget();
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);
    
    // Units group
    QGroupBox* unitsGroup = new QGroupBox("Measurement Units");
    QVBoxLayout* unitsLayout = new QVBoxLayout(unitsGroup);
    
    QRadioButton* metricRadio = new QRadioButton("Metric (kilometers, meters)");
    QRadioButton* imperialRadio = new QRadioButton("Imperial (miles, feet)");
    
    QSettings settings;
    bool useMetric = settings.value("useMetricUnits", false).toBool();
    
    metricRadio->setChecked(useMetric);
    imperialRadio->setChecked(!useMetric);
    
    unitsLayout->addWidget(metricRadio);
    unitsLayout->addWidget(imperialRadio);
    generalLayout->addWidget(unitsGroup);
    
    // Map settings group
    QGroupBox* mapGroup = new QGroupBox("Map Settings");
    QVBoxLayout* mapLayout = new QVBoxLayout(mapGroup);
    
    QCheckBox* showLabelsCheck = new QCheckBox("Show distance markers");
    showLabelsCheck->setChecked(settings.value("showDistanceMarkers", true).toBool());
    
    QCheckBox* showElevationCheck = new QCheckBox("Show elevation color coding");
    showElevationCheck->setChecked(settings.value("showElevationColors", true).toBool());
    
    mapLayout->addWidget(showLabelsCheck);
    mapLayout->addWidget(showElevationCheck);
    generalLayout->addWidget(mapGroup);
    
    // Add spacer
    generalLayout->addStretch(1);
    
    // 3D View Settings tab
    QWidget* view3DTab = new QWidget();
    QVBoxLayout* view3DLayout = new QVBoxLayout(view3DTab);
    
    QGroupBox* performanceGroup = new QGroupBox("Performance Settings");
    QVBoxLayout* perfLayout = new QVBoxLayout(performanceGroup);
    
    QLabel* qualityLabel = new QLabel("Rendering Quality:");
    QComboBox* qualityCombo = new QComboBox();
    qualityCombo->addItem("Low (Better Performance)");
    qualityCombo->addItem("Medium");
    qualityCombo->addItem("High (Better Quality)");
    qualityCombo->setCurrentIndex(settings.value("3DRenderingQuality", 1).toInt());
    
    QHBoxLayout* qualityLayout = new QHBoxLayout();
    qualityLayout->addWidget(qualityLabel);
    qualityLayout->addWidget(qualityCombo);
    perfLayout->addLayout(qualityLayout);
    
    QLabel* elevScaleLabel = new QLabel("Elevation Scale Factor:");
    QSlider* elevScaleSlider = new QSlider(Qt::Horizontal);
    elevScaleSlider->setRange(5, 30);
    elevScaleSlider->setValue(settings.value("elevationScale", 15).toInt());
    elevScaleSlider->setTickPosition(QSlider::TicksBelow);
    
    QHBoxLayout* elevScaleLayout = new QHBoxLayout();
    elevScaleLayout->addWidget(elevScaleLabel);
    elevScaleLayout->addWidget(elevScaleSlider);
    perfLayout->addLayout(elevScaleLayout);
    
    view3DLayout->addWidget(performanceGroup);
    
    // Camera settings group
    QGroupBox* cameraGroup = new QGroupBox("Camera Settings");
    QVBoxLayout* cameraLayout = new QVBoxLayout(cameraGroup);
    
    QCheckBox* flyoverModeCheck = new QCheckBox("Start in flyover mode");
    flyoverModeCheck->setChecked(settings.value("flyoverModeDefault", false).toBool());
    
    QLabel* flySpeedLabel = new QLabel("Default fly speed:");
    QSlider* flySpeedSlider = new QSlider(Qt::Horizontal);
    flySpeedSlider->setRange(1, 10);
    flySpeedSlider->setValue(settings.value("flythroughSpeed", 5).toInt());
    flySpeedSlider->setTickPosition(QSlider::TicksBelow);
    
    cameraLayout->addWidget(flyoverModeCheck);
    
    QHBoxLayout* flySpeedLayout = new QHBoxLayout();
    flySpeedLayout->addWidget(flySpeedLabel);
    flySpeedLayout->addWidget(flySpeedSlider);
    cameraLayout->addLayout(flySpeedLayout);
    
    view3DLayout->addWidget(cameraGroup);
    view3DLayout->addStretch(1);
    
    // Add tabs to the tab widget
    tabWidget->addTab(generalTab, "General");
    tabWidget->addTab(view3DTab, "3D View");
    
    layout->addWidget(tabWidget);
    
    // Add dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    
    connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
        // Save all settings
        settings.setValue("useMetricUnits", metricRadio->isChecked());
        settings.setValue("showDistanceMarkers", showLabelsCheck->isChecked());
        settings.setValue("showElevationColors", showElevationCheck->isChecked());
        settings.setValue("3DRenderingQuality", qualityCombo->currentIndex());
        settings.setValue("elevationScale", elevScaleSlider->value());
        settings.setValue("flyoverModeDefault", flyoverModeCheck->isChecked());
        settings.setValue("flythroughSpeed", flySpeedSlider->value());
        
        // Apply settings to current view
        if (m_elevation3DView) {
            m_elevation3DView->setElevationScale(elevScaleSlider->value() / 10.0f);
        }
        
        settingsDialog.accept();
    });
    
    connect(buttonBox, &QDialogButtonBox::rejected, &settingsDialog, &QDialog::reject);
    
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, [&]() {
        // Save all settings
        settings.setValue("useMetricUnits", metricRadio->isChecked());
        settings.setValue("showDistanceMarkers", showLabelsCheck->isChecked());
        settings.setValue("showElevationColors", showElevationCheck->isChecked());
        settings.setValue("3DRenderingQuality", qualityCombo->currentIndex());
        settings.setValue("elevationScale", elevScaleSlider->value());
        settings.setValue("flyoverModeDefault", flyoverModeCheck->isChecked());
        settings.setValue("flythroughSpeed", flySpeedSlider->value());
        
        // Apply settings to current view
        if (m_elevation3DView) {
            m_elevation3DView->setElevationScale(elevScaleSlider->value() / 10.0f);
        }
    });
    
    layout->addWidget(buttonBox);
    
    settingsDialog.exec();
}

void MainWindow::show3DView() {
    // First show the main view
    showMainView();
    
    // If there's no data loaded, offer to load a sample route
    if (m_gpxParser.getPoints().empty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "No Route Loaded", 
            "No route is currently loaded. Would you like to load a sample route for viewing in 3D?",
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::Yes) {
            // Try to load the example route
            QString samplePath = ":/samples/example_route.gpx";
            openFile(samplePath);
        } else {
            // Return to landing page if user doesn't want to load a sample
            showLandingPage();
            return;
        }
    }
    
    // Switch to the 3D tab (index 1)
    switchToTab(1);
}
