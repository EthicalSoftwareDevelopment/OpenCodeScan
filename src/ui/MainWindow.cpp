#include "ui/MainWindow.hpp"
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include "core/Diagnostic.hpp"
#include "core/Logging.hpp"
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
    details += QStringLiteral("Remediation Hint\n----------------\n%1").arg(
        diagnostic.remediationHint.isEmpty() ? QStringLiteral("No hint available yet.") : diagnostic.remediationHint);
    return details;
}
} // namespace
MainWindow::MainWindow(opencodescan::SettingsService& settingsService,
                       opencodescan::IAnalysisEngine& analysisEngine,
                       QWidget* parent)
    : QMainWindow(parent)
    , settingsService_(settingsService)
    , analysisEngine_(analysisEngine) {
    setupUi();
    restoreProjectSelection();
}
void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("OpenCodeScan - Phase 1 Shell"));
    resize(1180, 760);
    auto* centralWidget = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(centralWidget);
    auto* projectLayout = new QHBoxLayout();
    auto* projectLabel = new QLabel(QStringLiteral("Project root:"), centralWidget);
    projectPathEdit_ = new QLineEdit(centralWidget);
    browseButton_ = new QPushButton(QStringLiteral("Browse..."), centralWidget);
    loadProjectButton_ = new QPushButton(QStringLiteral("Load Project"), centralWidget);
    summaryLabel_ = new QLabel(QStringLiteral("Select a folder to initialize the OpenCodeScan application shell."), centralWidget);
    summaryLabel_->setWordWrap(true);
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
    detailsView_->setPlaceholderText(QStringLiteral("Select a diagnostic to inspect details."));
    auto* splitter = new QSplitter(Qt::Vertical, centralWidget);
    splitter->addWidget(diagnosticsView_);
    splitter->addWidget(detailsView_);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    projectLayout->addWidget(projectLabel);
    projectLayout->addWidget(projectPathEdit_, 1);
    projectLayout->addWidget(browseButton_);
    projectLayout->addWidget(loadProjectButton_);
    rootLayout->addLayout(projectLayout);
    rootLayout->addWidget(summaryLabel_);
    rootLayout->addWidget(splitter, 1);
    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("Ready"));
    connect(browseButton_, &QPushButton::clicked, this, &MainWindow::browseForProject);
    connect(loadProjectButton_, &QPushButton::clicked, this, [this]() {
        loadProject(projectPathEdit_->text().trimmed());
    });
    connect(diagnosticsView_->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &MainWindow::updateDiagnosticDetails);
}
void MainWindow::restoreProjectSelection() {
    const auto lastProjectPath = settingsService_.lastProjectPath();
    if (!lastProjectPath.isEmpty() && QDir(lastProjectPath).exists()) {
        loadProject(lastProjectPath);
        return;
    }
    const auto fallbackPath = QDir::currentPath();
    if (QDir(fallbackPath).exists()) {
        projectPathEdit_->setText(QDir::cleanPath(fallbackPath));
        loadProject(fallbackPath);
    }
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
        loadProject(selectedPath);
    }
}
void MainWindow::loadProject(const QString& projectRootPath) {
    const auto normalizedPath = QDir::cleanPath(projectRootPath);
    if (normalizedPath.isEmpty() || !QDir(normalizedPath).exists()) {
        QMessageBox::warning(this,
                             QStringLiteral("Invalid project path"),
                             QStringLiteral("Select an existing project directory before loading it."));
        return;
    }
    projectPathEdit_->setText(normalizedPath);
    settingsService_.setLastProjectPath(normalizedPath);
    opencodescan::Logger::info(QStringLiteral("Loaded project shell for %1").arg(normalizedPath));
    opencodescan::AnalysisRequest request;
    request.projectRootPath = normalizedPath;
    const auto result = analysisEngine_.analyzeProject(request);
    diagnosticsModel_->setDiagnostics(result.diagnostics);
    summaryLabel_->setText(QStringLiteral("Project loaded: %1\nDiagnostics: %2")
                               .arg(normalizedPath)
                               .arg(result.diagnostics.size()));
    showNotes(result.notes);
    if (diagnosticsModel_->rowCount() > 0) {
        diagnosticsView_->selectRow(0);
        updateDiagnosticDetails(diagnosticsModel_->index(0, 0), {});
    } else {
        detailsView_->clear();
    }
    statusBar()->showMessage(QStringLiteral("Loaded %1").arg(normalizedPath), 3000);
}
void MainWindow::updateDiagnosticDetails(const QModelIndex& current, const QModelIndex&) {
    if (!current.isValid()) {
        return;
    }
    const auto diagnostic = diagnosticsModel_->diagnosticAt(current.row());
    detailsView_->setPlainText(formatDiagnosticDetails(diagnostic));
}
void MainWindow::showNotes(const QStringList& notes) {
    if (notes.isEmpty()) {
        return;
    }
    const auto existingText = detailsView_->toPlainText();
    const auto notesText = QStringLiteral("Phase Notes\n-----------\n%1")
                               .arg(notes.join(QStringLiteral("\n")));
    if (existingText.isEmpty()) {
        detailsView_->setPlainText(notesText);
    } else {
        detailsView_->setPlainText(notesText + QStringLiteral("\n\n") + existingText);
    }
}
