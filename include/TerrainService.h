#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QImage>
#include <QGeoCoordinate>
#include <vector>

/**
 * @brief Structure to hold terrain data
 */
struct TerrainData {
    std::vector<std::vector<float>> elevationGrid;
    QImage satelliteImage;
    QString satelliteImagePath;
    QGeoCoordinate topLeft;
    QGeoCoordinate bottomRight;
};

/**
 * @brief Service for fetching terrain data, including elevation and satellite imagery.
 */
class TerrainService : public QObject {
    Q_OBJECT

public:
    explicit TerrainService(QObject* parent = nullptr);
    ~TerrainService() = default;

    /**
     * @brief Fetches terrain data for a given bounding box.
     * @param northLat North latitude of the bounding box.
     * @param southLat South latitude of the bounding box.
     * @param westLon West longitude of the bounding box.
     * @param eastLon East longitude of the bounding box.
     * @param width The desired width of the elevation grid and satellite image.
     * @param height The desired height of the elevation grid and satellite image.
     */
    void fetchTerrainData(double northLat, double southLat, double westLon, double eastLon, int width, int height);

signals:
    /**
     * @brief Signal emitted when terrain data is successfully fetched and processed.
     * @param data The fetched terrain data.
     */
    void terrainDataReady(const TerrainData& data);

    /**
     * @brief Signal emitted when an error occurs during data fetching.
     * @param errorMessage A description of the error.
     */
    void error(const QString& errorMessage);

private slots:
    void handleElevationReply();
    void handleSatelliteImageReply();

private:
    QNetworkAccessManager* m_networkManager;
    TerrainData m_terrainData;
    int m_gridWidth;
    int m_gridHeight;

    void fetchElevationData(double northLat, double southLat, double westLon, double eastLon, int width, int height);
    void fetchSatelliteImage(double northLat, double southLat, double westLon, double eastLon, int width, int height);
};
