# AI Implementation Plan (Phased) - OpenCodeScan

## Goal
Implement AI capabilities in OpenCodeScan incrementally so the product remains deterministic, responsive, and cross-platform while adding meaningful developer assistance.

## Guiding Principles
- Local-first by default: source code stays on device unless user explicitly enables external services.
- Rule-grounded AI: AI suggestions must reference concrete diagnostics/rules.
- Deterministic core first: static analysis pipeline quality before AI enhancement.
- User control: AI features are opt-in, transparent, and easily disabled.

---

## Phase 1: Foundation and Architecture Baseline
**Objective:** Stabilize project structure and cross-platform build baseline.

### Deliverables
- Finalize modular folder layout from `docs/architecture/folder-structure.md`.
- Move app entrypoint into `src/app` and keep startup wiring minimal.
- Create interfaces for analysis engine, diagnostics model, and reporting.
- Add basic logging and configuration persistence skeleton.

### AI in this phase
- No generation yet.
- Prepare extension points (`IAiAdvisor`, `IHintProvider`) as empty contracts.

### Definition of Done
- Builds on Windows and Linux using CMake + Qt6.
- UI boots and can load a sample project path.
- Architecture boundaries are enforced (UI not coupled to rule internals).

---

## Phase 2: Deterministic Analysis Core
**Objective:** Build a reliable static analysis pipeline for C/C++.

### Deliverables
- File discovery and project scan orchestration.
- Translation unit configuration (include paths, defines, exclusions).
- Initial parser integration (recommended: `libclang`-based pipeline).
- Cancellation and progress reporting from worker threads.

### AI in this phase
- AI still disabled for end users.
- Collect structured context objects that future AI can consume safely:
  - Rule metadata
  - AST snippets (bounded)
  - Diagnostic traces

### Definition of Done
- Same input/config yields same diagnostics.
- Partial parse failures do not crash full scan.
- Cancellation works without freezing UI.

---

## Phase 3: Rules MVP + Quality Baseline
**Objective:** Deliver practical analyzer value before adding AI assistance.

### Deliverables
- Implement first 10-15 high-value rules across:
  - correctness
  - safety
  - maintainability
- Rule registry and severity model.
- Unit/integration tests with fixtures under `tests/`.
- JSON and HTML report output.

### AI in this phase
- Optional internal-only prototype: explanation generator behind dev flag.
- No default user exposure.

### Definition of Done
- MVP requirements in `docs/requirements/product-requirements.md` section 6.2/6.3 are met.
- False-positive rate is acceptable for selected fixtures.
- Exported reports are stable and parseable.

---

## Phase 4: AI Assist v1 (Explain + Suggest)
**Objective:** Introduce trustworthy AI assistance tied directly to diagnostics.

### Deliverables
- Add AI panel in UI for selected diagnostic.
- Provide:
  - Plain-language explanation
  - Root-cause summary
  - Suggested remediation steps
- Confidence indicator and "why this suggestion" section.
- Strict grounding: AI must include rule ID and file/line references.

### Recommended Runtime Options
1. Local model runtime first (`llama.cpp` or ONNX Runtime).
2. Optional external provider adapter behind explicit opt-in.
3. Unified provider interface to switch backends.

### Risk Controls
- Block free-form suggestions with no diagnostic context.
- Label low-confidence output clearly.
- Add policy checks for unsafe or non-actionable suggestions.

### Definition of Done
- AI output is toggleable per user settings.
- Suggestions are anchored to existing diagnostics.
- App remains responsive while AI runs asynchronously.

---

## Phase 5: AI Assist v2 (Prioritization + Batch Guidance)
**Objective:** Use AI to improve triage flow, not replace rule engine.

### Deliverables
- "Top Issues First" ranking by impact and confidence.
- Similar-diagnostic clustering to reduce noise.
- Batch summary per scan:
  - key risk themes
  - hot files/modules
  - recommended fix order

### Safety/Trust Additions
- Explain ranking factors (severity, frequency, spread).
- Never suppress core diagnostics silently.
- Keep deterministic analyzer output separate from AI ranking metadata.

### Definition of Done
- Users can sort by deterministic fields or AI-priority field.
- Batch summary links back to concrete findings.
- No analyzer regressions in performance-critical scenarios.

---

## Phase 6: Production Hardening and Release
**Objective:** Make AI features robust for real-world use.

### Deliverables
- Cross-platform CI with Windows/Linux matrix.
- Privacy defaults and clear consent UX for any remote inference.
- Telemetry-off default; optional diagnostics feedback channel.
- Packaging and release playbook updates.

### Operational Requirements
- Version AI prompts/templates with changelog.
- Add regression suite for AI-grounded outputs using golden samples.
- Track quality metrics (acceptance, override rate, hallucination incidents).

### Definition of Done
- Release checklist passes on both target platforms.
- AI features can be disabled globally with no core feature loss.
- Documentation updated with limitations and support boundaries.

---

## Recommended Timeline (Practical)
- Phase 1-2: 2-4 weeks
- Phase 3: 2-3 weeks
- Phase 4: 2-4 weeks
- Phase 5: 2-3 weeks
- Phase 6: 1-2 weeks

(Adjust by team size and parser/rule complexity.)

## Immediate Next Actions
1. Complete Phase 1 structure refactor (`main.cpp` relocation + CMake source updates).
2. Choose parser strategy (`libclang` direct is recommended for deterministic control).
3. Define first 10 MVP rules and corresponding fixtures.
4. Add `IAiAdvisor` interface now, implementation later in Phase 4.

