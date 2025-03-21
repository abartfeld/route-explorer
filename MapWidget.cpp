#include "MapWidget.h"
#include <QDebug>
#include <QGuiApplication>
#include <QPainter>
#include <cmath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainterPath>  // Add this include for QPainterPath
#include <QRandomGenerator> // Added for QRandomGenerator

// Constants for tile handling
const int TILE_SIZE = 256;
const QString TILE_SERVER = "https://a.tile.openstreetmap.org/%1/%2/%3.png";

MapWidget::MapWidget(QWidget* parent) : QWidget(parent),
    mZoom(5),
    mIsPanning(false),
    mLastMousePos(0, 0),
    mNetworkManager(new QNetworkAccessManager(this)),
    mTileCache(200) // Cache up to 200 tiles
{
    // Set basic widget properties
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Initialize to center on continental US
    mCenterCoordinate = QGeoCoordinate(39.8283, -98.5795);
    
    // Initialize route and marker
    mRouteCoordinates.clear();
    mCurrentMarkerCoordinate = mCenterCoordinate;
}

MapWidget::~MapWidget()
{
    // Cleanup is handled by Qt's parent-child memory management
}

void MapWidget::setRoute(const std::vector<QGeoCoordinate>& coordinates) {
    // Clear any previous route
    mRouteCoordinates.clear();
    if (coordinates.empty()) return;
    
    for (const auto& coord : coordinates) {
        mRouteCoordinates.append(coord);
    }
    
    // Calculate bounds of the route
    double minLat = coordinates[0].latitude();
    double maxLat = minLat;
    double minLon = coordinates[0].longitude();
    double maxLon = minLon;
    
    // Find the actual bounds (more efficiently)
    for (const auto& coord : coordinates) {
        if (coord.latitude() < minLat) minLat = coord.latitude();
        else if (coord.latitude() > maxLat) maxLat = coord.latitude();
        
        if (coord.longitude() < minLon) minLon = coord.longitude();
        else if (coord.longitude() > maxLon) maxLon = coord.longitude();
    }
    
    // Center on the route
    mCenterCoordinate = QGeoCoordinate((minLat + maxLat) / 2.0, (minLon + maxLon) / 2.0);
    
    // Calculate better zoom to fit the route
    double latSpan = maxLat - minLat;
    double lonSpan = maxLon - minLon;
    
    // Add padding
    latSpan *= 1.2;
    lonSpan *= 1.2;
    
    // Calculate appropriate zoom level
    int zoomLat = floor(log2(360.0 / latSpan));
    int zoomLon = floor(log2(360.0 / lonSpan));
    mZoom = qMax(1, qMin(18, qMin(zoomLat, zoomLon)));
    
    // Set marker to the first point
    if (!coordinates.empty()) {
        mCurrentMarkerCoordinate = coordinates[0];
    }
    
    // Schedule a repaint (more efficient than immediate update)
    update();
}

void MapWidget::updateMarker(const QGeoCoordinate& coordinate)
{
    mCurrentMarkerCoordinate = coordinate;
    update();
}

void MapWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill background
    painter.fillRect(rect(), QColor(240, 240, 240));
    
    // Calculate visible area - store these for reuse to avoid recalculation
    int tilesPerSide = 1 << mZoom;
    double centerX = (mCenterCoordinate.longitude() + 180.0) / 360.0 * tilesPerSide;
    double centerY = (1.0 - log(tan(mCenterCoordinate.latitude() * M_PI / 180.0) + 
                            1.0 / cos(mCenterCoordinate.latitude() * M_PI / 180.0)) / M_PI) / 2.0 * tilesPerSide;
    
    // Calculate viewport dimensions in tiles
    int halfWidgetWidth = width() / 2;
    int halfWidgetHeight = height() / 2;
    
    // Calculate tile range to display - optimize by using integer math where possible
    int minTileX = floor(centerX - (double)halfWidgetWidth / TILE_SIZE);
    int maxTileX = ceil(centerX + (double)halfWidgetWidth / TILE_SIZE);
    int minTileY = floor(centerY - (double)halfWidgetHeight / TILE_SIZE);
    int maxTileY = ceil(centerY + (double)halfWidgetHeight / TILE_SIZE);
    
    // Clamp tile coordinates to valid range
    minTileX = qMax(0, minTileX);
    maxTileX = qMin(tilesPerSide - 1, maxTileX);
    minTileY = qMax(0, minTileY);
    maxTileY = qMin(tilesPerSide - 1, maxTileY);
    
    // Calculate pixel offset for center tile
    int centerPixelX = width() / 2 - (centerX - floor(centerX)) * TILE_SIZE;
    int centerPixelY = height() / 2 - (centerY - floor(centerY)) * TILE_SIZE;
    
    // Draw visible tiles - improved efficiency
    QPixmap placeholder(TILE_SIZE, TILE_SIZE);
    placeholder.fill(QColor(240, 240, 240));
    
    for (int y = minTileY; y <= maxTileY; ++y) {
        int pixelY = centerPixelY + (y - floor(centerY)) * TILE_SIZE;
        
        for (int x = minTileX; x <= maxTileX; ++x) {
            int pixelX = centerPixelX + (x - floor(centerX)) * TILE_SIZE;
            
            // Draw the tile
            QPixmap tile = getTile(x, y, mZoom);
            painter.drawPixmap(pixelX, pixelY, tile);
        }
    }
    
    // Draw the route if we have one - optimize by using path and checking path emptiness first
    if (mRouteCoordinates.size() > 1) {
        QPainterPath path;
        bool first = true;
        
        for (const auto& coord : mRouteCoordinates) {
            QPoint point = geoToPixel(coord, mCenterCoordinate, mZoom, size());
            if (first) {
                path.moveTo(point);
                first = false;
            } else {
                path.lineTo(point);
            }
        }
        
        // Only draw if path is not empty
        if (!path.isEmpty()) {
            painter.setPen(QPen(QColor("#2196F3"), 3.0));
            painter.drawPath(path);
        }
    }
    
    // Draw the marker
    QPoint markerPos = geoToPixel(mCurrentMarkerCoordinate, mCenterCoordinate, mZoom, size());
    
    // Define marker appearance
    const int markerSize = 12;
    QRect markerRect(markerPos.x() - markerSize/2, markerPos.y() - markerSize/2, markerSize, markerSize);
    
    // Draw a red circle with white border
    painter.setBrush(QBrush(QColor("#F44336")));
    painter.setPen(QPen(Qt::white, 2));
    painter.drawEllipse(markerRect);
}

void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        mIsPanning = true;
        mLastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mIsPanning) {
        // Calculate movement in pixels
        QPoint delta = event->pos() - mLastMousePos;
        mLastMousePos = event->pos();
        
        // Convert pixel movement to geo coordinates at current zoom level
        double tilesPerSide = 1 << mZoom;
        double dx = -delta.x() / (TILE_SIZE * tilesPerSide) * 360.0;
        
        // Latitude movement needs special handling due to Mercator projection
        double currentLatRad = mCenterCoordinate.latitude() * M_PI / 180.0;
        double dyRad = delta.y() / (TILE_SIZE * tilesPerSide) * 2 * M_PI;
        double newLatRad = currentLatRad + dyRad;
        
        // Clamp latitude to valid range (-85.05 to 85.05 degrees)
        newLatRad = qMax(-1.4, qMin(1.4, newLatRad));
        
        // Update the center coordinate
        mCenterCoordinate.setLatitude(newLatRad * 180.0 / M_PI);
        mCenterCoordinate.setLongitude(mCenterCoordinate.longitude() + dx);
        
        // Redraw the map
        update();
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && mIsPanning) {
        mIsPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
}

void MapWidget::wheelEvent(QWheelEvent *event)
{
    // Calculate zoom change
    int numSteps = event->angleDelta().y() / 120;
    
    // Get the position under the mouse in geo coordinates
    QPoint mousePos = event->position().toPoint();
    QGeoCoordinate coordUnderMouse = pixelToGeo(mousePos, mCenterCoordinate, mZoom, size());
    
    // Change zoom level with finer control
    int oldZoom = mZoom;
    mZoom = qBound(1, mZoom + numSteps, 19); // Allow higher zoom level (19 instead of 18)
    
    // Only proceed if zoom actually changed
    if (oldZoom != mZoom) {
        // If we're zooming at a location other than center, adjust center to maintain mouse position
        if (mousePos != QPoint(width()/2, height()/2)) {
            // Calculate where the point under the mouse would end up after zoom
            QPoint newPoint = geoToPixel(coordUnderMouse, mCenterCoordinate, mZoom, size());
            
            // Calculate the difference and shift the center accordingly
            QPoint delta = mousePos - newPoint;
            
            // Convert pixel shift to geo coordinates at new zoom level
            double tilesPerSide = 1 << mZoom;
            double dx = -delta.x() / (TILE_SIZE * tilesPerSide) * 360.0;
            
            // Latitude movement needs special handling due to Mercator projection
            double currentLatRad = mCenterCoordinate.latitude() * M_PI / 180.0;
            double dyRad = delta.y() / (TILE_SIZE * tilesPerSide) * 2 * M_PI;
            double newLatRad = currentLatRad + dyRad;
            
            // Clamp latitude to valid range (-85.05 to 85.05 degrees)
            newLatRad = qMax(-1.4, qMin(1.4, newLatRad));
            
            // Update the center coordinate
            mCenterCoordinate.setLatitude(newLatRad * 180.0 / M_PI);
            mCenterCoordinate.setLongitude(mCenterCoordinate.longitude() + dx);
        }
        
        update();
    }
    
    event->accept();
}

