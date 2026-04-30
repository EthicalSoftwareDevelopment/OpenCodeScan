# Product Requirements - OpenCodeScan

## 1. Purpose
OpenCodeScan is a Qt desktop application for static analysis of C and C++ projects. It scans source code without executing binaries and reports potential issues with actionable diagnostics.

## 2. Goals
- Provide a fast desktop workflow for local code scanning.
- Surface diagnostics with clear severity, location, and rule identifiers.
- Support Windows and Linux with one codebase and CMake build flow.
- Keep architecture extensible so new analysis rules can be added safely.

## 3. Target Users
- C/C++ developers working on native applications.
- Small to medium teams that need lightweight local static analysis.
- CI maintainers who need exportable scan artifacts.

## 4. In Scope (MVP)
- Open and scan a local project directory.
- Configure include paths, macro definitions, and ignored paths.
- Run rule-based analysis and show diagnostics in a Qt UI.
- Filter diagnostics by severity, rule ID, file path, and text search.
- Export results as JSON and HTML reports.

## 5. Out of Scope (Initial Release)
- Cloud-hosted scanning service.
- Full IDE plugin suite.
- Automatic source code rewriting.
- Multi-language analysis beyond C/C++.

## 6. Functional Requirements

### 6.1 Project Input
- User can choose a root folder for analysis.
- User can add include directories and preprocessor defines.
- User can save and reload scan profiles.

### 6.2 Analysis Workflow
- Analyzer discovers C/C++ files under selected root.
- Analyzer parses translation units using configured options.
- Enabled rules execute and produce diagnostics.
- User can cancel a running scan.

### 6.3 Diagnostics
Each diagnostic must include:
- Rule ID
- Severity (`Info`, `Warning`, `Error`, `Critical`)
- File path and line/column
- Human-readable message
- Optional remediation hint

### 6.4 UI
Main window includes:
- Project and configuration panel
- Rule enable/disable panel
- Scan controls (`Start`, `Cancel`)
- Results table/tree
- Diagnostic detail pane

### 6.5 Reporting
- Export complete diagnostics to JSON.
- Export summary report to HTML.
- Preserve latest run metadata for quick reopen.

## 7. Acceptance Criteria (MVP)
- App launches on Windows and Linux with Qt 6.
- User can scan a sample C++ project and see diagnostics.
- Filters reduce visible diagnostics in real time.
- JSON and HTML exports produce valid files.
- Cancel action stops analysis without freezing UI.

## 8. Milestones
1. Baseline app shell + project loading + mock diagnostics.
2. First real parser integration + initial rule pack.
3. Filtering, export, and profile persistence.
4. Cross-platform QA and packaging hardening.

