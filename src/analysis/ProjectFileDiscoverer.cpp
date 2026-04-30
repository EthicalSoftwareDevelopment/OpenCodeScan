#include "analysis/ProjectFileDiscoverer.hpp"
#include <QDir>
#include <QDirIterator>
#include <algorithm>
namespace opencodescan {
namespace {
QStringList defaultExcludedFragments() {
    return {
        QStringLiteral("/.git/"),
        QStringLiteral("/build/"),
        QStringLiteral("/cmake-build-debug/"),
        QStringLiteral("/cmake-build-release/"),
        QStringLiteral("/out/")
    };
}
QString normalizedAbsolutePath(const QString& rootPath, const QString& path) {
    const QDir rootDirectory(rootPath);
    if (QDir::isAbsolutePath(path)) {
        return QDir::fromNativeSeparators(QDir::cleanPath(path));
    }
    return QDir::fromNativeSeparators(QDir::cleanPath(rootDirectory.filePath(path)));
}
bool containsIgnoredFragment(const QString& normalizedPath) {
    const auto pathWithSlashes = QString(normalizedPath).replace('\\', '/');
    for (const auto& fragment : defaultExcludedFragments()) {
        if (pathWithSlashes.contains(fragment, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}
bool isExcluded(const QString& rootPath, const QString& filePath, const QStringList& excludedPaths) {
    const auto normalizedFilePath = QDir::fromNativeSeparators(QDir::cleanPath(filePath));
    if (containsIgnoredFragment(normalizedFilePath)) {
        return true;
    }
    for (const auto& excludedPath : excludedPaths) {
        const auto normalizedExcludedPath = normalizedAbsolutePath(rootPath, excludedPath);
        if (normalizedFilePath.compare(normalizedExcludedPath, Qt::CaseInsensitive) == 0
            || normalizedFilePath.startsWith(normalizedExcludedPath + QLatin1Char('/'), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}
} // namespace
QStringList ProjectFileDiscoverer::discover(const AnalysisRequest& request) const {
    static const QStringList filters {
        QStringLiteral("*.c"),
        QStringLiteral("*.cc"),
        QStringLiteral("*.cpp"),
        QStringLiteral("*.cxx"),
        QStringLiteral("*.h"),
        QStringLiteral("*.hh"),
        QStringLiteral("*.hpp"),
        QStringLiteral("*.hxx")
    };
    QStringList files;
    QDirIterator iterator(request.projectRootPath,
                          filters,
                          QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const auto filePath = QDir::cleanPath(iterator.next());
        if (!isExcluded(request.projectRootPath, filePath, request.excludedPaths)) {
            files << filePath;
        }
    }
    std::sort(files.begin(), files.end(), [](const QString& left, const QString& right) {
        const auto leftLower = left.toLower();
        const auto rightLower = right.toLower();
        return leftLower == rightLower ? left < right : leftLower < rightLower;
    });
    return files;
}
} // namespace opencodescan
