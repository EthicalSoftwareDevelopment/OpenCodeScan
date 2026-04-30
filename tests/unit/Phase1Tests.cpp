#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "analysis/CompileCommandsLoader.hpp"
#include "analysis/DeterministicAnalysisEngine.hpp"
#include "analysis/ProjectFileDiscoverer.hpp"
#include "analysis/rules/RuleRegistry.hpp"
#include "core/Diagnostic.hpp"
#include "core/SettingsService.hpp"
#include "reporting/HtmlReportExporter.hpp"
#include "reporting/JsonReportExporter.hpp"
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

bool containsRuleId(const QVector<opencodescan::Diagnostic>& diagnostics, const QString& ruleId) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.ruleId == ruleId) {
            return true;
        }
    }
    return false;
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

void testRuleRegistryExposesExpectedBuiltins() {
    opencodescan::RuleRegistry registry;
    const auto availableRules = registry.availableRules();
    const auto defaultRuleIds = registry.defaultEnabledRuleIds();

    require(availableRules.size() >= 6, "Phase 3 should expose a non-trivial built-in rule set");
    require(defaultRuleIds.contains(QStringLiteral("OCS-RULE-USING-NAMESPACE-STD")), "using namespace std should be enabled by default");
    require(defaultRuleIds.contains(QStringLiteral("OCS-RULE-RAW-MEMORY")), "raw memory rule should be enabled by default");
}

