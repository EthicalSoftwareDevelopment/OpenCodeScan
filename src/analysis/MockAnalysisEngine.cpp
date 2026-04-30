#include "analysis/MockAnalysisEngine.hpp"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QStringList>
namespace opencodescan {
namespace {
QString findRepresentativeSourceFile(const QString& projectRootPath) {
    static const QStringList filters {
        QStringLiteral("*.c"),
        QStringLiteral("*.cc"),
        QStringLiteral("*.cpp"),
        QStringLiteral("*.cxx"),
        QStringLiteral("*.h"),
        QStringLiteral("*.hh"),
        QStringLiteral("*.hpp"),
        QStringLiteral("*.hxx")
    };
    QDirIterator iterator(projectRootPath,
                          filters,
                          QDir::Files,
                          QDirIterator::Subdirectories);
    if (iterator.hasNext()) {
        return iterator.next();
    }
    return {};
}
} // namespace
AnalysisResult MockAnalysisEngine::analyzeProject(const AnalysisRequest& request) {
    AnalysisResult result;
    if (request.projectRootPath.isEmpty()) {
        result.notes << QStringLiteral("No project path selected.");
        return result;
    }
    const QDir projectDirectory(request.projectRootPath);
    if (!projectDirectory.exists()) {
        result.notes << QStringLiteral("Selected project path does not exist.");
        return result;
    }
    const auto representativeSource = findRepresentativeSourceFile(request.projectRootPath);
    const auto displayPath = representativeSource.isEmpty() ? request.projectRootPath : representativeSource;
    result.notes << QStringLiteral("Loaded project root: %1").arg(QDir::cleanPath(request.projectRootPath));
    result.diagnostics.push_back(Diagnostic {
        .ruleId = QStringLiteral("OCS-PHASE1"),
        .severity = Severity::Info,
        .filePath = displayPath,
        .line = 1,
        .column = 1,
        .message = QStringLiteral("Phase 1 application shell is wired successfully. Static analysis rules land in Phase 2 and Phase 3."),
        .remediationHint = QStringLiteral("Use this screen to validate project loading, diagnostics rendering, and settings persistence.")
    });
    if (QFile::exists(projectDirectory.filePath(QStringLiteral("compile_commands.json")))) {
        result.notes << QStringLiteral("Found compile_commands.json for future parser integration.");
    } else {
        result.diagnostics.push_back(Diagnostic {
            .ruleId = QStringLiteral("OCS-CONFIG"),
            .severity = Severity::Warning,
            .filePath = request.projectRootPath,
            .line = 0,
            .column = 0,
            .message = QStringLiteral("No compile_commands.json was found at the project root. Parser integration may need manual include-path configuration later."),
            .remediationHint = QStringLiteral("Generate compile_commands.json from your build system or provide include paths in scan settings.")
        });
    }
    if (representativeSource.isEmpty()) {
        result.diagnostics.push_back(Diagnostic {
            .ruleId = QStringLiteral("OCS-EMPTY"),
            .severity = Severity::Info,
            .filePath = request.projectRootPath,
            .line = 0,
            .column = 0,
            .message = QStringLiteral("No representative C/C++ source file was found yet. The project shell still loaded successfully."),
            .remediationHint = QStringLiteral("Select a source tree containing C or C++ files when validating upcoming analyzer phases.")
        });
    }
    return result;
}
} // namespace opencodescan
