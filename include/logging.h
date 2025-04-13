#pragma once

#include <QString>
#include <QDebug>

/**
 * @brief Utility functions for structured logging
 * 
 * These functions provide consistent logging across the application
 * with module identification, standardized formatting, and colorized output.
 */

namespace LogColors {
    // ANSI Color codes for console output
    const char* const RESET   = "\033[0m";
    const char* const RED     = "\033[31m";
    const char* const GREEN   = "\033[32m";
    const char* const YELLOW  = "\033[33m";
    const char* const BLUE    = "\033[34m";
    const char* const MAGENTA = "\033[35m";
    const char* const CYAN    = "\033[36m";
    const char* const WHITE   = "\033[37m";
    const char* const BOLD    = "\033[1m";
}

/**
 * @brief Log a debug message with module identification and cyan color
 * @param module The module/component name
 * @param message The message to log
 */
inline void logDebug(const QString& module, const QString& message) {
    qDebug().noquote() << QString("%1[DEBUG]%2 %3[%4]%5 %6")
        .arg(LogColors::CYAN)
        .arg(LogColors::RESET)
        .arg(LogColors::BLUE)
        .arg(module)
        .arg(LogColors::RESET)
        .arg(message);
}

/**
 * @brief Log an info message with module identification and green color
 * @param module The module/component name
 * @param message The message to log
 */
inline void logInfo(const QString& module, const QString& message) {
    qInfo().noquote() << QString("%1[INFO]%2 %3[%4]%5 %6")
        .arg(LogColors::GREEN)
        .arg(LogColors::RESET)
        .arg(LogColors::BLUE)
        .arg(module)
        .arg(LogColors::RESET)
        .arg(message);
}

/**
 * @brief Log a warning message with module identification and yellow color
 * @param module The module/component name
 * @param message The message to log
 */
inline void logWarning(const QString& module, const QString& message) {
    qWarning().noquote() << QString("%1[WARNING]%2 %3[%4]%5 %6")
        .arg(LogColors::YELLOW)
        .arg(LogColors::RESET)
        .arg(LogColors::BLUE)
        .arg(module)
        .arg(LogColors::RESET)
        .arg(message);
}

/**
 * @brief Log a critical message with module identification and red color
 * @param module The module/component name
 * @param message The message to log
 */
inline void logCritical(const QString& module, const QString& message) {
    qCritical().noquote() << QString("%1%2[CRITICAL]%3 %4[%5]%6 %7")
        .arg(LogColors::BOLD)
        .arg(LogColors::RED)
        .arg(LogColors::RESET)
        .arg(LogColors::BLUE)
        .arg(module)
        .arg(LogColors::RESET)
        .arg(message);
}
