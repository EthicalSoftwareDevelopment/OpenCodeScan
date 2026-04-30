#include "ui/DiagnosticsTableModel.hpp"
#include <QBrush>
#include <QColor>
namespace {
QColor severityColor(const opencodescan::Severity severity) {
    switch (severity) {
    case opencodescan::Severity::Info:
        return QColor(41, 128, 185);
    case opencodescan::Severity::Warning:
        return QColor(211, 84, 0);
    case opencodescan::Severity::Error:
        return QColor(192, 57, 43);
    case opencodescan::Severity::Critical:
        return QColor(142, 68, 173);
    }
    return QColor(80, 80, 80);
}
} // namespace
DiagnosticsTableModel::DiagnosticsTableModel(QObject* parent)
    : QAbstractTableModel(parent) {
}
int DiagnosticsTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : diagnostics_.size();
}
int DiagnosticsTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 5;
}
QVariant DiagnosticsTableModel::data(const QModelIndex& index, const int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= diagnostics_.size()) {
        return {};
    }
    const auto& diagnostic = diagnostics_.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return opencodescan::toDisplayString(diagnostic.severity);
        case 1:
            return diagnostic.ruleId;
        case 2:
            return diagnostic.filePath;
        case 3:
            return diagnostic.line > 0 ? QVariant(diagnostic.line) : QVariant(QStringLiteral("-"));
        case 4:
            return diagnostic.message;
        default:
            return {};
        }
    }
    if (role == Qt::ToolTipRole) {
        return diagnostic.remediationHint;
    }
    if (role == Qt::ForegroundRole && index.column() == 0) {
        return QBrush(severityColor(diagnostic.severity));
    }
    if (role == Qt::TextAlignmentRole && index.column() == 3) {
        return Qt::AlignCenter;
    }
    return {};
}
QVariant DiagnosticsTableModel::headerData(const int section, const Qt::Orientation orientation, const int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    switch (section) {
    case 0:
        return QStringLiteral("Severity");
    case 1:
        return QStringLiteral("Rule ID");
    case 2:
        return QStringLiteral("File / Path");
    case 3:
        return QStringLiteral("Line");
    case 4:
        return QStringLiteral("Message");
    default:
        return {};
    }
}
void DiagnosticsTableModel::setDiagnostics(const QVector<opencodescan::Diagnostic>& diagnostics) {
    beginResetModel();
    diagnostics_ = diagnostics;
    endResetModel();
}
void DiagnosticsTableModel::clearDiagnostics() {
    beginResetModel();
    diagnostics_.clear();
    endResetModel();
}
QVector<opencodescan::Diagnostic> DiagnosticsTableModel::diagnostics() const {
    return diagnostics_;
}
opencodescan::Diagnostic DiagnosticsTableModel::diagnosticAt(const int row) const {
    if (row < 0 || row >= diagnostics_.size()) {
        return {};
    }
    return diagnostics_.at(row);
}
