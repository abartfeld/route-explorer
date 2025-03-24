#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

/**
 * Initialize application-specific folders and settings
 */
void initializeApp() {
    // Get the application directory
    QString appDir = QCoreApplication::applicationDirPath();
    qDebug() << "Application directory:" << appDir;
    
    // Create application directories if they don't exist
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dataDir.exists()) {
        bool success = dataDir.mkpath(".");
        qDebug() << "Created app data directory:" << dataDir.absolutePath() << (success ? "(success)" : "(failed)");
    }
    
    // Create cache directory
    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (!cacheDir.exists()) {
        bool success = cacheDir.mkpath(".");
        qDebug() << "Created cache directory:" << cacheDir.absolutePath() << (success ? "(success)" : "(failed)");
        
        // Create subdirectories for map tiles and other cached data
        cacheDir.mkpath("maptiles");
        qDebug() << "Created maptiles subdirectory";
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application metadata
    QCoreApplication::setOrganizationName("RouteExplorer");
    QCoreApplication::setApplicationName("GPX Viewer");
    QCoreApplication::setApplicationVersion("1.0");
    
    // Initialize application directories and settings
    initializeApp();
    
    qDebug() << "Working directory:" << QDir::currentPath();
    
    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}
