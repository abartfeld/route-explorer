#pragma once

#include "RouteData.h"
#include <QObject>

namespace Qt3DRender { class QCamera; }
class QTimer;

class FlythroughController : public QObject {
    Q_OBJECT

public:
    FlythroughController(RouteData* routeData, Qt3DRender::QCamera* camera, QObject* parent = nullptr);
    ~FlythroughController();

public slots:
    void start();
    void pause();
    void stop();
    void setSpeed(float speed);

signals:
    void positionChanged(int pointIndex);

private slots:
    void updateAnimation();

private:
    RouteData* m_routeData;
    Qt3DRender::QCamera* m_camera;

    // Animation state
    QTimer* m_timer;
    bool m_isActive;
    float m_progress; // 0.0 to 1.0
    float m_speed;
};
