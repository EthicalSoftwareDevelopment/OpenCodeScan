#pragma once
#include <QMainWindow>
#include "core/IAnalysisEngine.hpp"
#include "core/SettingsService.hpp"
class DiagnosticsTableModel;
class QLineEdit;
class QPushButton;
class QTableView;
class QTextEdit;
class QLabel;
class QModelIndex;
class MainWindow final : public QMainWindow {
public:
    MainWindow(opencodescan::SettingsService& settingsService,
               opencodescan::IAnalysisEngine& analysisEngine,
               QWidget* parent = nullptr);
private:
    void setupUi();
    void restoreProjectSelection();
    void browseForProject();
    void loadProject(const QString& projectRootPath);
    void updateDiagnosticDetails(const QModelIndex& current, const QModelIndex& previous);
    void showNotes(const QStringList& notes);
    opencodescan::SettingsService& settingsService_;
    opencodescan::IAnalysisEngine& analysisEngine_;
    QLineEdit* projectPathEdit_ {nullptr};
    QPushButton* browseButton_ {nullptr};
    QPushButton* loadProjectButton_ {nullptr};
    QLabel* summaryLabel_ {nullptr};
    QTableView* diagnosticsView_ {nullptr};
    QTextEdit* detailsView_ {nullptr};
    DiagnosticsTableModel* diagnosticsModel_ {nullptr};
};
