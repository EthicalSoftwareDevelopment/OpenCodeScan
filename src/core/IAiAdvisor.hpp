#pragma once
#include <optional>
#include "core/Diagnostic.hpp"
namespace opencodescan {
struct AiContext {
    QString projectRootPath;
    Diagnostic diagnostic;
    QString codeExcerpt;
};
struct AiSuggestion {
    QString summary;
    QString rationale;
    QString remediation;
    double confidence {0.0};
};
class IAiAdvisor {
public:
    virtual ~IAiAdvisor() = default;
    [[nodiscard]] virtual std::optional<AiSuggestion> suggestFix(const AiContext& context) = 0;
};
} // namespace opencodescan
