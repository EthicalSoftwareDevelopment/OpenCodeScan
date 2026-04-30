# Folder Structure - OpenCodeScan

## Recommended Structure

```text
OpenCodeScan/
  CMakeLists.txt
  main.cpp                      # temporary entrypoint (to be moved into src/app)
  README.md
  cmake/
  docs/
    requirements/
      product-requirements.md
      non-functional-requirements.md
    architecture/
      folder-structure.md
  src/
    app/                        # app bootstrap and startup wiring
    ui/                         # Qt widgets/windows/models for presentation
    analysis/                   # static analysis pipeline and rule execution
    core/                       # shared domain models and services
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
Move `main.cpp` to `src/app/main.cpp` and update `CMakeLists.txt` to use module-based sources.

