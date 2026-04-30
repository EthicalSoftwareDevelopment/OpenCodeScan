#pragma once
#include <QAbstractTableModel>
#include "core/IDiagnosticsModel.hpp"
class DiagnosticsTableModel final : public QAbstractTableModel, public opencodescan::IDiagnosticsModel {
public:
    explicit DiagnosticsTableModel(QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void setDiagnostics(const QVector<opencodescan::Diagnostic>& diagnostics) override;
    void clearDiagnostics() override;
    [[nodiscard]] QVector<opencodescan::Diagnostic> diagnostics() const override;
    [[nodiscard]] opencodescan::Diagnostic diagnosticAt(int row) const;
private:
    QVector<opencodescan::Diagnostic> diagnostics_;
};
