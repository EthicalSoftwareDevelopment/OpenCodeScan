#pragma once
#include "core/Diagnostic.hpp"
namespace opencodescan {
class IAnalysisEngine {
public:
    virtual ~IAnalysisEngine() = default;
    [[nodiscard]] virtual AnalysisResult analyzeProject(const AnalysisRequest& request) = 0;
};
} // namespace opencodescan
