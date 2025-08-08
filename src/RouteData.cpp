#include "RouteData.h"
#include "logging.h"
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

RouteData::RouteData(const std::vector<TrackPoint>& trackPoints, float elevationScale) {
    logDebug("RouteData", "Processing track points...");
    if (!trackPoints.empty()) {
        processPoints(trackPoints, elevationScale);
        logInfo("RouteData", QString("Successfully processed %1 points.").arg(trackPoints.size()));
    } else {
        logWarning("RouteData", "Track points vector is empty, nothing to process.");
    }
}

RouteData::~RouteData() {
    logDebug("RouteData", "Destroying RouteData object.");
}

void RouteData::processPoints(const std::vector<TrackPoint>& trackPoints, float elevationScale) {
    m_positions.clear();
    m_positions.reserve(trackPoints.size());
    m_routePoints.clear();
    m_routePoints.reserve(trackPoints.size());
    m_totalDistance = 0.0f;

    if (trackPoints.empty()) {
        return;
    }

    // First pass: convert all points to local 3D coordinates
    const double originLon = trackPoints.front().coord.longitude();
    const double originLat = trackPoints.front().coord.latitude();
    for (const auto& point : trackPoints) {
        QVector2D mercatorCoords = lonLatToMercator(
            point.coord.longitude(), point.coord.latitude(), originLon, originLat);
        float elevation = static_cast<float>(point.elevation) * elevationScale;
        m_positions.emplace_back(mercatorCoords.x(), elevation, mercatorCoords.y());
    }

    // Second pass: calculate distances and directions
    for (size_t i = 0; i < m_positions.size(); ++i) {
        RoutePoint rp;
        rp.position = m_positions[i];

        if (i > 0) {
            m_totalDistance += (m_positions[i] - m_positions[i-1]).length();
        }
        rp.distance = m_totalDistance;

        if (i < m_positions.size() - 1) {
            rp.direction = (m_positions[i+1] - m_positions[i]).normalized();
        } else if (m_positions.size() > 1) {
            rp.direction = (m_positions[i] - m_positions[i-1]).normalized();
        } else {
            rp.direction = QVector3D(1, 0, 0); // Default direction
        }
        m_routePoints.push_back(rp);
    }
}

RoutePoint RouteData::getPointAtProgress(float progress) const
{
    if (m_routePoints.empty()) {
        return RoutePoint();
    }

    const float targetDistance = progress * m_totalDistance;

    // Find the segment that contains the target distance
    size_t segmentEnd = 1;
    while (segmentEnd < m_routePoints.size() && m_routePoints[segmentEnd].distance < targetDistance) {
        segmentEnd++;
    }
    if (segmentEnd >= m_routePoints.size()) {
        return m_routePoints.back();
    }

    const size_t segmentStart = segmentEnd - 1;

    const RoutePoint& p1 = m_routePoints[segmentStart];
    const RoutePoint& p2 = m_routePoints[segmentEnd];

    // Interpolate between the two points
    const float segmentLength = p2.distance - p1.distance;
    if (segmentLength < 0.001f) {
        return p1; // Avoid division by zero
    }

    const float segmentProgress = (targetDistance - p1.distance) / segmentLength;

    RoutePoint result;
    result.position = p1.position + (p2.position - p1.position) * segmentProgress;
    result.direction = (p1.direction + (p2.direction - p1.direction) * segmentProgress).normalized();
    result.distance = targetDistance;

    return result;
}

size_t RouteData::getIndexAtProgress(float progress) const
{
    if (m_routePoints.empty()) {
        return 0;
    }

    const float targetDistance = progress * m_totalDistance;

    // Find the segment that contains the target distance
    size_t segmentEnd = 1;
    while (segmentEnd < m_routePoints.size() && m_routePoints[segmentEnd].distance < targetDistance) {
        segmentEnd++;
    }
    if (segmentEnd >= m_routePoints.size()) {
        return m_routePoints.size() - 1;
    }

    // Return the index of the point that starts the segment.
    return segmentEnd - 1;
}
