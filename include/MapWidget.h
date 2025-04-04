#pragma once
#include <QWidget>
#include <QtPositioning/QGeoCoordinate>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QCache>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <vector>
#include <QToolTip>
#include "TrackStatsWidget.h" // Include to use TrackSegment

/**
 * @brief Widget that displays an interactive map with routes and markers
 * 
 * This simpler map widget implementation focuses on reliable panning and
 * zooming with proper tile loading.
 */
class QCPPainter; // Forward declaration

class MapWidget : public QWidget {
    Q_OBJECT
public:
    MapWidget(QWidget* parent = nullptr);
    ~MapWidget();
    
    void setRoute(const std::vector<QGeoCoordinate>& coordinates);
    void updateMarker(const QGeoCoordinate& coordinate);
    
    // New method to set route with segment information
    void setRouteWithSegments(const std::vector<QGeoCoordinate>& coordinates, 
                             const std::vector<TrackSegment>& segments,
                             const std::vector<TrackPoint>& points);
                             
    // Get the raw track points for hover information
    void setTrackPoints(const std::vector<TrackPoint>& points);
    
signals:
    // Signal to notify about hover position change
    void routeHovered(int pointIndex);
    
protected:
    // Event handlers
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    
private:
    // Map state
    int mZoom;
    QGeoCoordinate mCenterCoordinate;
    bool mIsPanning;
    QPoint mLastMousePos;
    
    // Route and marker
    QList<QGeoCoordinate> mRouteCoordinates;
    QGeoCoordinate mCurrentMarkerCoordinate;
    std::vector<TrackPoint> mTrackPoints;
    
    // Hover detection
    int mHoverPointIndex;
    QPoint mHoverPoint;
    bool mShowTooltip;
    
    // Segments for colored route display
    struct RouteSegment {
        QList<QGeoCoordinate> coordinates;
        QColor color;
    };
    QList<RouteSegment> mRouteSegments;
    bool mHasSegments;
    
    // Tile management
    QNetworkAccessManager* mNetworkManager;
    QCache<QString, QPixmap> mTileCache;
    
    // Helper methods
    QPixmap getTile(int x, int y, int z);
    QPoint geoToPixel(const QGeoCoordinate& coord, const QGeoCoordinate& center, int zoom, const QSize& size);
    QGeoCoordinate pixelToGeo(const QPoint& pixel, const QGeoCoordinate& center, int zoom, const QSize& size);
    QColor getSegmentColor(const TrackSegment& segment) const;
    QColor enhanceColor(const QColor& color) const; // Helper method to improve color visibility
    
    // Route hover detection
    int findClosestRoutePoint(const QPoint& mousePos);
    double calculateDistanceToLine(const QPoint& mousePos, const QPoint& lineStart, const QPoint& lineEnd);
};
