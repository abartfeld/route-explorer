#include "WeatherService.h"
#include <QUrlQuery>
#include <QDebug>

WeatherService::WeatherService(QObject* parent) 
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this))
{
    // No API key needed for Open-Meteo - it's completely free
}

void WeatherService::getWeatherForPoint(const QGeoCoordinate& coord, const QDateTime& timestamp) {
    // Clear existing data if this is a new request
    if (m_weatherData.size() == 1) {
        m_weatherData.clear();
    }
    
    // Determine if we need historical or forecast data
    QDateTime now = QDateTime::currentDateTime();
    bool isHistorical = timestamp < now.addDays(-1); 

    QUrl url;
    QUrlQuery query;
    
    // Basic parameters common to both APIs
    query.addQueryItem("latitude", QString::number(coord.latitude()));
    query.addQueryItem("longitude", QString::number(coord.longitude()));
    
    if (isHistorical) {
        // Use historical API
        url = QUrl("https://archive-api.open-meteo.com/v1/archive");
        
        // Format dates for the API
        QString startDate = timestamp.date().addDays(-1).toString("yyyy-MM-dd");
        QString endDate = timestamp.date().addDays(1).toString("yyyy-MM-dd");
        
        query.addQueryItem("start_date", startDate);
        query.addQueryItem("end_date", endDate);
        query.addQueryItem("hourly", "temperature_2m,precipitation,weathercode,windspeed_10m,winddirection_10m");
        query.addQueryItem("timezone", "auto");
    } else {
        // Use forecast API
        url = QUrl("https://api.open-meteo.com/v1/forecast");
        
        // Only include relevant parameters
        query.addQueryItem("hourly", "temperature_2m,precipitation,weathercode,windspeed_10m,winddirection_10m");
        query.addQueryItem("forecast_days", "3");
        query.addQueryItem("timezone", "auto");
    }
    
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);
    m_pendingReplies.push_back(reply);
    
    connect(reply, &QNetworkReply::finished, this, &WeatherService::handleWeatherReply);
    
    setStatus("Fetching weather data...");
}

void WeatherService::getWeatherForTrack(const std::vector<QGeoCoordinate>& coordinates, 
                                      const std::vector<QDateTime>& timestamps) 
{
    if (coordinates.empty() || timestamps.empty()) {
        return;
    }
    
    // Clear existing data
    m_weatherData.clear();
    m_weatherData.resize(coordinates.size());
    
    // For simplicity, we'll sample weather at intervals along the track
    const int MAX_WEATHER_POINTS = 10; // Open-Meteo is more generous with request limits
    
    // Determine sampling interval
    int interval = std::max(1, static_cast<int>(coordinates.size() / MAX_WEATHER_POINTS));
    
    // Cancel any pending requests
    for (auto* reply : m_pendingReplies) {
        if (reply && !reply->isFinished()) {
            disconnect(reply, nullptr, this, nullptr);
            reply->abort();
            reply->deleteLater();
        }
    }
    m_pendingReplies.clear();
    
    // Request weather for sampled points
    for (size_t i = 0; i < coordinates.size(); i += interval) {
        if (i < timestamps.size() && timestamps[i].isValid()) {
            getWeatherForPoint(coordinates[i], timestamps[i]);
        }
    }
    
    setStatus(QString("Fetching weather data for %1 points...").arg(1 + coordinates.size() / interval));
}

const WeatherInfo& WeatherService::getWeatherAtIndex(size_t index) const {
    static WeatherInfo emptyInfo;
    
    if (m_weatherData.empty()) {
        return emptyInfo;
    }
    
    // If we don't have weather for every point, interpolate from nearest point
    if (index >= m_weatherData.size() || !m_weatherData[index].timestamp.isValid()) {
        // Find nearest point with valid weather data
        size_t nearestIndex = 0;
        double minDistance = std::numeric_limits<double>::max();
        
        for (size_t i = 0; i < m_weatherData.size(); i++) {
            if (m_weatherData[i].timestamp.isValid()) {
                double distance = std::abs(static_cast<double>(i) - static_cast<double>(index));
                if (distance < minDistance) {
                    minDistance = distance;
                    nearestIndex = i;
                }
            }
        }
        
        if (m_weatherData[nearestIndex].timestamp.isValid()) {
            return m_weatherData[nearestIndex];
        }
        
        return emptyInfo;
    }
    
    return m_weatherData[index];
}

