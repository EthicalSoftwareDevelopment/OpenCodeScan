#include "analysis/rules/RuleRegistry.hpp"

#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>

namespace opencodescan {
namespace {


Diagnostic makeDiagnostic(const RuleMetadata& metadata,
                         const QString& filePath,
                         const int line,
                         const QString& message,
                         const QString& remediationHint) {
    Diagnostic diagnostic;
    diagnostic.ruleId = metadata.id;
    diagnostic.severity = metadata.defaultSeverity;
    diagnostic.filePath = filePath;
    diagnostic.line = line;
    diagnostic.column = 1;
    diagnostic.message = message;
    diagnostic.remediationHint = remediationHint;
    return diagnostic;
}

class TodoCommentRule final : public IRule {
public:
    [[nodiscard]] RuleMetadata metadata() const override {
        return {QStringLiteral("OCS-RULE-TODO"),
                QStringLiteral("TODO / FIXME comments"),
                QStringLiteral("Flags TODO and FIXME comments that should be resolved or tracked externally."),
                RuleCategory::Maintainability,
                Severity::Info,
                true};
    }

    [[nodiscard]] QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const override {
        static const QRegularExpression expression(QStringLiteral("\\b(TODO|FIXME)\\b"), QRegularExpression::CaseInsensitiveOption);
        QVector<Diagnostic> diagnostics;
        const auto ruleMetadata = metadata();
        for (int index = 0; index < sourceFile.sourceLines.size(); ++index) {
            if (expression.match(sourceFile.sourceLines.at(index)).hasMatch()) {
                diagnostics.push_back(makeDiagnostic(ruleMetadata,
                                                     sourceFile.translationUnit.filePath,
                                                     index + 1,
                                                     QStringLiteral("Found TODO/FIXME marker in source."),
                                                     QStringLiteral("Resolve the task or move it into an issue tracker to keep production code tidy.")));
            }
        }
        return diagnostics;
    }
};

class UsingNamespaceStdRule final : public IRule {
public:
    [[nodiscard]] RuleMetadata metadata() const override {
        return {QStringLiteral("OCS-RULE-USING-NAMESPACE-STD"),
                QStringLiteral("Avoid using namespace std"),
                QStringLiteral("Detects file-scope 'using namespace std;' declarations."),
                RuleCategory::Maintainability,
                Severity::Warning,
                true};
    }

    [[nodiscard]] QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const override {
        static const QRegularExpression expression(QStringLiteral("^\\s*using\\s+namespace\\s+std\\s*;"));
        QVector<Diagnostic> diagnostics;
        const auto ruleMetadata = metadata();
        for (int index = 0; index < sourceFile.sourceLines.size(); ++index) {
            if (expression.match(sourceFile.sourceLines.at(index)).hasMatch()) {
                diagnostics.push_back(makeDiagnostic(ruleMetadata,
                                                     sourceFile.translationUnit.filePath,
                                                     index + 1,
                                                     QStringLiteral("Avoid 'using namespace std;' in shared or global scope."),
                                                     QStringLiteral("Qualify standard library symbols explicitly or use narrower using declarations.")));
            }
        }
        return diagnostics;
    }
};

class NullLiteralRule final : public IRule {
public:
    [[nodiscard]] RuleMetadata metadata() const override {
        return {QStringLiteral("OCS-RULE-NULL-LITERAL"),
                QStringLiteral("Prefer nullptr over NULL"),
                QStringLiteral("Detects use of the legacy NULL macro in C++ code."),
                RuleCategory::Correctness,
                Severity::Warning,
                true};
    }

    [[nodiscard]] QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const override {
        static const QRegularExpression expression(QStringLiteral("\\bNULL\\b"));
        QVector<Diagnostic> diagnostics;
        const auto ruleMetadata = metadata();
        for (int index = 0; index < sourceFile.sourceLines.size(); ++index) {
            if (expression.match(sourceFile.sourceLines.at(index)).hasMatch()) {
                diagnostics.push_back(makeDiagnostic(ruleMetadata,
                                                     sourceFile.translationUnit.filePath,
                                                     index + 1,
                                                     QStringLiteral("Use nullptr instead of NULL in modern C++."),
                                                     QStringLiteral("Replace NULL with nullptr for clearer type-safe null pointer semantics.")));
            }
        }
        return diagnostics;
    }
};

class RawNewDeleteRule final : public IRule {
public:
    [[nodiscard]] RuleMetadata metadata() const override {
        return {QStringLiteral("OCS-RULE-RAW-MEMORY"),
                QStringLiteral("Avoid raw new/delete"),
                QStringLiteral("Detects direct use of raw new/delete expressions."),
                RuleCategory::Safety,
                Severity::Warning,
                true};
    }

