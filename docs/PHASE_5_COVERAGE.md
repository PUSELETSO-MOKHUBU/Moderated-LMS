# Phase 5: Business-Logic Coverage Report

## Measurement definition

- **Metric:** executable line coverage (gcovr).
- **Denominator:** C++ implementation sources under `src/`, excluding `src/main.cpp` (console presentation).
- **Included business logic:** SQLite persistence/access, file storage, late-penalty Strategy, password hashing, and domain models.
- **Required threshold:** at least 70% covered lines.
- **Instrumentation:** GCC/Clang `--coverage`, Debug configuration, `-O0`.
- **Enforcement:** `.github/workflows/ci.yml` runs gcovr with `--fail-under-line 70`; the job fails when the measured result is below the threshold.

## Measured result

**Measured result: 91.9% (852/927 executable lines), passing the 70% requirement by 21.9 percentage points.** The instrumented six-test run used GCC 14.2.0 and gcovr 8.8 on Windows x64. The report also measured 99.3% function coverage and 46.9% branch coverage; only line coverage has a required threshold.

| Source file | Lines covered | Lines valid | Line coverage |
|---|---:|---:|---:|
| `src/Database.cpp` | 614 | 669 | 91.8% |
| `src/FileStorage.cpp` | 67 | 77 | 87.0% |
| `src/LatePenaltyStrategy.cpp` | 9 | 9 | 100.0% |
| `src/PasswordHasher.cpp` | 67 | 77 | 87.0% |
| `src/models/DomainModels.cpp` | 95 | 95 | 100.0% |
| **Business-logic total** | **852** | **927** | **91.9% (threshold: >= 70%)** |

## Reproduction

Run the CMake configure, build, and CTest commands in `PHASE_5_TEST_PLAN.md`, then:

```powershell
gcovr --root . --filter 'src/' --exclude 'src/main.cpp' --print-summary --fail-under-line 70 --xml-pretty --xml coverage.xml --html-details coverage.html
```

The local generated XML and HTML report are in the ignored `build-phase5-gcc` directory. CI retains its generated XML and HTML reports as an artifact, including per-file and aggregate coverage. Attach the corresponding CI artifact after the workflow's first remote run.
