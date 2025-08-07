#include "GpxParser.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>
#include <QDateTime>
#include <QVector>
#include <cmath>
#include <algorithm>

// Add constants for unit conversion
namespace {
    const double METERS_TO_FEET = 3.28084;
    const double METERS_TO_MILES = 0.000621371;
    const int GRADIENT_WINDOW_SIZE = 5; // Points to include in moving average (must be odd)
    const double GRADIENT_THRESHOLD = 0.1; // Minimum elevation change to consider for gradient in meters
    const double DISTANCE_THRESHOLD = 2.0; // Minimum distance for gradient calculation in meters
    const double MAX_GRADIENT = 35.0; // Maximum reasonable gradient in percent
}

bool GPXParser::parse(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open file" << filename;
        return false;
    }
    QXmlStreamReader xml(&file);
    return parseXmlStream(xml);
}

bool GPXParser::parseData(const QString& data) {
    QXmlStreamReader xml(data);
    return parseXmlStream(xml);
}

// Centralized parsing logic
bool GPXParser::parseXmlStream(QXmlStreamReader& xml) {
    clear();

    double totalDistance = 0.0;
    QGeoCoordinate lastCoord;
    bool firstPoint = true;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("trkpt")) {
            if (processTrackPoint(xml)) {
                // Calculate cumulative distance
                if (!firstPoint) {
                    double segmentDistance = lastCoord.distanceTo(m_points.back().coord);
                    totalDistance += segmentDistance;
                    m_points.back().distance = totalDistance;
                } else {
                    m_points.back().distance = 0.0;
                    firstPoint = false;
                }

                // Track min/max elevation
                double elevation = m_points.back().elevation;
                if (m_points.size() == 1) {
                    m_minElevation = m_maxElevation = elevation;
                } else {
                    if (elevation < m_minElevation) m_minElevation = elevation;
                    if (elevation > m_maxElevation) m_maxElevation = elevation;
                }

                lastCoord = m_points.back().coord;
            }
        }
    }

    if (xml.hasError()) {
        qDebug() << "XML error:" << xml.errorString();
        clear();
        return false;
    }

    calculateGradients();
    return !m_points.empty();
}

// New method to calculate gradients for all track points
void GPXParser::calculateGradients() {
    if (m_points.size() < 2) {
        return;
    }
    
    // First pass: calculate raw point-to-point gradients
    std::vector<double> rawGradients(m_points.size(), 0.0);
    
    for (size_t i = 1; i < m_points.size(); i++) {
        double distDiff = m_points[i].distance - m_points[i-1].distance;
        double elevDiff = m_points[i].elevation - m_points[i-1].elevation;
        
        if (distDiff > DISTANCE_THRESHOLD) {
            double gradient = (elevDiff / distDiff) * 100.0;
            
            // Clamp to reasonable gradient values
            rawGradients[i] = std::max(std::min(gradient, MAX_GRADIENT), -MAX_GRADIENT);
        } else {
            // For very close points (< 2m), use the previous gradient to avoid spikes
            rawGradients[i] = (i > 1) ? rawGradients[i-1] : 0.0;
        }
    }
    
    // Second pass: apply weighted moving average for smoother gradients
    std::vector<double> smoothGradients(m_points.size(), 0.0);
    
    for (size_t i = 0; i < m_points.size(); i++) {
        double weightedSum = 0.0;
        double weightSum = 0.0;
        int halfWindow = GRADIENT_WINDOW_SIZE / 2;
        
        // Apply Gaussian-like weighting to the window
        for (int j = -halfWindow; j <= halfWindow; j++) {
            int idx = static_cast<int>(i) + j;
            if (idx >= 0 && idx < static_cast<int>(m_points.size())) {
                // Use a triangular weight - closer points have more influence
                double weight = halfWindow + 1 - std::abs(j);
                weightedSum += rawGradients[idx] * weight;
                weightSum += weight;
            }
        }
        
        // Store the smoothed gradient
        if (weightSum > 0) {
            smoothGradients[i] = weightedSum / weightSum;
        }
    }
    
    // Third pass: segment-aware gradient smoothing to maintain consistency within segments
    for (size_t i = 0; i < m_points.size(); i++) {
        // Store the smoothed gradient in the point data
        m_points[i].gradient = smoothGradients[i];
    }
}

