#include "LandingPage.h"
#include "build_info.h"
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QPixmap>
#include <QIcon>
#include <QFontDatabase>
#include <QFile>
#include <QMessageBox>
#include <QPainter>
#include <QDesktopServices>
#include <QUrl>
#include <QDate>

LandingPage::LandingPage(QWidget *parent)
    : QWidget(parent),
      m_currentTipIndex(0)
{
    setupStyles();
    setupUI();
    loadTips();
    updateRecentFiles();
    loadSampleRoutes();
    
    // Show a welcome message with current date and build timestamp
    QString welcomeMessage = "Welcome to Route Explorer! Today is " + 
                            QDate::currentDate().toString("dddd, MMMM d, yyyy") +
                            " | Build: " + BUILD_TIMESTAMP;
    statusBar()->showMessage(welcomeMessage, 5000);
    
    // Add a persistent build timestamp label at the bottom of the page
    QLabel* buildLabel = new QLabel("Build: " + BUILD_TIMESTAMP, this);
    buildLabel->setStyleSheet("font-size: 10px; color: #757575;");
    buildLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    buildLabel->setContentsMargins(0, 0, 10, 5);
    
    // Add the build label to the main layout at the bottom
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->addWidget(buildLabel);
    }
}

void LandingPage::setupStyles() {
    // Add Roboto font if not already loaded
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Light.ttf");
    
    // Set stylesheet for the landing page
    setStyleSheet(
        "QWidget#landingPage { background-color: #f5f5f5; }"
        "QLabel#titleLabel { font-family: 'Roboto'; font-size: 32px; font-weight: bold; color: #2196F3; }"
        "QLabel#subtitleLabel { font-family: 'Roboto'; font-size: 16px; color: #757575; }"
        "QLabel#sectionLabel { font-family: 'Roboto'; font-size: 18px; font-weight: bold; color: #424242; }"
        "QLabel#headerBanner { background-color: #2196F3; color: white; border-radius: 8px; }"
        "QLabel#tipLabel { font-family: 'Roboto'; font-size: 14px; color: #424242; background-color: #e3f2fd; "
        "                 padding: 12px; border-radius: 6px; }"
        "QPushButton#tipButton { background-color: transparent; border: none; color: #2196F3; }"
        "QPushButton#tipButton:hover { color: #1976D2; }"
        "QListWidget { background-color: white; border: 1px solid #e0e0e0; border-radius: 6px; padding: 8px; }"
        "QListWidget::item { padding: 8px; border-radius: 4px; }"
        "QListWidget::item:hover { background-color: #f5f5f5; }"
        "QListWidget::item:selected { background-color: #e3f2fd; color: #2196F3; }"
        "QPushButton#actionButton { font-family: 'Roboto'; font-size: 16px; font-weight: bold; "
        "                          background-color: white; color: #2196F3; border: 1px solid #e0e0e0; "
        "                          border-radius: 8px; padding: 16px; text-align: left; }"
        "QPushButton#actionButton:hover { background-color: #f5f5f5; border: 1px solid #bdbdbd; }"
        "QLabel#actionDescription { font-family: 'Roboto'; font-size: 12px; color: #757575; }"
        "QPushButton#linkButton { background-color: transparent; border: none; color: #2196F3; "
        "                        text-decoration: underline; text-align: left; }"
        "QPushButton#linkButton:hover { color: #1976D2; }"
        "QStatusBar { background-color: #e3f2fd; color: #424242; border-top: 1px solid #bbdefb; }"
    );
}

QStatusBar* LandingPage::statusBar() {
    if (!m_statusBar) {
        m_statusBar = new QStatusBar(this);
        m_statusBar->setSizeGripEnabled(false);
    }
    return m_statusBar;
}

