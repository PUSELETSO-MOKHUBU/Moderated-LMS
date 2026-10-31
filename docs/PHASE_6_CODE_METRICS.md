# Phase 6: Code Metrics

## Measurement method

Metrics were collected on 2026-10-31 with Lizard 1.24.3 from the C++ implementation, public headers, and test source. NLOC is Lizard's non-comment source-line count; it is not raw physical lines or a measure of delivered functionality. Cyclomatic complexity (CCN) is reported per function. Test/fixture and UI code are shown separately in the hotspots so their complexity is not mistaken for business-logic complexity.

Reproduce the analysis from the project directory:

```powershell
python -m pip install lizard
python -m lizard src include tests -l cpp -C 999
```

## Size summary

| Scope | Files | Non-comment source lines (NLOC) | Functions |
|---|---:|---:|---:|
| Production C++ implementation (`src/`) | 6 | 1,745 | 162 |
| Public interfaces (`include/`) | 9 | 328 | 1 |
| Automated tests (`tests/`) | 1 | 491 | 9 |
| **Total measured** | **16** | **2,564** | **172** |

## Key complexity measures

| Function | File and line | CCN | NLOC | Assessment |
|---|---|---:|---:|---|
| `runAdmin` | `src/main.cpp:175` | 32 | 90 | Largest production UI branch; primary refactoring candidate. |
| `runStudent` | `src/main.cpp:433` | 24 | 79 | Role menu combines display, upload, submission and results; primary refactoring candidate. |
| `runLecturer` | `src/main.cpp:266` | 21 | 85 | Role menu coordinates assignment, upload, extension and marking flows. |
| `runHod` | `src/main.cpp:352` | 21 | 80 | Role menu coordinates moderation and release flows. |
| `FileStorage::storeDocument` | `src/FileStorage.cpp:57` | 14 | 35 | Validation and safe storage branches; close to the usual CCN review threshold of 15. |
| `Database::updateUser` | `src/Database.cpp:429` | 13 | 34 | Validation and optional-password update branches. |
| `PasswordHasher::verify` | `src/PasswordHasher.cpp:64` | 13 | 31 | Encoded-hash parsing and verification branches. |
| `Database::saveMark` | `src/Database.cpp:766` | 11 | 56 | Transactional state/mark/audit workflow; retain focused regression coverage. |
| Integration/acceptance `TEST_CASE` | `tests/test_lms.cpp:254` | 43 | 259 | One long end-to-end fixture is complex; split into smaller scenario fixtures when expanding acceptance coverage. |

The mean CCN across measured functions was 3.3. The elevated console-menu functions and large integration case are documented refactoring risks, not analyzer-reported defects. Do not lower test coverage or weaken authorization checks to reduce a metric.

## Interpretation and follow-up

The business-logic line-coverage result is separately measured and recorded in `PHASE_5_COVERAGE.md`. Metrics are a maintainability signal, not an acceptance criterion on their own. Future changes should prefer extracting menu actions and reusable acceptance fixtures while preserving role boundaries and behavior; rerun the unit, integration, coverage, and static-analysis gates after any such refactor.
