#pragma once
#include "core/IReportExporter.hpp"
namespace opencodescan {
class HtmlReportExporter final : public IReportExporter {
public:
    [[nodiscard]] bool exportReport(const AnalysisResult& result,
                                    const QString& targetFilePath,
                                    QString* errorMessage = nullptr) const override;
};
} // namespace opencodescan
