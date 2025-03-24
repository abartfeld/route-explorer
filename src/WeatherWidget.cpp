#include "WeatherWidget.h"
#include <QPainter>
#include <QInputDialog>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QStandardPaths>
#include <cmath> // Add this for trigonometric functions and M_PI


class WindArrowWidget : public QWidget {
public:
    WindArrowWidget(QWidget* parent = nullptr) : QWidget(parent), m_direction(0), m_visible(false) {
        setMinimumSize(24, 24);
        setMaximumSize(24, 24);
        setStyleSheet("background-color: transparent;");
    }
    
    void setDirection(double direction) {
        m_direction = direction;
        update();
    }
    
    void setArrowVisible(bool visible) {
        m_visible = visible;
        setVisible(visible);
    }
    
protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        
        if (!m_visible) return;
        
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw the arrow based on wind direction
        QPen pen(QColor("#1976D2"), 2);
        painter.setPen(pen);
        
        // Arrow points in the direction the wind is blowing TO
        // Weather API gives direction the wind is coming FROM, so add 180°
        double arrowDirection = m_direction + 180.0;
        if (arrowDirection >= 360.0) {
            arrowDirection -= 360.0;
        }
        
        // Convert to radians
        double radians = arrowDirection * M_PI / 180.0;
        
        // Calculate arrow points
        int centerX = width() / 2;
        int centerY = height() / 2;
        int radius = std::min(centerX, centerY) - 2;
        
        // Line from center to edge
        int endX = centerX + radius * cos(radians);
        int endY = centerY + radius * sin(radians);
        painter.drawLine(centerX, centerY, endX, endY);
        
        // Arrow head
        double arrowSize = radius * 0.3;
        double arrowAngle1 = radians - M_PI * 0.25; // 45 degrees
        double arrowAngle2 = radians + M_PI * 0.25; // 45 degrees
        
        int arrowX1 = endX - arrowSize * cos(arrowAngle1);
        int arrowY1 = endY - arrowSize * sin(arrowAngle1);
        int arrowX2 = endX - arrowSize * cos(arrowAngle2);
        int arrowY2 = endY - arrowSize * sin(arrowAngle2);
        
        painter.drawLine(endX, endY, arrowX1, arrowY1);
        painter.drawLine(endX, endY, arrowX2, arrowY2);
    }
    
private:
    double m_direction;
    bool m_visible;
};

WeatherWidget::WeatherWidget(QWidget *parent)
    : QWidget(parent), 
      m_weatherService(nullptr),
      m_currentPointIndex(0)
{
    // Modern styling with Material Design influence
    setMinimumWidth(280);
    setMaximumWidth(280);
    setStyleSheet("background-color: white; border-radius: 8px; border: 1px solid #e0e0e0;");
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);
    
    // Title with config button
    QHBoxLayout* titleLayout = new QHBoxLayout();
    m_titleLabel = new QLabel("Weather Information", this);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1976D2;");
    titleLayout->addWidget(m_titleLabel, 1);
    
    m_configButton = new QPushButton(this);
    m_configButton->setIcon(QIcon(":/icons/settings.svg"));
    m_configButton->setToolTip("Configure Weather API");
    m_configButton->setFlat(true);
    m_configButton->setMaximumSize(24, 24);
    m_configButton->setStyleSheet("QPushButton:hover { background-color: #f0f0f0; border-radius: 12px; }");
    connect(m_configButton, &QPushButton::clicked, this, &WeatherWidget::configureFetchOptions);
    titleLayout->addWidget(m_configButton);
    
    mainLayout->addLayout(titleLayout);
    
    // Weather icon and temperature in a horizontal layout
    QHBoxLayout* weatherLayout = new QHBoxLayout();
    weatherLayout->setContentsMargins(0, 8, 0, 8);
    
    m_weatherIconLabel = new QLabel(this);
    m_weatherIconLabel->setMinimumSize(64, 64);
    m_weatherIconLabel->setMaximumSize(64, 64);
    m_weatherIconLabel->setScaledContents(true);
    weatherLayout->addWidget(m_weatherIconLabel);
    
    m_temperatureLabel = new QLabel("--°C", this);
    m_temperatureLabel->setStyleSheet("font-size: 26px; font-weight: bold; color: #212121;");
    m_temperatureLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    weatherLayout->addWidget(m_temperatureLabel, 1);
    
    mainLayout->addLayout(weatherLayout);
    
    // Separator line
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("border: none; background-color: #e0e0e0; max-height: 1px;");
    mainLayout->addWidget(line);
    
    // Conditions
    m_conditionsLabel = new QLabel("No weather data", this);
    m_conditionsLabel->setStyleSheet("font-size: 14px; color: #424242;");
    m_conditionsLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_conditionsLabel);
    
    // Wind information with direction arrow
    QHBoxLayout* windLayout = new QHBoxLayout();
    windLayout->setContentsMargins(0, 8, 0, 0);
    
    m_windLabel = new QLabel("Wind: --", this);
    m_windLabel->setStyleSheet("font-size: 13px; color: #616161;");
    windLayout->addWidget(m_windLabel);
    
    // Create a proper widget for the wind arrow
    m_windArrow = new WindArrowWidget(this);
    windLayout->addWidget(m_windArrow);
    windLayout->addStretch();
    
    mainLayout->addLayout(windLayout);
    
    // Precipitation info
    m_precipitationLabel = new QLabel("Precipitation: --", this);
    m_precipitationLabel->setStyleSheet("font-size: 13px; color: #616161;");
    mainLayout->addWidget(m_precipitationLabel);
    
    // Status message
    m_statusLabel = new QLabel("Weather data not available", this);
    m_statusLabel->setStyleSheet("font-size: 12px; color: #9e9e9e; font-style: italic;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);
    
    mainLayout->addStretch();
    
    // Create cache directory for weather icons if it doesn't exist
    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/weather-icons");
    if (!cacheDir.exists()) {
        cacheDir.mkpath(".");
    }
}