void LandingPage::setupUI() {
    // Set object name for styling
    setObjectName("landingPage");
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);
    
    // Create a header banner with app logo
    QLabel* headerBanner = new QLabel(this);
    headerBanner->setObjectName("headerBanner");
    headerBanner->setFixedHeight(100);
    QHBoxLayout* headerLayout = new QHBoxLayout(headerBanner);
    headerLayout->setContentsMargins(20, 10, 20, 10);
    
    // App logo (could be replaced with an actual image)
    QLabel* logoLabel = new QLabel(this);
    logoLabel->setPixmap(QIcon(":/icons/map-marker.svg").pixmap(64, 64));
    headerLayout->addWidget(logoLabel);
    
    // Title section
    QVBoxLayout* titleLayout = new QVBoxLayout();
    m_titleLabel = new QLabel("Route Explorer", this);
    m_titleLabel->setObjectName("titleLabel");
    titleLayout->addWidget(m_titleLabel);
    
    m_subtitleLabel = new QLabel("Visualize and analyze your GPX routes with ease", this);
    m_subtitleLabel->setObjectName("subtitleLabel");
    titleLayout->addWidget(m_subtitleLabel);
    
    headerLayout->addLayout(titleLayout, 1);
    
    // Weather widget placeholder (could be expanded in the future)
    QLabel* weatherWidget = new QLabel("Weather: 68°F, Sunny", this);
    weatherWidget->setStyleSheet("color: white; font-size: 14px;");
    headerLayout->addWidget(weatherWidget);
    
    mainLayout->addWidget(headerBanner);
    
    // Horizontal layout for main content
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(24);
    
    // Left side - Recent files and Samples
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(16);
    
    // Recent files section
    QLabel* recentFilesLabel = new QLabel("Recent Files", this);
    recentFilesLabel->setObjectName("sectionLabel");
    leftLayout->addWidget(recentFilesLabel);
    
    m_recentFilesListWidget = new QListWidget(this);
    m_recentFilesListWidget->setMinimumWidth(300);
    m_recentFilesListWidget->setMinimumHeight(200);
    leftLayout->addWidget(m_recentFilesListWidget);
    
    // Sample routes section
    QLabel* samplesLabel = new QLabel("Sample Routes", this);
    samplesLabel->setObjectName("sectionLabel");
    leftLayout->addWidget(samplesLabel);
    
    m_samplesListWidget = new QListWidget(this);
    m_samplesListWidget->setMinimumHeight(150);
    leftLayout->addWidget(m_samplesListWidget);
    
    // Tips section
    QWidget* tipContainer = new QWidget(this);
    QVBoxLayout* tipLayout = new QVBoxLayout(tipContainer);
    tipLayout->setContentsMargins(0, 12, 0, 0);
    
    QLabel* tipsLabel = new QLabel("Tip of the Day", this);
    tipsLabel->setObjectName("sectionLabel");
    tipLayout->addWidget(tipsLabel);
    
    m_tipLabel = new QLabel(this);
    m_tipLabel->setObjectName("tipLabel");
    m_tipLabel->setWordWrap(true);
    m_tipLabel->setMinimumHeight(80);
    tipLayout->addWidget(m_tipLabel);
    
    m_newTipButton = new QPushButton("Next Tip", this);
    m_newTipButton->setObjectName("tipButton");
    m_newTipButton->setCursor(Qt::PointingHandCursor);
    QHBoxLayout* tipButtonLayout = new QHBoxLayout();
    tipButtonLayout->addStretch();
    tipButtonLayout->addWidget(m_newTipButton);
    tipLayout->addLayout(tipButtonLayout);
    
    leftLayout->addWidget(tipContainer);
    
    contentLayout->addLayout(leftLayout, 1);
    
    // Right side - Quick actions
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(16);
    
    QLabel* quickActionsLabel = new QLabel("Quick Actions", this);
    quickActionsLabel->setObjectName("sectionLabel");
    rightLayout->addWidget(quickActionsLabel);
    
    // Open file button
    rightLayout->addWidget(createActionButton(
        "Open GPX File", 
        ":/icons/open-file.svg", 
        [this]() { handleBrowseClicked(); },
        "Open and visualize an existing GPX route file"
    ));
    
    // New route button
    rightLayout->addWidget(createActionButton(
        "Create New Route", 
        ":/icons/map-marker.svg", 
        [this]() { handleCreateNewRouteClicked(); },
        "Design a new route by placing points on the map"
    ));
    
    // 3D Flyover button - adding this new option
    rightLayout->addWidget(createActionButton(
        "3D Flyover View", 
        ":/icons/map-marker.svg", 
        [this]() { handleShow3DViewClicked(); },
        "Explore loaded routes in an immersive 3D view"
    ));
    
    // Help/documentation button
    rightLayout->addWidget(createActionButton(
        "Help & Documentation", 
        ":/icons/settings.svg",  // Using settings icon as placeholder 
        [this]() { handleHelpClicked(); },
        "Get started with tutorials and documentation"
    ));
    
    // Settings button
    rightLayout->addWidget(createActionButton(
        "Settings", 
        ":/icons/settings.svg", 
        [this]() { handleSettingsClicked(); },
        "Configure application settings and preferences"
    ));
    
    // Add spacer
    rightLayout->addStretch();
    
    // Version info
    QLabel* versionLabel = new QLabel(QString("Version %1").arg(QApplication::applicationVersion()), this);
    versionLabel->setAlignment(Qt::AlignRight);
    versionLabel->setStyleSheet("color: #9e9e9e; font-size: 12px;");
    rightLayout->addWidget(versionLabel);
    
    contentLayout->addLayout(rightLayout, 1);
    
    mainLayout->addLayout(contentLayout, 1);
    
    // Create footer with links
    QWidget* footer = new QWidget(this);
    QHBoxLayout* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, 10, 0, 0);
    
    // Add useful links
    QPushButton* websiteLink = new QPushButton("Website", this);
    websiteLink->setObjectName("linkButton");
    connect(websiteLink, &QPushButton::clicked, [this]() {
        QDesktopServices::openUrl(QUrl("https://route-explorer.example.com"));
    });
    footerLayout->addWidget(websiteLink);
    
    QPushButton* reportBugLink = new QPushButton("Report Bug", this);
    reportBugLink->setObjectName("linkButton");
    connect(reportBugLink, &QPushButton::clicked, [this]() {
        QDesktopServices::openUrl(QUrl("https://github.com/example/route-explorer/issues"));
    });
    footerLayout->addWidget(reportBugLink);
    
    QPushButton* releasesLink = new QPushButton("Latest Release", this);
    releasesLink->setObjectName("linkButton");
    connect(releasesLink, &QPushButton::clicked, [this]() {
        QDesktopServices::openUrl(QUrl("https://github.com/example/route-explorer/releases"));
    });
    footerLayout->addWidget(releasesLink);
    
    footerLayout->addStretch(1);
    
    mainLayout->addWidget(footer);
    
    // Add status bar to the bottom
    mainLayout->addWidget(statusBar());
    
    // Connect signals
    connect(m_recentFilesListWidget, &QListWidget::itemClicked, this, &LandingPage::handleRecentFileClicked);
    connect(m_samplesListWidget, &QListWidget::itemClicked, this, &LandingPage::handleSampleClicked);
    connect(m_newTipButton, &QPushButton::clicked, this, &LandingPage::handleTipButtonClicked);
    
    // Show initial tip
    showNextTip();
}

