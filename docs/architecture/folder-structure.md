# Folder Structure - OpenCodeScan

## Recommended Structure

```text
OpenCodeScan/
  CMakeLists.txt
  main.cpp                      # legacy bootstrap file kept outside the active target
  README.md
  cmake/
  docs/
    requirements/
      product-requirements.md
      non-functional-requirements.md
    architecture/
      folder-structure.md
    ai-phased-implementation-plan.md
  src/
    app/                        # app bootstrap and startup wiring
      main.cpp
      ApplicationBootstrap.hpp
      ApplicationBootstrap.cpp
    ui/                         # Qt widgets/windows/models for presentation
      MainWindow.hpp
      MainWindow.cpp
      DiagnosticsTableModel.hpp
      DiagnosticsTableModel.cpp
    analysis/                   # static analysis pipeline and rule execution
      MockAnalysisEngine.hpp
      MockAnalysisEngine.cpp
    core/                       # shared domain models and services
      Diagnostic.hpp
      IAnalysisEngine.hpp
      IDiagnosticsModel.hpp
      IReportExporter.hpp
      IAiAdvisor.hpp
      IHintProvider.hpp
      Logging.hpp
      Logging.cpp
      SettingsService.hpp
      SettingsService.cpp
  include/
    opencodescan/               # public headers (if needed for modularization)
  resources/
    icons/
    styles/
  tests/
    unit/
    integration/
    fixtures/
  scripts/
    ci/
```

## Layering Rules
- `src/ui` depends on `src/core` and orchestration interfaces, not concrete rule implementations.
- `src/analysis` depends on `src/core` for domain models.
- `src/core` must not depend on `src/ui`.
- `tests` can reference all runtime modules.

## Next Refactor Step
Begin Phase 2 by replacing `MockAnalysisEngine` with a real project scanner + parser integration while keeping the UI and core contracts stable.

