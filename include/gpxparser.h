#pragma once
#include <QGeoCoordinate>
#include <QString>
#include <QXmlStreamReader>
#include <QDateTime>
#include <vector>
#include <memory>

/**
 * @brief Structure to hold track point data with geographical and metric information
 */
struct TrackPoint {
    QGeoCoordinate coord;     ///< Geographical coordinates (lat/lon)
    double elevation = 0.0;   ///< Elevation in meters
    double distance = 0.0;    ///< Cumulative distance in meters from start
    QDateTime timestamp;      ///< Timestamp of the track point
    
    // Default constructor
    TrackPoint() = default;
    
    // Constructor with all fields
    TrackPoint(const QGeoCoordinate& c, double elev, double dist, const QDateTime& time = QDateTime()) :
        coord(c), elevation(elev), distance(dist), timestamp(time) {}
};

/**
 * @brief Parser for GPX track files
 * 
 * Reads and parses GPX files, extracting track points and calculating
 * cumulative statistics like distance and elevation gain.
 */
class GPXParser {
public:
    /**
     * @brief Default constructor
     */
    GPXParser() = default;
    
    /**
     * @brief Parse a GPX file
     * @param filename Path to GPX file
     * @return True if parsing successful, false otherwise
     */
    bool parse(const QString& filename);
    
    /**
     * @brief Parse GPX data from an XML string
     * @param data XML string containing GPX data
     * @return True if parsing successful, false otherwise
     */
    bool parseData(const QString& data);

    /**
     * @brief Get all parsed track points
     * @return Vector of track points
     */
    const std::vector<TrackPoint>& getPoints() const { return m_points; }

    /**
     * @brief Calculate cumulative elevation gain up to specific point
     * @param upToIndex Index of point to calculate gain to
     * @return Total elevation gain in meters
     */
    double getCumulativeElevationGain(int upToIndex) const;
    
    /**
     * @brief Get the total distance of the track
     * @return Total distance in meters
     */
    double getTotalDistance() const;
    
    /**
     * @brief Get the total elevation gain
     * @return Total elevation gain in meters
     */
    double getTotalElevationGain() const;
    
    /**
     * @brief Get the maximum elevation
     * @return Maximum elevation in meters
     */
    double getMaxElevation() const;
    
    /**
     * @brief Get the minimum elevation
     * @return Minimum elevation in meters
     */
    double getMinElevation() const;
    
    /**
     * @brief Clear all parsed data
     */
    void clear();

private:
    std::vector<TrackPoint> m_points;  ///< Storage for parsed track points
    double m_minElevation = 0.0;
    double m_maxElevation = 0.0;
    
    /**
     * @brief Process a track point from XML
     * @param xml XML stream reader positioned at a track point
     * @return True if point was successfully read
     */
    bool processTrackPoint(QXmlStreamReader& xml);
    
    /**
     * @brief Calculate distances between consecutive points
     */
    void calculateDistances();
};
