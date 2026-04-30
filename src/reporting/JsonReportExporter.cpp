#include "reporting/JsonReportExporter.hpp"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
namespace opencodescan {
bool JsonReportExporter::exportReport(const AnalysisResult& result,
                                      const QString& targetFilePath,
                                      QString* errorMessage) const {
    QFile file(targetFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Unable to open report file for writing: %1").arg(QDir::cleanPath(targetFilePath));
        }
        return false;
    }
    QJsonArray diagnosticsArray;
    for (const auto& diagnostic : result.diagnostics) {
        QJsonObject diagnosticObject;
        diagnosticObject.insert(QStringLiteral("ruleId"), diagnostic.ruleId);
        diagnosticObject.insert(QStringLiteral("severity"), toDisplayString(diagnostic.severity));
        diagnosticObject.insert(QStringLiteral("filePath"), diagnostic.filePath);
        diagnosticObject.insert(QStringLiteral("line"), diagnostic.line);
        diagnosticObject.insert(QStringLiteral("column"), diagnostic.column);
        diagnosticObject.insert(QStringLiteral("message"), diagnostic.message);
        diagnosticObject.insert(QStringLiteral("remediationHint"), diagnostic.remediationHint);
        diagnosticsArray.push_back(diagnosticObject);
    }
    QJsonArray enabledRulesArray;
    for (const auto& enabledRuleId : result.enabledRuleIds) {
        enabledRulesArray.push_back(enabledRuleId);
    }
    QJsonArray availableRulesArray;
    for (const auto& rule : result.availableRules) {
        QJsonObject ruleObject;
        ruleObject.insert(QStringLiteral("id"), rule.id);
        ruleObject.insert(QStringLiteral("name"), rule.name);
        ruleObject.insert(QStringLiteral("description"), rule.description);
        ruleObject.insert(QStringLiteral("category"), toDisplayString(rule.category));
        ruleObject.insert(QStringLiteral("defaultSeverity"), toDisplayString(rule.defaultSeverity));
        ruleObject.insert(QStringLiteral("enabledByDefault"), rule.enabledByDefault);
        availableRulesArray.push_back(ruleObject);
    }
    QJsonArray notesArray;
    for (const auto& note : result.notes) {
        notesArray.push_back(note);
    }
    QJsonObject summaryObject;
    summaryObject.insert(QStringLiteral("discoveredFileCount"), result.summary.discoveredFileCount);
    summaryObject.insert(QStringLiteral("translationUnitCount"), result.summary.translationUnitCount);
    summaryObject.insert(QStringLiteral("compileCommandEntryCount"), result.summary.compileCommandEntryCount);
    summaryObject.insert(QStringLiteral("filesWithoutCompileCommands"), result.summary.filesWithoutCompileCommands);
    summaryObject.insert(QStringLiteral("executedRuleCount"), result.summary.executedRuleCount);
    summaryObject.insert(QStringLiteral("emittedRuleDiagnostics"), result.summary.emittedRuleDiagnostics);
    summaryObject.insert(QStringLiteral("cancelled"), result.summary.cancelled);
    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("projectRootPath"), result.projectRootPath);
    rootObject.insert(QStringLiteral("generatedAtUtc"), result.generatedAtUtc.toString(Qt::ISODate));
    rootObject.insert(QStringLiteral("enabledRuleIds"), enabledRulesArray);
    rootObject.insert(QStringLiteral("availableRules"), availableRulesArray);
    rootObject.insert(QStringLiteral("summary"), summaryObject);
    rootObject.insert(QStringLiteral("notes"), notesArray);
    rootObject.insert(QStringLiteral("diagnostics"), diagnosticsArray);
    file.write(QJsonDocument(rootObject).toJson(QJsonDocument::Indented));
    return true;
}
} // namespace opencodescan
