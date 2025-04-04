#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QFileInfo>
#include <QMenu>
#include <QList>
#include <QStandardPaths>
#include <QDir>
#include <functional>
#include <vector>
#include <memory>

/**
 * @brief A landing page widget shown when the application starts
 * 
 * Displays recent files, samples, quick actions, and tips for users
 */
class LandingPage : public QWidget {
    Q_OBJECT
    
public:
    explicit LandingPage(QWidget *parent = nullptr);
    ~LandingPage() = default;
    
    /**
     * @brief Updates the list of recent files displayed
     */
    void updateRecentFiles();
    
signals:
    /**
     * @brief Signal emitted when a file should be opened
     * @param filePath Path to the file to open
     */
    void openFile(const QString& filePath);
    
    /**
     * @brief Signal emitted when the user wants to browse for a file
     */
    void browse();
    
    /**
     * @brief Signal emitted when the user wants to create a new route
     */
    void createNewRoute();
    
private slots:
    void handleRecentFileClicked(QListWidgetItem* item);
    void handleSampleClicked(QListWidgetItem* item);
    void handleCreateNewRouteClicked();
    void handleBrowseClicked();
    void handleTipButtonClicked();
    
private:
    static const int MAX_RECENT_FILES = 10;
    static const int MAX_DISPLAYED_TIPS = 5;
    
    // UI Components
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QListWidget* m_recentFilesListWidget;
    QListWidget* m_samplesListWidget;
    QLabel* m_tipLabel;
    QPushButton* m_newTipButton;
    QList<QString> m_tips;
    int m_currentTipIndex;
    
    // Helper methods
    void setupUI();
    void setupStyles();
    QString truncateFilePath(const QString& path, int maxLength = 50);
    void loadSampleRoutes();
    void loadTips();
    void showNextTip();
    QWidget* createActionButton(const QString& text, 
                               const QString& iconPath, 
                               const std::function<void()>& action,
                               const QString& description = QString());
};
