#include "analysis/DeterministicAnalysisEngine.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
namespace opencodescan {
namespace {
Diagnostic makeDiagnostic(QString ruleId,
                          Severity severity,
                          QString filePath,
                          QString message,
                          QString remediationHint,
                          int line = 0,
                          int column = 0) {
    Diagnostic diagnostic;
    diagnostic.ruleId = std::move(ruleId);
    diagnostic.severity = severity;
    diagnostic.filePath = std::move(filePath);
    diagnostic.line = line;
    diagnostic.column = column;
    diagnostic.message = std::move(message);
    diagnostic.remediationHint = std::move(remediationHint);
    return diagnostic;
}
} // namespace
AnalysisResult DeterministicAnalysisEngine::analyzeProject(const AnalysisRequest& request,
                                                           const ProgressCallback& progressCallback,
                                                           const CancelToken& cancelToken) {
    AnalysisResult result;
    reportProgress(progressCallback,
                   ScanState::Preparing,
                   0,
                   0,
                   {},
                   QStringLiteral("Validating project path."));
    if (request.projectRootPath.trimmed().isEmpty()) {
        result.notes << QStringLiteral("No project path selected.");
        return result;
    }
    const auto normalizedRootPath = QDir::cleanPath(request.projectRootPath);
    const QDir projectDirectory(normalizedRootPath);
    if (!projectDirectory.exists()) {
        result.notes << QStringLiteral("Selected project path does not exist.");
        return result;
    }
    reportProgress(progressCallback,
                   ScanState::LoadingCompileCommands,
                   0,
                   0,
                   {},
                   QStringLiteral("Loading compile commands."));
    const auto compileCommandsResult = request.preferCompileCommands
        ? compileCommandsLoader_.load(normalizedRootPath)
        : CompileCommandsLoadResult {};
    result.notes << compileCommandsResult.notes;
    result.summary.compileCommandEntryCount = compileCommandsResult.translationUnitsByFile.size();
    reportProgress(progressCallback,
                   ScanState::DiscoveringFiles,
                   0,
                   0,
                   {},
                   QStringLiteral("Discovering C/C++ files."));
    AnalysisRequest normalizedRequest = request;
    normalizedRequest.projectRootPath = normalizedRootPath;
    const auto discoveredFiles = fileDiscoverer_.discover(normalizedRequest);
    result.summary.discoveredFileCount = discoveredFiles.size();
    int filesWithoutCompileCommands = 0;
    int processedFiles = 0;
    for (const auto& filePath : discoveredFiles) {
        if (isCancellationRequested(cancelToken)) {
            result.summary.cancelled = true;
            result.notes << QStringLiteral("Scan cancelled by user.");
            reportProgress(progressCallback,
                           ScanState::Cancelled,
                           discoveredFiles.size(),
                           processedFiles,
                           filePath,
                           QStringLiteral("Cancelling scan."));
            break;
        }
        reportProgress(progressCallback,
                       ScanState::BuildingTranslationUnits,
                       discoveredFiles.size(),
                       processedFiles,
                       filePath,
                       QStringLiteral("Preparing translation unit configuration."));
        bool hasCompileCommand = false;
        const auto translationUnit = makeTranslationUnitConfig(filePath,
                                                               normalizedRequest,
                                                               compileCommandsResult.translationUnitsByFile,
                                                               &hasCompileCommand);
        if (!hasCompileCommand) {
            ++filesWithoutCompileCommands;
        }
        QFile sourceFile(filePath);
        if (!sourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            result.diagnostics.push_back(makeDiagnostic(QStringLiteral("OCS-FILE-OPEN"),
                                                        Severity::Error,
                                                        filePath,
                                                        QStringLiteral("Unable to read source file while preparing translation unit configuration."),
                                                        QStringLiteral("Verify file permissions and that the file still exists.")));
        }
        result.translationUnits.push_back(translationUnit);
        ++processedFiles;
    }
    result.summary.translationUnitCount = result.translationUnits.size();
    result.summary.filesWithoutCompileCommands = filesWithoutCompileCommands;
    result.diagnostics.push_back(makeDiagnostic(QStringLiteral("OCS-SCAN"),
                                                Severity::Info,
                                                normalizedRootPath,
                                                QStringLiteral("Prepared %1 translation units across %2 discovered C/C++ files.")
                                                    .arg(result.summary.translationUnitCount)
                                                    .arg(result.summary.discoveredFileCount),
                                                QStringLiteral("Phase 3 builds on these deterministic translation unit inputs to run rule-based analysis.")));
    if (compileCommandsResult.compileCommandsPath.isEmpty()) {
        result.diagnostics.push_back(makeDiagnostic(QStringLiteral("OCS-COMPILE-COMMANDS"),
                                                    Severity::Warning,
                                                    normalizedRootPath,
                                                    QStringLiteral("No compile_commands.json was found. Translation units are using manual include paths and defines only."),
                                                    QStringLiteral("Generate compile_commands.json from your build system for more accurate parser configuration.")));
    } else if (filesWithoutCompileCommands > 0) {
        result.diagnostics.push_back(makeDiagnostic(QStringLiteral("OCS-TU-COVERAGE"),
                                                    Severity::Warning,
                                                    normalizedRootPath,
                                                    QStringLiteral("%1 discovered files did not have matching compile_commands.json entries.")
                                                        .arg(filesWithoutCompileCommands),
                                                    QStringLiteral("Ensure your build exports compile commands for all relevant translation units or provide manual include paths and defines.")));
    }
    if (result.summary.cancelled) {
        result.diagnostics.push_back(makeDiagnostic(QStringLiteral("OCS-CANCELLED"),
                                                    Severity::Info,
                                                    normalizedRootPath,
                                                    QStringLiteral("Scan cancelled after preparing %1 translation units.")
                                                        .arg(result.summary.translationUnitCount),
                                                    QStringLiteral("Restart the scan when you are ready to finish preparing the remaining files.")));
    }
    result.notes << QStringLiteral("Discovered %1 C/C++ files.").arg(result.summary.discoveredFileCount);
    result.notes << QStringLiteral("Prepared %1 translation unit configurations.").arg(result.summary.translationUnitCount);
    result.notes << QStringLiteral("Files without compile command coverage: %1.").arg(result.summary.filesWithoutCompileCommands);
    reportProgress(progressCallback,
                   result.summary.cancelled ? ScanState::Cancelled : ScanState::Completed,
                   discoveredFiles.size(),
                   processedFiles,
                   {},
                   result.summary.cancelled ? QStringLiteral("Scan cancelled.") : QStringLiteral("Scan completed."));
    return result;
}
TranslationUnitConfig DeterministicAnalysisEngine::makeTranslationUnitConfig(
    const QString& filePath,
    const AnalysisRequest& request,
    const QHash<QString, TranslationUnitConfig>& compileCommandsByFile,
    bool* hasCompileCommand) const {
    TranslationUnitConfig config;
    config.filePath = filePath;
    config.workingDirectory = request.projectRootPath;
    config.includePaths = request.includePaths;
    config.defines = request.defines;
    const auto normalizedFilePath = QDir::cleanPath(filePath);
    const auto iterator = compileCommandsByFile.constFind(normalizedFilePath);
    if (iterator != compileCommandsByFile.cend()) {
        config = iterator.value();
        if (!request.includePaths.isEmpty()) {
            for (const auto& includePath : request.includePaths) {
                if (!config.includePaths.contains(includePath)) {
                    config.includePaths << includePath;
                }
            }
        }
        if (!request.defines.isEmpty()) {
            for (const auto& define : request.defines) {
                if (!config.defines.contains(define)) {
                    config.defines << define;
                }
            }
        }
        if (hasCompileCommand != nullptr) {
            *hasCompileCommand = true;
        }
        return config;
    }
    if (hasCompileCommand != nullptr) {
        *hasCompileCommand = false;
    }
    config.workingDirectory = QFileInfo(filePath).absolutePath();
    return config;
}
void DeterministicAnalysisEngine::reportProgress(const ProgressCallback& progressCallback,
                                                 const ScanState state,
                                                 const int totalFiles,
                                                 const int processedFiles,
                                                 const QString& currentFilePath,
                                                 const QString& message) const {
    if (!progressCallback) {
        return;
    }
    ScanProgress progress;
    progress.state = state;
    progress.totalFiles = totalFiles;
    progress.processedFiles = processedFiles;
    progress.currentFilePath = currentFilePath;
    progress.message = message;
    progressCallback(progress);
}
} // namespace opencodescan
