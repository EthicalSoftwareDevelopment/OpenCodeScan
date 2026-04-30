#pragma once
#include "analysis/CompileCommandsLoader.hpp"
#include "analysis/ProjectFileDiscoverer.hpp"
#include "analysis/rules/RuleRegistry.hpp"
#include "core/IAnalysisEngine.hpp"
namespace opencodescan {
class DeterministicAnalysisEngine final : public IAnalysisEngine {
public:
    [[nodiscard]] QVector<RuleMetadata> availableRules() const override;
    [[nodiscard]] AnalysisResult analyzeProject(const AnalysisRequest& request,
                                                const ProgressCallback& progressCallback = {},
                                                const CancelToken& cancelToken = {}) override;
private:
    [[nodiscard]] TranslationUnitConfig makeTranslationUnitConfig(const QString& filePath,
                                                                 const AnalysisRequest& request,
                                                                 const QHash<QString, TranslationUnitConfig>& compileCommandsByFile,
                                                                 bool* hasCompileCommand) const;
    void reportProgress(const ProgressCallback& progressCallback,
                        ScanState state,
                        int totalFiles,
                        int processedFiles,
                        const QString& currentFilePath,
                        const QString& message) const;
    ProjectFileDiscoverer fileDiscoverer_;
    CompileCommandsLoader compileCommandsLoader_;
    RuleRegistry ruleRegistry_;
};
} // namespace opencodescan
