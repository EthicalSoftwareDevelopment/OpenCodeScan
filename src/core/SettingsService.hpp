#pragma once
#include <QSettings>
#include <QString>
namespace opencodescan {
class SettingsService {
public:
    explicit SettingsService(QString organizationName = {}, QString applicationName = {});
    [[nodiscard]] QString lastProjectPath() const;
    void setLastProjectPath(const QString& projectPath);
private:
    QSettings settings_;
};
} // namespace opencodescan
