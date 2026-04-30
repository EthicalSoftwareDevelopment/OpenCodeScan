#pragma once
#include "core/Diagnostic.hpp"
namespace opencodescan {
class IAnalysisEngine {
public:
    virtual ~IAnalysisEngine() = default;
    [[nodiscard]] virtual AnalysisResult analyzeProject(const AnalysisRequest& request,
                                                        const ProgressCallback& progressCallback = {},
                                                        const CancelToken& cancelToken = {}) = 0;
};
} // namespace opencodescan
