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

/**
 * @brief Widget that displays an interactive map with routes and markers
 * 
 * This simpler map widget implementation focuses on reliable panning and
 * zooming with proper tile loading.
 */
class MapWidget : public QWidget {
    Q_OBJECT
public:
    MapWidget(QWidget* parent = nullptr);
    ~MapWidget();
    
    void setRoute(const std::vector<QGeoCoordinate>& coordinates);
    void updateMarker(const QGeoCoordinate& coordinate);
    
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
    
    // Tile management
    QNetworkAccessManager* mNetworkManager;
    QCache<QString, QPixmap> mTileCache;
    
    // Helper methods
    QPixmap getTile(int x, int y, int z);
    QPoint geoToPixel(const QGeoCoordinate& coord, const QGeoCoordinate& center, int zoom, const QSize& size);
    QGeoCoordinate pixelToGeo(const QPoint& pixel, const QGeoCoordinate& center, int zoom, const QSize& size);
};
