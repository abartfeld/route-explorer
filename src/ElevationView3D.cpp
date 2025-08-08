#include "ElevationView3D.h"
#include "logging.h"
#include "RouteData.h"
#include "RouteRenderer.h"
#include "FlythroughController.h"
#include <QVBoxLayout>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QPointLight>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStyle>
#include <cmath>
#include <QVector2D>

namespace {
    // Constants for coordinate conversion
    constexpr double EARTH_RADIUS_METERS = 6378137.0;

    // Function to convert latitude/longitude to local X/Z coordinates using Mercator projection
    QVector2D lonLatToMercator(double lon, double lat, double originLon, double originLat) {
        double x = EARTH_RADIUS_METERS * (lon - originLon) * (M_PI / 180.0) * std::cos(originLat * (M_PI / 180.0));
        double z = EARTH_RADIUS_METERS * (lat - originLat) * (M_PI / 180.0);
        return QVector2D(static_cast<float>(x), static_cast<float>(z));
    }
}


ElevationView3D::ElevationView3D(QWidget* parent)
    : QWidget(parent),
      m_3dWindow(new Qt3DExtras::Qt3DWindow()),
      m_rootEntity(new Qt3DCore::QEntity()),
      m_routeData(nullptr),
      m_routeRenderer(nullptr),
      m_flythroughController(nullptr),
      m_markerEntity(nullptr),
      m_terrainEntity(nullptr),
      m_terrainService(new TerrainService(this)),
      m_elevationScale(1.0f),
      m_playPauseButton(nullptr),
      m_stopButton(nullptr),
      m_speedSlider(nullptr)
{
    logDebug("ElevationView3D", "Initializing new ElevationView3D");
    connect(m_terrainService, &TerrainService::terrainDataReady, this, &ElevationView3D::onTerrainDataReady);
    setupUi();
}

ElevationView3D::~ElevationView3D()
{
    logDebug("ElevationView3D", "Destroying ElevationView3D");
    // UI elements are managed by Qt's parent-child system.
    // We still need to manage our custom objects.
    delete m_routeData;
    delete m_routeRenderer;
    delete m_flythroughController;
}

void ElevationView3D::setupUi()
{
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Create a container for the 3D window
    QWidget* container = QWidget::createWindowContainer(m_3dWindow);
    container->setMinimumSize(640, 480);
    mainLayout->addWidget(container, 1);

    // --- Control Panel ---
    auto* controlPanel = new QGroupBox("Animation Controls", this);
    auto* controlLayout = new QHBoxLayout(controlPanel);

    m_playPauseButton = new QPushButton(this);
    m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playPauseButton->setToolTip("Start/Pause Flythrough");
    m_playPauseButton->setCheckable(true);
    m_playPauseButton->setEnabled(false);
    controlLayout->addWidget(m_playPauseButton);

    m_stopButton = new QPushButton(this);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setToolTip("Stop Flythrough");
    m_stopButton->setEnabled(false);
    controlLayout->addWidget(m_stopButton);

    auto* overviewButton = new QPushButton("Overview", this);
    controlLayout->addWidget(overviewButton);
    connect(overviewButton, &QPushButton::clicked, this, [this](){
        m_orbitController->setEnabled(true);
        resetCameraView();
    });

    controlLayout->addWidget(new QLabel("Speed:", this));
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setRange(1, 20); // 0.1x to 2.0x speed
    m_speedSlider->setValue(10);
    m_speedSlider->setEnabled(false);
    controlLayout->addWidget(m_speedSlider);

    mainLayout->addWidget(controlPanel);


    // Basic scene setup
    m_3dWindow->setRootEntity(m_rootEntity);
    m_3dWindow->defaultFrameGraph()->setClearColor(QColor(210, 230, 255));

    // Camera
    Qt3DRender::QCamera* camera = m_3dWindow->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.1f, 10000.0f); // Increased far plane
    camera->setPosition(QVector3D(0, 40, 80));
    camera->setViewCenter(QVector3D(0, 0, 0));

    // Add a camera controller for easy navigation
    m_orbitController = new Qt3DExtras::QOrbitCameraController(m_rootEntity);
    m_orbitController->setCamera(camera);
    m_orbitController->setLinearSpeed(200.0f); // Adjust speed for larger scenes
    m_orbitController->setLookSpeed(180.0f);

    // Add global light
    auto* lightEntity = new Qt3DCore::QEntity(m_rootEntity);
    auto* light = new Qt3DRender::QPointLight(lightEntity);
    light->setColor(QColor(255, 255, 255));
    light->setIntensity(1.0f);
    lightEntity->addComponent(light);
    auto* lightTransform = new Qt3DCore::QTransform(lightEntity);
    lightTransform->setTranslation(QVector3D(0, 500, 0));
    lightEntity->addComponent(lightTransform);

    // Create marker entity
    m_markerEntity = new Qt3DCore::QEntity(m_rootEntity);
    auto* markerMesh = new Qt3DExtras::QSphereMesh();
    markerMesh->setRadius(2.0f);
    auto* markerMaterial = new Qt3DExtras::QPhongMaterial();
    markerMaterial->setDiffuse(QColor(QRgb(0xFF0000)));
    auto* markerTransform = new Qt3DCore::QTransform();
    m_markerEntity->addComponent(markerMesh);
    m_markerEntity->addComponent(markerMaterial);
    m_markerEntity->addComponent(markerTransform);
    m_markerEntity->setEnabled(false); // Initially hidden
}

