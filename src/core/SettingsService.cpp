#include "core/SettingsService.hpp"
#include <QCoreApplication>
#include <QDir>
namespace opencodescan {
namespace {
constexpr auto kLastProjectPath = "project/lastPath";
constexpr auto kScanIncludePaths = "scan/includePaths";
constexpr auto kScanDefines = "scan/defines";
constexpr auto kScanExcludedPaths = "scan/excludedPaths";
constexpr auto kEnabledRuleIds = "scan/enabledRuleIds";
}
SettingsService::SettingsService(const QString& organizationName, const QString& applicationName)
    : settings_(organizationName.isEmpty() ? QCoreApplication::organizationName() : organizationName,
                applicationName.isEmpty() ? QCoreApplication::applicationName() : applicationName) {
}
QString SettingsService::lastProjectPath() const {
    return settings_.value(QLatin1String(kLastProjectPath)).toString();
}
void SettingsService::setLastProjectPath(const QString& projectPath) {
    settings_.setValue(QLatin1String(kLastProjectPath), QDir::cleanPath(projectPath));
    settings_.sync();
}
QStringList SettingsService::scanIncludePaths() const {
    return valueList(QLatin1String(kScanIncludePaths));
}
void SettingsService::setScanIncludePaths(const QStringList& includePaths) {
    setValueList(QLatin1String(kScanIncludePaths), includePaths);
}
QStringList SettingsService::scanDefines() const {
    return valueList(QLatin1String(kScanDefines));
}
void SettingsService::setScanDefines(const QStringList& defines) {
    setValueList(QLatin1String(kScanDefines), defines);
}
QStringList SettingsService::excludedPaths() const {
    return valueList(QLatin1String(kScanExcludedPaths));
}
void SettingsService::setExcludedPaths(const QStringList& excludedPaths) {
    setValueList(QLatin1String(kScanExcludedPaths), excludedPaths);
}
QStringList SettingsService::enabledRuleIds() const {
    return valueList(QLatin1String(kEnabledRuleIds));
}
void SettingsService::setEnabledRuleIds(const QStringList& ruleIds) {
    setValueList(QLatin1String(kEnabledRuleIds), ruleIds);
}
QStringList SettingsService::valueList(const QString& key) const {
    return settings_.value(key).toStringList();
}
void SettingsService::setValueList(const QString& key, const QStringList& values) {
    QStringList normalizedValues;
    normalizedValues.reserve(values.size());
    for (const auto& value : values) {
        const auto trimmed = value.trimmed();
        if (!trimmed.isEmpty()) {
            normalizedValues << trimmed;
        }
    }
    normalizedValues.removeDuplicates();
    settings_.setValue(key, normalizedValues);
    settings_.sync();
}
} // namespace opencodescan