void WeatherService::handleWeatherReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    // Remove from pending replies
    auto it = std::find(m_pendingReplies.begin(), m_pendingReplies.end(), reply);
    if (it != m_pendingReplies.end()) {
        m_pendingReplies.erase(it);
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject root = doc.object();
            
            // Extract the URL to identify which track point this data belongs to
            QUrl url = reply->request().url();
            QUrlQuery query(url.query());
            
            double lat = query.queryItemValue("latitude").toDouble();
            double lon = query.queryItemValue("longitude").toDouble();
            
            // Parse data from Open-Meteo format
            if (root.contains("hourly")) {
                WeatherInfo info = parseOpenMeteoData(root);
                
                // Find the appropriate point to store this weather data
                // Simple approach: find first point without data
                for (size_t i = 0; i < m_weatherData.size(); i++) {
                    if (!m_weatherData[i].timestamp.isValid()) {
                        m_weatherData[i] = info;
                        break;
                    }
                }
            } else {
                qDebug() << "Weather API response format not recognized.";
            }
        }
        
        if (m_pendingReplies.empty()) {
            // All requests complete
            emit weatherDataUpdated();
            setStatus("Weather data updated");
        }
    } else {
        QString errorMsg = reply->errorString();
        emit weatherError("Weather API error: " + errorMsg);
        setStatus("Error fetching weather: " + errorMsg);
    }
    
    reply->deleteLater();
}

WeatherInfo WeatherService::parseOpenMeteoData(const QJsonObject& root) {
    WeatherInfo info;
    
    // Get the hourly data arrays
    QJsonObject hourly = root["hourly"].toObject();
    QJsonArray time = hourly["time"].toArray();
    QJsonArray temp = hourly["temperature_2m"].toArray();
    QJsonArray precip = hourly["precipitation"].toArray();
    QJsonArray wcode = hourly["weathercode"].toArray();
    QJsonArray wspeed = hourly["windspeed_10m"].toArray();
    QJsonArray wdir = hourly["winddirection_10m"].toArray();
    
    // Find the middle of the array for representative data 
    // (or closest to current time for forecast data)
    int idx = time.size() / 2;
    
    if (idx >= 0 && idx < time.size()) {
        // Parse timestamp
        QString timeStr = time[idx].toString();
        info.timestamp = QDateTime::fromString(timeStr, Qt::ISODate);
        
        // Parse temperature
        if (idx < temp.size()) {
            info.temperature = temp[idx].toDouble();
        }
        
        // Parse precipitation
        if (idx < precip.size()) {
            info.precipitation = precip[idx].toDouble();
        }
        
        // Parse wind speed
        if (idx < wspeed.size()) {
            info.windSpeed = wspeed[idx].toDouble();
        }
        
        // Parse wind direction
        if (idx < wdir.size()) {
            info.windDirection = wdir[idx].toDouble();
        }
        
        // Parse weather code and map to conditions
        if (idx < wcode.size()) {
            int weatherCode = wcode[idx].toInt();
            info.conditions = getWeatherConditionText(weatherCode);
            info.iconCode = getWeatherIconCode(weatherCode, info.timestamp.time().hour());
        }
    }
    
    return info;
}

QString WeatherService::getWeatherConditionText(int weatherCode) {
    // Map WMO weather codes to text descriptions
    // See https://open-meteo.com/en/docs
    switch (weatherCode) {
        case 0: return "Clear sky";
        case 1: return "Mainly clear";
        case 2: return "Partly cloudy";
        case 3: return "Overcast";
        case 45: case 48: return "Fog";
        case 51: case 53: case 55: return "Drizzle";
        case 56: case 57: return "Freezing Drizzle";
        case 61: case 63: case 65: return "Rain";
        case 66: case 67: return "Freezing Rain";
        case 71: case 73: case 75: return "Snow";
        case 77: return "Snow grains";
        case 80: case 81: case 82: return "Showers";
        case 85: case 86: return "Snow showers";
        case 95: return "Thunderstorm";
        case 96: case 99: return "Thunderstorm with hail";
        default: return "Unknown";
    }
}

QString WeatherService::getWeatherIconCode(int weatherCode, int hour) {
    // Determine if it's day or night
    bool isNight = (hour < 6 || hour > 18); // Simple day/night detection
    QString dayNight = isNight ? "n" : "d";
    
    // Map WMO weather codes to icon codes similar to OpenWeatherMap format
    // for compatibility with existing icon rendering code
    switch (weatherCode) {
        case 0: return "01" + dayNight; // Clear
        case 1: return "02" + dayNight; // Mainly clear 
        case 2: return "03" + dayNight; // Partly cloudy
        case 3: return "04" + dayNight; // Overcast
        case 45: case 48: return "50" + dayNight; // Fog
        case 51: case 53: case 55: return "09" + dayNight; // Drizzle
        case 56: case 57: return "09" + dayNight; // Freezing Drizzle
        case 61: case 63: case 65: return "10" + dayNight; // Rain
        case 66: case 67: return "10" + dayNight; // Freezing Rain
        case 71: case 73: case 75: case 77: return "13" + dayNight; // Snow
        case 80: case 81: case 82: return "09" + dayNight; // Showers
        case 85: case 86: return "13" + dayNight; // Snow showers
        case 95: case 96: case 99: return "11" + dayNight; // Thunderstorm
        default: return "01" + dayNight; // Default to clear
    }
}

void WeatherService::setStatus(const QString& message) {
    m_statusMessage = message;
    emit statusChanged(message);
}