void ElevationView3D::setTrackData(const std::vector<TrackPoint>& points)
{
    logInfo("ElevationView3D", QString("Setting new track data with %1 points.").arg(points.size()));
    m_trackPoints = points;

    // 1. Clean up old data
    delete m_flythroughController;
    m_flythroughController = nullptr;
    delete m_routeRenderer;
    m_routeRenderer = nullptr;
    delete m_routeData;
    m_routeData = nullptr;
    m_markerEntity->setEnabled(false);

    if (m_trackPoints.size() < 2) {
        logWarning("ElevationView3D", "Not enough points to draw a route.");
        m_playPauseButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        m_speedSlider->setEnabled(false);
        return;
    }

    // 2. Create data and renderer
    m_routeData = new RouteData(m_trackPoints, m_elevationScale);
    m_routeRenderer = new RouteRenderer(m_routeData, m_rootEntity);

    // 3. Create controller and connect UI
    m_flythroughController = new FlythroughController(m_routeData, m_3dWindow->camera(), this);
    
    connect(m_playPauseButton, &QPushButton::toggled, this, &ElevationView3D::onPlayPause);
    connect(m_stopButton, &QPushButton::clicked, m_flythroughController, &FlythroughController::stop);
    connect(m_stopButton, &QPushButton::clicked, this, [this](){
        m_orbitController->setEnabled(true);
        m_playPauseButton->setChecked(false);
    });
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int value) {
        m_flythroughController->setSpeed(value / 10.0f);
    });

    // Propagate signal to MainWindow and update marker
    connect(m_flythroughController, &FlythroughController::positionChanged, this, &ElevationView3D::positionChanged);
    connect(m_flythroughController, &FlythroughController::positionChanged, this, &ElevationView3D::updatePosition);


    // Enable controls
    m_playPauseButton->setEnabled(true);
    m_stopButton->setEnabled(true);
    m_speedSlider->setEnabled(true);
    m_markerEntity->setEnabled(true);

    // Reset camera to view the new route and update marker to start
    resetCameraView();
    updatePosition(0);

    // 4. Fetch terrain data
    double minLat = m_trackPoints.front().coord.latitude();
    double maxLat = m_trackPoints.front().coord.latitude();
    double minLon = m_trackPoints.front().coord.longitude();
    double maxLon = m_trackPoints.front().coord.longitude();
    for (const auto& point : m_trackPoints) {
        minLat = std::min(minLat, point.coord.latitude());
        maxLat = std::max(maxLat, point.coord.latitude());
        minLon = std::min(minLon, point.coord.longitude());
        maxLon = std::max(maxLon, point.coord.longitude());
    }
    double latBuffer = (maxLat - minLat) * 0.2; // Add a 20% buffer
    double lonBuffer = (maxLon - minLon) * 0.2;
    m_terrainService->fetchTerrainData(maxLat + latBuffer, minLat - latBuffer, minLon - lonBuffer, maxLon + lonBuffer, 100, 100);
}