void WeatherWidget::setWeatherService(WeatherService* service) {
    if (m_weatherService) {
        disconnect(m_weatherService, nullptr, this, nullptr);
    }
    
    m_weatherService = service;
    
    if (m_weatherService) {
        connect(m_weatherService, &WeatherService::weatherDataUpdated, 
                this, &WeatherWidget::handleWeatherUpdate);
        connect(m_weatherService, &WeatherService::weatherError,
                this, &WeatherWidget::handleWeatherError);
        connect(m_weatherService, &WeatherService::statusChanged,
                m_statusLabel, &QLabel::setText);
    }
}

void WeatherWidget::updateWeatherDisplay(size_t pointIndex) {
    m_currentPointIndex = pointIndex;
    
    if (!m_weatherService || !m_weatherService->hasWeatherData()) {
        m_temperatureLabel->setText("--°C");
        m_conditionsLabel->setText("No weather data");
        m_windLabel->setText("Wind: --");
        m_precipitationLabel->setText("Precipitation: --");
        static_cast<WindArrowWidget*>(m_windArrow)->setArrowVisible(false);
        return;
    }
    
    const WeatherInfo& info = m_weatherService->getWeatherAtIndex(pointIndex);
    
    if (!info.timestamp.isValid()) {
        m_statusLabel->setText("Weather data not available for this location");
        return;
    }
    
    // Update temperature
    m_temperatureLabel->setText(formatTemperature(info.temperature));
    
    // Update conditions
    m_conditionsLabel->setText(info.conditions);
    
    // Update wind info
    m_windLabel->setText(formatWindInfo(info.windSpeed, info.windDirection));
    
    // Update wind arrow
    WindArrowWidget* arrow = static_cast<WindArrowWidget*>(m_windArrow);
    arrow->setDirection(info.windDirection);
    arrow->setArrowVisible(info.windSpeed > 0);
    
    // Update precipitation
    m_precipitationLabel->setText(formatPrecipitation(info.precipitation));
    
    // Update weather icon
    loadWeatherIcon(info.iconCode);
    
    // Update status message with timestamp
    m_statusLabel->setText("Weather data from: " + info.timestamp.toString("yyyy-MM-dd hh:mm AP"));
}

void WeatherWidget::handleWeatherUpdate() {
    updateWeatherDisplay(m_currentPointIndex);
}

void WeatherWidget::handleWeatherError(const QString& errorMessage) {
    m_statusLabel->setText(errorMessage);
    QMessageBox::warning(this, "Weather Data Error", errorMessage, QMessageBox::Ok);
}

void WeatherWidget::configureFetchOptions() {
    // Modified to explain that no API key is needed
    QMessageBox::information(this, "Weather API Information", 
                          "This application now uses Open-Meteo weather API, which is "
                          "completely free and doesn't require an API key. Weather "
                          "data will be automatically fetched when you load a GPX file "
                          "with valid timestamps.", 
                          QMessageBox::Ok);
    
    m_statusLabel->setText("Ready to fetch weather data. No API key needed!");
}

QString WeatherWidget::formatTemperature(double tempCelsius) const {
    int roundedTemp = qRound(tempCelsius);
    return QString("%1°C").arg(roundedTemp);
}

QString WeatherWidget::formatWindInfo(double speed, double direction) const {
    // Convert speed from m/s to km/h
    double speedKmh = speed * 3.6;
    
    // Get cardinal direction
    static const QStringList directions = {
        "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", 
        "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
    };
    
    int index = qRound(direction / 22.5) % 16;
    return QString("Wind: %1 km/h %2").arg(qRound(speedKmh)).arg(directions[index]);
}

