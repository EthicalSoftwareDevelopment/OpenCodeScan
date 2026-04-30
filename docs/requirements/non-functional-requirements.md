# Non-Functional Requirements - OpenCodeScan

## 1. Compatibility
- Must run on Windows 10/11 and Linux (Ubuntu LTS class distributions).
- Must build with CMake and Qt 6.
- Must support MSVC on Windows and GCC or Clang on Linux.

## 2. Performance
- UI remains responsive during scans (analysis must run off the UI thread).
- Large projects should provide incremental progress updates.
- Rule execution must support cancellation checks at safe points.

## 3. Reliability
- Identical source input and config should produce deterministic diagnostics.
- Parse failures in one file must not abort the full scan.
- App should recover cleanly from canceled runs.

## 4. Usability
- Diagnostics must be readable and sortable.
- Core workflow (open project, scan, filter, export) should be available within one main window.
- Common actions should be accessible in <= 2 clicks from main screen.

## 5. Maintainability
- Clear separation between UI, orchestration, and analysis engine.
- New rules should be addable without UI code changes.
- Shared models (diagnostics, severity, rule metadata) should be centrally defined.

## 6. Observability
- Provide structured logging with log levels (`debug`, `info`, `warn`, `error`).
- Log scan lifecycle events and non-fatal parser failures.
- Keep logs local by default.

## 7. Security and Privacy
- Analyze files locally by default; no upload required.
- No telemetry unless explicitly enabled by user.
- Treat scanned repositories as untrusted input and avoid unsafe file operations.

## 8. Testing and Quality Gates
- Unit tests for rule engine, configuration, and diagnostic formatting.
- Integration tests for end-to-end scan flow on fixtures.
- CI matrix should validate builds on Windows and Linux.
- New rules require tests and documented expected outputs.

