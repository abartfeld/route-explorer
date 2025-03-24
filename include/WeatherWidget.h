#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <memory>
#include "WeatherService.h"

/**
 * @brief Widget for displaying weather information for the track
 */
class WeatherWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit WeatherWidget(QWidget *parent = nullptr);
    ~WeatherWidget() = default;
    
    /**
     * @brief Update the displayed weather based on track position
     * @param pointIndex Index of the current point on the track
     */
    void updateWeatherDisplay(size_t pointIndex);
    
    /**
     * @brief Set the weather service
     * @param service Pointer to weather service
     */
    void setWeatherService(WeatherService* service);
    
private slots:
    void handleWeatherUpdate();
    void handleWeatherError(const QString& errorMessage);
    void configureFetchOptions();
    
private:
    // UI Elements
    QLabel* m_titleLabel;
    QLabel* m_temperatureLabel;
    QLabel* m_conditionsLabel;
    QLabel* m_windLabel;
    QLabel* m_precipitationLabel;
    QLabel* m_statusLabel;
    QLabel* m_weatherIconLabel;
    QPushButton* m_configButton;
    
    // Wind direction arrow widget
    QWidget* m_windArrow;
    
    // Weather service
    WeatherService* m_weatherService;
    size_t m_currentPointIndex;
    
    // Helper methods
    QString formatTemperature(double tempCelsius) const;
    QString formatWindInfo(double speed, double direction) const;
    QString formatPrecipitation(double amount) const;
    void loadWeatherIcon(const QString& iconCode);
    void paintWindArrow(double direction);
    QPixmap getWeatherIcon(const QString& iconCode);
};
