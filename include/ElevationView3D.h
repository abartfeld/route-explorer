#pragma once

#include <QWidget>
#include <vector>
#include "GpxParser.h"
#include "TerrainService.h"
#include <QPushButton>
#include <QSlider>

// Forward declarations for our new architecture
class RouteData;
class RouteRenderer;
class FlythroughController;
namespace Qt3DExtras {
    class Qt3DWindow;
    class QOrbitCameraController;
}
namespace Qt3DCore { class QEntity; }

class ElevationView3D : public QWidget {
    Q_OBJECT

public:
    explicit ElevationView3D(QWidget* parent = nullptr);
    ~ElevationView3D();

    // Public API used by MainWindow
    void setTrackData(const std::vector<TrackPoint>& points);
    void updatePosition(size_t pointIndex);
    void setElevationScale(float scale);

signals:
    // Public signal used by MainWindow
    void positionChanged(int pointIndex);

private slots:
    void onPlayPause(bool checked);
    void onTerrainDataReady(const TerrainData& data);

private:
    void setupUi();
    void resetCameraView();

    // Main Qt3D components
    Qt3DExtras::Qt3DWindow* m_3dWindow;
    Qt3DCore::QEntity* m_rootEntity;

    // New modular components
    RouteData* m_routeData;
    RouteRenderer* m_routeRenderer;
    FlythroughController* m_flythroughController;
    Qt3DCore::QEntity* m_markerEntity;
    Qt3DExtras::QOrbitCameraController* m_orbitController;
    Qt3DCore::QEntity* m_terrainEntity;

    // Services
    TerrainService* m_terrainService;

    // Data
    std::vector<TrackPoint> m_trackPoints;
    float m_elevationScale;

    // UI Elements
    QPushButton* m_playPauseButton;
    QPushButton* m_stopButton;
    QSlider* m_speedSlider;
};