QPixmap MapWidget::getTile(int x, int y, int z)
{
    // Create a cache key
    QString cacheKey = QString("%1/%2/%3").arg(z).arg(x).arg(y);
    
    // Check if the tile is in the cache
    QPixmap* cachedTile = mTileCache.object(cacheKey);
    if (cachedTile) {
        return *cachedTile;
    }
    
    // Check if the tile is on disk
    QString filename = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + 
                      QString("/maptiles/%1-%2-%3.png").arg(z).arg(x).arg(y);
    QFile file(filename);
    
    if (file.exists()) {
        QPixmap pixmap;
        if (pixmap.load(filename)) {
            mTileCache.insert(cacheKey, new QPixmap(pixmap));
            return pixmap;
        }
    }
    
    // If not found, create a placeholder tile and request it from the server
    QPixmap placeholder(TILE_SIZE, TILE_SIZE);
    placeholder.fill(QColor(240, 240, 240));  // Light gray
    
    // Add URLs for multiple tile servers to handle load balancing and avoid rate limiting
    QStringList tileServers;
    tileServers << "https://a.tile.openstreetmap.org/%1/%2/%3.png"
                << "https://b.tile.openstreetmap.org/%1/%2/%3.png"
                << "https://c.tile.openstreetmap.org/%1/%2/%3.png";
    
    // Replace qrand() with QRandomGenerator
    QString urlTemplate = tileServers.at(QRandomGenerator::global()->bounded(tileServers.size()));
    QString url = urlTemplate.arg(z).arg(x).arg(y);
    
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "GPX Viewer App/1.0");
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    
    QNetworkReply *reply = mNetworkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pixmap;
            if (pixmap.loadFromData(reply->readAll())) {
                // Cache the tile
                mTileCache.insert(cacheKey, new QPixmap(pixmap));
                
                // Save to disk
                QDir().mkpath(QFileInfo(filename).absolutePath());
                pixmap.save(filename);
                
                // Update the map if this is a visible tile
                update();
            }
        }
    });
    
    return placeholder;
}

QPoint MapWidget::geoToPixel(const QGeoCoordinate& coord, const QGeoCoordinate& center, int zoom, const QSize& size)
{
    double tilesPerSide = 1 << zoom;
    
    // Convert center to normalized coordinates (0-1)
    double centerX = (center.longitude() + 180.0) / 360.0;
    double centerY = (1.0 - log(tan(center.latitude() * M_PI / 180.0) + 
                            1.0 / cos(center.latitude() * M_PI / 180.0)) / M_PI) / 2.0;
    
    // Convert coord to normalized coordinates (0-1)
    double x = (coord.longitude() + 180.0) / 360.0;
    double y = (1.0 - log(tan(coord.latitude() * M_PI / 180.0) + 
                        1.0 / cos(coord.latitude() * M_PI / 180.0)) / M_PI) / 2.0;
    
    // Convert to pixel position relative to center
    int pixelX = size.width() / 2 + (x - centerX) * tilesPerSide * TILE_SIZE;
    int pixelY = size.height() / 2 + (y - centerY) * tilesPerSide * TILE_SIZE;
    
    return QPoint(pixelX, pixelY);
}

QGeoCoordinate MapWidget::pixelToGeo(const QPoint& pixel, const QGeoCoordinate& center, int zoom, const QSize& size)
{
    double tilesPerSide = 1 << zoom;
    
    // Convert center to normalized coordinates (0-1)
    double centerX = (center.longitude() + 180.0) / 360.0;
    double centerY = (1.0 - log(tan(center.latitude() * M_PI / 180.0) + 
                            1.0 / cos(center.latitude() * M_PI / 180.0)) / M_PI) / 2.0;
    
    // Calculate normalized coordinates from pixel position
    double x = centerX + (pixel.x() - size.width() / 2) / (tilesPerSide * TILE_SIZE);
    double y = centerY + (pixel.y() - size.height() / 2) / (tilesPerSide * TILE_SIZE);
    
    // Convert to geo coordinates
    double lon = x * 360.0 - 180.0;
    double latRad = atan(sinh(M_PI * (1 - 2 * y)));
    double lat = latRad * 180.0 / M_PI;
    
    return QGeoCoordinate(lat, lon);
}
