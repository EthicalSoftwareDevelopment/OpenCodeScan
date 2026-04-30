#pragma once
#include <QSettings>
#include <QString>
#include <QStringList>
namespace opencodescan {
class SettingsService {
public:
    explicit SettingsService(const QString& organizationName = {}, const QString& applicationName = {});
    [[nodiscard]] QString lastProjectPath() const;
    void setLastProjectPath(const QString& projectPath);
    [[nodiscard]] QStringList scanIncludePaths() const;
    void setScanIncludePaths(const QStringList& includePaths);
    [[nodiscard]] QStringList scanDefines() const;
    void setScanDefines(const QStringList& defines);
    [[nodiscard]] QStringList excludedPaths() const;
    void setExcludedPaths(const QStringList& excludedPaths);
private:
    [[nodiscard]] QStringList valueList(const QString& key) const;
    void setValueList(const QString& key, const QStringList& values);
    QSettings settings_;
};
} // namespace opencodescan
