#include "MainWindow.h"
#include "debug_helper.h"
#include "build_info.h"
#include "logging.h"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QTimer>
#include <QDateTime>

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
    // Install signal handlers for better crash debug info
    DebugHelper::installSignalHandlers();
    
    // Use high DPI scaling for better display
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    
    // Set rendering to prefer performance over quality
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, false);
    
    QApplication app(argc, argv);
    
    // Set application metadata
    QCoreApplication::setOrganizationName("RouteExplorer");
    QCoreApplication::setApplicationName("GPX Viewer");
    QCoreApplication::setApplicationVersion("1.0");
    
    // Increase debug verbosity
    qSetMessagePattern("[%{time hh:mm:ss.zzz}] %{type}: %{message} (%{file}:%{line})");
    
    // Initialize application directories and settings
    initializeApp();
    
    logInfo("Main", QString("Starting Route Explorer v%1").arg(QCoreApplication::applicationVersion()));
    logInfo("Main", QString("Working directory: %1").arg(QDir::currentPath()));
    logInfo("Main", QString("Build timestamp: %1").arg(BUILD_TIMESTAMP));
    
    // Create and show the main window
    MainWindow mainWindow;
    
    // Explicitly force showing the landing page (to fix startup issue)
    QTimer::singleShot(0, &mainWindow, &MainWindow::showLandingPage);
    
    mainWindow.show();
    
    return app.exec();
}
