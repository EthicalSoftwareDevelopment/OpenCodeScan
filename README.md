# OpenCodeScan

Qt-based desktop static analysis tool for C/C++ projects.

## Current Status
This repository now includes requirements documents and a scalable folder structure for a cross-platform (Windows/Linux) implementation.

## Documents
- `docs/requirements/product-requirements.md`
- `docs/requirements/non-functional-requirements.md`
- `docs/architecture/folder-structure.md`

## Quick Build (example)
This project currently has a root `main.cpp` and `CMakeLists.txt` configured for Qt.

```powershell
cmake -S . -B build
cmake --build build
```

## Notes
- Keep Qt discovery configurable per machine (avoid hardcoded local paths when moving to shared builds).
- Next structural change: move `main.cpp` to `src/app/main.cpp` and update CMake source lists.

