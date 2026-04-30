#pragma once
#include "core/Diagnostic.hpp"
namespace opencodescan {
class IDiagnosticsModel {
public:
    virtual ~IDiagnosticsModel() = default;
    virtual void setDiagnostics(const QVector<Diagnostic>& diagnostics) = 0;
    virtual void clearDiagnostics() = 0;
    [[nodiscard]] virtual QVector<Diagnostic> diagnostics() const = 0;
};
} // namespace opencodescan
