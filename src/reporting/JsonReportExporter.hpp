#pragma once
#include "core/IReportExporter.hpp"
namespace opencodescan {
class JsonReportExporter final : public IReportExporter {
public:
    [[nodiscard]] bool exportReport(const AnalysisResult& result,
                                    const QString& targetFilePath,
                                    QString* errorMessage = nullptr) const override;
};
} // namespace opencodescan
