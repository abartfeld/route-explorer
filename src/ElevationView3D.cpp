#include "ElevationView3D.h"
#include "logging.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QStyle>
#include <QDebug>
#include <Qt3DExtras/QDiffuseSpecularMaterial>
#include <Qt3DExtras/QPhongAlphaMaterial>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QFirstPersonCameraController>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QExtrudedTextMesh>
#include <QCoreApplication>
#include <cmath>
#include <algorithm>
#include <limits>
#include <QElapsedTimer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>
#include <QRandomGenerator>

// Constants for 3D visualization with optimization settings
namespace {
    const float DEFAULT_ELEVATION_SCALE = 2.0f;
    const float TERRAIN_WIDTH = 1000.0f;
    const float MARKER_SIZE = 2.0f;
    const float DEFAULT_CAMERA_TILT = 30.0f;
    const int DEFAULT_FLYTHROUGH_INTERVAL = 50; // milliseconds
    const float DEFAULT_FLYTHROUGH_SPEED = 0.0015f; // progress per update
    
    // Performance optimization parameters
    const int ROUTE_SEGMENT_BATCH_SIZE = 25; // Batch this many segments into a single mesh
    const int MAX_ROUTE_VERTICES = 5000; // Maximum number of vertices for route
    const int LOD_DISTANCE_THRESHOLD = 200; // Use lower detail for segments farther than this
    const int FPS_UPDATE_INTERVAL = 1000; // Update FPS display every second
    const bool ENABLE_CULLING = true; // Enable culling of segments outside view frustum
    
    #ifndef M_PI
    const double M_PI = 3.14159265358979323846;
    #endif
}

ElevationView3D::ElevationView3D(QWidget* parent)
    : QWidget(parent),
      m_currentPositionIndex(0),
      m_flythroughActive(false),
      m_flythroughSpeed(DEFAULT_FLYTHROUGH_SPEED),
      m_flythroughProgress(0.0f),
      m_animSegment(0),
      m_firstPersonMode(false),
      m_elevationScale(DEFAULT_ELEVATION_SCALE),
      m_cameraTilt(DEFAULT_CAMERA_TILT),
      m_frameCount(0),
      m_fps(0),
      m_showDebugInfo(false),
      m_lastUpdateTime(0)
{
    setupUI();
    
    // Initialize animation timer
    m_flythroughTimer = new QTimer(this);
    m_flythroughTimer->setInterval(DEFAULT_FLYTHROUGH_INTERVAL);
    connect(m_flythroughTimer, &QTimer::timeout, this, &ElevationView3D::updateFlythrough);
    
    // Initialize FPS timer
    m_fpsTimer = new QTimer(this);
    m_fpsTimer->setInterval(FPS_UPDATE_INTERVAL);
    connect(m_fpsTimer, &QTimer::timeout, this, &ElevationView3D::updateFPS);
    m_fpsTimer->start();
    
    // Performance monitoring
    m_frameCounter.start();
}

ElevationView3D::~ElevationView3D()
{
    // Clean up is handled automatically through Qt parent/child relationships
}

void ElevationView3D::setupUI()
{
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create 3D window
    m_3dWindow = new Qt3DExtras::Qt3DWindow();
    m_3dWindow->defaultFrameGraph()->setClearColor(QColor(210, 230, 255));
    
    // Create root entity
    m_rootEntity = new Qt3DCore::QEntity();
    
    // Create camera
    m_camera = m_3dWindow->camera();
    m_camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_camera->setPosition(QVector3D(0, 80, 80));
    m_camera->setViewCenter(QVector3D(0, 0, 0));
    m_camera->setUpVector(QVector3D(0, 1, 0));
    
    // Add camera controller
    setupCamera();
    
    // Add global light
    Qt3DCore::QEntity* lightEntity = new Qt3DCore::QEntity(m_rootEntity);
    Qt3DRender::QPointLight* light = new Qt3DRender::QPointLight(lightEntity);
    light->setColor(QColor(255, 255, 255));
    light->setIntensity(1.0f);
    lightEntity->addComponent(light);
    
    Qt3DCore::QTransform* lightTransform = new Qt3DCore::QTransform(lightEntity);
    lightTransform->setTranslation(QVector3D(0, 150, 0));
    lightEntity->addComponent(lightTransform);
    
    // Set root entity
    m_3dWindow->setRootEntity(m_rootEntity);
    
    // Create container widget for 3D window
    QWidget* container = QWidget::createWindowContainer(m_3dWindow);
    container->setMinimumSize(640, 480);
    container->setFocusPolicy(Qt::StrongFocus);
    mainLayout->addWidget(container, 1);
    
    // Create control panel
    QGroupBox* controlPanel = new QGroupBox("3D Controls", this);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setContentsMargins(8, 8, 8, 8);
    controlLayout->setSpacing(4);
    
    // Create camera controls
    QHBoxLayout* cameraLayout = new QHBoxLayout();
    QPushButton* cameraToggleButton = new QPushButton("Toggle Camera Mode", this);
    connect(cameraToggleButton, &QPushButton::clicked, this, &ElevationView3D::toggleCameraMode);
    cameraLayout->addWidget(cameraToggleButton);
    
    // Elevation scale control
    QLabel* scaleLabel = new QLabel("Elevation Scale:", this);
    QSlider* scaleSlider = new QSlider(Qt::Horizontal, this);
    scaleSlider->setRange(10, 50);
    scaleSlider->setValue(m_elevationScale * 10);
    scaleSlider->setTickPosition(QSlider::TicksBelow);
    connect(scaleSlider, &QSlider::valueChanged, [this](int value) {
        setElevationScale(value / 10.0f);
    });
    cameraLayout->addWidget(scaleLabel);
    cameraLayout->addWidget(scaleSlider, 1);
    
    controlLayout->addLayout(cameraLayout);
    
    // Create flythrough controls
    QHBoxLayout* flythroughLayout = new QHBoxLayout();
    
    QPushButton* startButton = new QPushButton(this);
    startButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    startButton->setToolTip("Start Flythrough");
    connect(startButton, &QPushButton::clicked, this, &ElevationView3D::startFlythrough);
    flythroughLayout->addWidget(startButton);
    
    QPushButton* pauseButton = new QPushButton(this);
    pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    pauseButton->setToolTip("Pause Flythrough");
    connect(pauseButton, &QPushButton::clicked, this, &ElevationView3D::pauseFlythrough);
    flythroughLayout->addWidget(pauseButton);
    
    QPushButton* stopButton = new QPushButton(this);
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopButton->setToolTip("Stop Flythrough");
    connect(stopButton, &QPushButton::clicked, this, &ElevationView3D::stopFlythrough);
    flythroughLayout->addWidget(stopButton);
    
    // Speed control
    QLabel* speedLabel = new QLabel("Speed:", this);
    QSlider* speedSlider = new QSlider(Qt::Horizontal, this);
    speedSlider->setRange(1, 10);
    speedSlider->setValue(5);
    speedSlider->setTickPosition(QSlider::TicksBelow);
    connect(speedSlider, &QSlider::valueChanged, [this](int value) {
        m_flythroughSpeed = DEFAULT_FLYTHROUGH_SPEED * (value / 5.0f);
        updateAnimationSpeed();
    });
    
    flythroughLayout->addWidget(speedLabel);
    flythroughLayout->addWidget(speedSlider, 1);
    
    controlLayout->addLayout(flythroughLayout);
    
    // Camera tilt control (for first-person mode)
    QHBoxLayout* tiltLayout = new QHBoxLayout();
    QLabel* tiltLabel = new QLabel("Camera Tilt:", this);
    QSlider* tiltSlider = new QSlider(Qt::Horizontal, this);
    tiltSlider->setRange(0, 90);
    tiltSlider->setValue(m_cameraTilt);
    tiltSlider->setTickPosition(QSlider::TicksBelow);
    connect(tiltSlider, &QSlider::valueChanged, [this](int value) {
        setCameraTilt(value);
    });
    
    tiltLayout->addWidget(tiltLabel);
    tiltLayout->addWidget(tiltSlider, 1);
    
    controlLayout->addLayout(tiltLayout);
    
    // Add debug info label
    m_debugInfoLabel = new QLabel(this);
    m_debugInfoLabel->setStyleSheet("background-color: rgba(0,0,0,120); color: white; padding: 4px; font-family: monospace;");
    m_debugInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_debugInfoLabel->setVisible(m_showDebugInfo);
    
    // Add toggle for debug info
    QHBoxLayout* debugLayout = new QHBoxLayout();
    QPushButton* debugButton = new QPushButton("Toggle Performance Info", this);
    connect(debugButton, &QPushButton::clicked, [this]() {
        m_showDebugInfo = !m_showDebugInfo;
        m_debugInfoLabel->setVisible(m_showDebugInfo);
    });
    debugLayout->addWidget(debugButton);
    controlLayout->addLayout(debugLayout);
    
    mainLayout->addWidget(controlPanel);
}

