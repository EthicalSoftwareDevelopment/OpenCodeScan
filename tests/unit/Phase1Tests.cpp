#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "analysis/MockAnalysisEngine.hpp"
#include "core/Diagnostic.hpp"
#include "core/SettingsService.hpp"
#include "ui/DiagnosticsTableModel.hpp"
namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
void requireEqual(const QString& actual, const QString& expected, const char* message) {
    if (actual != expected) {
        throw std::runtime_error(std::string(message) + " | expected='" + expected.toStdString() + "' actual='" + actual.toStdString() + "'");
    }
}
void requireEqual(const int actual, const int expected, const char* message) {
    if (actual != expected) {
        throw std::runtime_error(std::string(message) + " | expected=" + std::to_string(expected) + " actual=" + std::to_string(actual));
    }
}
void testSeverityDisplayString() {
    using opencodescan::Severity;
    using opencodescan::toDisplayString;
    requireEqual(toDisplayString(Severity::Info), QStringLiteral("Info"), "Severity::Info should map to Info");
    requireEqual(toDisplayString(Severity::Warning), QStringLiteral("Warning"), "Severity::Warning should map to Warning");
    requireEqual(toDisplayString(Severity::Error), QStringLiteral("Error"), "Severity::Error should map to Error");
    requireEqual(toDisplayString(Severity::Critical), QStringLiteral("Critical"), "Severity::Critical should map to Critical");
}
void testMockAnalysisEngineRejectsEmptyPath() {
    opencodescan::MockAnalysisEngine engine;
    const auto result = engine.analyzeProject({});
    require(result.diagnostics.isEmpty(), "Empty project path should not produce diagnostics");
    requireEqual(result.notes.value(0), QStringLiteral("No project path selected."), "Empty project path should produce the expected note");
}
void testMockAnalysisEngineRejectsMissingDirectory() {
    opencodescan::MockAnalysisEngine engine;
    opencodescan::AnalysisRequest request;
    request.projectRootPath = QStringLiteral("C:/__opencodescan_missing_project__");
    const auto result = engine.analyzeProject(request);
    require(result.diagnostics.isEmpty(), "Missing project path should not produce diagnostics");
    requireEqual(result.notes.value(0), QStringLiteral("Selected project path does not exist."), "Missing project path should produce the expected note");
}
void testMockAnalysisEngineFindsSourceAndWarnsWhenCompileCommandsMissing() {
    QTemporaryDir projectDir;
    require(projectDir.isValid(), "Temporary directory for project fixture should be created");
    QFile sourceFile(projectDir.filePath(QStringLiteral("main.cpp")));
    require(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text), "Source fixture should be writable");
    sourceFile.write("int main() { return 0; }\n");
    sourceFile.close();
    opencodescan::MockAnalysisEngine engine;
    opencodescan::AnalysisRequest request;
    request.projectRootPath = projectDir.path();
    const auto result = engine.analyzeProject(request);
    requireEqual(result.diagnostics.size(), 2, "Source project without compile_commands.json should produce two diagnostics");
    requireEqual(result.diagnostics.at(0).ruleId, QStringLiteral("OCS-PHASE1"), "First diagnostic should be the Phase 1 shell marker");
    requireEqual(QDir::cleanPath(result.diagnostics.at(0).filePath), QDir::cleanPath(sourceFile.fileName()), "Phase 1 diagnostic should point at the representative source file");
    requireEqual(result.diagnostics.at(1).ruleId, QStringLiteral("OCS-CONFIG"), "Second diagnostic should warn about missing compile_commands.json");
    require(result.notes.value(0).contains(QStringLiteral("Loaded project root:")), "Project scan note should describe the loaded root");
}
void testMockAnalysisEngineHonorsCompileCommandsAndEmptySourceTree() {
    QTemporaryDir projectDir;
    require(projectDir.isValid(), "Temporary directory for compile_commands fixture should be created");
    QFile compileCommands(projectDir.filePath(QStringLiteral("compile_commands.json")));
    require(compileCommands.open(QIODevice::WriteOnly | QIODevice::Text), "compile_commands.json fixture should be writable");
    compileCommands.write("[]\n");
    compileCommands.close();
    opencodescan::MockAnalysisEngine engine;
    opencodescan::AnalysisRequest request;
    request.projectRootPath = projectDir.path();
    const auto result = engine.analyzeProject(request);
    requireEqual(result.diagnostics.size(), 2, "Empty source tree with compile_commands.json should produce two diagnostics");
    requireEqual(result.diagnostics.at(0).ruleId, QStringLiteral("OCS-PHASE1"), "First diagnostic should still be the Phase 1 shell marker");
    requireEqual(result.diagnostics.at(1).ruleId, QStringLiteral("OCS-EMPTY"), "Second diagnostic should note the empty source tree");
    require(result.notes.contains(QStringLiteral("Found compile_commands.json for future parser integration.")), "Scan notes should acknowledge compile_commands.json");
}
void testSettingsServicePersistsNormalizedProjectPath() {
    const auto organizationName = QStringLiteral("OpenCodeScanTests");
    const auto applicationName = QStringLiteral("SettingsServicePhase1Test");
    QSettings cleanup(organizationName, applicationName);
    cleanup.clear();
    cleanup.sync();
    opencodescan::SettingsService writer(organizationName, applicationName);
    writer.setLastProjectPath(QStringLiteral("C:/Temp/../Temp/OpenCodeScanProject"));
    opencodescan::SettingsService reader(organizationName, applicationName);
    requireEqual(reader.lastProjectPath(), QStringLiteral("C:/Temp/OpenCodeScanProject"), "SettingsService should persist a normalized project path");
    cleanup.clear();
    cleanup.sync();
}
void testDiagnosticsTableModelExposesDiagnosticData() {
    DiagnosticsTableModel model;
    opencodescan::Diagnostic diagnostic;
    diagnostic.ruleId = QStringLiteral("OCS-TST");
    diagnostic.severity = opencodescan::Severity::Warning;
    diagnostic.filePath = QStringLiteral("src/example.cpp");
    diagnostic.line = 42;
    diagnostic.column = 7;
    diagnostic.message = QStringLiteral("Example warning");
    diagnostic.remediationHint = QStringLiteral("Rename the variable.");
    model.setDiagnostics({diagnostic});
    requireEqual(model.rowCount(), 1, "Model row count should match inserted diagnostics");
    requireEqual(model.columnCount(), 5, "Model column count should remain fixed");
    requireEqual(model.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("Severity"), "Header column 0 should be Severity");
    requireEqual(model.data(model.index(0, 0), Qt::DisplayRole).toString(), QStringLiteral("Warning"), "Column 0 should display severity text");
    requireEqual(model.data(model.index(0, 1), Qt::DisplayRole).toString(), QStringLiteral("OCS-TST"), "Column 1 should display the rule id");
    requireEqual(model.data(model.index(0, 2), Qt::DisplayRole).toString(), QStringLiteral("src/example.cpp"), "Column 2 should display the file path");
    requireEqual(model.data(model.index(0, 3), Qt::DisplayRole).toInt(), 42, "Column 3 should display the line number");
    requireEqual(model.data(model.index(0, 4), Qt::DisplayRole).toString(), QStringLiteral("Example warning"), "Column 4 should display the message");
    requireEqual(model.data(model.index(0, 0), Qt::ToolTipRole).toString(), QStringLiteral("Rename the variable."), "Tooltip should expose the remediation hint");
    requireEqual(model.diagnosticAt(0).column, 7, "diagnosticAt should return the original diagnostic payload");
    model.clearDiagnostics();
    requireEqual(model.rowCount(), 0, "clearDiagnostics should remove all rows");
    require(model.diagnosticAt(0).ruleId.isEmpty(), "diagnosticAt for an invalid row should return a default diagnostic");
}
} // namespace
int main(int argc, char* argv[]) {
    QCoreApplication application(argc, argv);
    const std::vector<std::pair<const char*, std::function<void()>>> tests {
        {"Severity display strings", testSeverityDisplayString},
        {"Mock analysis engine rejects empty path", testMockAnalysisEngineRejectsEmptyPath},
        {"Mock analysis engine rejects missing directory", testMockAnalysisEngineRejectsMissingDirectory},
        {"Mock analysis engine warns when compile commands are missing", testMockAnalysisEngineFindsSourceAndWarnsWhenCompileCommandsMissing},
        {"Mock analysis engine handles compile commands and empty source tree", testMockAnalysisEngineHonorsCompileCommandsAndEmptySourceTree},
        {"Settings service persists normalized project path", testSettingsServicePersistsNormalizedProjectPath},
        {"Diagnostics table model exposes diagnostic data", testDiagnosticsTableModelExposesDiagnosticData}
    };
    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& exception) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << exception.what() << '\n';
        }
    }
    if (failures != 0) {
        std::cerr << failures << " test(s) failed." << std::endl;
        return 1;
    }
    std::cout << "All Phase 1 unit tests passed." << std::endl;
    return 0;
}