void testDeterministicAnalysisEngineRunsRulesAndProducesDiagnostics() {
    QTemporaryDir projectDir;
    require(projectDir.isValid(), "Engine fixture directory should exist");

    createTextFile(projectDir.filePath(QStringLiteral("sample.hpp")), "int add(int left, int right);\n");
    createTextFile(projectDir.filePath(QStringLiteral("sample.cpp")),
                   "#include <iostream>\n"
                   "// TODO: remove this\n"
                   "using namespace std;\n"
                   "int add(int left, int right) {\n"
                   "    int* value = new int(NULL);\n"
                   "    cout << std::endl;\n"
                   "    delete value;\n"
                   "    return left + right;\n"
                   "}\n");
    createTextFile(projectDir.filePath(QStringLiteral("compile_commands.json")),
                   "[{\"directory\":\"" + projectDir.path().toUtf8() + "\",\"file\":\"sample.cpp\",\"arguments\":[\"clang++\",\"-Iinclude\",\"-DDEBUG\",\"sample.cpp\"]}]\n");

    opencodescan::DeterministicAnalysisEngine engine;
    opencodescan::AnalysisRequest request;
    request.projectRootPath = projectDir.path();

    std::vector<opencodescan::ScanProgress> progressEvents;
    const auto result = engine.analyzeProject(request,
                                              [&progressEvents](const opencodescan::ScanProgress& progress) {
                                                  progressEvents.push_back(progress);
                                              });

    requireEqual(result.summary.discoveredFileCount, 2, "Engine should discover both source and header files");
    requireEqual(result.summary.translationUnitCount, 2, "Engine should prepare a translation unit for each discovered file");
    require(result.summary.executedRuleCount >= 5, "Engine should execute the default Phase 3 rule set");
    require(result.summary.emittedRuleDiagnostics >= 5, "Engine should emit diagnostics from the built-in rules");
    require(containsRuleId(result.diagnostics, QStringLiteral("OCS-RULE-TODO")), "TODO rule should emit a diagnostic");
    require(containsRuleId(result.diagnostics, QStringLiteral("OCS-RULE-USING-NAMESPACE-STD")), "using namespace std rule should emit a diagnostic");
    require(containsRuleId(result.diagnostics, QStringLiteral("OCS-RULE-NULL-LITERAL")), "NULL rule should emit a diagnostic");
    require(containsRuleId(result.diagnostics, QStringLiteral("OCS-RULE-RAW-MEMORY")), "raw memory rule should emit a diagnostic");
    require(containsRuleId(result.diagnostics, QStringLiteral("OCS-RULE-HEADER-GUARD")), "header guard rule should emit a diagnostic");
    require(containsRuleId(result.diagnostics, QStringLiteral("OCS-RULE-STD-ENDL")), "std::endl rule should emit a diagnostic");
    require(progressEvents.size() >= 4, "Engine should report progress through file discovery, preparation, and rule execution");
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

void testSettingsServicePersistsPhaseThreeSettings() {
    const auto organizationName = QStringLiteral("OpenCodeScanTests");
    const auto applicationName = QStringLiteral("SettingsServicePhase3Test");

    QSettings cleanup(organizationName, applicationName);
    cleanup.clear();
    cleanup.sync();

    opencodescan::SettingsService writer(organizationName, applicationName);
    writer.setLastProjectPath(QStringLiteral("C:/Temp/OpenCodeScanProject"));
    writer.setScanIncludePaths({QStringLiteral("include"), QStringLiteral("include")});
    writer.setScanDefines({QStringLiteral("DEBUG"), QStringLiteral("DEBUG")});
    writer.setExcludedPaths({QStringLiteral("build"), QStringLiteral("build")});
    writer.setEnabledRuleIds({QStringLiteral("OCS-RULE-TODO"), QStringLiteral("OCS-RULE-TODO")});

    opencodescan::SettingsService reader(organizationName, applicationName);
    requireEqual(reader.lastProjectPath(), QStringLiteral("C:/Temp/OpenCodeScanProject"), "SettingsService should persist the last project path");
    requireEqual(reader.scanIncludePaths().size(), 1, "SettingsService should deduplicate include paths");
    requireEqual(reader.scanDefines().size(), 1, "SettingsService should deduplicate defines");
    requireEqual(reader.excludedPaths().size(), 1, "SettingsService should deduplicate excluded paths");
    requireEqual(reader.enabledRuleIds().size(), 1, "SettingsService should persist enabled rule selections");

    cleanup.clear();
    cleanup.sync();
}

void testReportExportersWriteJsonAndHtml() {
    QTemporaryDir reportDir;
    require(reportDir.isValid(), "Report fixture directory should exist");

    opencodescan::AnalysisResult result;
    result.projectRootPath = QStringLiteral("C:/Dev/OpenCodeScan");
    result.generatedAtUtc = QDateTime::currentDateTimeUtc();
    result.enabledRuleIds = {QStringLiteral("OCS-RULE-TODO")};
    result.availableRules = {{QStringLiteral("OCS-RULE-TODO"),
                              QStringLiteral("TODO / FIXME comments"),
                              QStringLiteral("Flags TODO comments"),
                              opencodescan::RuleCategory::Maintainability,
                              opencodescan::Severity::Info,
                              true}};
    result.summary.discoveredFileCount = 1;
    result.summary.translationUnitCount = 1;
    result.summary.executedRuleCount = 1;
    result.summary.emittedRuleDiagnostics = 1;
    result.notes = {QStringLiteral("Sample note")};
    result.diagnostics = {{QStringLiteral("OCS-RULE-TODO"),
                           opencodescan::Severity::Info,
                           QStringLiteral("sample.cpp"),
                           3,
                           1,
                           QStringLiteral("Found TODO/FIXME marker in source."),
                           QStringLiteral("Resolve the task.")}};

    const auto jsonPath = reportDir.filePath(QStringLiteral("report.json"));
    const auto htmlPath = reportDir.filePath(QStringLiteral("report.html"));

    QString errorMessage;
    require(opencodescan::JsonReportExporter {}.exportReport(result, jsonPath, &errorMessage), "JSON exporter should succeed");
    require(opencodescan::HtmlReportExporter {}.exportReport(result, htmlPath, &errorMessage), "HTML exporter should succeed");

    QFile jsonFile(jsonPath);
    require(jsonFile.open(QIODevice::ReadOnly | QIODevice::Text), "JSON report should be readable");
    const auto jsonContents = QString::fromUtf8(jsonFile.readAll());
    require(jsonContents.contains(QStringLiteral("OCS-RULE-TODO")), "JSON report should contain the rule ID");

    QFile htmlFile(htmlPath);
    require(htmlFile.open(QIODevice::ReadOnly | QIODevice::Text), "HTML report should be readable");
    const auto htmlContents = QString::fromUtf8(htmlFile.readAll());
    require(htmlContents.contains(QStringLiteral("OpenCodeScan Report")), "HTML report should contain the report title");
    require(htmlContents.contains(QStringLiteral("sample.cpp")), "HTML report should contain the diagnostic file path");
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
        {"Rule registry exposes expected builtins", testRuleRegistryExposesExpectedBuiltins},
        {"Deterministic analysis engine runs rules", testDeterministicAnalysisEngineRunsRulesAndProducesDiagnostics},
        {"Deterministic analysis engine supports cancellation", testDeterministicAnalysisEngineSupportsCancellation},
        {"Settings service persists phase three settings", testSettingsServicePersistsPhaseThreeSettings},
        {"Report exporters write JSON and HTML", testReportExportersWriteJsonAndHtml},
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
