#pragma once
#include <QAbstractTableModel>
#include "core/Diagnostic.hpp"
class DiagnosticsTableModel final : public QAbstractTableModel {
public:
    explicit DiagnosticsTableModel(QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void setDiagnostics(const QVector<opencodescan::Diagnostic>& diagnostics);
    void clearDiagnostics();
    [[nodiscard]] QVector<opencodescan::Diagnostic> diagnostics() const;
    [[nodiscard]] opencodescan::Diagnostic diagnosticAt(int row) const;
private:
    QVector<opencodescan::Diagnostic> diagnostics_;
};