void ElevationView3D::setupCamera()
{
    // Remove any existing controller
    for (Qt3DCore::QComponent* component : m_camera->components()) {
        m_camera->removeComponent(component);
    }
    
    if (m_firstPersonMode) {
        // First person camera controller (for flythrough)
        Qt3DExtras::QFirstPersonCameraController* camController = new Qt3DExtras::QFirstPersonCameraController(m_rootEntity);
        camController->setCamera(m_camera);
        camController->setLinearSpeed(50.0f);
        camController->setLookSpeed(180.0f);
    } else {
        // Orbit camera controller (for overview)
        Qt3DExtras::QOrbitCameraController* camController = new Qt3DExtras::QOrbitCameraController(m_rootEntity);
        camController->setCamera(m_camera);
        camController->setLinearSpeed(50.0f);
        camController->setLookSpeed(180.0f);
    }
}

void ElevationView3D::setTrackData(const std::vector<TrackPoint>& points)
{
    QElapsedTimer totalTimer;
    totalTimer.start();
    logInfo("ElevationView3D", QString("Starting 3D track data processing with %1 points").arg(points.size()));
    
    if (points.empty()) {
        logWarning("ElevationView3D", "No points provided, returning");
        return;
    }
    
    // Use a guard flag to prevent reentrancy issues
    static bool isUpdating = false;
    if (isUpdating) {
        logWarning("ElevationView3D", "Already updating, ignoring recursive call");
        return;
    }
    
    // Set the guard flag
    isUpdating = true;
    
    // Use a single shot timer to defer processing and keep UI responsive
    if (points.size() > 1000) {
        logInfo("ElevationView3D", "Large dataset detected, deferring 3D processing");
        QTimer::singleShot(100, this, [this, points]() {
            this->processTrackDataDeferred(points);
        });
        isUpdating = false;
        return;
    }
    
    try {
        // Clear existing track data first to prevent access to invalid memory
        logDebug("ElevationView3D", "Clearing track positions");
        m_trackPositions.clear();
        
        // Store original track points
        logDebug("ElevationView3D", "Storing track points");
        m_trackPoints = points;
        
        // Safely clean up existing entities
        safelyCleanupEntities();
        
        // Ensure we have a valid root entity
        logDebug("ElevationView3D", "Checking root entity");
        if (!m_rootEntity || m_rootEntity->parent() == nullptr) {
            logDebug("ElevationView3D", "Creating new root entity");
            m_rootEntity = new Qt3DCore::QEntity();
            if (m_3dWindow) {
                m_3dWindow->setRootEntity(m_rootEntity);
            } else {
                logWarning("ElevationView3D", "No 3D window available");
                isUpdating = false; // Reset guard flag
                return;
            }
        }
        
        // Convert track points to 3D positions
        convertTrackPointsTo3D(points);
        
        // Create new terrain and route entities
        createSceneEntities();
        
        // Reset camera to view the entire route
        resetCameraView();
        
        logDebug("ElevationView3D", "Completed successfully");
    } catch (const std::exception& e) {
        logCritical("ElevationView3D", QString("Exception during processing: %1").arg(e.what()));
    } catch (...) {
        logCritical("ElevationView3D", "Unknown exception during processing");
    }
    
    // Reset the guard flag
    isUpdating = false;
    
    // The guard flag will be automatically reset when this function exits
}

