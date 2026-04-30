#pragma once
#include <QStringList>
#include "core/Diagnostic.hpp"
namespace opencodescan {
class ProjectFileDiscoverer {
public:
    [[nodiscard]] QStringList discover(const AnalysisRequest& request) const;
};
} // namespace opencodescan
