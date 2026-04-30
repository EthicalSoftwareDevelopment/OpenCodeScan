#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "analysis/CompileCommandsLoader.hpp"
#include "analysis/DeterministicAnalysisEngine.hpp"
#include "analysis/ProjectFileDiscoverer.hpp"
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
void requireEqual(int actual, int expected, const char* message) {
    if (actual != expected) {
        throw std::runtime_error(std::string(message) + " | expected=" + std::to_string(expected) + " actual=" + std::to_string(actual));
    }
}
QString createTextFile(const QString& path, const QByteArray& contents) {
    QFile file(path);
    require(file.open(QIODevice::WriteOnly | QIODevice::Text), "Fixture file should be writable");
    file.write(contents);
    file.close();
    return path;
}
void testSeverityDisplayString() {
    using opencodescan::Severity;
    using opencodescan::toDisplayString;
    requireEqual(toDisplayString(Severity::Info), QStringLiteral("Info"), "Severity::Info should map to Info");
    requireEqual(toDisplayString(Severity::Warning), QStringLiteral("Warning"), "Severity::Warning should map to Warning");
    requireEqual(toDisplayString(Severity::Error), QStringLiteral("Error"), "Severity::Error should map to Error");
    requireEqual(toDisplayString(Severity::Critical), QStringLiteral("Critical"), "Severity::Critical should map to Critical");
}
void testProjectFileDiscovererFindsSortedSourceFiles() {
    QTemporaryDir projectDir;
    require(projectDir.isValid(), "Project fixture directory should exist");
    createTextFile(projectDir.filePath(QStringLiteral("zeta.cpp")), "int zeta();\n");
    createTextFile(projectDir.filePath(QStringLiteral("Alpha.hpp")), "#pragma once\n");
    QDir().mkpath(projectDir.filePath(QStringLiteral("ignored/sub")));
    createTextFile(projectDir.filePath(QStringLiteral("ignored/sub/skip.cpp")), "int skip();\n");
    opencodescan::AnalysisRequest request;
    request.projectRootPath = projectDir.path();
    request.excludedPaths = {QStringLiteral("ignored")};
    opencodescan::ProjectFileDiscoverer discoverer;
    const auto files = discoverer.discover(request);
    requireEqual(files.size(), 2, "Discoverer should skip excluded paths");
    require(files.at(0).endsWith(QStringLiteral("Alpha.hpp")), "Files should be returned in deterministic sorted order");
    require(files.at(1).endsWith(QStringLiteral("zeta.cpp")), "Files should include remaining source files");
}
void testCompileCommandsLoaderParsesArguments() {
    QTemporaryDir projectDir;
    require(projectDir.isValid(), "compile_commands fixture directory should exist");
    createTextFile(projectDir.filePath(QStringLiteral("main.cpp")), "int main() { return 0; }\n");
    createTextFile(projectDir.filePath(QStringLiteral("compile_commands.json")),
                   "[{\"directory\":\"" + projectDir.path().toUtf8() + "\",\"file\":\"main.cpp\",\"arguments\":[\"clang++\",\"-Iinclude\",\"-DDEBUG\",\"main.cpp\"]}]\n");
    opencodescan::CompileCommandsLoader loader;
    const auto result = loader.load(projectDir.path());
    const auto filePath = QDir(projectDir.path()).filePath(QStringLiteral("main.cpp"));
    require(!result.compileCommandsPath.isEmpty(), "Loader should find compile_commands.json");
    require(result.translationUnitsByFile.contains(QDir::cleanPath(filePath)), "Loader should register a translation unit for the source file");
    const auto translationUnit = result.translationUnitsByFile.value(QDir::cleanPath(filePath));
    require(translationUnit.fromCompileCommands, "Translation unit should be marked as originating from compile_commands.json");
    require(translationUnit.includePaths.contains(QDir(projectDir.path()).filePath(QStringLiteral("include"))), "Include path should be normalized relative to the working directory");
    require(translationUnit.defines.contains(QStringLiteral("DEBUG")), "Preprocessor defines should be extracted from compiler arguments");
}
void testDeterministicAnalysisEngineBuildsTranslationUnitsAndWarnings() {
    QTemporaryDir projectDir;
    require(projectDir.isValid(), "Engine fixture directory should exist");
    createTextFile(projectDir.filePath(QStringLiteral("alpha.cpp")), "int alpha() { return 1; }\n");
    createTextFile(projectDir.filePath(QStringLiteral("beta.hpp")), "#pragma once\n");
    createTextFile(projectDir.filePath(QStringLiteral("compile_commands.json")),
                   "[{\"directory\":\"" + projectDir.path().toUtf8() + "\",\"file\":\"alpha.cpp\",\"arguments\":[\"clang++\",\"-Iinclude\",\"-DDEBUG\",\"alpha.cpp\"]}]\n");
    opencodescan::DeterministicAnalysisEngine engine;
    opencodescan::AnalysisRequest request;
    request.projectRootPath = projectDir.path();
    request.includePaths = {QStringLiteral("manual/include")};
    std::vector<opencodescan::ScanProgress> progressEvents;
    const auto result = engine.analyzeProject(request,
                                              [&progressEvents](const opencodescan::ScanProgress& progress) {
                                                  progressEvents.push_back(progress);
                                              });
    requireEqual(result.summary.discoveredFileCount, 2, "Engine should discover both C/C++ files");
    requireEqual(result.summary.translationUnitCount, 2, "Engine should prepare a translation unit for each discovered file");
    requireEqual(result.summary.compileCommandEntryCount, 1, "Engine should record compile_commands.json coverage");
    requireEqual(result.summary.filesWithoutCompileCommands, 1, "Engine should count files without compile command coverage");
    require(!result.translationUnits.isEmpty(), "Engine should return translation units");
    require(result.translationUnits.at(0).fromCompileCommands, "First translation unit should come from compile_commands.json");
    require(result.diagnostics.size() >= 2, "Engine should emit summary diagnostics");
    requireEqual(result.diagnostics.at(0).ruleId, QStringLiteral("OCS-SCAN"), "First diagnostic should summarize the prepared translation units");
    require(progressEvents.size() >= 3, "Engine should report progress through the analysis pipeline");
    require(progressEvents.back().state == opencodescan::ScanState::Completed, "Final progress event should mark the scan as completed");
}
void testDeterministicAnalysisEngineSupportsCancellation() {
    QTemporaryDir projectDir;
    require(projectDir.isValid(), "Cancellation fixture directory should exist");
    createTextFile(projectDir.filePath(QStringLiteral("alpha.cpp")), "int alpha() { return 1; }\n");
    createTextFile(projectDir.filePath(QStringLiteral("beta.cpp")), "int beta() { return 2; }\n");
    opencodescan::DeterministicAnalysisEngine engine;
    opencodescan::AnalysisRequest request;
    request.projectRootPath = projectDir.path();
    auto cancelToken = std::make_shared<std::atomic_bool>(false);
    int progressCount = 0;
    const auto result = engine.analyzeProject(request,
                                              [&cancelToken, &progressCount](const opencodescan::ScanProgress& progress) {
                                                  ++progressCount;
                                                  if (progress.state == opencodescan::ScanState::BuildingTranslationUnits
                                                      && progress.processedFiles == 0) {
                                                      cancelToken->store(true);
                                                  }
                                              },
                                              cancelToken);
    require(result.summary.cancelled, "Cancellation token should stop the deterministic scan");
    require(result.summary.translationUnitCount < 2, "Cancelled scan should stop before processing all files");
    require(progressCount > 0, "Cancellation test should still observe progress events");
}
void testSettingsServicePersistsPhaseTwoSettings() {
    const auto organizationName = QStringLiteral("OpenCodeScanTests");
    const auto applicationName = QStringLiteral("SettingsServicePhase2Test");
    QSettings cleanup(organizationName, applicationName);
    cleanup.clear();
    cleanup.sync();
    opencodescan::SettingsService writer(organizationName, applicationName);
    writer.setLastProjectPath(QStringLiteral("C:/Temp/OpenCodeScanProject"));
    writer.setScanIncludePaths({QStringLiteral("include"), QStringLiteral("include")});
    writer.setScanDefines({QStringLiteral("DEBUG"), QStringLiteral("DEBUG")});
    writer.setExcludedPaths({QStringLiteral("build"), QStringLiteral("build")});
    opencodescan::SettingsService reader(organizationName, applicationName);
    requireEqual(reader.lastProjectPath(), QStringLiteral("C:/Temp/OpenCodeScanProject"), "SettingsService should persist the last project path");
    requireEqual(reader.scanIncludePaths().size(), 1, "SettingsService should deduplicate include paths");
    requireEqual(reader.scanDefines().size(), 1, "SettingsService should deduplicate defines");
    requireEqual(reader.excludedPaths().size(), 1, "SettingsService should deduplicate excluded paths");
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
        {"Project file discoverer finds sorted files", testProjectFileDiscovererFindsSortedSourceFiles},
        {"Compile commands loader parses arguments", testCompileCommandsLoaderParsesArguments},
        {"Deterministic analysis engine builds translation units", testDeterministicAnalysisEngineBuildsTranslationUnitsAndWarnings},
        {"Deterministic analysis engine supports cancellation", testDeterministicAnalysisEngineSupportsCancellation},
        {"Settings service persists phase two settings", testSettingsServicePersistsPhaseTwoSettings},
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
    std::cout << "All OpenCodeScan unit tests passed." << std::endl;
    return 0;
}
