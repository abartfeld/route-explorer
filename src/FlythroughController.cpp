#include "FlythroughController.h"
#include "logging.h"
#include <QTimer>
#include <Qt3DRender/QCamera>

namespace {
    constexpr int ANIMATION_INTERVAL_MS = 16; // ~60 FPS
    constexpr float DEFAULT_SPEED = 0.001f; // Progress per update
    constexpr float CAMERA_HEIGHT_OFFSET = 1.8f; // meters above track
}

FlythroughController::FlythroughController(RouteData* routeData, Qt3DRender::QCamera* camera, QObject* parent)
    : QObject(parent),
      m_routeData(routeData),
      m_camera(camera),
      m_timer(new QTimer(this)),
      m_isActive(false),
      m_progress(0.0f),
      m_speed(DEFAULT_SPEED)
{
    logDebug("FlythroughController", "Creating controller.");
    connect(m_timer, &QTimer::timeout, this, &FlythroughController::updateAnimation);
}

FlythroughController::~FlythroughController()
{
    logDebug("FlythroughController", "Destroying controller.");
}

void FlythroughController::start()
{
    logDebug("FlythroughController", "Start requested.");
    if (m_progress >= 1.0f) {
        m_progress = 0.0f;
    }
    m_isActive = true;
    m_timer->start(ANIMATION_INTERVAL_MS);
}

void FlythroughController::pause()
{
    logDebug("FlythroughController", "Pause requested.");
    if (m_isActive) {
        m_isActive = false;
        m_timer->stop();
    }
}

void FlythroughController::stop()
{
    logDebug("FlythroughController", "Stop requested.");
    m_isActive = false;
    m_timer->stop();
    m_progress = 0.0f;

    // Manually move camera to the start position
    if (m_routeData) {
        RoutePoint startPoint = m_routeData->getPointAtProgress(0.0f);
        QVector3D cameraPos = startPoint.position + QVector3D(0, CAMERA_HEIGHT_OFFSET, 0);
        QVector3D lookTarget = cameraPos + startPoint.direction * 10.0f;
        m_camera->setPosition(cameraPos);
        m_camera->setViewCenter(lookTarget);
    }

    emit positionChanged(0);
}

void FlythroughController::setSpeed(float speed)
{
    logDebug("FlythroughController", QString("Set speed to %1.").arg(speed));
    // The input speed is a multiplier, e.g., 1.0f is normal speed
    m_speed = DEFAULT_SPEED * speed;
}

void FlythroughController::updateAnimation()
{
    if (!m_isActive) {
        return;
    }

    m_progress += m_speed;
    if (m_progress >= 1.0f) {
        stop();
        return;
    }

    RoutePoint currentPoint = m_routeData->getPointAtProgress(m_progress);

    // Update camera
    QVector3D cameraPos = currentPoint.position + QVector3D(0, CAMERA_HEIGHT_OFFSET, 0);
    QVector3D lookTarget = cameraPos + currentPoint.direction * 10.0f; // Look 10m ahead
    m_camera->setPosition(cameraPos);
    m_camera->setViewCenter(lookTarget);

    // Emit positionChanged with the correct point index.
    size_t currentIndex = m_routeData->getIndexAtProgress(m_progress);
    emit positionChanged(static_cast<int>(currentIndex));
}