double GPXParser::getCumulativeElevationGain(int upToIndex) const {
    if (m_points.empty() || upToIndex < 0) {
        return 0.0;
    }
    
    int lastIndex = std::min(upToIndex, static_cast<int>(m_points.size()) - 1);
    double elevationGain = 0.0;
    const double ELEVATION_THRESHOLD = 0.6; // Threshold of 0.6 meters to ignore small changes
    
    for (int i = 1; i <= lastIndex; ++i) {
        double diff = m_points[i].elevation - m_points[i-1].elevation;
        // Only count elevation gains greater than the threshold
        if (diff > ELEVATION_THRESHOLD) {
            elevationGain += diff;
        }
    }
    
    // Return in meters (don't convert to feet here - that's done in the UI layer)
    return elevationGain;
}

double GPXParser::getTotalDistance() const {
    if (m_points.empty()) {
        return 0.0;
    }
    // Return in meters (don't convert to miles here - that's done in the UI layer)
    return m_points.back().distance;
}

double GPXParser::getTotalElevationGain() const {
    return getCumulativeElevationGain(m_points.size() - 1);
}

double GPXParser::getMaxElevation() const {
    return m_maxElevation;
}

double GPXParser::getMinElevation() const {
    return m_minElevation;
}

double GPXParser::getGradientAtPoint(int pointIndex) const {
    if (pointIndex < 0 || pointIndex >= static_cast<int>(m_points.size())) {
        return 0.0;
    }
    
    // Return the pre-calculated gradient
    return m_points[pointIndex].gradient;
}

void GPXParser::clear() {
    m_points.clear();
    m_minElevation = 0.0;
    m_maxElevation = 0.0;
}

bool GPXParser::processTrackPoint(QXmlStreamReader& xml) {
    // Get latitude and longitude from attributes
    QXmlStreamAttributes attrs = xml.attributes();
    
    if (!attrs.hasAttribute("lat") || !attrs.hasAttribute("lon")) {
        qDebug() << "Error: trackpoint missing lat/lon attributes";
        return false;
    }
    
    bool okLat = false, okLon = false;
    double lat = attrs.value("lat").toDouble(&okLat);
    double lon = attrs.value("lon").toDouble(&okLon);
    
    if (!okLat || !okLon) {
        qDebug() << "Error: invalid lat/lon values";
        return false;
    }
    
    QGeoCoordinate coord(lat, lon);
    
    // Find the elevation and timestamp
    double elevation = 0.0;
    QDateTime timestamp;
    
    // Process all elements within the trackpoint
    while (!(xml.isEndElement() && xml.name() == QLatin1String("trkpt"))) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("ele")) {
                QString eleText = xml.readElementText();
                bool ok = false;
                double ele = eleText.toDouble(&ok);
                if (ok) {
                    elevation = ele;
                }
            } else if (xml.name() == QLatin1String("time")) {
                QString timeText = xml.readElementText();
                // Parse ISO 8601 timestamp (format: yyyy-MM-ddTHH:mm:ssZ)
                timestamp = QDateTime::fromString(timeText, Qt::ISODate);
                if (!timestamp.isValid()) {
                    // Try alternate format without Z
                    timestamp = QDateTime::fromString(timeText, "yyyy-MM-ddTHH:mm:ss");
                }
            }
        }
    }
    
    // Create and add the track point
    TrackPoint point(coord, elevation, 0.0, timestamp); // Distance will be calculated later
    m_points.push_back(point);
    
    return true;
}
