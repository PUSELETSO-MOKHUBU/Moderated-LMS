# Phase 5: Test Results

## Test execution record

| Run | Environment | Command / evidence | Result |
|---|---|---|---|
| Baseline (before Catch2 migration) | Windows, existing MinGW build | `ctest --test-dir build --output-on-failure` | Passed: 1/1 existing workflow test. This is baseline evidence only, not the final Phase 5 result. |
| Final automated run | Windows x64; GCC 14.2.0 (MSYS2 UCRT64); CMake/CTest 4.4.4; gcovr 8.8; 2026-10-31. | Coverage-instrumented Debug build; `ctest --test-dir build-phase5-gcc --output-on-failure --output-junit build-phase5-gcc/test-results.xml`. | Passed: 6/6 discovered test cases; 100% passed. |

## Test-case results

| Case | Classification | Result | Notes |
|---|---|---|---|
| Late-penalty rules cover equivalence classes and boundaries | Unit | Passed | 0–4 late days, mark floor, multiple maximum marks, and invalid inputs. |
| Role permissions and factory preserve role boundaries | Unit | Passed | All four roles, every permission class, and unknown-role rejection. |
| Domain records initialize, expose, and update their values | Unit | Passed | Constructors, getters, and setters for course, assignment, submission and audit models. |
| Password hashing rejects invalid input and verifies encoded hashes | Unit | Passed | Salted encoding, correct/wrong/empty password and malformed hash. |
| Document storage enforces supported formats and size boundaries | Unit | Passed | 25 MiB accepted; 25 MiB + 1 byte rejected; unsupported, missing, empty and out-of-scope paths rejected. |
| Role-scoped LMS workflow persists and releases moderated results | Integration / system / acceptance | Passed | Administrator CRUD; role/department scoping; extensions; versioned and deadline-boundary submissions; marking, moderation, release, audit; invalid and unauthorized operations. |

## Exit assessment

All six local test cases passed and measured business-logic coverage exceeds 70%. The GitHub Actions workflow is configured but has not yet run remotely; attach its first successful run and report artifact after the branch is pushed.
