#include "analysis/CompileCommandsLoader.hpp"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
namespace opencodescan {
namespace {
QStringList compileCommandsCandidates(const QString& projectRootPath) {
    const QDir root(projectRootPath);
    return {
        root.filePath(QStringLiteral("compile_commands.json")),
        root.filePath(QStringLiteral("build/compile_commands.json")),
        root.filePath(QStringLiteral("cmake-build-debug/compile_commands.json")),
        root.filePath(QStringLiteral("cmake-build-release/compile_commands.json")),
        root.filePath(QStringLiteral("out/build/compile_commands.json"))
    };
}
QString normalizePath(const QString& workingDirectory, const QString& path) {
    if (QDir::isAbsolutePath(path)) {
        return QDir::cleanPath(path);
    }
    return QDir::cleanPath(QDir(workingDirectory).filePath(path));
}
QStringList toArguments(const QJsonObject& entry) {
    if (entry.contains(QStringLiteral("arguments")) && entry.value(QStringLiteral("arguments")).isArray()) {
        QStringList arguments;
        const auto array = entry.value(QStringLiteral("arguments")).toArray();
        arguments.reserve(array.size());
        for (const auto& value : array) {
            arguments << value.toString();
        }
        return arguments;
    }
    if (entry.contains(QStringLiteral("command"))) {
        return QProcess::splitCommand(entry.value(QStringLiteral("command")).toString());
    }
    return {};
}
void absorbCompilerArguments(TranslationUnitConfig& config, const QStringList& arguments) {
    config.compilerArguments = arguments;
    for (int index = 0; index < arguments.size(); ++index) {
        const auto& argument = arguments.at(index);
        if (argument.startsWith(QStringLiteral("-I")) && argument.size() > 2) {
            config.includePaths << argument.mid(2);
        } else if (argument == QStringLiteral("-I") && index + 1 < arguments.size()) {
            config.includePaths << arguments.at(++index);
        } else if (argument.startsWith(QStringLiteral("/I")) && argument.size() > 2) {
            config.includePaths << argument.mid(2);
        } else if (argument == QStringLiteral("/I") && index + 1 < arguments.size()) {
            config.includePaths << arguments.at(++index);
        } else if (argument.startsWith(QStringLiteral("-D")) && argument.size() > 2) {
            config.defines << argument.mid(2);
        } else if (argument == QStringLiteral("-D") && index + 1 < arguments.size()) {
            config.defines << arguments.at(++index);
        } else if (argument.startsWith(QStringLiteral("/D")) && argument.size() > 2) {
            config.defines << argument.mid(2);
        } else if (argument == QStringLiteral("/D") && index + 1 < arguments.size()) {
            config.defines << arguments.at(++index);
        }
    }
    for (auto& includePath : config.includePaths) {
        includePath = normalizePath(config.workingDirectory, includePath);
    }
    config.includePaths.removeDuplicates();
    config.defines.removeDuplicates();
}
} // namespace
CompileCommandsLoadResult CompileCommandsLoader::load(const QString& projectRootPath) const {
    CompileCommandsLoadResult result;
    QString compileCommandsPath;
    for (const auto& candidate : compileCommandsCandidates(projectRootPath)) {
        if (QFile::exists(candidate)) {
            compileCommandsPath = candidate;
            break;
        }
    }
    if (compileCommandsPath.isEmpty()) {
        result.notes << QStringLiteral("No compile_commands.json was found in the project root or common build directories.");
        return result;
    }
    QFile file(compileCommandsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.notes << QStringLiteral("Unable to open compile_commands.json at %1.").arg(QDir::cleanPath(compileCommandsPath));
        return result;
    }
    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        result.notes << QStringLiteral("compile_commands.json did not contain a JSON array: %1").arg(QDir::cleanPath(compileCommandsPath));
        return result;
    }
    result.compileCommandsPath = QDir::cleanPath(compileCommandsPath);
    const auto entries = document.array();
    for (const auto& value : entries) {
        const auto entry = value.toObject();
        const auto workingDirectory = QDir::cleanPath(entry.value(QStringLiteral("directory")).toString(projectRootPath));
        const auto rawFilePath = entry.value(QStringLiteral("file")).toString();
        if (rawFilePath.isEmpty()) {
            continue;
        }
        TranslationUnitConfig config;
        config.workingDirectory = workingDirectory;
        config.filePath = normalizePath(workingDirectory, rawFilePath);
        config.fromCompileCommands = true;
        absorbCompilerArguments(config, toArguments(entry));
        result.translationUnitsByFile.insert(config.filePath, config);
    }
    result.notes << QStringLiteral("Loaded %1 compile command entries from %2.")
                        .arg(result.translationUnitsByFile.size())
                        .arg(result.compileCommandsPath);
    return result;
}
} // namespace opencodescan
