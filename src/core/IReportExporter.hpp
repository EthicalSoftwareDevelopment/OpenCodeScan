#pragma once

#include <QString>

#include "core/Diagnostic.hpp"

namespace opencodescan {

class IReportExporter {
public:
    virtual ~IReportExporter() = default;

    [[nodiscard]] virtual bool exportReport(const AnalysisResult& result,
                                            const QString& targetFilePath,
                                            QString* errorMessage = nullptr) const = 0;
};

} // namespace opencodescan

