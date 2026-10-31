# Phase 6: Static Analysis Report

## Tool and scope

- **Analyzer:** GCC 14.2.0 `-fanalyzer`.
- **Scope:** production implementation targets `lms_core` and `ModeratedLMS`, including persistence, storage, password, model, and console code.
- **Analysis configuration:** CMake option `ENABLE_GCC_ANALYZER=ON`; adds `-fanalyzer -Werror`. The `analyzer-use-of-uninitialized-value` diagnostic remains visible as a warning instead of an error because GCC 14.2 reports it for standard-library string/path operations even in isolated minimal programs. All other diagnostics remain errors.
- **Automation:** the C++ CI workflow enables this option for every push and pull request and uploads the build log.

The analyzer was selected because GCC is the project's available Windows compiler and its built-in interprocedural analyzer can be used without a separate, unverified tool installation. Both the application and business-logic library compile with the project's warning flags. Catch2 itself is a third-party dependency and is not included in the analyzer scope.

## Local execution and result

Run from the `ModeratedLMS` project directory with GCC and the project's SQLite/OpenSSL dependencies installed:

```powershell
cmake -S . -B build-qa -G "MinGW Makefiles" -DBUILD_TESTING=ON -DENABLE_GCC_ANALYZER=ON -DOPENSSL_ROOT_DIR=C:/msys64/ucrt64
cmake --build build-qa --parallel 4
```

**Result:** the MSYS2 GCC 14.2.0 application and library build completed successfully with the analyzer enabled. It emitted 38 `analyzer-use-of-uninitialized-value` diagnostic events, traced to six call paths in `FileStorage.cpp` (`std::filesystem::u8path`/path conversion) and `main.cpp` (standard-library string operations). I reproduced this diagnostic with minimal programs containing only `std::filesystem::u8path` and standard-library string concatenation; traces terminate in MinGW libstdc++ internals. This known GCC 14.2 standard-library false-positive category remains visible in compiler output and the CI analyzer log; all other compiler/analyzer findings remain errors. No actionable project-code analyzer finding remained after manual review.

| Finding | Severity | Disposition |
|---|---|---|
| 38 `analyzer-use-of-uninitialized-value` diagnostic events in standard-library path/string call paths | False positive (verified with minimal reproducers) | Kept visible as warnings for manual triage; all other GCC compiler and analyzer diagnostics remain errors. |
| Other production-code compiler or analyzer diagnostics | None | No actionable finding remained; `-Werror` makes new diagnostics fail the build. |

This is a GCC analyzer result, not a claim that Cppcheck, clang-tidy, or a security audit was run.
