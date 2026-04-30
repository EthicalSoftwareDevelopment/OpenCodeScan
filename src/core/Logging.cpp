#include "core/Logging.hpp"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <QtLogging>
#include <memory>
namespace opencodescan {
namespace {
QMutex g_logMutex;
std::unique_ptr<QFile> g_logFile;
QString g_logFilePath;
void writeMessage(const QString& level, const QString& message) {
    const QMutexLocker locker(&g_logMutex);
    const auto timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream stream(g_logFile.get());
        stream << timestamp << " [" << level << "] " << message << Qt::endl;
        stream.flush();
    }
    if (level == QStringLiteral("ERROR")) {
        qCritical().noquote() << message;
    } else if (level == QStringLiteral("WARNING")) {
        qWarning().noquote() << message;
    } else {
        qInfo().noquote() << message;
    }
}
} // namespace
void Logger::initialize() {
    const QMutexLocker locker(&g_logMutex);
    if (g_logFile && g_logFile->isOpen()) {
        return;
    }
    const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(appDataPath);
    g_logFilePath = QDir(appDataPath).filePath(QStringLiteral("OpenCodeScan.log"));
    g_logFile = std::make_unique<QFile>(g_logFilePath);
    if (!g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning().noquote() << QStringLiteral("Unable to open log file: %1").arg(g_logFilePath);
        g_logFile.reset();
        return;
    }
    QTextStream stream(g_logFile.get());
    stream << "--- OpenCodeScan session started: "
           << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
           << " ---"
           << Qt::endl;
    stream.flush();
}
void Logger::info(const QString& message) {
    writeMessage(QStringLiteral("INFO"), message);
}
void Logger::warning(const QString& message) {
    writeMessage(QStringLiteral("WARNING"), message);
}
void Logger::error(const QString& message) {
    writeMessage(QStringLiteral("ERROR"), message);
}
QString Logger::logFilePath() {
    const QMutexLocker locker(&g_logMutex);
    return g_logFilePath;
}
} // namespace opencodescan
