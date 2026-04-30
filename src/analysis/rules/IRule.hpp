#pragma once

#include <QVector>

#include "core/Diagnostic.hpp"

namespace opencodescan {

struct PreparedSourceFile {
    TranslationUnitConfig translationUnit;
    QString sourceText;
    QStringList sourceLines;
};

class IRule {
public:
    virtual ~IRule() = default;

    [[nodiscard]] virtual RuleMetadata metadata() const = 0;
    [[nodiscard]] virtual QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const = 0;
};

} // namespace opencodescan

