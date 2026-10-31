# Phase 8 - CR1 Regression Test Results

## Local release-candidate run

| Environment | Command | Result |
|---|---|---|
| Windows x64, GCC 14.2.0, CMake 4.4.4; CMake configured with `BUILD_TESTING=ON`, `ENABLE_GCC_ANALYZER=ON` and `ENABLE_COVERAGE=ON` | `cmake --build build --parallel`; `ctest --test-dir build --output-on-failure` | Build passed. All 6/6 CTest cases passed (100%); total test time 14.25 seconds. |

The existing test cases were rerun, not replaced. The role-scoped LMS workflow case now also exercises CR1 (AC-10): lecturer authorization and course scope, an unenrolled student's isolation and later enrolment, class/individual deadline precedence, rejection of a shortened class deadline and post-marking change, active status refresh, resubmission using the class deadline, and mark calculation using the effective deadline.

| Test case | Result |
|---|---|
| Late-penalty rules cover equivalence classes and boundaries | Passed |
| Role permissions and factory preserve role boundaries | Passed |
| Domain records initialize, expose, and update their values | Passed |
| Password hashing rejects invalid input and verifies encoded hashes | Passed |
| Document storage enforces supported formats and size boundaries | Passed |
| Role-scoped LMS workflow persists and releases moderated results, including CR1 assertions | Passed |

An isolated console smoke check also passed: a lecturer created an assignment and granted a class extension, then the enrolled student saw the later effective deadline and the `extension applied` label in the assignment list. The check used a temporary data directory, which was removed afterward.

## Hosted release checks

The hosted GitHub Actions run, coverage artifact, merged commit and `v1.1` tag are added here after the release-candidate PR passes and is merged. A v1.1 release is not complete until those hosted checks are green.
