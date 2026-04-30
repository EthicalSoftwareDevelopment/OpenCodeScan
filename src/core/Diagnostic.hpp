#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <functional>
#include <memory>
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
enum class ScanState {
    Idle,
    Preparing,
    DiscoveringFiles,
    LoadingCompileCommands,
    BuildingTranslationUnits,
    Completed,
    Cancelled
};
inline QString toDisplayString(const ScanState state) {
    switch (state) {
    case ScanState::Idle:
        return QStringLiteral("Idle");
    case ScanState::Preparing:
        return QStringLiteral("Preparing");
    case ScanState::DiscoveringFiles:
        return QStringLiteral("Discovering files");
    case ScanState::LoadingCompileCommands:
        return QStringLiteral("Loading compile commands");
    case ScanState::BuildingTranslationUnits:
        return QStringLiteral("Building translation units");
    case ScanState::Completed:
        return QStringLiteral("Completed");
    case ScanState::Cancelled:
        return QStringLiteral("Cancelled");
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
struct TranslationUnitConfig {
    QString filePath;
    QString workingDirectory;
    QStringList includePaths;
    QStringList defines;
    QStringList compilerArguments;
    bool fromCompileCommands {false};
};
struct AnalysisRequest {
    QString projectRootPath;
    QStringList includePaths;
    QStringList defines;
    QStringList excludedPaths;
    bool preferCompileCommands {true};
};
struct ScanProgress {
    ScanState state {ScanState::Idle};
    int totalFiles {0};
    int processedFiles {0};
    QString currentFilePath;
    QString message;
};
struct AnalysisSummary {
    int discoveredFileCount {0};
    int translationUnitCount {0};
    int compileCommandEntryCount {0};
    int filesWithoutCompileCommands {0};
    bool cancelled {false};
};
struct AnalysisResult {
    QVector<Diagnostic> diagnostics;
    QVector<TranslationUnitConfig> translationUnits;
    QStringList notes;
    AnalysisSummary summary;
};
using CancelToken = std::shared_ptr<std::atomic_bool>;
using ProgressCallback = std::function<void(const ScanProgress&)>;
inline bool isCancellationRequested(const CancelToken& token) {
    return token && token->load();
}
} // namespace opencodescan
