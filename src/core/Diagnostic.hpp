#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
namespace opencodescan {
enum class Severity {
    Info,
    Warning,
    Error,
    Critical
};
inline QString toDisplayString(const Severity severity) {
    switch (severity) {
    case Severity::Info:
        return QStringLiteral("Info");
    case Severity::Warning:
        return QStringLiteral("Warning");
    case Severity::Error:
        return QStringLiteral("Error");
    case Severity::Critical:
        return QStringLiteral("Critical");
    }
    return QStringLiteral("Unknown");
}
struct Diagnostic {
    QString ruleId;
    Severity severity {Severity::Info};
    QString filePath;
    int line {0};
    int column {0};
    QString message;
    QString remediationHint;
};
struct AnalysisRequest {
    QString projectRootPath;
    QStringList includePaths;
    QStringList defines;
    QStringList excludedPaths;
};
struct AnalysisResult {
    QVector<Diagnostic> diagnostics;
    QStringList notes;
};
} // namespace opencodescan
