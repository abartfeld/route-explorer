#include "TerrainService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>
#include <QTemporaryFile>
#include <QDir>

// Constants for the APIs
namespace {
    // Open-Meteo Elevation API
    const QString ELEVATION_API_URL = "https://api.open-meteo.com/v1/elevation";

    // Copernicus WMS API
    // Italian National Geoportal WMS
    const QString SATELLITE_API_URL = "http://wms.pcn.minambiente.it/cgi-bin/mapserv.exe?map=/ms_ogc/service/ortofoto_colore_06.map";
    const QString SATELLITE_LAYER = "ortofoto_colore_06_32,ortofoto_colore_06_33";
}

TerrainService::TerrainService(QObject* parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)), m_gridWidth(0), m_gridHeight(0)
{
}

void TerrainService::fetchTerrainData(double northLat, double southLat, double westLon, double eastLon, int width, int height)
{
    m_terrainData = TerrainData();
    m_terrainData.topLeft = QGeoCoordinate(northLat, westLon);
    m_terrainData.bottomRight = QGeoCoordinate(southLat, eastLon);
    m_gridWidth = width;
    m_gridHeight = height;

    fetchElevationData(northLat, southLat, westLon, eastLon, width, height);
    fetchSatelliteImage(northLat, southLat, westLon, eastLon, width, height);
}

void TerrainService::fetchElevationData(double northLat, double southLat, double westLon, double eastLon, int width, int height)
{
    // Implementation of fetching elevation data from Open-Meteo
    // This will involve creating a grid of points and making one or more requests.
    // For simplicity in this first pass, I'll make a single request with a limited number of points.
    // A more robust implementation would handle batching.

    QUrl url(ELEVATION_API_URL);
    QUrlQuery query;

    QString latitudes;
    QString longitudes;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            double lat = southLat + (northLat - southLat) * i / (height - 1);
            double lon = westLon + (eastLon - westLon) * j / (width - 1);
            if (!latitudes.isEmpty()) {
                latitudes += ",";
                longitudes += ",";
            }
            latitudes += QString::number(lat, 'f', 4);
            longitudes += QString::number(lon, 'f', 4);
        }
    }

    query.addQueryItem("latitude", latitudes);
    query.addQueryItem("longitude", longitudes);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &TerrainService::handleElevationReply);
}

void TerrainService::fetchSatelliteImage(double northLat, double southLat, double westLon, double eastLon, int width, int height)
{
    // Implementation of fetching satellite image from Copernicus WMS
    QUrl url(SATELLITE_API_URL);
    QUrlQuery query;
    query.addQueryItem("SERVICE", "WMS");
    query.addQueryItem("VERSION", "1.3.0");
    query.addQueryItem("REQUEST", "GetMap");
    query.addQueryItem("LAYERS", SATELLITE_LAYER);
    query.addQueryItem("BBOX", QString("%1,%2,%3,%4").arg(westLon).arg(southLat).arg(eastLon).arg(northLat));
    query.addQueryItem("CRS", "EPSG:4326");
    query.addQueryItem("WIDTH", QString::number(width));
    query.addQueryItem("HEIGHT", QString::number(height));
    query.addQueryItem("FORMAT", "image/png");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &TerrainService::handleSatelliteImageReply);
}

void TerrainService::handleElevationReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error()) {
        emit error("Elevation data request failed: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();
    QJsonArray elevations = obj["elevation"].toArray();

    m_terrainData.elevationGrid.clear();
    m_terrainData.elevationGrid.resize(m_gridHeight, std::vector<float>(m_gridWidth));

    int index = 0;
    for (int i = 0; i < m_gridHeight; ++i) {
        for (int j = 0; j < m_gridWidth; ++j) {
            if (index < elevations.size()) {
                m_terrainData.elevationGrid[i][j] = elevations[index++].toDouble();
            }
        }
    }

    reply->deleteLater();

    // If satellite image is also ready, emit the signal
    if (!m_terrainData.satelliteImage.isNull()) {
        emit terrainDataReady(m_terrainData);
    }
}

void TerrainService::handleSatelliteImageReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error()) {
        emit error("Satellite image request failed: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray imageData = reply->readAll();
    QTemporaryFile tempFile(QDir::tempPath() + "/satellite_XXXXXX.png");
    if (tempFile.open()) {
        tempFile.write(imageData);
        m_terrainData.satelliteImagePath = tempFile.fileName();
        m_terrainData.satelliteImage.loadFromData(imageData);
        tempFile.close();
    } else {
        emit error("Could not create temporary file for satellite image.");
    }

    reply->deleteLater();

    // If elevation data is also ready, emit the signal
    if (!m_terrainData.elevationGrid.empty()) {
        emit terrainDataReady(m_terrainData);
    }
}
