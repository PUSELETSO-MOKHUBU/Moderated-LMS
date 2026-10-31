# Phase 5: Defect Log

This log records defects found during Phase 5 verification and their retest status. Keep resolved entries so the test-and-fix history remains auditable. Severity: Critical / High / Medium / Low.

| ID | Date found | Severity | Summary | Cause | Fix | Retest result | Status |
|---|---|---|---|---|---|---|---|
| D-01 | 2026-10-31 | Low | Coverage-instrumented build initially failed to link the application executable. | `lms_core` received `--coverage`, but the application target did not link the gcov runtime. | Added the coverage linker option to `ModeratedLMS` when coverage is enabled. | Full build passed; all 6/6 tests passed; measured line coverage 91.9%. | Closed |
| D-02 | 2026-10-31 | Low | Test source failed to compile after adding the deadline-boundary helper. | The anonymous namespace was missing its closing brace. | Added the missing namespace close. | Full build passed; all 6/6 tests passed; measured line coverage 91.9%. | Closed |
| D-03 | 2026-10-31 | Low | Hosted Linux CI failed to compile date/time conversion. | `Database.cpp` called the Windows-only `localtime_s` API on a POSIX runner. | Added a thread-safe platform wrapper using `localtime_s` on Windows and `localtime_r` on POSIX, with explicit conversion-failure handling. | Local Windows build and all 6/6 tests passed; [hosted CI run 2](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/actions/runs/41245914776) passed build, tests, and coverage. | Closed |

No product-behavior defects remained or were identified by the final automated run.

## Retest policy

For each defect, record the test case that exposed it, the root cause, the minimal correction, and the passing retest (including regression suite result). Do not mark an issue resolved without a passing retest. Critical and High defects block Phase 5 exit.