QString WeatherWidget::formatPrecipitation(double amount) const {
    if (amount < 0.1) {
        return "Precipitation: None";
    } else {
        return QString("Precipitation: %1 mm").arg(amount, 0, 'f', 1);
    }
}

void WeatherWidget::loadWeatherIcon(const QString& iconCode) {
    if (iconCode.isEmpty()) {
        m_weatherIconLabel->clear();
        return;
    }
    
    QPixmap icon = getWeatherIcon(iconCode);
    if (!icon.isNull()) {
        m_weatherIconLabel->setPixmap(icon);
    } else {
        m_weatherIconLabel->clear();
    }
}

QPixmap WeatherWidget::getWeatherIcon(const QString& iconCode) {
    // Check if icon is already cached
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + 
                      "/weather-icons/" + iconCode + ".png";
    
    QFile cacheFile(cachePath);
    if (cacheFile.exists()) {
        QPixmap cachedPixmap;
        if (cachedPixmap.load(cachePath)) {
            return cachedPixmap;
        }
    }
    
    // Otherwise, use a simple icon based on the code
    // In a real app, you'd download from OpenWeatherMap:
    // https://openweathermap.org/img/wn/{icon}@2x.png
    QPixmap generatedIcon(64, 64);
    generatedIcon.fill(Qt::transparent);
    
    QPainter painter(&generatedIcon);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Based on icon code, draw a simple representation
    // First digit is for day/night: d=day, n=night
    bool isNight = iconCode.endsWith("n");
    
    // First character is the condition code
    QChar conditionCode = iconCode.at(0);
    
    if (isNight) {
        // Draw moon
        painter.setBrush(QColor(30, 30, 70));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(16, 16, 32, 32);
    } else {
        // Draw sun
        painter.setBrush(QColor(255, 200, 0));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(16, 16, 32, 32);
    }
    
    // Add condition details
    if (conditionCode == '0' || conditionCode == '1') {
        // Clear sky - already drew sun/moon
    } else if (conditionCode == '0' && iconCode.at(1) == '2') {
        // Few clouds
        painter.setBrush(QColor(200, 200, 200, 200));
        painter.drawEllipse(26, 26, 24, 18);
    } else if (conditionCode == '0' && iconCode.at(1) == '3') {
        // Scattered clouds
        painter.setBrush(QColor(180, 180, 180));
        painter.drawEllipse(20, 28, 30, 20);
    } else if (conditionCode == '0' && iconCode.at(1) == '4') {
        // Broken/overcast clouds
        painter.setBrush(QColor(150, 150, 150));
        painter.drawEllipse(14, 28, 36, 24);
    } else if (conditionCode == '0' && iconCode.at(1) == '9') {
        // Shower rain
        painter.setBrush(QColor(100, 100, 220));
        painter.drawEllipse(16, 16, 32, 22);
        
        // Draw raindrops
        painter.setPen(QPen(QColor(30, 100, 255), 2));
        painter.drawLine(24, 45, 20, 55);
        painter.drawLine(32, 45, 28, 55);
        painter.drawLine(40, 45, 36, 55);
    } else if (conditionCode == '1' && iconCode.at(1) == '0') {
        // Rain
        painter.setBrush(QColor(80, 80, 180));
        painter.drawEllipse(16, 16, 32, 22);
        
        // Draw rain
        painter.setPen(QPen(QColor(30, 100, 255), 2));
        for (int i = 20; i <= 44; i += 8) {
            painter.drawLine(i, 42, i - 2, 55);
        }
    } else if (conditionCode == '1' && iconCode.at(1) == '1') {
        // Thunderstorm
        painter.setBrush(QColor(60, 60, 140));
        painter.drawEllipse(16, 16, 32, 22);
        
        // Draw lightning
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 0));
        QPolygonF lightning;
        lightning << QPointF(32, 36) << QPointF(28, 45) << QPointF(34, 45) << QPointF(30, 55);
        painter.drawPolygon(lightning);
    } else if (conditionCode == '1' && iconCode.at(1) == '3') {
        // Snow
        painter.setBrush(QColor(200, 200, 220));
        painter.drawEllipse(16, 16, 32, 22);
        
        // Draw snowflakes
        painter.setPen(QPen(Qt::white, 2));
        for (int i = 20; i <= 44; i += 8) {
            painter.drawEllipse(i, 48, 4, 4);
        }
    } else if (conditionCode == '5' && iconCode.at(1) == '0') {
        // Mist/fog
        painter.setBrush(QColor(180, 180, 180, 150));
        painter.drawEllipse(16, 16, 32, 32);
        
        // Draw mist lines
        painter.setPen(QPen(QColor(200, 200, 200), 2));
        for (int y = 32; y <= 52; y += 6) {
            painter.drawLine(16, y, 48, y);
        }
    }
    
    // Save to cache
    generatedIcon.save(cachePath);
    
    return generatedIcon;
}