    [[nodiscard]] QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const override {
        static const QRegularExpression expression(QStringLiteral("\\b(new|delete)\\b"));
        QVector<Diagnostic> diagnostics;
        const auto ruleMetadata = metadata();
        for (int index = 0; index < sourceFile.sourceLines.size(); ++index) {
            if (expression.match(sourceFile.sourceLines.at(index)).hasMatch()) {
                diagnostics.push_back(makeDiagnostic(ruleMetadata,
                                                     sourceFile.translationUnit.filePath,
                                                     index + 1,
                                                     QStringLiteral("Raw memory management detected via 'new' or 'delete'."),
                                                     QStringLiteral("Prefer RAII containers or smart pointers to reduce ownership and lifetime bugs.")));
            }
        }
        return diagnostics;
    }
};

class HeaderGuardRule final : public IRule {
public:
    [[nodiscard]] RuleMetadata metadata() const override {
        return {QStringLiteral("OCS-RULE-HEADER-GUARD"),
                QStringLiteral("Header should use a guard or pragma once"),
                QStringLiteral("Detects headers missing #pragma once or classic include guards."),
                RuleCategory::Correctness,
                Severity::Warning,
                true};
    }

    [[nodiscard]] QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const override {
        const QFileInfo fileInfo(sourceFile.translationUnit.filePath);
        const auto suffix = fileInfo.suffix().toLower();
        if (suffix != QStringLiteral("h")
            && suffix != QStringLiteral("hh")
            && suffix != QStringLiteral("hpp")
            && suffix != QStringLiteral("hxx")) {
            return {};
        }

        const auto hasPragmaOnce = sourceFile.sourceText.contains(QStringLiteral("#pragma once"));
        const auto hasIfndef = sourceFile.sourceText.contains(QStringLiteral("#ifndef"));
        const auto hasDefine = sourceFile.sourceText.contains(QStringLiteral("#define"));
        if (hasPragmaOnce || (hasIfndef && hasDefine)) {
            return {};
        }

        return {makeDiagnostic(metadata(),
                               sourceFile.translationUnit.filePath,
                               1,
                               QStringLiteral("Header does not appear to use #pragma once or a classic include guard."),
                               QStringLiteral("Add #pragma once or an #ifndef/#define guard to prevent duplicate inclusion."))};
    }
};

class StdEndlRule final : public IRule {
public:
    [[nodiscard]] RuleMetadata metadata() const override {
        return {QStringLiteral("OCS-RULE-STD-ENDL"),
                QStringLiteral("Prefer '\\n' over std::endl when flushing is not needed"),
                QStringLiteral("Detects std::endl, which performs an implicit flush."),
                RuleCategory::Performance,
                Severity::Info,
                true};
    }

    [[nodiscard]] QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const override {
        QVector<Diagnostic> diagnostics;
        const auto ruleMetadata = metadata();
        for (int index = 0; index < sourceFile.sourceLines.size(); ++index) {
            if (sourceFile.sourceLines.at(index).contains(QStringLiteral("std::endl"))) {
                diagnostics.push_back(makeDiagnostic(ruleMetadata,
                                                     sourceFile.translationUnit.filePath,
                                                     index + 1,
                                                     QStringLiteral("std::endl flushes the stream and can be slower than a plain newline."),
                                                     QStringLiteral("Use '\\n' unless an immediate flush is explicitly required.")));
            }
        }
        return diagnostics;
    }
};

class TrailingWhitespaceRule final : public IRule {
public:
    [[nodiscard]] RuleMetadata metadata() const override {
        return {QStringLiteral("OCS-RULE-TRAILING-WHITESPACE"),
                QStringLiteral("Avoid trailing whitespace"),
                QStringLiteral("Detects lines that end with trailing spaces or tabs."),
                RuleCategory::Maintainability,
                Severity::Info,
                false};
    }