// New method to defer processing of large track data sets to keep UI responsive
void ElevationView3D::processTrackDataDeferred(const std::vector<TrackPoint>& points)
{
    QElapsedTimer totalTimer;
    totalTimer.start();
    static bool isUpdating = false;
    
    if (isUpdating) {
        logWarning("ElevationView3D", "Already processing in deferred mode, skipping");
        return;
    }
    
    isUpdating = true;
    
    try {
        // Clear existing track data first to prevent access to invalid memory
        logDebug("ElevationView3D", "Clearing track positions");
        m_trackPositions.clear();
        QCoreApplication::processEvents();
        
        // Store original track points
        logDebug("ElevationView3D", "Storing track points");
        m_trackPoints = points;
        QCoreApplication::processEvents();
        
        // Safely clean up existing entities
        safelyCleanupEntities();
        QCoreApplication::processEvents();
        
        // Ensure we have a valid root entity
        logDebug("ElevationView3D", "Checking root entity");
        if (!m_rootEntity || m_rootEntity->parent() == nullptr) {
            logDebug("ElevationView3D", "Creating new root entity");
            m_rootEntity = new Qt3DCore::QEntity();
            if (m_3dWindow) {
                m_3dWindow->setRootEntity(m_rootEntity);
            } else {
                logWarning("ElevationView3D", "No 3D window available");
                isUpdating = false; // Reset guard flag
                return;
            }
        }
        QCoreApplication::processEvents();
        
        // Convert track points to 3D positions - this can be intensive
        QElapsedTimer timer;
        timer.start();
        logInfo("ElevationView3D", "Converting track points to 3D positions...");
        convertTrackPointsTo3D(points);
        logInfo("ElevationView3D", QString("Conversion completed in %1 ms").arg(timer.elapsed()));
        QCoreApplication::processEvents();
        
        // Create new terrain and route entities in stages
        timer.restart();
        logInfo("ElevationView3D", "Creating scene entities...");
        createSceneEntities();
        logInfo("ElevationView3D", QString("Scene creation completed in %1 ms").arg(timer.elapsed()));
        QCoreApplication::processEvents();
        
        // Reset camera to view the entire route
        timer.restart();
        logInfo("ElevationView3D", "Resetting camera view...");
        resetCameraView();
        logInfo("ElevationView3D", QString("Camera reset completed in %1 ms").arg(timer.elapsed()));
        
        logInfo("ElevationView3D", QString("Total 3D processing completed in %1 ms").arg(totalTimer.elapsed()));
    } catch (const std::exception& e) {
        logCritical("ElevationView3D", QString("Exception during deferred processing: %1").arg(e.what()));
    } catch (...) {
        logCritical("ElevationView3D", "Unknown exception during deferred processing");
    }
    
    // Reset the guard flag
    isUpdating = false;
}

void ElevationView3D::safelyCleanupEntities()
{
    qDebug() << "ElevationView3D::safelyCleanupEntities - Starting cleanup";
    
    // Use standard approach without QPointer
    QList<Qt3DCore::QEntity*> entitiesToDelete;
    
    if (m_terrainEntity) {
        entitiesToDelete.append(m_terrainEntity);
        m_terrainEntity = nullptr;
    }
    
    if (m_routeEntity) {
        entitiesToDelete.append(m_routeEntity);
        m_routeEntity = nullptr;
    }
    
    if (m_markerEntity) {
        entitiesToDelete.append(m_markerEntity);
        m_markerEntity = nullptr;
    }
    
    // Process deletions more safely to avoid reentrancy and segfault issues
    if (!entitiesToDelete.isEmpty()) {
        // Critical fix: Create a heap-allocated copy of the entity list
        // This ensures the entity list survives beyond this method's scope
        auto* deleteList = new QList<Qt3DCore::QEntity*>(entitiesToDelete);
        
        // Use a QObject to own the lambda, ensuring it's deleted after execution
        QObject* cleanupHandler = new QObject(this);
        // Use the C++11 lambda syntax that captures the deleteList by value
        QTimer::singleShot(0, cleanupHandler, [cleanupHandler, deleteList]() {
            // Process each entity in our heap-allocated list
            for (Qt3DCore::QEntity* entity : *deleteList) {
                if (entity) {
                    // Just schedule deletion without modifying the entity
                    entity->deleteLater();
                }
            }
            // Clean up our heap-allocated list and the cleanup handler
            delete deleteList;
            cleanupHandler->deleteLater();
        });
    }
    
    qDebug() << "ElevationView3D::safelyCleanupEntities - Scheduled" << entitiesToDelete.size() 
             << "entities for deletion";
}

void ElevationView3D::convertTrackPointsTo3D(const std::vector<TrackPoint>& points)
{
    qDebug() << "ElevationView3D::convertTrackPointsTo3D - Converting" << points.size() << "points";
    
    // Pre-allocate memory to avoid reallocations
    m_trackPositions.reserve(std::min(points.size(), size_t(10000)));
    
    // Sample a subset of points if there are too many 
    const size_t MAX_POINTS_TO_RENDER = 5000; // Limit for very large datasets
    size_t sampleEvery = (points.size() > MAX_POINTS_TO_RENDER) ? 
        points.size() / MAX_POINTS_TO_RENDER + 1 : 1;
        
    if (sampleEvery > 1) {
        qDebug() << "ElevationView3D::convertTrackPointsTo3D - Sampling every" << sampleEvery 
                 << "points due to large dataset";
    }
    
    // Convert points to 3D positions
    for (size_t i = 0; i < points.size(); i += sampleEvery) {
        m_trackPositions.push_back(trackPointToVector3D(points[i]));
    }
    
    // Always ensure the last point is included for proper route termination
    if (!points.empty()) {
        QVector3D lastPoint = trackPointToVector3D(points.back());
        if (m_trackPositions.empty() || m_trackPositions.back() != lastPoint) {
            m_trackPositions.push_back(lastPoint);
        }
    }
    
    qDebug() << "ElevationView3D::convertTrackPointsTo3D - Processed" << m_trackPositions.size() 
             << "3D positions";
}

void ElevationView3D::createSceneEntities()
{
    // Only proceed if we have valid track positions
    if (m_trackPositions.empty()) {
        qWarning() << "ElevationView3D::createSceneEntities - No track positions available";
        return;
    }
    
    // Create terrain mesh
    qDebug() << "ElevationView3D::createSceneEntities - Creating terrain";
    createTerrain();
    
    // Create route visualization
    qDebug() << "ElevationView3D::createSceneEntities - Creating route";
    createRoute();
    
    // Create position marker
    qDebug() << "ElevationView3D::createSceneEntities - Creating marker";
    createMarker();
}

void ElevationView3D::resetCameraView()
{
    if (m_trackPositions.empty() || !m_camera) {
        return;
    }
    
    qDebug() << "ElevationView3D::resetCameraView - Resetting camera position";
    
    // Find bounding box of the route
    QVector3D routeMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    QVector3D routeMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
    
    for (const auto& pos : m_trackPositions) {
        routeMin.setX(std::min(routeMin.x(), pos.x()));
        routeMin.setY(std::min(routeMin.y(), pos.y()));
        routeMin.setZ(std::min(routeMin.z(), pos.z()));
        
        routeMax.setX(std::max(routeMax.x(), pos.x()));
        routeMax.setY(std::max(routeMax.y(), pos.y()));
        routeMax.setZ(std::max(routeMax.z(), pos.z()));
    }
    
    // Calculate center and size of the route
    QVector3D routeCenter = (routeMin + routeMax) * 0.5f;
    float routeSize = (routeMax - routeMin).length();
    
    // Position camera to view the entire route
    m_camera->setPosition(routeCenter + QVector3D(routeSize * 0.5f, routeSize * 0.5f, routeSize * 0.5f));
    m_camera->setViewCenter(routeCenter);
    
    // Reset animation parameters
    m_flythroughProgress = 0.0f;
    m_currentPositionIndex = 0;
    
    // Update marker to first position if it exists
    if (m_markerEntity && !m_trackPositions.empty()) {
        updateMarkerPosition(0);
    }
        
    qDebug() << "ElevationView3D::setTrackData - Completed successfully";
}

