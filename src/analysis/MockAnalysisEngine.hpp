#pragma once
#include "core/IAnalysisEngine.hpp"
namespace opencodescan {
class MockAnalysisEngine final : public IAnalysisEngine {
public:
    [[nodiscard]] AnalysisResult analyzeProject(const AnalysisRequest& request) override;
};
} // namespace opencodescan
