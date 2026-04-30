#include "core/SettingsService.hpp"
#include <QCoreApplication>
#include <QDir>
namespace opencodescan {
SettingsService::SettingsService(QString organizationName, QString applicationName)
    : settings_(organizationName.isEmpty() ? QCoreApplication::organizationName() : organizationName,
                applicationName.isEmpty() ? QCoreApplication::applicationName() : applicationName) {
}
QString SettingsService::lastProjectPath() const {
    return settings_.value(QStringLiteral("project/lastPath")).toString();
}
void SettingsService::setLastProjectPath(const QString& projectPath) {
    settings_.setValue(QStringLiteral("project/lastPath"), QDir::cleanPath(projectPath));
    settings_.sync();
}
} // namespace opencodescan
