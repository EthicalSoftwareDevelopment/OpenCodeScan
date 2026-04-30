#include "ui/MainWindow.hpp"
#include <QAbstractItemView>
#include <QDir>
#include <QFileDialog>
#include <QFuture>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaObject>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
#include "core/IAnalysisEngine.hpp"
#include "core/Logging.hpp"
#include "core/SettingsService.hpp"
#include "reporting/HtmlReportExporter.hpp"
#include "reporting/JsonReportExporter.hpp"
#include "ui/DiagnosticsTableModel.hpp"
namespace {
QString formatDiagnosticDetails(const opencodescan::Diagnostic& diagnostic) {
    QString details;
    details += QStringLiteral("Rule ID: %1\n").arg(diagnostic.ruleId);
    details += QStringLiteral("Severity: %1\n").arg(opencodescan::toDisplayString(diagnostic.severity));
    details += QStringLiteral("File / Path: %1\n").arg(diagnostic.filePath.isEmpty() ? QStringLiteral("<none>") : diagnostic.filePath);
    details += QStringLiteral("Line: %1\n").arg(diagnostic.line > 0 ? QString::number(diagnostic.line) : QStringLiteral("-"));
    details += QStringLiteral("Column: %1\n\n").arg(diagnostic.column > 0 ? QString::number(diagnostic.column) : QStringLiteral("-"));
    details += QStringLiteral("Message\n-------\n%1\n\n").arg(diagnostic.message);
    details += QStringLiteral("Remediation Hint\n----------------\n%1")
                   .arg(diagnostic.remediationHint.isEmpty() ? QStringLiteral("No hint available yet.") : diagnostic.remediationHint);
    return details;
}
QString formatNotes(const QStringList& notes) {
    if (notes.isEmpty()) {
        return QStringLiteral("No additional notes.");
    }
    return QStringLiteral("Scan Notes\n----------\n%1").arg(notes.join(QStringLiteral("\n")));
}
} // namespace
MainWindow::MainWindow(opencodescan::SettingsService& settingsService,
                       opencodescan::IAnalysisEngine& analysisEngine,
                       QWidget* parent)
    : QMainWindow(parent)
    , settingsService_(settingsService)
    , analysisEngine_(analysisEngine) {
    setupUi();
    restoreSettings();
}
MainWindow::~MainWindow() {
    if (scanWatcher_ != nullptr && scanWatcher_->isRunning()) {
        cancelScan();
        scanWatcher_->waitForFinished();
    }
}
void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("OpenCodeScan - Phase 3 Rules MVP"));
    resize(1320, 860);
    auto* centralWidget = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(centralWidget);
    auto* configurationGroup = new QGroupBox(QStringLiteral("Scan Configuration"), centralWidget);
    auto* configurationLayout = new QGridLayout(configurationGroup);
    projectPathEdit_ = new QLineEdit(configurationGroup);
    includePathsEdit_ = new QLineEdit(configurationGroup);
    definesEdit_ = new QLineEdit(configurationGroup);
    excludedPathsEdit_ = new QLineEdit(configurationGroup);
    enabledRulesEdit_ = new QLineEdit(configurationGroup);
    browseButton_ = new QPushButton(QStringLiteral("Browse..."), configurationGroup);
    startScanButton_ = new QPushButton(QStringLiteral("Start Scan"), configurationGroup);
    cancelScanButton_ = new QPushButton(QStringLiteral("Cancel"), configurationGroup);
    exportJsonButton_ = new QPushButton(QStringLiteral("Export JSON"), configurationGroup);
    exportHtmlButton_ = new QPushButton(QStringLiteral("Export HTML"), configurationGroup);
    cancelScanButton_->setEnabled(false);
    exportJsonButton_->setEnabled(false);
    exportHtmlButton_->setEnabled(false);
    includePathsEdit_->setPlaceholderText(QStringLiteral("Semicolon-separated include paths"));
    definesEdit_->setPlaceholderText(QStringLiteral("Semicolon-separated defines (e.g. DEBUG;WIN32)"));
    excludedPathsEdit_->setPlaceholderText(QStringLiteral("Semicolon-separated excluded paths"));
    enabledRulesEdit_->setPlaceholderText(QStringLiteral("Semicolon-separated rule IDs"));
    enabledRulesEdit_->setToolTip(availableRulesTooltip());
    configurationLayout->addWidget(new QLabel(QStringLiteral("Project root:"), configurationGroup), 0, 0);
    configurationLayout->addWidget(projectPathEdit_, 0, 1);
    configurationLayout->addWidget(browseButton_, 0, 2);
    configurationLayout->addWidget(new QLabel(QStringLiteral("Include paths:"), configurationGroup), 1, 0);
    configurationLayout->addWidget(includePathsEdit_, 1, 1, 1, 2);
    configurationLayout->addWidget(new QLabel(QStringLiteral("Defines:"), configurationGroup), 2, 0);
    configurationLayout->addWidget(definesEdit_, 2, 1, 1, 2);
    configurationLayout->addWidget(new QLabel(QStringLiteral("Excluded paths:"), configurationGroup), 3, 0);
    configurationLayout->addWidget(excludedPathsEdit_, 3, 1, 1, 2);
    configurationLayout->addWidget(new QLabel(QStringLiteral("Enabled rules:"), configurationGroup), 4, 0);
    configurationLayout->addWidget(enabledRulesEdit_, 4, 1, 1, 2);
    configurationLayout->addWidget(startScanButton_, 5, 1);
    configurationLayout->addWidget(cancelScanButton_, 5, 2);
    configurationLayout->addWidget(exportJsonButton_, 6, 1);
    configurationLayout->addWidget(exportHtmlButton_, 6, 2);
    summaryLabel_ = new QLabel(QStringLiteral("Choose a project, select Phase 3 rules, and start a deterministic scan to produce actionable diagnostics."), centralWidget);
    summaryLabel_->setWordWrap(true);
    progressLabel_ = new QLabel(QStringLiteral("Idle"), centralWidget);
    progressBar_ = new QProgressBar(centralWidget);
    progressBar_->setRange(0, 1);
    progressBar_->setValue(0);
    diagnosticsModel_ = new DiagnosticsTableModel(this);
    diagnosticsView_ = new QTableView(centralWidget);
    diagnosticsView_->setModel(diagnosticsModel_);
    diagnosticsView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    diagnosticsView_->setSelectionMode(QAbstractItemView::SingleSelection);
    diagnosticsView_->setAlternatingRowColors(true);
    diagnosticsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    diagnosticsView_->horizontalHeader()->setStretchLastSection(true);
    diagnosticsView_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    diagnosticsView_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    diagnosticsView_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    diagnosticsView_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    detailsView_ = new QTextEdit(centralWidget);
    detailsView_->setReadOnly(true);
    detailsView_->setPlaceholderText(QStringLiteral("Select a diagnostic to inspect details, or run a scan to see scan notes and export the results."));
    auto* splitter = new QSplitter(Qt::Vertical, centralWidget);
    splitter->addWidget(diagnosticsView_);
    splitter->addWidget(detailsView_);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    rootLayout->addWidget(configurationGroup);
    rootLayout->addWidget(summaryLabel_);
    rootLayout->addWidget(progressLabel_);
    rootLayout->addWidget(progressBar_);
    rootLayout->addWidget(splitter, 1);
    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("Ready"));
    scanWatcher_ = new QFutureWatcher<opencodescan::AnalysisResult>(this);
    connect(browseButton_, &QPushButton::clicked, this, [this]() { browseForProject(); });
    connect(startScanButton_, &QPushButton::clicked, this, [this]() { startScan(); });
    connect(cancelScanButton_, &QPushButton::clicked, this, [this]() { cancelScan(); });
    connect(exportJsonButton_, &QPushButton::clicked, this, [this]() { exportJsonReport(); });
    connect(exportHtmlButton_, &QPushButton::clicked, this, [this]() { exportHtmlReport(); });
    connect(scanWatcher_, &QFutureWatcher<opencodescan::AnalysisResult>::finished, this, [this]() { handleScanFinished(); });
    connect(diagnosticsView_->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            [this](const QModelIndex& current, const QModelIndex& previous) {
                updateDiagnosticDetails(current, previous);
            });
}
void MainWindow::restoreSettings() {
    const auto lastProjectPath = settingsService_.lastProjectPath();
    projectPathEdit_->setText(lastProjectPath.isEmpty() ? QDir::currentPath() : lastProjectPath);
    includePathsEdit_->setText(settingsService_.scanIncludePaths().join(QStringLiteral(";")));
    definesEdit_->setText(settingsService_.scanDefines().join(QStringLiteral(";")));
    excludedPathsEdit_->setText(settingsService_.excludedPaths().join(QStringLiteral(";")));
    const auto enabledRuleIds = settingsService_.enabledRuleIds();
    if (enabledRuleIds.isEmpty()) {
        QStringList defaultRuleIds;
        for (const auto& rule : analysisEngine_.availableRules()) {
            if (rule.enabledByDefault) {
                defaultRuleIds << rule.id;
            }
        }
        enabledRulesEdit_->setText(defaultRuleIds.join(QStringLiteral(";")));
    } else {
        enabledRulesEdit_->setText(enabledRuleIds.join(QStringLiteral(";")));
    }
}
void MainWindow::persistSettings() {
    settingsService_.setLastProjectPath(projectPathEdit_->text().trimmed());
    settingsService_.setScanIncludePaths(parseMultiValue(includePathsEdit_->text()));
    settingsService_.setScanDefines(parseMultiValue(definesEdit_->text()));
    settingsService_.setExcludedPaths(parseMultiValue(excludedPathsEdit_->text()));
    settingsService_.setEnabledRuleIds(parseMultiValue(enabledRulesEdit_->text()));
}
void MainWindow::browseForProject() {
    const auto initialDirectory = projectPathEdit_->text().trimmed().isEmpty()
        ? QDir::homePath()
        : projectPathEdit_->text().trimmed();
    const auto selectedPath = QFileDialog::getExistingDirectory(this,
                                                                QStringLiteral("Select project root"),
                                                                initialDirectory,
                                                                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!selectedPath.isEmpty()) {
        projectPathEdit_->setText(QDir::cleanPath(selectedPath));
    }
}
void MainWindow::startScan() {
    const auto projectPath = projectPathEdit_->text().trimmed();
    if (projectPath.isEmpty() || !QDir(projectPath).exists()) {
        QMessageBox::warning(this,
                             QStringLiteral("Invalid project path"),
                             QStringLiteral("Select an existing project directory before starting a scan."));
        return;
    }
    if (scanInProgress_) {
        return;
    }
    persistSettings();
    scanInProgress_ = true;
    startScanButton_->setEnabled(false);
    cancelScanButton_->setEnabled(true);
    exportJsonButton_->setEnabled(false);
    exportHtmlButton_->setEnabled(false);
    progressBar_->setRange(0, 0);
    progressLabel_->setText(QStringLiteral("Preparing scan..."));
    summaryLabel_->setText(QStringLiteral("Preparing Phase 3 rule execution..."));
    diagnosticsModel_->clearDiagnostics();
    detailsView_->clear();
    statusBar()->showMessage(QStringLiteral("Scan started"));
    cancelToken_ = std::make_shared<std::atomic_bool>(false);
    const auto request = buildRequestFromUi();
    opencodescan::Logger::info(QStringLiteral("Starting Phase 3 analysis scan for %1").arg(request.projectRootPath));
    const auto progressCallback = [this](const opencodescan::ScanProgress& progress) {
        QMetaObject::invokeMethod(this,
                                  [this, progress]() { handleScanProgress(progress); },
                                  Qt::QueuedConnection);
    };
    scanWatcher_->setFuture(QtConcurrent::run([this, request, progressCallback, cancelToken = cancelToken_]() {
        return analysisEngine_.analyzeProject(request, progressCallback, cancelToken);
    }));
}
void MainWindow::cancelScan() {
    if (!scanInProgress_ || !cancelToken_) {
        return;
    }
    cancelToken_->store(true);
    progressLabel_->setText(QStringLiteral("Cancellation requested..."));
    statusBar()->showMessage(QStringLiteral("Cancellation requested"));
}
void MainWindow::exportJsonReport() {
    exportReport(false);
}
void MainWindow::exportHtmlReport() {
    exportReport(true);
}
void MainWindow::handleScanFinished() {
    scanInProgress_ = false;
    startScanButton_->setEnabled(true);
    cancelScanButton_->setEnabled(false);
    const auto result = scanWatcher_->result();
    showAnalysisResult(result);
    opencodescan::Logger::info(QStringLiteral("Phase 3 scan finished for %1 with %2 rule diagnostics.")
                                   .arg(projectPathEdit_->text().trimmed())
                                   .arg(result.summary.emittedRuleDiagnostics));
}
void MainWindow::handleScanProgress(const opencodescan::ScanProgress& progress) {
    progressLabel_->setText(QStringLiteral("%1 - %2")
                                .arg(opencodescan::toDisplayString(progress.state))
                                .arg(progress.message));
    if (progress.totalFiles > 0) {
        progressBar_->setRange(0, progress.totalFiles);
        progressBar_->setValue(progress.processedFiles);
    } else {
        progressBar_->setRange(0, 0);
    }
    if (!progress.currentFilePath.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("%1 (%2/%3)")
                                     .arg(progress.currentFilePath)
                                     .arg(progress.processedFiles)
                                     .arg(progress.totalFiles));
    }
}
void MainWindow::showAnalysisResult(const opencodescan::AnalysisResult& result) {
    lastResult_ = result;
    diagnosticsModel_->setDiagnostics(result.diagnostics);
    summaryLabel_->setText(formatSummaryText(result));
    progressBar_->setRange(0, result.summary.discoveredFileCount > 0 ? result.summary.discoveredFileCount : 1);
    progressBar_->setValue(result.summary.translationUnitCount > 0 ? result.summary.translationUnitCount : 0);
    progressLabel_->setText(result.summary.cancelled ? QStringLiteral("Cancelled") : QStringLiteral("Completed"));
    detailsView_->setPlainText(formatNotes(result.notes));
    exportJsonButton_->setEnabled(true);
    exportHtmlButton_->setEnabled(true);
    if (diagnosticsModel_->rowCount() > 0) {
        diagnosticsView_->selectRow(0);
        updateDiagnosticDetails(diagnosticsModel_->index(0, 0), {});
    }
    statusBar()->showMessage(result.summary.cancelled ? QStringLiteral("Scan cancelled") : QStringLiteral("Scan completed"), 5000);
}
void MainWindow::updateDiagnosticDetails(const QModelIndex& current, const QModelIndex&) {
    if (!current.isValid()) {
        return;
    }
    const auto diagnostic = diagnosticsModel_->diagnosticAt(current.row());
    detailsView_->setPlainText(formatDiagnosticDetails(diagnostic));
}
opencodescan::AnalysisRequest MainWindow::buildRequestFromUi() const {
    opencodescan::AnalysisRequest request;
    request.projectRootPath = QDir::cleanPath(projectPathEdit_->text().trimmed());
    request.includePaths = parseMultiValue(includePathsEdit_->text());
    request.defines = parseMultiValue(definesEdit_->text());
    request.excludedPaths = parseMultiValue(excludedPathsEdit_->text());
    request.enabledRuleIds = parseMultiValue(enabledRulesEdit_->text());
    request.preferCompileCommands = true;
    return request;
}
QStringList MainWindow::parseMultiValue(const QString& text) const {
    QStringList values;
    for (const auto& value : text.split(QRegularExpression(QStringLiteral("[;\\n\\r]+")), Qt::SkipEmptyParts)) {
        const auto trimmed = value.trimmed();
        if (!trimmed.isEmpty()) {
            values << trimmed;
        }
    }
    values.removeDuplicates();
    return values;
}
QString MainWindow::formatSummaryText(const opencodescan::AnalysisResult& result) const {
    return QStringLiteral("Project: %1\nDiscovered files: %2 | Translation units: %3 | Executed rules: %4 | Rule diagnostics: %5 | compile_commands entries: %6 | Uncovered files: %7%8")
        .arg(projectPathEdit_->text().trimmed())
        .arg(result.summary.discoveredFileCount)
        .arg(result.summary.translationUnitCount)
        .arg(result.summary.executedRuleCount)
        .arg(result.summary.emittedRuleDiagnostics)
        .arg(result.summary.compileCommandEntryCount)
        .arg(result.summary.filesWithoutCompileCommands)
        .arg(result.summary.cancelled ? QStringLiteral(" | Status: Cancelled") : QStringLiteral(" | Status: Completed"));
}
QString MainWindow::availableRulesTooltip() const {
    QStringList entries;
    for (const auto& rule : analysisEngine_.availableRules()) {
        entries << QStringLiteral("%1 - %2 (%3)").arg(rule.id, rule.name, opencodescan::toDisplayString(rule.category));
    }
    return entries.join(QStringLiteral("\n"));
}
bool MainWindow::exportReport(const bool exportHtml) {
    if (lastResult_.projectRootPath.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("No scan results"),
                                 QStringLiteral("Run a scan before exporting a report."));
        return false;
    }
    const auto defaultExtension = exportHtml ? QStringLiteral("html") : QStringLiteral("json");
    const auto filter = exportHtml ? QStringLiteral("HTML report (*.html)") : QStringLiteral("JSON report (*.json)");
    const auto filePath = QFileDialog::getSaveFileName(this,
                                                       exportHtml ? QStringLiteral("Export HTML report") : QStringLiteral("Export JSON report"),
                                                       QDir(lastResult_.projectRootPath).filePath(QStringLiteral("OpenCodeScan-report.%1").arg(defaultExtension)),
                                                       filter);
    if (filePath.isEmpty()) {
        return false;
    }
    QString errorMessage;
    const bool success = exportHtml
        ? opencodescan::HtmlReportExporter {}.exportReport(lastResult_, filePath, &errorMessage)
        : opencodescan::JsonReportExporter {}.exportReport(lastResult_, filePath, &errorMessage);
    if (!success) {
        QMessageBox::critical(this,
                              QStringLiteral("Export failed"),
                              errorMessage.isEmpty() ? QStringLiteral("Unable to export the report.") : errorMessage);
        return false;
    }
    statusBar()->showMessage(QStringLiteral("Exported report to %1").arg(QDir::cleanPath(filePath)), 5000);
    return true;
}
