# Phase 5: Verification and Validation Test Plan

## 1. Purpose and scope

This plan defines verification evidence for the Moderated LMS against the Phase 2 functional requirements and use cases, the Phase 3 design, and the Phase 5 assignment criteria. The test target is the C++ business-logic library (`src/Database.cpp`, `src/FileStorage.cpp`, `src/LatePenaltyStrategy.cpp`, `src/PasswordHasher.cpp`, and `src/models/DomainModels.cpp`). The interactive console (`src/main.cpp`) is excluded from the business-logic coverage denominator; its role workflows are exercised through the integrated application services, database, and domain model.

The test executable uses Catch2. CTest discovers and runs each Catch2 test case. Integration and system tests use a unique temporary SQLite database and temporary upload directory; they do not modify the checked-in demonstration database or application uploads.

## 2. V-Model verification and validation strategy

| V-Model level | Design/specification artefact verified | Verification activity and evidence | Entry criteria | Exit criteria |
|---|---|---|---|---|
| Unit | Phase 3 class design and business rules; Phase 2 BR-02, user roles, password and upload requirements | Catch2 tests for the late-penalty Strategy, role permission matrix/factory, password encoding and validation, and document storage limits. CTest/JUnit and line-coverage report are retained. | Source and interfaces compile; test data and expected results are specified; build dependencies resolve. | All unit tests pass; invalid and boundary inputs are asserted; no unexplained test failures; line coverage target is met in CI. |
| Integration | Phase 3 database/repository and layered design; Phase 2 FR-01–FR-18 and BR-02/BR-04 | Tests exercise role authentication, permission checks, SQLite persistence, extensions, versioned submission, marking, moderation, release, and audit as connected components. | Unit-level dependencies are buildable; each test has an isolated temporary database. | Assertions confirm persisted state and role scoping; transaction-backed workflows complete; denied and invalid operations fail explicitly; temporary data is cleaned. |
| System | Phase 2 user workflows and Phase 3 system design | Complete role-to-role workflows run across domain, database, hashing, late-penalty, and storage components. Observable outputs include visible queues, released marks, file paths, and audit records. | Integration suite passes; demo accounts and database schema can be initialized in an isolated test directory. | Each critical workflow and selected exception flow reaches its expected end state; unauthorized or unreleased information is not exposed. |
| Acceptance | Phase 2 UC-01 Submit assignment, UC-02 Mark submission, UC-03 Moderate and release marks; role-specific requirements FR-03–FR-18 | Acceptance scenarios below are executable assertions in the Catch2 suite, tagged `[system][acceptance]`, and run by CTest and CI. | User-story/use-case preconditions are met in a clean database; expected business outcomes are documented. | All Must acceptance scenarios pass; no critical/high open defects; results and known limitations are recorded. |

### Entry and exit control

Testing starts from a clean CMake build with SQLite and OpenSSL available. CI configures with `BUILD_TESTING=ON` and `ENABLE_COVERAGE=ON`, compiles the project, executes all discovered CTest cases, and rejects business-logic line coverage below 70%. A failed configure, build, test, or coverage gate is a failed verification run; it must not be reported as a pass. A release candidate is eligible to exit Phase 5 only when the full test job is green, all required acceptance cases pass, and any open high-severity issue has been resolved or formally accepted.

## 3. Equivalence partitioning and boundary-value analysis

