#pragma once
#include <QString>
#include "core/Diagnostic.hpp"
namespace opencodescan {
class IHintProvider {
public:
    virtual ~IHintProvider() = default;
    [[nodiscard]] virtual QString phaseName() const = 0;
    [[nodiscard]] virtual QString hintFor(const Diagnostic& diagnostic) const = 0;
};
} // namespace opencodescan