void ElevationView3D::resetCameraView()
{
    if (!m_routeData || m_routeData->getPositions().empty()) {
        return;
    }

    const auto& positions = m_routeData->getPositions();
    QVector3D routeMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    QVector3D routeMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    for (const auto& pos : positions) {
        routeMin.setX(std::min(routeMin.x(), pos.x()));
        routeMin.setY(std::min(routeMin.y(), pos.y()));
        routeMin.setZ(std::min(routeMin.z(), pos.z()));
        routeMax.setX(std::max(routeMax.x(), pos.x()));
        routeMax.setY(std::max(routeMax.y(), pos.y()));
        routeMax.setZ(std::max(routeMax.z(), pos.z()));
    }

    QVector3D routeCenter = (routeMin + routeMax) * 0.5f;
    float routeSize = (routeMax - routeMin).length();
    QVector3D cameraPos = routeCenter + QVector3D(0, routeSize * 0.5f, routeSize * 1.2f);

    logDebug("ElevationView3D", QString("Route BBox Min: (%1, %2, %3)").arg(routeMin.x()).arg(routeMin.y()).arg(routeMin.z()));
    logDebug("ElevationView3D", QString("Route BBox Max: (%1, %2, %3)").arg(routeMax.x()).arg(routeMax.y()).arg(routeMax.z()));
    logDebug("ElevationView3D", QString("Route Center: (%1, %2, %3)").arg(routeCenter.x()).arg(routeCenter.y()).arg(routeCenter.z()));
    logDebug("ElevationView3D", QString("Route Size: %1").arg(routeSize));
    logDebug("ElevationView3D", QString("Setting camera position to: (%1, %2, %3)").arg(cameraPos.x()).arg(cameraPos.y()).arg(cameraPos.z()));

    m_3dWindow->camera()->setPosition(cameraPos);
    m_3dWindow->camera()->setViewCenter(routeCenter);
}

void ElevationView3D::updatePosition(size_t pointIndex)
{
    if (!m_routeData || !m_markerEntity) {
        return;
    }

    const auto& positions = m_routeData->getPositions();
    if (pointIndex >= positions.size()) {
        return;
    }

    // Update marker position
    if (auto* transform = m_markerEntity->findChild<Qt3DCore::QTransform*>()) {
        transform->setTranslation(positions[pointIndex]);
    }
}

