#pragma once
#include <memory>
class QApplication;
class MainWindow;
namespace opencodescan {
class IAnalysisEngine;
class SettingsService;
}
class ApplicationBootstrap final {
public:
    explicit ApplicationBootstrap(QApplication& application);
    ~ApplicationBootstrap();
    int run();
private:
    QApplication& application_;
    std::unique_ptr<opencodescan::SettingsService> settingsService_;
    std::unique_ptr<opencodescan::IAnalysisEngine> analysisEngine_;
    std::unique_ptr<MainWindow> mainWindow_;
};