void ElevationView3D::createTerrain()
{
    qDebug() << "ElevationView3D::createTerrain - Starting";
    
    // Ensure we're not creating terrain with invalid data
    if (m_trackPoints.empty()) {
        qWarning() << "ElevationView3D::createTerrain - No track points available";
        return;
    }
    
    try {
        m_terrainEntity = createTerrainMesh(m_trackPoints);
        if (m_terrainEntity && m_rootEntity) {
            qDebug() << "ElevationView3D::createTerrain - Setting parent to root entity";
            m_terrainEntity->setParent(m_rootEntity);
        } else {
            qWarning() << "ElevationView3D::createTerrain - Failed to create terrain mesh or no root entity";
            if (m_terrainEntity) {
                m_terrainEntity->deleteLater();
                m_terrainEntity = nullptr;
            }
        }
    } catch (const std::exception& e) {
        qCritical() << "ElevationView3D::createTerrain - Exception:" << e.what();
    } catch (...) {
        qCritical() << "ElevationView3D::createTerrain - Unknown exception";
    }
    
    qDebug() << "ElevationView3D::createTerrain - Completed";
}

void ElevationView3D::createRoute()
{
    qDebug() << "ElevationView3D::createRoute - Starting";
    
    // Ensure we're not creating route with invalid data
    if (m_trackPoints.empty()) {
        qWarning() << "ElevationView3D::createRoute - No track points available";
        return;
    }
    
    try {
        QElapsedTimer timer;
        timer.start();
        
        m_routeEntity = createOptimizedRoutePath(m_trackPoints);
        if (m_routeEntity && m_rootEntity) {
            qDebug() << "ElevationView3D::createRoute - Setting parent to root entity";
            m_routeEntity->setParent(m_rootEntity);
        } else {
            qWarning() << "ElevationView3D::createRoute - Failed to create route path or no root entity";
            if (m_routeEntity) {
                m_routeEntity->deleteLater();
                m_routeEntity = nullptr;
            }
        }
        
        qDebug() << "ElevationView3D::createRoute - Completed in" << timer.elapsed() << "ms";
    } catch (const std::exception& e) {
        qCritical() << "ElevationView3D::createRoute - Exception:" << e.what();
    } catch (...) {
        qCritical() << "ElevationView3D::createRoute - Unknown exception";
    }
}

void ElevationView3D::createMarker()
{
    qDebug() << "ElevationView3D::createMarker - Starting";
    
    try {
        m_markerEntity = createPositionMarker();
        if (m_markerEntity && m_rootEntity) {
            qDebug() << "ElevationView3D::createMarker - Setting parent to root entity";
            m_markerEntity->setParent(m_rootEntity);
            
            // Position at start of route
            if (!m_trackPositions.empty()) {
                qDebug() << "ElevationView3D::createMarker - Updating marker position";
                updateMarkerPosition(0);
            } else {
                qWarning() << "ElevationView3D::createMarker - No track positions available";
            }
        } else {
            qWarning() << "ElevationView3D::createMarker - Failed to create position marker or no root entity";
            if (m_markerEntity) {
                m_markerEntity->deleteLater();
                m_markerEntity = nullptr;
            }
        }
    } catch (const std::exception& e) {
        qCritical() << "ElevationView3D::createMarker - Exception:" << e.what();
    } catch (...) {
        qCritical() << "ElevationView3D::createMarker - Unknown exception";
    }
    
    qDebug() << "ElevationView3D::createMarker - Completed";
}

Qt3DCore::QEntity* ElevationView3D::createTerrainMesh(const std::vector<TrackPoint>& points)
{
    qDebug() << "ElevationView3D::createTerrainMesh - Starting with" << points.size() << "points";
    
    if (points.empty()) {
        qWarning() << "ElevationView3D::createTerrainMesh - No points provided, returning nullptr";
        return nullptr;
    }
    
    try {
        // Create terrain entity
        Qt3DCore::QEntity* terrainEntity = new Qt3DCore::QEntity();
        
        // For a simple example, we'll create a cuboid mesh as a base
        Qt3DExtras::QCuboidMesh* terrainMesh = new Qt3DExtras::QCuboidMesh();
        
        // Find terrain dimensions
        double minLat = points[0].coord.latitude();
        double maxLat = points[0].coord.latitude();
        double minLon = points[0].coord.longitude();
        double maxLon = points[0].coord.longitude();
        double minEle = points[0].elevation;
        double maxEle = points[0].elevation;
        
        for (const auto& point : points) {
            minLat = std::min(minLat, point.coord.latitude());
            maxLat = std::max(maxLat, point.coord.latitude());
            minLon = std::min(minLon, point.coord.longitude());
            maxLon = std::max(maxLon, point.coord.longitude());
            minEle = std::min(minEle, point.elevation);
            maxEle = std::max(maxEle, point.elevation);
        }
        
        // Set dimensions
        float width = TERRAIN_WIDTH;
        float depth = TERRAIN_WIDTH * (maxLon - minLon) / (maxLat - minLat);
        float height = 5.0f; // Base height
        
        qDebug() << "ElevationView3D::createTerrainMesh - Setting terrain dimensions:" 
                 << "width =" << width << "depth =" << depth << "height =" << height;
        
        terrainMesh->setXExtent(width);
        terrainMesh->setYExtent(height);
        terrainMesh->setZExtent(depth);
        
        // Create material - use PhongAlpha material for better performance
        Qt3DExtras::QPhongAlphaMaterial* terrainMaterial = new Qt3DExtras::QPhongAlphaMaterial();
        terrainMaterial->setAmbient(QColor(100, 150, 100));
        terrainMaterial->setDiffuse(QColor(120, 180, 120));
        terrainMaterial->setSpecular(QColor(50, 50, 50));
        terrainMaterial->setShininess(10.0f);
        terrainMaterial->setAlpha(1.0f);
        
        // Create transform
        Qt3DCore::QTransform* terrainTransform = new Qt3DCore::QTransform();
        terrainTransform->setTranslation(QVector3D(0, -height / 2.0f, 0));
        
        // Add components to entity
        terrainEntity->addComponent(terrainMesh);
        terrainEntity->addComponent(terrainMaterial);
        terrainEntity->addComponent(terrainTransform);
        
        // Only add a few elevation markers, not for every interval
        int markerCount = 5; // Limit the number of elevation markers
        float elevRange = maxEle - minEle;
        float elevStep = elevRange / (markerCount - 1);
        
        for (int i = 0; i < markerCount; i++) {
            float elev = minEle + i * elevStep;
            
            Qt3DCore::QEntity* markerEntity = new Qt3DCore::QEntity(terrainEntity);
            
            // Create text mesh for elevation label
            Qt3DExtras::QExtrudedTextMesh* textMesh = new Qt3DExtras::QExtrudedTextMesh();
            textMesh->setText(QString("%1m").arg(qRound(elev)));
            textMesh->setDepth(0.2f);
            textMesh->setFont(QFont("Arial", 5));
            
            // Create transform - position at the far edge of the terrain
            Qt3DCore::QTransform* textTransform = new Qt3DCore::QTransform();
            textTransform->setTranslation(QVector3D(-TERRAIN_WIDTH/2, elev * m_elevationScale, -TERRAIN_WIDTH/2));
            textTransform->setScale(5.0f);
            
            // Create material - use a simpler material for text
            Qt3DExtras::QPhongAlphaMaterial* textMaterial = new Qt3DExtras::QPhongAlphaMaterial();
            textMaterial->setDiffuse(QColor(40, 40, 40));
            textMaterial->setAmbient(QColor(40, 40, 40));
            textMaterial->setSpecular(QColor(40, 40, 40, 128));
            textMaterial->setShininess(5.0f);
            
            // Add components
            markerEntity->addComponent(textMesh);
            markerEntity->addComponent(textMaterial);
            markerEntity->addComponent(textTransform);
        }
        
        qDebug() << "ElevationView3D::createTerrainMesh - Completed successfully";
        return terrainEntity;
    } catch (const std::exception& e) {
        qCritical() << "ElevationView3D::createTerrainMesh - Exception:" << e.what();
        return nullptr;
    } catch (...) {
        qCritical() << "ElevationView3D::createTerrainMesh - Unknown exception occurred";
        return nullptr;
    }
}

