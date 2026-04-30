#pragma once
#include <QHash>
#include <QString>
#include <QStringList>
#include "core/Diagnostic.hpp"
namespace opencodescan {
struct CompileCommandsLoadResult {
    QString compileCommandsPath;
    QHash<QString, TranslationUnitConfig> translationUnitsByFile;
    QStringList notes;
};
class CompileCommandsLoader {
public:
    [[nodiscard]] CompileCommandsLoadResult load(const QString& projectRootPath) const;
};
} // namespace opencodescan
