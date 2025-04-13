#pragma once

#include <QWidget>
#include <QLabel>
#include <QPointer>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QMesh>
#include <Qt3DRender/QMaterial>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QSphereMesh>
#include <QPropertyAnimation>
#include <QVector3D>
#include <QTimer>
#include <QElapsedTimer>
#include <vector>
#include <memory>
#include "GpxParser.h"

/**
 * @brief Widget for 3D visualization of elevation profiles
 * 
 * This class provides a 3D view of the route with realistic terrain 
 * representation and fly-through animation capabilities.
 */
class ElevationView3D : public QWidget {
    Q_OBJECT
    
public:
    explicit ElevationView3D(QWidget* parent = nullptr);
    ~ElevationView3D();
    
    /**
     * @brief Set the GPX track data for 3D visualization
     * @param points Track points to visualize
     */
    void setTrackData(const std::vector<TrackPoint>& points);
    
    /**
     * @brief Process track data in a deferred manner to prevent UI freezing
     * @param points Track points to visualize
     */
    void processTrackDataDeferred(const std::vector<TrackPoint>& points);
    
    /**
     * @brief Update the current position marker
     * @param pointIndex Index of the current point on the track
     */
    void updatePosition(size_t pointIndex);
    
public slots:
    /**
     * @brief Start fly-through animation along the route
     */
    void startFlythrough();
    
    /**
     * @brief Pause fly-through animation
     */
    void pauseFlythrough();
    
    /**
     * @brief Stop fly-through animation and reset camera
     */
    void stopFlythrough();
    
    /**
     * @brief Toggle between first-person and overview camera modes
     */
    void toggleCameraMode();
    
    /**
     * @brief Change the vertical scale factor for elevation
     * @param scale Elevation scale factor (1.0 = actual scale)
     */
    void setElevationScale(float scale);
    
    /**
     * @brief Set camera tilt angle for first-person view
     * @param angle Tilt angle in degrees (0 = level, 90 = looking down)
     */
    void setCameraTilt(float angle);
    
signals:
    /**
     * @brief Signal emitted when position changes during fly-through
     * @param pointIndex Index of the current position
     */
    void positionChanged(int pointIndex);
    
private slots:
    void updateFlythrough();
    void updateAnimationSpeed();
    void updateFPS();
    
private:
    // Qt3D setup
    Qt3DExtras::Qt3DWindow* m_3dWindow;
    Qt3DCore::QEntity* m_rootEntity;
    Qt3DRender::QCamera* m_camera;
    Qt3DCore::QEntity* m_terrainEntity;
    Qt3DCore::QEntity* m_routeEntity;
    Qt3DCore::QEntity* m_markerEntity;
    
    // Track data
    std::vector<QVector3D> m_trackPositions;
    std::vector<TrackPoint> m_trackPoints;
    size_t m_currentPositionIndex;
    
    // Animation
    QTimer* m_flythroughTimer;
    bool m_flythroughActive;
    float m_flythroughSpeed;
    float m_flythroughProgress;
    int m_animSegment;
    
    // Configuration
    bool m_firstPersonMode;
    float m_elevationScale;
    float m_cameraTilt;
    
    // Performance monitoring
    QElapsedTimer m_frameCounter;
    int m_frameCount;
    double m_fps;
    bool m_showDebugInfo;  // Reordered to fix initialization order warning
    QLabel* m_debugInfoLabel;
    QTimer* m_fpsTimer;
    qint64 m_lastUpdateTime;  // Moved this after m_showDebugInfo
    
    // Override event handling for frame counting
    bool event(QEvent* event) override;
    
    // Helper methods
    void setupUI();
    void setupCamera();
    
    // Memory-safe entity management methods
    void safelyCleanupEntities();
    void convertTrackPointsTo3D(const std::vector<TrackPoint>& points);
    void createSceneEntities();
    void resetCameraView();
    
    // Entity creation methods
    void createTerrain();
    void createRoute();
    void createMarker();
    Qt3DCore::QEntity* createTerrainMesh(const std::vector<TrackPoint>& points);
    Qt3DCore::QEntity* createOptimizedRoutePath(const std::vector<TrackPoint>& points);
    void createBatchedSegments(Qt3DCore::QEntity* parentEntity, const std::vector<TrackPoint>& points,
                             size_t startIdx, size_t endIdx);
    void simplifyRoute(const std::vector<TrackPoint>& input, std::vector<TrackPoint>& output, int maxPoints);
    Qt3DCore::QEntity* createPositionMarker();
    
    // Utility methods
    void updateMarkerPosition(size_t pointIndex);
    QVector3D trackPointToVector3D(const TrackPoint& point, bool scaleElevation = true);
    void setCameraPosition(const QVector3D& position, const QVector3D& target);
    size_t findClosestPointOnRoute(float progress);
};
