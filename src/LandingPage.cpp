#include "LandingPage.h"
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QPixmap>
#include <QIcon>
#include <QFontDatabase>
#include <QFile>
#include <QMessageBox>
#include <QPainter>

LandingPage::LandingPage(QWidget *parent)
    : QWidget(parent),
      m_currentTipIndex(0)
{
    setupStyles();
    setupUI();
    loadTips();
    updateRecentFiles();
    loadSampleRoutes();
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
    );
}

void LandingPage::setupUI() {
    // Set object name for styling
    setObjectName("landingPage");
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);
    
    // Title section
    m_titleLabel = new QLabel("Route Explorer", this);
    m_titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(m_titleLabel);
    
    m_subtitleLabel = new QLabel("Visualize and analyze your GPX routes with ease", this);
    m_subtitleLabel->setObjectName("subtitleLabel");
    mainLayout->addWidget(m_subtitleLabel);
    
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
    
    // Add spacer
    rightLayout->addStretch();
    
    // Version info
    QLabel* versionLabel = new QLabel(QString("Version %1").arg(QApplication::applicationVersion()), this);
    versionLabel->setAlignment(Qt::AlignRight);
    versionLabel->setStyleSheet("color: #9e9e9e; font-size: 12px;");
    rightLayout->addWidget(versionLabel);
    
    contentLayout->addLayout(rightLayout, 1);
    
    mainLayout->addLayout(contentLayout, 1);
    
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
    // Check for sample files in the application's samples directory
    QDir samplesDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/samples");
    
    // If the samples directory doesn't exist or is empty, use a built-in example
    if (!samplesDir.exists() || samplesDir.entryList(QDir::Files).isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/map-marker.svg"), "Example Route");
        item->setData(Qt::UserRole, ":/samples/example_route.gpx");
        m_samplesListWidget->addItem(item);
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
