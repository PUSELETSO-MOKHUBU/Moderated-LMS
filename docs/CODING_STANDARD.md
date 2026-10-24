# C++ Coding Standard

The project uses the following conventions to keep the implementation consistent:

- Compile as ISO C++17 with extensions disabled.
- Use four spaces for indentation, braces on the same line, and `PascalCase` for types.
- Use `camelCase` for functions and variables, and a trailing underscore for private data members.
- Prefer standard-library types, RAII, and `const` correctness; do not manually own SQLite statements.
- Keep SQL and SQLite handles inside the data-access layer. Bind all variable values with prepared statements.
- Validate user input and domain constraints before persistence. Report operational failures with exceptions
  and catch them at the console boundary.
- Copy document uploads into application-managed storage, allow only documented formats, enforce a size
  limit, and remove staged files if their database write fails.
- Open persisted files through the operating system's file association API; do not build shell command
  strings from user-provided paths.
- Enforce role permissions in the application/domain layer and repeat sensitive ownership checks in repository
  operations that modify data.
- Keep multi-row workflow changes transactional. Append audit records in the same transaction as the mark update.
- Do not store credentials in plaintext. Use OpenSSL's PBKDF2-HMAC-SHA-256 implementation with a
  cryptographically random 128-bit salt, 200,000 iterations, and constant-time digest comparison.
- Keep functions focused; use named domain types and avoid unexplained numeric literals.
- Build with CMake, enable compiler warnings, and run the CTest suite before delivery.

OpenSSL is used because PBKDF2-HMAC-SHA-256 is a recognized password-based key derivation function,
the library provides a cryptographically secure salt source, and it is already a commonly packaged
cryptographic dependency. Each database hash stores its algorithm, work factor, salt, and derived key so
that credentials can be verified and parameters can be upgraded in the future.