Qt3DCore::QEntity* ElevationView3D::createOptimizedRoutePath(const std::vector<TrackPoint>& points)
{
    if (points.size() < 2) {
        return nullptr;
    }
    
    // Create the root route entity
    Qt3DCore::QEntity* routeEntity = new Qt3DCore::QEntity();
    
    // Simplify the route if it's too detailed
    std::vector<TrackPoint> simplifiedPoints;
    simplifyRoute(points, simplifiedPoints, MAX_ROUTE_VERTICES);
    
    // Group segments into batches for better performance
    size_t currentBatch = 0;
    size_t totalBatches = (simplifiedPoints.size() / ROUTE_SEGMENT_BATCH_SIZE) + 1;
    
    while (currentBatch < totalBatches) {
        // Create custom mesh entity for this batch
        size_t startIdx = currentBatch * ROUTE_SEGMENT_BATCH_SIZE;
        size_t endIdx = std::min(startIdx + ROUTE_SEGMENT_BATCH_SIZE, simplifiedPoints.size() - 1);
        
        // Skip if we don't have enough points for this batch
        if (startIdx >= simplifiedPoints.size() - 1) {
            break;
        }
        
        // Create a batch entity
        createBatchedSegments(routeEntity, simplifiedPoints, startIdx, endIdx);
        currentBatch++;
    }
    
    return routeEntity;
}

