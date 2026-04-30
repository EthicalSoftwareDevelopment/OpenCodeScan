#include "app/ApplicationBootstrap.hpp"
#include <QApplication>
#include <memory>
#include "analysis/MockAnalysisEngine.hpp"
#include "core/Logging.hpp"
#include "core/SettingsService.hpp"
#include "ui/MainWindow.hpp"
ApplicationBootstrap::ApplicationBootstrap(QApplication& application)
    : application_(application) {
}
ApplicationBootstrap::~ApplicationBootstrap() = default;
int ApplicationBootstrap::run() {
    opencodescan::Logger::initialize();
    opencodescan::Logger::info(QStringLiteral("Starting OpenCodeScan Phase 1 shell."));
    settingsService_ = std::make_unique<opencodescan::SettingsService>();
    analysisEngine_ = std::make_unique<opencodescan::MockAnalysisEngine>();
    mainWindow_ = std::make_unique<MainWindow>(*settingsService_, *analysisEngine_);
    mainWindow_->show();
    return application_.exec();
}