QWidget* LandingPage::createActionButton(const QString& text, 
                                      const QString& iconPath, 
                                      const std::function<void()>& action,
                                      const QString& description) {
    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    
    QPushButton* button = new QPushButton(text, this);
    button->setObjectName("actionButton");
    button->setCursor(Qt::PointingHandCursor);
    
    // Set icon if provided
    if (!iconPath.isEmpty()) {
        button->setIcon(QIcon(iconPath));
        button->setIconSize(QSize(32, 32));
    }
    
    connect(button, &QPushButton::clicked, this, action);
    layout->addWidget(button);
    
    // Add description if provided
    if (!description.isEmpty()) {
        QLabel* descLabel = new QLabel(description, this);
        descLabel->setObjectName("actionDescription");
        layout->addWidget(descLabel);
    }
    
    return container;
}

void LandingPage::updateRecentFiles() {
    m_recentFilesListWidget->clear();
    
    QSettings settings;
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    
    if (recentFiles.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("No recent files");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        m_recentFilesListWidget->addItem(item);
    } else {
        for (const QString& filePath : recentFiles) {
            QFileInfo fileInfo(filePath);
            if (fileInfo.exists()) {
                QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/open-file.svg"), 
                                                           truncateFilePath(filePath));
                item->setData(Qt::UserRole, filePath);
                item->setToolTip(filePath);
                m_recentFilesListWidget->addItem(item);
            }
        }
    }
}

void LandingPage::loadSampleRoutes() {
    m_samplesListWidget->clear();
    
    // Check for sample files in the GPX directory first
    QDir gpxDir("../gpx/");
    if (gpxDir.exists()) {
        for (const QString& fileName : gpxDir.entryList(QStringList() << "*.gpx", QDir::Files)) {
            QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/map-marker.svg"), fileName);
            item->setData(Qt::UserRole, gpxDir.filePath(fileName));
            item->setToolTip("Sample route: " + fileName);
            m_samplesListWidget->addItem(item);
        }
        
        // If we found samples in the gpx directory, return
        if (m_samplesListWidget->count() > 0) {
            return;
        }
    }
    
    // Check for sample files in the application's samples directory
    QDir samplesDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/samples");
    
    // If the samples directory doesn't exist or is empty, do not add any built-in example
    if (!samplesDir.exists() || samplesDir.entryList(QDir::Files).isEmpty()) {
        return;
    }
    
    // Add all files from the samples directory
    for (const QString& fileName : samplesDir.entryList(QStringList() << "*.gpx", QDir::Files)) {
        QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/map-marker.svg"), fileName);
        item->setData(Qt::UserRole, samplesDir.filePath(fileName));
        m_samplesListWidget->addItem(item);
    }
}

