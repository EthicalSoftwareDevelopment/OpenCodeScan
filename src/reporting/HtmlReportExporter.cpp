#include "reporting/HtmlReportExporter.hpp"
#include <QDir>
#include <QFile>
#include <QTextStream>
namespace opencodescan {
namespace {
QString htmlEscape(const QString& value) {
    QString escaped = value;
    escaped.replace(QStringLiteral("&"), QStringLiteral("&amp;"));
    escaped.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
    escaped.replace(QStringLiteral(">"), QStringLiteral("&gt;"));
    escaped.replace(QStringLiteral("\""), QStringLiteral("&quot;"));
    return escaped;
}
} // namespace
bool HtmlReportExporter::exportReport(const AnalysisResult& result,
                                      const QString& targetFilePath,
                                      QString* errorMessage) const {
    QFile file(targetFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Unable to open report file for writing: %1").arg(QDir::cleanPath(targetFilePath));
        }
        return false;
    }
    QTextStream stream(&file);
    stream << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
           << "<title>OpenCodeScan Report</title>\n"
           << "<style>body{font-family:Segoe UI,Arial,sans-serif;margin:24px;}"
           << "table{border-collapse:collapse;width:100%;margin-top:16px;}"
           << "th,td{border:1px solid #d0d0d0;padding:8px;text-align:left;vertical-align:top;}"
           << "th{background:#f3f3f3;} .meta{margin-bottom:16px;} .note{margin:4px 0;}"
           << "</style>\n</head>\n<body>\n";
    stream << "<h1>OpenCodeScan Report</h1>\n";
    stream << "<div class=\"meta\"><strong>Project root:</strong> " << htmlEscape(result.projectRootPath)
           << "<br><strong>Generated (UTC):</strong> " << htmlEscape(result.generatedAtUtc.toString(Qt::ISODate))
           << "<br><strong>Translation units:</strong> " << result.summary.translationUnitCount
           << "<br><strong>Executed rules:</strong> " << result.summary.executedRuleCount
           << "<br><strong>Rule diagnostics:</strong> " << result.summary.emittedRuleDiagnostics
           << "</div>\n";
    stream << "<h2>Enabled Rules</h2><ul>\n";
    for (const auto& ruleId : result.enabledRuleIds) {
        stream << "<li>" << htmlEscape(ruleId) << "</li>\n";
    }
    stream << "</ul>\n";
    stream << "<h2>Notes</h2>\n";
    for (const auto& note : result.notes) {
        stream << "<div class=\"note\">" << htmlEscape(note) << "</div>\n";
    }
    stream << "<h2>Diagnostics</h2>\n<table>\n<thead><tr><th>Severity</th><th>Rule ID</th><th>File</th><th>Line</th><th>Message</th><th>Hint</th></tr></thead>\n<tbody>\n";
    for (const auto& diagnostic : result.diagnostics) {
        stream << "<tr><td>" << htmlEscape(toDisplayString(diagnostic.severity))
               << "</td><td>" << htmlEscape(diagnostic.ruleId)
               << "</td><td>" << htmlEscape(diagnostic.filePath)
               << "</td><td>" << diagnostic.line
               << "</td><td>" << htmlEscape(diagnostic.message)
               << "</td><td>" << htmlEscape(diagnostic.remediationHint)
               << "</td></tr>\n";
    }
    stream << "</tbody>\n</table>\n</body>\n</html>\n";
    return true;
}
} // namespace opencodescan
