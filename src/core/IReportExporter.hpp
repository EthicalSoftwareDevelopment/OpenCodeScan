#pragma once
#include <QString>
#include "core/Diagnostic.hpp"
namespace opencodescan {
class IReportExporter {
public:
    virtual ~IReportExporter() = default;
    [[nodiscard]] virtual bool exportReport(const QVector<Diagnostic>& diagnostics,
                                            const QString& targetFilePath) = 0;
};
} // namespace opencodescan