void ElevationView3D::onTerrainDataReady(const TerrainData& data)
{
    logInfo("ElevationView3D", "Received terrain data. Generating mesh...");

    // Clean up old terrain entity
    delete m_terrainEntity;
    m_terrainEntity = new Qt3DCore::QEntity(m_rootEntity);

    if (data.elevationGrid.empty() || data.elevationGrid[0].empty()) {
        logWarning("ElevationView3D", "Terrain data is empty, cannot generate mesh.");
        return;
    }

    auto* geometry = new Qt3DRender::QGeometry(m_terrainEntity);
    QByteArray vertexBufferData;
    QByteArray indexBufferData;

    const int gridHeight = data.elevationGrid.size();
    const int gridWidth = data.elevationGrid[0].size();
    const int numVertices = gridWidth * gridHeight;
    const int numIndices = (gridWidth - 1) * (gridHeight - 1) * 2 * 3;

    vertexBufferData.resize(numVertices * (3 + 3) * sizeof(float)); // Pos + Normal
    float* p = reinterpret_cast<float*>(vertexBufferData.data());

    indexBufferData.resize(numIndices * sizeof(unsigned int));
    unsigned int* indices = reinterpret_cast<unsigned int*>(indexBufferData.data());

    const double originLon = m_trackPoints.front().coord.longitude();
    const double originLat = m_trackPoints.front().coord.latitude();

    // Generate vertices
    for (int i = 0; i < gridHeight; ++i) {
        for (int j = 0; j < gridWidth; ++j) {
            double lat = data.topLeft.latitude() - i * (data.topLeft.latitude() - data.bottomRight.latitude()) / (gridHeight - 1);
            double lon = data.topLeft.longitude() + j * (data.bottomRight.longitude() - data.topLeft.longitude()) / (gridWidth - 1);

            QVector2D mercatorCoords = lonLatToMercator(lon, lat, originLon, originLat);
            float elevation = data.elevationGrid[i][j] * m_elevationScale;

            *p++ = mercatorCoords.x();
            *p++ = elevation;
            *p++ = mercatorCoords.y();

            // Placeholder for normals
            *p++ = 0.0f;
            *p++ = 1.0f;
            *p++ = 0.0f;
        }
    }

    // Generate indices
    for (int i = 0; i < gridHeight - 1; ++i) {
        for (int j = 0; j < gridWidth - 1; ++j) {
            unsigned int i0 = i * gridWidth + j;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = (i + 1) * gridWidth + j;
            unsigned int i3 = i2 + 1;
            *indices++ = i0; *indices++ = i2; *indices++ = i1;
            *indices++ = i1; *indices++ = i2; *indices++ = i3;
        }
    }

    auto* vertexBuffer = new Qt3DRender::QBuffer(geometry);
    vertexBuffer->setData(vertexBufferData);
    auto* indexBuffer = new Qt3DRender::QBuffer(geometry);
    indexBuffer->setData(indexBufferData);

    // Attributes
    auto* posAttr = new Qt3DRender::QAttribute(geometry);
    posAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    posAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
    posAttr->setVertexSize(3);
    posAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    posAttr->setBuffer(vertexBuffer);
    posAttr->setByteStride(6 * sizeof(float));
    posAttr->setCount(numVertices);
    geometry->addAttribute(posAttr);

    auto* normAttr = new Qt3DRender::QAttribute(geometry);
    normAttr->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());
    normAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
    normAttr->setVertexSize(3);
    normAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normAttr->setBuffer(vertexBuffer);
    normAttr->setByteOffset(3 * sizeof(float));
    normAttr->setByteStride(6 * sizeof(float));
    normAttr->setCount(numVertices);
    geometry->addAttribute(normAttr);

    auto* indexAttr = new Qt3DRender::QAttribute(geometry);
    indexAttr->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    indexAttr->setBuffer(indexBuffer);
    indexAttr->setVertexBaseType(Qt3DRender::QAttribute::UnsignedInt);
    indexAttr->setCount(numIndices);
    geometry->addAttribute(indexAttr);

    auto* renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setGeometry(geometry);
    renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);

    auto* material = new Qt3DExtras::QPhongMaterial();
    material->setDiffuse(QColor(QRgb(0x8B4513))); // SaddleBrown

    m_terrainEntity->addComponent(renderer);
    m_terrainEntity->addComponent(material);
}

void ElevationView3D::setElevationScale(float scale)
{
    logInfo("ElevationView3D", QString("Setting elevation scale to %1x").arg(scale));
    if (qFuzzyCompare(m_elevationScale, scale)) {
        return;
    }

    m_elevationScale = scale;

    // Trigger a full rebuild of the scene with the new scale
    if (!m_trackPoints.empty()) {
        setTrackData(m_trackPoints);
    }
}

void ElevationView3D::onPlayPause(bool checked)
{
    if (checked) {
        // Play
        m_orbitController->setEnabled(false);
        m_flythroughController->start();
        m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    } else {
        // Pause
        m_flythroughController->pause();
        m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}