void ElevationView3D::createBatchedSegments(Qt3DCore::QEntity* parentEntity, 
                                          const std::vector<TrackPoint>& points,
                                          size_t startIdx, size_t endIdx)
{
    qDebug() << "ElevationView3D::createBatchedSegments - Starting for indices" 
             << startIdx << "to" << endIdx << "(total points:" << points.size() << ")";
    
    if (startIdx >= points.size() - 1 || endIdx >= points.size()) {
        qWarning() << "ElevationView3D::createBatchedSegments - Invalid indices, returning";
        return;
    }
    
    // Create entity for this batch
    Qt3DCore::QEntity* batchEntity = new Qt3DCore::QEntity(parentEntity);
    
    // Create a custom mesh for better performance
    Qt3DRender::QGeometry* geometry = new Qt3DRender::QGeometry(batchEntity);
    
    // Create buffers with correct UsageType enum values
    Qt3DRender::QBuffer* positionBuffer = new Qt3DRender::QBuffer(geometry);
    positionBuffer->setUsage(Qt3DRender::QBuffer::StaticDraw); // Changed from VertexBuffer to StaticDraw
    
    Qt3DRender::QBuffer* indexBuffer = new Qt3DRender::QBuffer(geometry);
    indexBuffer->setUsage(Qt3DRender::QBuffer::StaticDraw);  // Changed from IndexBuffer to StaticDraw
    
    Qt3DRender::QBuffer* colorBuffer = new Qt3DRender::QBuffer(geometry);
    colorBuffer->setUsage(Qt3DRender::QBuffer::StaticDraw);  // Changed from VertexBuffer to StaticDraw
    
    // Calculate average color and gradient for this batch
    QVector<QVector3D> positions;
    QVector<QVector3D> colors;
    QVector<uint> indices;
    
    const float lineRadius = 0.4f; // Thinner lines for better performance
    int segmentResolution = 6; // Lower resolution for better performance
    
    // Generate a cylinder-like geometry for each segment
    for (size_t i = startIdx; i < endIdx; i++) {
        // Calculate segment properties
        QVector3D start = trackPointToVector3D(points[i]);
        QVector3D end = trackPointToVector3D(points[i + 1]);
        
        // Calculate segment color based on gradient
        float gradient = 0.0f;
        if (i < points.size() - 1) {
            float elevDiff = points[i + 1].elevation - points[i].elevation;
            float distance = points[i + 1].distance - points[i].distance;
            if (distance > 0) {
                gradient = (elevDiff / distance) * 100.0f;
            }
        }
        
        // Choose color based on gradient
        QVector3D segmentColor;
        if (gradient > 10.0f) {
            segmentColor = QVector3D(1.0f, 0.0f, 0.0f); // Steep climb - red
        } else if (gradient > 5.0f) {
            segmentColor = QVector3D(1.0f, 0.65f, 0.0f); // Moderate climb - orange
        } else if (gradient > 2.0f) {
            segmentColor = QVector3D(1.0f, 1.0f, 0.0f); // Mild climb - yellow
        } else if (gradient < -10.0f) {
            segmentColor = QVector3D(0.5f, 0.0f, 0.5f); // Steep descent - purple
        } else if (gradient < -5.0f) {
            segmentColor = QVector3D(0.0f, 0.0f, 1.0f); // Moderate descent - blue
        } else if (gradient < -2.0f) {
            segmentColor = QVector3D(0.4f, 0.7f, 1.0f); // Mild descent - light blue
        } else {
            segmentColor = QVector3D(0.0f, 0.5f, 0.0f); // Flat - green
        }
        
        // Calculate segment direction
        QVector3D direction = end - start;
        direction.normalize();
        
        // Define cylinder vertices around the segment
        QVector3D right, up;
        
        // Find perpendicular vectors to create a cylinder
        if (qAbs(direction.y()) > 0.99f) {
            right = QVector3D(1.0f, 0.0f, 0.0f); // Handle vertical segments
        } else {
            right = QVector3D::crossProduct(direction, QVector3D(0.0f, 1.0f, 0.0f)).normalized();
        }
        up = QVector3D::crossProduct(right, direction).normalized();
        
        // Starting index for this segment
        uint baseIndex = positions.size();
        
        // Generate a simpler tube for the segment
        for (int j = 0; j < segmentResolution; j++) {
            float angle = j * 2.0f * M_PI / segmentResolution;
            float x = ::cos(angle);
            float y = ::sin(angle);
            
            // Add vertex at the start cap
            QVector3D offset = right * x * lineRadius + up * y * lineRadius;
            positions.append(start + offset);
            colors.append(segmentColor);
            
            // Add vertex at the end cap
            positions.append(end + offset);
            colors.append(segmentColor);
            
            // Add indices for triangles (2 triangles per side of cylinder)
            uint nextJ = (j + 1) % segmentResolution;
            uint curr1 = baseIndex + j * 2;
            uint curr2 = baseIndex + j * 2 + 1;
            uint next1 = baseIndex + nextJ * 2;
            uint next2 = baseIndex + nextJ * 2 + 1;
            
            // First triangle
            indices.append(curr1);
            indices.append(next1);
            indices.append(curr2);
            
            // Second triangle
            indices.append(curr2);
            indices.append(next1);
            indices.append(next2);
        }
    }
    
    // Set buffer data
    QByteArray positionData(reinterpret_cast<const char*>(positions.constData()), 
                          positions.size() * 3 * sizeof(float));
    positionBuffer->setData(positionData);
    
    QByteArray colorData(reinterpret_cast<const char*>(colors.constData()),
                       colors.size() * 3 * sizeof(float));
    colorBuffer->setData(colorData);
    
    QByteArray indexData(reinterpret_cast<const char*>(indices.constData()),
                       indices.size() * sizeof(uint));
    indexBuffer->setData(indexData);
    
    // Add attributes to the geometry
    Qt3DRender::QAttribute* positionAttribute = new Qt3DRender::QAttribute(geometry);
    positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(positionBuffer);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setCount(positions.size());
    geometry->addAttribute(positionAttribute);
    
    Qt3DRender::QAttribute* colorAttribute = new Qt3DRender::QAttribute(geometry);
    colorAttribute->setName(Qt3DRender::QAttribute::defaultColorAttributeName());
    colorAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    colorAttribute->setVertexSize(3);
    colorAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    colorAttribute->setBuffer(colorBuffer);
    colorAttribute->setByteOffset(0);
    colorAttribute->setByteStride(3 * sizeof(float));
    colorAttribute->setCount(colors.size());
    geometry->addAttribute(colorAttribute);
    
    Qt3DRender::QAttribute* indexAttribute = new Qt3DRender::QAttribute(geometry);
    indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    indexAttribute->setVertexBaseType(Qt3DRender::QAttribute::UnsignedInt);
    indexAttribute->setBuffer(indexBuffer);
    indexAttribute->setCount(indices.size());
    geometry->addAttribute(indexAttribute);
    
    // Create a geometry renderer
    Qt3DRender::QGeometryRenderer* mesh = new Qt3DRender::QGeometryRenderer();
    mesh->setGeometry(geometry);
    mesh->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
    
    // Create material - use faster PhongAlphaMaterial
    Qt3DExtras::QPhongAlphaMaterial* material = new Qt3DExtras::QPhongAlphaMaterial();
    material->setAmbient(QColor(50, 50, 50));
    material->setDiffuse(QColor(180, 180, 180)); // Will be modulated by vertex colors
    material->setSpecular(QColor(80, 80, 80));
    material->setShininess(20.0f);
    material->setAlpha(1.0f);
    
    // Add components to entity
    batchEntity->addComponent(mesh);
    batchEntity->addComponent(material);
}

// New helper method to simplify route points for better performance
void ElevationView3D::simplifyRoute(const std::vector<TrackPoint>& input, 
                                  std::vector<TrackPoint>& output, 
                                  int maxPoints)
{
    qDebug() << "ElevationView3D::simplifyRoute - Starting with" << input.size() 
             << "points, max output =" << maxPoints;
    
    output.clear(); // Make sure output is empty before starting
    
    if (input.empty()) {
        qWarning() << "ElevationView3D::simplifyRoute - No input points, returning early";
        return; // Return early if there's no input
    }
    
    try {
        if (static_cast<size_t>(maxPoints) >= input.size()) {
            // No simplification needed
            qDebug() << "ElevationView3D::simplifyRoute - No simplification needed, copying all points";
            output = input;
            return;
        }
        
        // Calculate how many points to keep
        int keepEveryNth = std::max(1, static_cast<int>(input.size() / maxPoints));
        qDebug() << "ElevationView3D::simplifyRoute - Will keep every" << keepEveryNth << "point";
        
        // Allocate enough space upfront to avoid reallocations
        output.reserve(maxPoints + 2);
        
        // Start with the first point
        output.push_back(input.front());
        
        // Add the subset of points
        for (size_t i = 1; i < input.size() - 1; i += keepEveryNth) {
            output.push_back(input[i]);
        }
        
        // Always add the last point if not already added
        if (output.size() == 1 || output.back().coord != input.back().coord) {
            output.push_back(input.back());
        }
        
        qDebug() << "ElevationView3D::simplifyRoute - Route simplified from" << input.size() 
                 << "to" << output.size() << "points";
    } catch (const std::exception& e) {
        qCritical() << "ElevationView3D::simplifyRoute - Exception:" << e.what();
    } catch (...) {
        qCritical() << "ElevationView3D::simplifyRoute - Unknown exception occurred";
    }
}