| Input/rule | Equivalence partitions | Boundary values and expected behavior | Test evidence |
|---|---|---|---|
| Days late and late deduction | On time; 1–3 days late; more than 3 days late; negative input | 0 days gives no deduction; 1 day deducts 10%; 2 days 20%; 3 days 30%; 4+ days is capped at 30% for penalty and final mark is zero; negative days are rejected. | `Late-penalty rules cover equivalence classes and boundaries` |
| Mark values | Valid mark; negative; above maximum; non-finite value | 0 and maximum are valid; negative, above-maximum, NaN/infinity values are rejected by the relevant business rules. A deduction cannot produce a negative final mark. | Late-penalty unit test; marking integration scenario |
| Assignment deadline | Valid local `YYYY-MM-DD HH:MM`; malformed; impossible calendar date; before-deadline and just-after-deadline submissions | A valid deadline is normalized; malformed or impossible date/time is rejected. A submission before the effective deadline has no penalty; a submission just after it is late. | Integration checks use a deadline five minutes ahead and one minute in the past; invalid calendar date is rejected. |
| Individual and class extension | Assigned lecturer; individual student or whole assignment course; wrong role/course/student; date not later than original; shortened class update; valid later date | Individual extension applies only to its named student. A class extension applies to current and future enrollees. Effective deadline is the latest of original, class and individual deadlines; marked results cannot be changed. | UC-01 extension and CR1 AC-10 integration assertions |
| Document upload | Supported/unsupported extension; regular/missing/empty file; valid size/oversized file | Non-empty supported file at 25 MiB is accepted; 25 MiB + 1 byte is rejected. Missing, whitespace path, empty, unsupported, or out-of-scope stored path is rejected. | `Document storage enforces supported formats and size boundaries` |
| Authentication and authorization | Valid/invalid credentials; active/disabled identity; allowed/denied role | Correct credentials authenticate; wrong credentials do not. Inactive or stale identity and forbidden role actions are denied. Password hashes are salted and plaintext is not accepted as an encoded hash. | Role unit test; authentication and authorization integration scenarios |

## 4. Requirements-to-test traceability

| Requirement/use case | Verification case(s) | Level | Observable acceptance result |
|---|---|---|---|
| FR-01–FR-02: authentication and role-scoped access | Role permissions and factory; role authentication; denied student assignment creation; disabled account check | Unit, integration, acceptance | Correct roles authenticate; forbidden or stale sessions cannot perform protected operations. |
| FR-03–FR-05: administrator account, department, course and enrolment management | AC-06 administrator CRUD and course-scope workflow | Integration, system, acceptance | User/password, department, course, lecturer-assignment, and enrolment changes persist; each role sees only its permitted courses. |
| FR-06–FR-07: lecturer assignment and individual/class extension | UC-01 workflow and CR1 AC-10 | Integration, system, acceptance | Individual extension updates only its student; class extension applies to course enrollees; each student's effective deadline is displayed. |
| FR-08–FR-12, FR-19, BR-02, BR-04: view, submit/resubmit, deadline and late handling | UC-01 workflow; late-penalty unit boundaries; late submission integration scenario | Unit, integration, acceptance | Versions persist; extension and late rules are applied; work over three days late receives zero unless extended. |
| FR-13–FR-14, FR-17: mark, feedback and audit | UC-02 workflow | Integration, system, acceptance | Lecturer feedback and computed mark persist; initial and revised marks are audited; unreleased marks remain hidden. |
| FR-15–FR-17: department-scoped moderation, release and audit | UC-03 workflow | Integration, system, acceptance | HoD sees departmental work only; moderation is recorded; result is visible to the student only after release. |
| FR-18: student released-result access | UC-03 workflow | Integration, acceptance | A student can view their released result, and cannot view another student's result or unreleased work. |
| FR-19: submission state tracking | UC-01–UC-03 workflow and submission/mark/moderation/release queue assertions | Integration, acceptance | Tests observe submitted, late, marked, moderated, and released states in their role-specific queues. |
| FR-20: authorized search and viewing | Administrator lists; lecturer/student/HoD course, assignment and submission queries | Integration, system, acceptance | Records are returned to authorized roles and filtered by role, enrolment, course assignment, or department. |
| NFR-04 and NFR-08: password protection and auditability | Password unit test; authentication and audit integration scenarios | Unit, integration | Salted PBKDF2 hash verifies credentials; audit entries capture mark changes. |
| Upload validation and persistence | Document-storage unit test and assignment/submission integration scenario | Unit, integration | Supported files are copied to controlled storage; invalid inputs and size overflow are rejected. |

## 5. System and acceptance scenarios

The executable scenarios are kept in `tests/test_lms.cpp`; focused cases carry `[unit]` tags and the database-backed role-to-role workflow carries `[integration][system][acceptance]` tags.

