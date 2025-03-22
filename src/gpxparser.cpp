#include "gpxparser.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>
#include <QDateTime>  // Add this include for QDateTime

// Add constants for unit conversion
namespace {
    const double METERS_TO_FEET = 3.28084;
    const double METERS_TO_MILES = 0.000621371;
}

bool GPXParser::parse(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open file" << filename;
        return false;
    }

    // Clear existing data
    clear();
    
    QXmlStreamReader xml(&file);
    
    // Track segment properties
    double totalDistance = 0.0;
    QGeoCoordinate lastCoord;
    bool firstPoint = true;
    
    // Parse the XML
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
                if (m_points.size() == 1 || elevation < m_minElevation) {
                    m_minElevation = elevation;
                }
                if (m_points.size() == 1 || elevation > m_maxElevation) {
                    m_maxElevation = elevation;
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
    
    file.close();
    return !m_points.empty();
}

bool GPXParser::parseData(const QString& data) {
    // Clear existing data
    clear();
    
    QXmlStreamReader xml(data);
    
    // Track segment properties
    double totalDistance = 0.0;
    QGeoCoordinate lastCoord;
    bool firstPoint = true;
    
    // Parse the XML
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
                
                lastCoord = m_points.back().coord;
            }
        }
    }
    
    if (xml.hasError()) {
        qDebug() << "XML error:" << xml.errorString();
        clear();
        return false;
    }
    
    return !m_points.empty();
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