Qt3DCore::QEntity* ElevationView3D::createPositionMarker()
{
    // Create a sphere to represent the current position
    Qt3DCore::QEntity* markerEntity = new Qt3DCore::QEntity();
    
    // Create mesh
    Qt3DExtras::QSphereMesh* markerMesh = new Qt3DExtras::QSphereMesh();
    markerMesh->setRadius(MARKER_SIZE);
    markerMesh->setRings(16);
    markerMesh->setSlices(16);
    
    // Create material
    Qt3DExtras::QDiffuseSpecularMaterial* markerMaterial = new Qt3DExtras::QDiffuseSpecularMaterial();
    markerMaterial->setDiffuse(QColor(255, 0, 0));
    markerMaterial->setSpecular(QColor(255, 255, 255));
    markerMaterial->setShininess(50.0f);
    
    // Create transform
    Qt3DCore::QTransform* markerTransform = new Qt3DCore::QTransform();
    
    // Add components
    markerEntity->addComponent(markerMesh);
    markerEntity->addComponent(markerMaterial);
    markerEntity->addComponent(markerTransform);
    
    return markerEntity;
}

void ElevationView3D::updateMarkerPosition(size_t pointIndex)
{
    qDebug() << "ElevationView3D::updateMarkerPosition - Updating to index" << pointIndex
             << "(total positions:" << m_trackPositions.size() << ")";
    
    if (pointIndex >= m_trackPositions.size() || !m_markerEntity) {
        qWarning() << "ElevationView3D::updateMarkerPosition - Invalid index or no marker entity, returning";
        return;
    }
    
    m_currentPositionIndex = pointIndex;
    
    // Update marker position
    Qt3DCore::QTransform* markerTransform = m_markerEntity->findChild<Qt3DCore::QTransform*>();
    if (markerTransform) {
        markerTransform->setTranslation(m_trackPositions[pointIndex]);
    } else {
        qWarning() << "ElevationView3D::updateMarkerPosition - No transform component found on marker";
    }
}

void ElevationView3D::updatePosition(size_t pointIndex)
{
    if (pointIndex >= m_trackPositions.size()) {
        return;
    }
    
    // Update marker
    updateMarkerPosition(pointIndex);
    
    // If in first-person mode, update camera position
    if (m_firstPersonMode && !m_flythroughActive) {
        // Get current position
        QVector3D currentPos = m_trackPositions[pointIndex];
        
        // Calculate look direction
        QVector3D lookDir;
        if (pointIndex < m_trackPositions.size() - 1) {
            lookDir = (m_trackPositions[pointIndex + 1] - currentPos).normalized();
        } else if (pointIndex > 0) {
            lookDir = (currentPos - m_trackPositions[pointIndex - 1]).normalized();
        } else {
            lookDir = QVector3D(1, 0, 0); // Default look direction
        }
        
        // Apply camera tilt
        QVector3D upVector(0, 1, 0);
        QQuaternion tiltRotation = QQuaternion::fromAxisAndAngle(
            QVector3D::crossProduct(lookDir, upVector).normalized(), 
            -m_cameraTilt
        );
        lookDir = tiltRotation.rotatedVector(lookDir);
        
        // Set camera position slightly above the track
        QVector3D cameraPos = currentPos + QVector3D(0, 1.5f, 0);
        QVector3D lookTarget = cameraPos + lookDir * 10.0f;
        
        // Update camera
        m_camera->setPosition(cameraPos);
        m_camera->setViewCenter(lookTarget);
    }
}

void ElevationView3D::startFlythrough()
{
    if (m_trackPositions.empty()) {
        return;
    }
    
    // Start animation
    m_flythroughActive = true;
    m_flythroughTimer->start();
    
    // Switch to first-person mode
    m_firstPersonMode = true;
    setupCamera();
    
    // Set camera to starting position
    m_flythroughProgress = 0.0f;
    updateFlythrough();
}

void ElevationView3D::pauseFlythrough()
{
    m_flythroughActive = !m_flythroughActive;
    
    if (m_flythroughActive) {
        m_flythroughTimer->start();
    } else {
        m_flythroughTimer->stop();
    }
}

void ElevationView3D::stopFlythrough()
{
    // Stop the animation
    m_flythroughActive = false;
    m_flythroughTimer->stop();
    m_flythroughProgress = 0.0f;
    
    // Reset camera to overview mode
    m_firstPersonMode = false;
    setupCamera();
    
    // Reset marker to start
    updateMarkerPosition(0);
    
    // Reset camera to view the entire route
    if (!m_trackPositions.empty()) {
        // Find center of route
        QVector3D routeMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        QVector3D routeMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
        
        for (const auto& pos : m_trackPositions) {
            routeMin.setX(std::min(routeMin.x(), pos.x()));
            routeMin.setY(std::min(routeMin.y(), pos.y()));
            routeMin.setZ(std::min(routeMin.z(), pos.z()));
            
            routeMax.setX(std::max(routeMax.x(), pos.x()));
            routeMax.setY(std::max(routeMax.y(), pos.y()));
            routeMax.setZ(std::max(routeMax.z(), pos.z()));
        }
        
        QVector3D routeCenter = (routeMin + routeMax) * 0.5f;
        float routeSize = (routeMax - routeMin).length();
        
        // Position camera to view the entire route
        m_camera->setPosition(routeCenter + QVector3D(routeSize * 0.5f, routeSize * 0.5f, routeSize * 0.5f));
        m_camera->setViewCenter(routeCenter);
    }
    
    // Emit position changed signal for the first point
    emit positionChanged(0);
}

void ElevationView3D::toggleCameraMode()
{
    m_firstPersonMode = !m_firstPersonMode;
    setupCamera();
    
    if (m_firstPersonMode) {
        // In first-person mode, move camera to current position
        updatePosition(m_currentPositionIndex);
    } else {
        // In overview mode, show entire route
        if (!m_trackPositions.empty()) {
            // Find center of route
            QVector3D routeMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
            QVector3D routeMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
            
            for (const auto& pos : m_trackPositions) {
                routeMin.setX(std::min(routeMin.x(), pos.x()));
                routeMin.setY(std::min(routeMin.y(), pos.y()));
                routeMin.setZ(std::min(routeMin.z(), pos.z()));
                
                routeMax.setX(std::max(routeMax.x(), pos.x()));
                routeMax.setY(std::max(routeMax.y(), pos.y()));
                routeMax.setZ(std::max(routeMax.z(), pos.z()));
            }
            
            QVector3D routeCenter = (routeMin + routeMax) * 0.5f;
            float routeSize = (routeMax - routeMin).length();
            
            // Position camera to view the entire route
            m_camera->setPosition(routeCenter + QVector3D(routeSize * 0.5f, routeSize * 0.5f, routeSize * 0.5f));
            m_camera->setViewCenter(routeCenter);
        }
    }
}