    [[nodiscard]] QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const override {
        static const QRegularExpression expression(QStringLiteral("[ \\t]+$"));
        QVector<Diagnostic> diagnostics;
        const auto ruleMetadata = metadata();
        for (int index = 0; index < sourceFile.sourceLines.size(); ++index) {
            if (expression.match(sourceFile.sourceLines.at(index)).hasMatch()) {
                diagnostics.push_back(makeDiagnostic(ruleMetadata,
                                                     sourceFile.translationUnit.filePath,
                                                     index + 1,
                                                     QStringLiteral("Trailing whitespace detected."),
                                                     QStringLiteral("Trim trailing spaces and tabs to keep diffs cleaner.")));
            }
        }
        return diagnostics;
    }
};

class CStyleCastRule final : public IRule {
public:
    [[nodiscard]] RuleMetadata metadata() const override {
        return {QStringLiteral("OCS-RULE-C-STYLE-CAST"),
                QStringLiteral("Avoid C-style casts"),
                QStringLiteral("Detects simple C-style casts in assignments and returns."),
                RuleCategory::Correctness,
                Severity::Warning,
                false};
    }

    [[nodiscard]] QVector<Diagnostic> run(const PreparedSourceFile& sourceFile) const override {
        static const QRegularExpression expression(QStringLiteral("(=|return)\\s*\\([A-Za-z_][A-Za-z0-9_:<>&* ]*\\)\\s*[A-Za-z_(]"));
        QVector<Diagnostic> diagnostics;
        const auto ruleMetadata = metadata();
        for (int index = 0; index < sourceFile.sourceLines.size(); ++index) {
            const auto& line = sourceFile.sourceLines.at(index);
            if (line.contains(QStringLiteral("static_cast"))
                || line.contains(QStringLiteral("reinterpret_cast"))
                || line.contains(QStringLiteral("const_cast"))
                || line.contains(QStringLiteral("dynamic_cast"))) {
                continue;
            }
            if (expression.match(line).hasMatch()) {
                diagnostics.push_back(makeDiagnostic(ruleMetadata,
                                                     sourceFile.translationUnit.filePath,
                                                     index + 1,
                                                     QStringLiteral("Simple C-style cast detected."),
                                                     QStringLiteral("Prefer a named C++ cast to make intent and risk clearer.")));
            }
        }
        return diagnostics;
    }
};

} // namespace

RuleRegistry::RuleRegistry()
    : rules_ {
          std::make_shared<TodoCommentRule>(),
          std::make_shared<UsingNamespaceStdRule>(),
          std::make_shared<NullLiteralRule>(),
          std::make_shared<RawNewDeleteRule>(),
          std::make_shared<HeaderGuardRule>(),
          std::make_shared<StdEndlRule>(),
          std::make_shared<TrailingWhitespaceRule>(),
          std::make_shared<CStyleCastRule>()} {
}

QVector<RuleMetadata> RuleRegistry::availableRules() const {
    QVector<RuleMetadata> metadata;
    metadata.reserve(rules_.size());
    for (const auto& rule : rules_) {
        metadata.push_back(rule->metadata());
    }
    std::sort(metadata.begin(), metadata.end(), [](const RuleMetadata& left, const RuleMetadata& right) {
        return left.id < right.id;
    });
    return metadata;
}

QStringList RuleRegistry::defaultEnabledRuleIds() const {
    QStringList ruleIds;
    for (const auto& metadata : availableRules()) {
        if (metadata.enabledByDefault) {
            ruleIds << metadata.id;
        }
    }
    return ruleIds;
}

QVector<std::shared_ptr<const IRule>> RuleRegistry::enabledRules(const QStringList& enabledRuleIds) const {
    const auto normalizedRuleIds = enabledRuleIds.isEmpty() ? defaultEnabledRuleIds() : enabledRuleIds;
    QVector<std::shared_ptr<const IRule>> enabledRules;
    for (const auto& rule : rules_) {
        if (normalizedRuleIds.contains(rule->metadata().id)) {
            enabledRules.push_back(rule);
        }
    }

    std::sort(enabledRules.begin(), enabledRules.end(), [](const auto& left, const auto& right) {
        return left->metadata().id < right->metadata().id;
    });
    return enabledRules;
}

} // namespace opencodescan

