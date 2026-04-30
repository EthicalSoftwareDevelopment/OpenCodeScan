#pragma once

#include <memory>

#include "analysis/rules/IRule.hpp"

namespace opencodescan {

class RuleRegistry {
public:
    RuleRegistry();

    [[nodiscard]] QVector<RuleMetadata> availableRules() const;
    [[nodiscard]] QStringList defaultEnabledRuleIds() const;
    [[nodiscard]] QVector<std::shared_ptr<const IRule>> enabledRules(const QStringList& enabledRuleIds) const;

private:
    QVector<std::shared_ptr<const IRule>> rules_;
};

} // namespace opencodescan