| ID | Use case / actor | Preconditions and action | Expected result / exception flow |
|---|---|---|---|
| AC-01 | FR-01–FR-02, all roles | Initialize a clean database, seed four demo roles, authenticate each role and attempt a student-only forbidden assignment-creation action. | Each seeded role maps to the correct domain type; invalid credentials fail; unauthorized action is rejected. |
| AC-02 | UC-01, lecturer/student | Lecturer creates an assignment and grants an individual extension; student views the assignment, submits twice, then inspects their versions. | Effective deadline changes for that student; resubmission creates a new active version while earlier versions remain stored. |
| AC-03 | UC-01, student/lecturer | Submit against a deadline more than three days in the past; lecturer marks it. | Submission is marked late and the calculated final mark is zero. |
| AC-04 | UC-02, lecturer/student | Lecturer marks the latest submission with feedback; student checks results before release. | Mark and feedback are stored and queued for moderation; result is hidden before release. |
| AC-05 | UC-03, HoD/student | HoD revises a mark with a reason, checks the release queue, releases it, and student views the result. | Mark and reason are audited; moderated result is not visible until release and is visible afterward. |
| AC-06 | FR-03–FR-05, administrator/lecturer/student | Administrator creates and edits a department, course, lecturer, and student; assigns/enrols those users; updates a password; then each role queries its courses. | Changes persist; duplicate assignment/enrolment and invalid account creation fail; lecturers, HoD, students, and administrators receive the correct scope. |
| AC-07 | FR-01, administrator/student | Disable the student's account and use the previously authenticated user object. | The stale session is rejected on the next protected operation. |
| AC-08 | File validation, lecturer/student | Attempt an unsupported-format upload, missing source upload, missing document open, oversized file, and path outside controlled storage. | Each invalid operation reports failure; no invalid upload is accepted. |
| AC-09 | FR-01, all roles | Exercise unit-level role permission/factory, domain record, password, late-penalty and document-size equivalence classes. | Each role boundary, invalid-input exception, and specified numeric/file-size boundary behaves as expected. |
| AC-10 | CR1, lecturer/student | Lecturer grants a class extension, updates it without shortening, then students view and submit against the effective deadline. Exercise an unenrolled/newly enrolled student, a longer individual extension, an existing active submission, an unauthorized lecturer and an already-marked assignment. | Only authorized assigned lecturers can grant the change; the class deadline is visible to all current/future enrollees, not outsiders; the later individual deadline wins; status and mark penalty use the effective deadline; invalid or post-marking changes fail without altering data. |

These are automated system-level acceptance checks through the application service/domain and persistence layers. They do not claim a separate human usability assessment or automated keystroke-level test of the console menus.

## 6. Automation, execution, and reproducibility

The project pins Catch2 v3.7.1 using CMake `FetchContent`; CTest discovers individual Catch2 test cases. The CI workflow at `.github/workflows/ci.yml` builds on Ubuntu for every push and pull request, runs CTest with JUnit output, measures business-logic coverage with gcovr, and fails below the required 70% line threshold.

Reproduce the CI checks from the `ModeratedLMS` project directory:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure --output-junit test-results.xml
gcovr --root . --filter 'src/' --exclude 'src/main.cpp' --print-summary --fail-under-line 70 --xml-pretty --xml coverage.xml --html-details coverage.html
```

The first configure requires access to the pinned Catch2 source unless it is already cached by CMake. GCC or Clang is required for the coverage option. CI uploads JUnit XML, analyzer output, and HTML/XML coverage reports as the `ci-quality-reports` artifact.

## 7. Known limitations and risk controls

- CI currently verifies the Linux/GCC build; the documented Windows build remains useful but is not a second CI matrix job.
- `FileStorage::openDocument` launches a registered Windows application. Automated tests verify invalid/missing-file rejection; they do not launch a desktop application.
- Calendar/deadline calculations use local time. Automated tests avoid relying on a particular CI timezone by using dates safely in the past or future; an exact real-time midnight transition is not simulated.
- Coverage excludes the console presentation file, not domain or persistence logic. The coverage report records the exact source filters and measured percentage.
