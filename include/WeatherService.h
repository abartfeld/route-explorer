#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QGeoCoordinate>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include <memory>

/**
 * @brief Structure to hold weather data for a specific point on the track
 */
struct WeatherInfo {
    QDateTime timestamp;       // Time of the weather data
    double temperature = 0.0;  // Temperature in celsius
    double windSpeed = 0.0;    // Wind speed in m/s
    double windDirection = 0.0; // Wind direction in degrees (0-360)
    double precipitation = 0.0; // Precipitation amount in mm
    QString conditions;        // Text description of conditions (e.g., "Clear", "Rain")
    QString iconCode;          // Icon code for weather condition
    
    WeatherInfo() = default;
};

/**
 * @brief Service for fetching weather data for the track
 * 
 * Uses Open-Meteo free weather API to get historical or forecast weather data
 * for points along the track based on timestamps.
 */
class WeatherService : public QObject {
    Q_OBJECT
    
public:
    explicit WeatherService(QObject* parent = nullptr);
    ~WeatherService() = default;
    
    /**
     * @brief Get weather data for a track point
     * @param coord Geographic coordinates
     * @param timestamp Time of the track point
     */
    void getWeatherForPoint(const QGeoCoordinate& coord, const QDateTime& timestamp);
    
    /**
     * @brief Get weather data for a track
     * @param coordinates List of geographic coordinates
     * @param timestamps List of timestamps for each coordinate
     */
    void getWeatherForTrack(const std::vector<QGeoCoordinate>& coordinates, 
                           const std::vector<QDateTime>& timestamps);
    
    /**
     * @brief Set the API key (not needed for Open-Meteo, but kept for interface compatibility)
     * @param apiKey The API key
     */
    void setApiKey(const QString& apiKey) { /* No-op - Open-Meteo is free */ }
    
    /**
     * @brief Get weather information for a specific index along the track
     * @param index Index of the point
     * @return Weather information at that point
     */
    const WeatherInfo& getWeatherAtIndex(size_t index) const;
    
    /**
     * @brief Check if weather data is available for the current track
     * @return True if weather data is available
     */
    bool hasWeatherData() const { return !m_weatherData.empty(); }
    
    /**
     * @brief Get all weather data for the track
     * @return Vector of weather information
     */
    const std::vector<WeatherInfo>& getAllWeatherData() const { return m_weatherData; }
    
    /**
     * @brief Get the current status message
     * @return Status message
     */
    QString statusMessage() const { return m_statusMessage; }
    
signals:
    /**
     * @brief Signal emitted when weather data is updated
     */
    void weatherDataUpdated();
    
    /**
     * @brief Signal emitted when an error occurs
     * @param errorMessage Description of the error
     */
    void weatherError(const QString& errorMessage);
    
    /**
     * @brief Signal emitted when the status changes
     * @param message New status message
     */
    void statusChanged(const QString& message);
    
private slots:
    void handleWeatherReply();
    
private:
    QNetworkAccessManager* m_networkManager;
    std::vector<WeatherInfo> m_weatherData;
    std::vector<QNetworkReply*> m_pendingReplies;
    QString m_statusMessage;
    
    // Helper methods for Open-Meteo API
    WeatherInfo parseOpenMeteoData(const QJsonObject& data);
    QString getWeatherConditionText(int weatherCode);
    QString getWeatherIconCode(int weatherCode, int hour);
    void setStatus(const QString& message);
};
