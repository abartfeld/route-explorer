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
    mCenterCoordinate(39.8283, -98.5795), // Initialize to center on continental US
    mIsPanning(false),
    mLastMousePos(0, 0),
    mHasSegments(false),
    mNetworkManager(new QNetworkAccessManager(this)),
    mTileCache(200) // Cache up to 200 tiles
{
    // Set basic widget properties
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
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

void MapWidget::setRouteWithSegments(const std::vector<QGeoCoordinate>& coordinates, 
                                    const std::vector<TrackSegment>& segments,
                                    const std::vector<TrackPoint>& points) {
    // Clear previous route data
    mRouteCoordinates.clear();
    mRouteSegments.clear();
    
    if (coordinates.empty()) {
        mHasSegments = false;
        return;
    }
    
    // Always store the full route coordinates for fallback rendering
    for (const auto& coord : coordinates) {
        mRouteCoordinates.append(coord);
    }
    
    // Create colored segments if available
    if (!segments.empty() && !points.empty()) {
        // Create a boolean array to track which points are covered by segments
        std::vector<bool> coveredPoints(points.size(), false);
        
        // First pass: Create segments from the TrackSegment data
        for (const auto& segment : segments) {
            RouteSegment routeSegment;
            routeSegment.color = getSegmentColor(segment);
            
            // Extract points for this segment
            for (size_t i = segment.startIndex; i <= segment.endIndex && i < points.size(); i++) {
                routeSegment.coordinates.append(points[i].coord);
                coveredPoints[i] = true;
            }
            
            if (routeSegment.coordinates.size() > 1) {
                mRouteSegments.append(routeSegment);
            }
        }
        
        // Second pass: Create segments for uncovered parts of the route
        if (std::find(coveredPoints.begin(), coveredPoints.end(), false) != coveredPoints.end()) {
            RouteSegment unclassifiedSegment;
            unclassifiedSegment.color = QColor("#A0A0A0"); // Gray for unclassified parts
            
            for (size_t i = 0; i < points.size(); i++) {
                if (!coveredPoints[i]) {
                    unclassifiedSegment.coordinates.append(points[i].coord);
                } else if (!unclassifiedSegment.coordinates.isEmpty()) {
                    // End current unclassified segment and add it if it has at least 2 points
                    if (unclassifiedSegment.coordinates.size() > 1) {
                        mRouteSegments.append(unclassifiedSegment);
                    }
                    unclassifiedSegment.coordinates.clear();
                }
            }
            
            // Add final unclassified segment if it exists
            if (unclassifiedSegment.coordinates.size() > 1) {
                mRouteSegments.append(unclassifiedSegment);
            }
        }
        
        mHasSegments = !mRouteSegments.isEmpty();
    } else {
        mHasSegments = false;
    }
    
    // Calculate bounds of the route and center the map
    double minLat = coordinates[0].latitude();
    double maxLat = minLat;
    double minLon = coordinates[0].longitude();
    double maxLon = minLon;
    
    // Find the actual bounds
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
    
    // Schedule a repaint
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
    
    // Draw the route with segments if available
    if (mHasSegments && !mRouteSegments.isEmpty()) {
        // First draw a thicker shadow/outline to make route stand out against map
        // CHANGE: Reduce shadow thickness from 7.0 to 5.0 for a sleeker look
        if (mRouteCoordinates.size() > 1) {
            QPainterPath basePath;
            bool first = true;
            
            for (const auto& coord : mRouteCoordinates) {
                QPoint point = geoToPixel(coord, mCenterCoordinate, mZoom, size());
                if (first) {
                    basePath.moveTo(point);
                    first = false;
                } else {
                    basePath.lineTo(point);
                }
            }
            
            // Draw an outline/shadow with reduced thickness
            painter.setPen(QPen(QColor(50, 50, 50, 80), 5.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPath(basePath);
        }
        
        // Then draw each segment with its color
        // CHANGE: Reduce line thickness from 5.0 to 3.5
        for (const auto& segment : mRouteSegments) {
            if (segment.coordinates.size() < 2) continue;
            
            QPainterPath path;
            bool first = true;
            
            for (const auto& coord : segment.coordinates) {
                QPoint point = geoToPixel(coord, mCenterCoordinate, mZoom, size());
                if (first) {
                    path.moveTo(point);
                    first = false;
                } else {
                    path.lineTo(point);
                }
            }
            
            // Draw the segment with enhanced colors and reduced thickness
            QColor enhancedColor = enhanceColor(segment.color);
            painter.setPen(QPen(enhancedColor, 3.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPath(path);
        }
    } else if (mRouteCoordinates.size() > 1) {
        // Fall back to default styling if no segments - with enhanced visibility
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
        
        // CHANGE: Reduce shadow thickness from 7.0 to 5.0
        painter.setPen(QPen(QColor(0, 0, 0, 80), 5.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(path);
        
        // CHANGE: Reduce route thickness from 5.0 to 3.5
        painter.setPen(QPen(QColor(0, 120, 255), 3.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(path);
    }
    
    // Draw the marker with improved visibility
    QPoint markerPos = geoToPixel(mCurrentMarkerCoordinate, mCenterCoordinate, mZoom, size());
    
    // Define marker appearance - make it larger and more visible
    const int markerSize = 14; // Increased from 12
    QRect markerRect(markerPos.x() - markerSize/2, markerPos.y() - markerSize/2, markerSize, markerSize);
    
    // Draw a shadow/outline for the marker
    painter.setPen(QPen(QColor(0, 0, 0, 100), 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(markerRect.adjusted(-1, -1, 1, 1));
    
    // Draw the marker with bright color
    painter.setBrush(QBrush(QColor("#FF4136"))); // Brighter red
    painter.setPen(QPen(Qt::white, 2.5));
    painter.drawEllipse(markerRect);
    
    // Draw hover indicator if hovering over the route
    if (mHoverPointIndex >= 0 && mShowTooltip) {
        const int hoverSize = 8;
        painter.setPen(QPen(Qt::black, 2));
        painter.setBrush(QBrush(Qt::white));
        painter.drawEllipse(mHoverPoint.x() - hoverSize/2, mHoverPoint.y() - hoverSize/2, hoverSize, hoverSize);
    }
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
    } else {
        // Check if we're hovering over the route
        int newHoverIndex = findClosestRoutePoint(event->pos());
        
        if (newHoverIndex >= 0 && newHoverIndex < static_cast<int>(mTrackPoints.size())) {
            if (newHoverIndex != mHoverPointIndex) {
                mHoverPointIndex = newHoverIndex;
                mShowTooltip = true;
                
                // Convert to screen coordinates for tooltip and highlighting
                if (mHoverPointIndex >= 0 && mHoverPointIndex < static_cast<int>(mRouteCoordinates.size())) {
                    mHoverPoint = geoToPixel(mRouteCoordinates[mHoverPointIndex], mCenterCoordinate, mZoom, size());
                    
                    // Show tooltip with track information
                    if (mHoverPointIndex >= 0 && mHoverPointIndex < static_cast<int>(mTrackPoints.size())) {
                        const TrackPoint& point = mTrackPoints[mHoverPointIndex];
                        
                        // Use pre-calculated gradient for more consistent values
                        double gradient = point.gradient;
                        
                        // Format the tooltip
                        QString tooltip = QString(
                            "<div style='background-color:rgba(255,255,255,0.9); padding:6px; border-radius:4px; border: 1px solid #ccc;'>"
                            "<b>Distance:</b> %1 mi<br>"
                            "<b>Elevation:</b> %2 ft<br>"
                            "<b>Gradient:</b> %3%"
                            "</div>"
                        ).arg(point.distance * 0.000621371, 0, 'f', 2)  // Convert to miles
                         .arg(point.elevation * 3.28084, 0, 'f', 0)     // Convert to feet
                         .arg(gradient, 0, 'f', 1);                     // Use pre-calculated gradient
                         
                        QToolTip::showText(event->globalPos(), tooltip, this);
                        
                        // Emit signal for slider to update
                        emit routeHovered(mHoverPointIndex);
                    }
                }
                
                update(); // Redraw to show hover indicator
            }
            return;
        } else if (mHoverPointIndex >= 0) {
            // Mouse moved away from route
            mHoverPointIndex = -1;
            mShowTooltip = false;
            QToolTip::hideText();
            update(); // Redraw to hide hover indicator
        }
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

QColor MapWidget::getSegmentColor(const TrackSegment& segment) const {
    // Return color based on segment type and gradient with enhanced visibility
    QColor color;
    
    switch (segment.type) {
        case TrackSegment::CLIMB:
            if (segment.avgGradient > 10.0)
                color = QColor(220, 20, 20);  // Steep climb - deeper red
            else if (segment.avgGradient > 5.0)
                color = QColor(255, 140, 0); // Moderate climb - deeper orange
            else
                color = QColor(240, 230, 0); // Easy climb - gold-yellow
            break;
        case TrackSegment::DESCENT:
            if (segment.avgGradient < -10.0)
                color = QColor(128, 0, 128); // Steep descent - purple
            else if (segment.avgGradient < -5.0)
                color = QColor(30, 30, 220);   // Moderate descent - deeper blue
            else
                color = QColor(100, 180, 255); // Easy descent - brighter blue
            break;
        default:
            color = QColor(0, 160, 0);       // Flat - deeper green
            break;
    }
    
    return color;
}

QColor MapWidget::enhanceColor(const QColor& color) const {
    // Make colors more vibrant
    QColor enhanced = color;
    
    // Increase saturation 
    int h, s, v, a;
    enhanced.getHsv(&h, &s, &v, &a);
    
    // Boost saturation and value for more vivid colors
    s = qMin(255, s + 30);
    v = qMin(255, v + 20);
    
    enhanced.setHsv(h, s, v, a);
    return enhanced;
}

void MapWidget::setTrackPoints(const std::vector<TrackPoint>& points) {
    mTrackPoints = points;
}

// Helper method to find the closest point on the route to the mouse position
int MapWidget::findClosestRoutePoint(const QPoint& mousePos) {
    if (mRouteCoordinates.isEmpty()) {
        return -1;
    }
    
    // Threshold for how close the mouse needs to be to the route (in pixels)
    const double HOVER_THRESHOLD = 10.0;
    
    int closestIndex = -1;
    double minDistance = HOVER_THRESHOLD;
    
    // Check each line segment in the route
    for (int i = 0; i < mRouteCoordinates.size() - 1; i++) {
        QPoint p1 = geoToPixel(mRouteCoordinates[i], mCenterCoordinate, mZoom, size());
        QPoint p2 = geoToPixel(mRouteCoordinates[i+1], mCenterCoordinate, mZoom, size());
        
        double distance = calculateDistanceToLine(mousePos, p1, p2);
        
        if (distance < minDistance) {
            minDistance = distance;
            
            // Determine which endpoint is closer
            double d1 = QLineF(mousePos, p1).length();
            double d2 = QLineF(mousePos, p2).length();
            closestIndex = (d1 < d2) ? i : i+1;
        }
    }
    
    return closestIndex;
}

// Calculate the distance from a point to a line segment
double MapWidget::calculateDistanceToLine(const QPoint& point, const QPoint& lineStart, const QPoint& lineEnd) {
    QLineF line(lineStart, lineEnd);
    
    // If the line is a point, return distance to that point
    if (lineStart == lineEnd) {
        return QLineF(point, lineStart).length();
    }
    
    // Calculate projection of point onto line
    double lineLength = line.length();
    double u = ((point.x() - lineStart.x()) * (lineEnd.x() - lineStart.x()) + 
               (point.y() - lineStart.y()) * (lineEnd.y() - lineStart.y())) / 
               (lineLength * lineLength);
    
    // If projection is outside the line segment, use distance to closest endpoint
    if (u < 0) {
        return QLineF(point, lineStart).length();
    }
    if (u > 1) {
        return QLineF(point, lineEnd).length();
    }
    
    // Calculate the closest point on the line segment
    QPointF closestPoint = lineStart + u * (lineEnd - lineStart);
    
    // Return the distance to that point
    return QLineF(point, closestPoint).length();
}