void ElevationView3D::setElevationScale(float scale)
{
    if (qFuzzyCompare(m_elevationScale, scale)) {
        return;
    }
    
    // Store new scale
    m_elevationScale = scale;
    
    // Regenerate the visualization with the new scale
    if (!m_trackPoints.empty()) {
        setTrackData(m_trackPoints);
    }
}

void ElevationView3D::setCameraTilt(float angle)
{
    m_cameraTilt = angle;
    
    // Update camera view if in first person mode
    if (m_firstPersonMode && !m_flythroughActive) {
        updatePosition(m_currentPositionIndex);
    }
}

void ElevationView3D::updateFlythrough()
{
    if (!m_flythroughActive || m_trackPositions.size() < 2) {
        return;
    }
    
    // Limit updates if we're running slowly
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastUpdateTime < DEFAULT_FLYTHROUGH_INTERVAL) {
        return; // Skip this update to maintain performance
    }
    m_lastUpdateTime = now;
    
    // Increment progress along the route
    m_flythroughProgress += m_flythroughSpeed;
    
    // Check if we've completed the route
    if (m_flythroughProgress >= 1.0f) {
        stopFlythrough();
        return;
    }
    
    // Find current position in the route
    size_t pointIndex = findClosestPointOnRoute(m_flythroughProgress);
    
    // Update marker to current position
    updateMarkerPosition(pointIndex);
    
    // Emit position changed signal
    emit positionChanged(static_cast<int>(pointIndex));
    
    // Calculate camera position
    QVector3D currentPos = m_trackPositions[pointIndex];
    
    // Calculate look direction (toward next point)
    QVector3D lookDir;
    if (pointIndex < m_trackPositions.size() - 1) {
        lookDir = (m_trackPositions[pointIndex + 1] - currentPos).normalized();
    } else {
        lookDir = (currentPos - m_trackPositions[pointIndex - 1]).normalized();
    }
    
    // Apply camera tilt
    QVector3D upVector(0, 1, 0);
    QQuaternion tiltRotation = QQuaternion::fromAxisAndAngle(
        QVector3D::crossProduct(lookDir, upVector).normalized(), 
        -m_cameraTilt
    );
    lookDir = tiltRotation.rotatedVector(lookDir);
    
    // Set camera position slightly above the track
    QVector3D cameraPos = currentPos + QVector3D(0, 1.5f, 0);
    QVector3D lookTarget = cameraPos + lookDir * 10.0f;
    
    // Update camera
    m_camera->setPosition(cameraPos);
    m_camera->setViewCenter(lookTarget);
}

size_t ElevationView3D::findClosestPointOnRoute(float progress)
{
    if (m_trackPositions.empty()) {
        return 0;
    }
    
    // Calculate total route length
    float totalLength = 0.0f;
    std::vector<float> cumulativeDistances;
    cumulativeDistances.push_back(0.0f);
    
    for (size_t i = 1; i < m_trackPositions.size(); i++) {
        float segmentLength = (m_trackPositions[i] - m_trackPositions[i-1]).length();
        totalLength += segmentLength;
        cumulativeDistances.push_back(totalLength);
    }
    
    // Normalize to 0.0 - 1.0
    for (auto& dist : cumulativeDistances) {
        dist /= totalLength;
    }
    
    // Find the segment containing the target progress
    float targetDist = progress;
    
    for (size_t i = 0; i < cumulativeDistances.size() - 1; i++) {
        if (targetDist >= cumulativeDistances[i] && targetDist <= cumulativeDistances[i+1]) {
            // The target is between i and i+1
            return i;
        }
    }
    
    // If we get here, we're at the end of the route
    return m_trackPositions.size() - 1;
}

void ElevationView3D::updateAnimationSpeed()
{
    // No need to adjust timer interval, the speed is controlled by progress increment
}

QVector3D ElevationView3D::trackPointToVector3D(const TrackPoint& point, bool scaleElevation)
{
    // For visualization purpose, convert GPS coordinates to a local 3D space
    static double originLat = 0.0;
    static double originLon = 0.0;
    
    // Set origin to the first point in the track if not yet set
    if (qFuzzyIsNull(originLat) && qFuzzyIsNull(originLon) && !m_trackPoints.empty()) {
        originLat = m_trackPoints[0].coord.latitude();
        originLon = m_trackPoints[0].coord.longitude();
        qDebug() << "ElevationView3D::trackPointToVector3D - Set origin to" 
                 << originLat << "," << originLon;
    }
    
    // Scale factor to convert degrees to a reasonable distance in 3D space
    // Using an approximate conversion (111.32 km per degree latitude)
    const double scale = 111320.0 / 10.0; // Scale down by a factor of 10
    
    // Calculate X (east-west) component - use cos from cmath directly
    double x = (point.coord.longitude() - originLon) * scale * ::cos(originLat * M_PI / 180.0);
    
    // Calculate Z (north-south) component
    double z = (point.coord.latitude() - originLat) * scale;
    
    // Y is elevation
    double y = point.elevation;
    if (scaleElevation) {
        y *= m_elevationScale;
    }
    
    QVector3D result(x, y, z);
    // Only log occasionally to avoid flooding the console
    // Fix: Replace deprecated qrand() with QRandomGenerator
    if (QRandomGenerator::global()->bounded(1000) == 0) {
        qDebug() << "ElevationView3D::trackPointToVector3D - Converted" 
                 << point.coord.latitude() << "," << point.coord.longitude() 
                 << "," << point.elevation << "to" << result;
    }
    
    return result;
}

void ElevationView3D::setCameraPosition(const QVector3D& position, const QVector3D& target)
{
    m_camera->setPosition(position);
    m_camera->setViewCenter(target);
}

void ElevationView3D::updateFPS()
{
    // Calculate frames per second
    qint64 elapsed = m_frameCounter.elapsed();
    m_fps = m_frameCount * 1000.0 / elapsed;
    m_frameCount = 0;
    m_frameCounter.restart();
    
    // Update debug info
    if (m_showDebugInfo) {
        QString debugText = QString("FPS: %1\n").arg(m_fps, 0, 'f', 1);
        debugText += QString("Points: %1\n").arg(m_trackPositions.size());
        debugText += QString("Elev. Scale: %1x\n").arg(m_elevationScale);
        debugText += QString("Camera Mode: %1\n").arg(m_firstPersonMode ? "First-Person" : "Orbit");
        debugText += QString("Animation: %1\n").arg(m_flythroughActive ? "Active" : "Stopped");
        
        m_debugInfoLabel->setText(debugText);
    }
}

// Called by Qt3D when a frame is rendered
bool ElevationView3D::event(QEvent* event)
{
    // Count frames for FPS calculation
    if (event->type() == QEvent::UpdateRequest) {
        m_frameCount++;
    }
    
    return QWidget::event(event);
}
