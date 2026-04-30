#pragma once
#include <QFutureWatcher>
#include <QMainWindow>
#include "core/Diagnostic.hpp"
namespace opencodescan {
class IAnalysisEngine;
class SettingsService;
}
class DiagnosticsTableModel;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTableView;
class QTextEdit;
class QModelIndex;
class MainWindow final : public QMainWindow {
public:
    MainWindow(opencodescan::SettingsService& settingsService,
               opencodescan::IAnalysisEngine& analysisEngine,
               QWidget* parent = nullptr);
    ~MainWindow() override;
private:
    void setupUi();
    void restoreSettings();
    void persistSettings();
    void browseForProject();
    void startScan();
    void cancelScan();
    void handleScanFinished();
    void handleScanProgress(const opencodescan::ScanProgress& progress);
    void showAnalysisResult(const opencodescan::AnalysisResult& result);
    void updateDiagnosticDetails(const QModelIndex& current, const QModelIndex& previous);
    [[nodiscard]] opencodescan::AnalysisRequest buildRequestFromUi() const;
    [[nodiscard]] QStringList parseMultiValue(const QString& text) const;
    [[nodiscard]] QString formatSummaryText(const opencodescan::AnalysisResult& result) const;
    opencodescan::SettingsService& settingsService_;
    opencodescan::IAnalysisEngine& analysisEngine_;
    QLineEdit* projectPathEdit_ {nullptr};
    QLineEdit* includePathsEdit_ {nullptr};
    QLineEdit* definesEdit_ {nullptr};
    QLineEdit* excludedPathsEdit_ {nullptr};
    QPushButton* browseButton_ {nullptr};
    QPushButton* startScanButton_ {nullptr};
    QPushButton* cancelScanButton_ {nullptr};
    QLabel* summaryLabel_ {nullptr};
    QLabel* progressLabel_ {nullptr};
    QProgressBar* progressBar_ {nullptr};
    QTableView* diagnosticsView_ {nullptr};
    QTextEdit* detailsView_ {nullptr};
    DiagnosticsTableModel* diagnosticsModel_ {nullptr};
    QFutureWatcher<opencodescan::AnalysisResult>* scanWatcher_ {nullptr};
    opencodescan::CancelToken cancelToken_;
    bool scanInProgress_ {false};
};
