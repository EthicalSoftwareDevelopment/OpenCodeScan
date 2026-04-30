# OpenCodeScan

Qt-based desktop static analysis tool for C/C++ projects.

## Current Status
Phase 3 is implemented: the repository now includes a deterministic analysis core, a built-in rule registry with MVP text-based rules, JSON/HTML report export, persistent rule selection, a Qt desktop workflow for running scans and exporting results, and unit tests.

## Documents
- `docs/requirements/product-requirements.md`
- `docs/requirements/non-functional-requirements.md`
- `docs/architecture/folder-structure.md`
- `docs/ai-phased-implementation-plan.md`

## Quick Build (example)
The application entrypoint lives in `src/app/main.cpp` and the project is built through the top-level `CMakeLists.txt`.

If Qt is not in a default system location, point CMake at your local Qt installation before configuring.

If you build from an external PowerShell session with the bundled CLion MinGW toolchain, add the MinGW `bin` directory to `PATH` first so the GCC frontend can start correctly.

```powershell
$env:Path = "C:\Program Files\JetBrains\CLion 2026.1\bin\mingw\bin;" + $env:Path
$env:Qt6_DIR = "C:\Qt\6.11.0\mingw_64\lib\cmake\Qt6"
cmake -S . -B build
cmake --build build
.\build\OpenCodeScan.exe
```

## Run Unit Tests

```powershell
$env:Path = "C:\Program Files\JetBrains\CLion 2026.1\bin\mingw\bin;C:\Qt\6.11.0\mingw_64\bin;" + $env:Path
ctest --test-dir build --output-on-failure
```

## Notes
- Qt discovery is machine-configurable through `Qt6_DIR`/`CMAKE_PREFIX_PATH` rather than a hardcoded local path.
- `main.cpp` at the repository root remains legacy scaffolding and is no longer part of the active target.
- The Phase 3 UI lets you configure include paths/defines/exclusions, choose enabled rule IDs, run scans on a worker thread, inspect actionable diagnostics, and export reports as JSON or HTML.
- The Phase 3 MVP rules are text-based and deterministic; a future parser-backed rule implementation can replace their internals without changing the UI or reporting flow.