void LandingPage::loadTips() {
    m_tips.clear();
    
    // Add some useful tips about the application
    m_tips << "Use the mouse wheel to zoom in and out on the map.";
    m_tips << "Hold Shift while moving the slider to precisely position the marker.";
    m_tips << "Toggle between imperial and metric units in the Statistics panel.";
    m_tips << "The 3D view allows you to experience your route in a virtual flythrough.";
    m_tips << "Click on any segment in the Statistics panel to see detailed information.";
    m_tips << "The gradient colors show uphill (red/orange) and downhill (blue/purple) sections.";
    m_tips << "Hover over any point on the map or elevation profile to see its details.";
    m_tips << "You can pause the flythrough animation at any point by clicking the pause button.";
    m_tips << "Use keyboard shortcuts: Ctrl+O to open files, + and - to zoom in/out.";
    m_tips << "The elevation profile shows your climb and descent throughout the route.";
    m_tips << "Click the Home button in the toolbar to return to this landing page.";
    m_tips << "Use the Map View tab for 2D analysis and 3D View for elevation visualization.";
    m_tips << "The camera tilt slider in 3D View lets you adjust your perspective.";
    m_tips << "You can adjust the speed of the 3D flythrough using the speed slider.";
    m_tips << "GPX files can be downloaded from various fitness services or created with this app.";
}

void LandingPage::showNextTip() {
    if (m_tips.isEmpty()) {
        m_tipLabel->setText("No tips available.");
        return;
    }
    
    // Show a random tip
    int newIndex = QRandomGenerator::global()->bounded(m_tips.size());
    if (m_tips.size() > 1 && newIndex == m_currentTipIndex) {
        // Avoid showing the same tip twice in a row
        newIndex = (newIndex + 1) % m_tips.size();
    }
    
    m_currentTipIndex = newIndex;
    m_tipLabel->setText(m_tips[m_currentTipIndex]);
}

QString LandingPage::truncateFilePath(const QString& path, int maxLength) {
    if (path.length() <= maxLength) {
        return path;
    }
    
    QFileInfo fileInfo(path);
    QString fileName = fileInfo.fileName();
    QString directory = fileInfo.path();
    
    int availableChars = maxLength - fileName.length() - 6; // 6 chars for ".../"
    
    if (availableChars < 4) {
        // If file name itself is very long, truncate it
        return "..." + fileName.right(maxLength - 3);
    }
    
    return directory.left(3) + "..." + directory.right(availableChars) + "/" + fileName;
}

void LandingPage::handleRecentFileClicked(QListWidgetItem* item) {
    if (!item) return;
    
    QString filePath = item->data(Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        emit openFile(filePath);
    }
}

void LandingPage::handleSampleClicked(QListWidgetItem* item) {
    if (!item) return;
    
    QString filePath = item->data(Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        emit openFile(filePath);
    }
}

void LandingPage::handleCreateNewRouteClicked() {
    emit createNewRoute();
}

void LandingPage::handleBrowseClicked() {
    emit browse();
}

void LandingPage::handleTipButtonClicked() {
    showNextTip();
}

void LandingPage::handleHelpClicked() {
    // Show a dialog with basic help information
    QMessageBox::information(this, "Route Explorer Help",
        "<h3>Getting Started with Route Explorer</h3>"
        "<p>Route Explorer lets you visualize and analyze GPX route files from your outdoor activities.</p>"
        "<h4>Basic Usage:</h4>"
        "<ul>"
        "<li>Open a GPX file from your computer</li>"
        "<li>View your route on the map</li>"
        "<li>Analyze elevation profiles</li>"
        "<li>See detailed statistics</li>"
        "<li>Experience your route in 3D</li>"
        "</ul>"
        "<p>For more detailed documentation, visit our website or check out the sample routes.</p>");
}

void LandingPage::handleSettingsClicked() {
    // Emit a signal for the main window to handle settings
    emit showSettings();
}

void LandingPage::handleShow3DViewClicked() {
    // Emit a signal to show the 3D view
    emit show3DView();
}
