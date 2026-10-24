# Correction to the Phase 1–3 Documents and Start of Implementation

## 1. Purpose of this correction document

This document records the missing items that should be added to the earlier project documentation so that the work aligns fully with the final assignment brief and the software engineering lifecycle expected by the course.

## 2. What the final assignment requires that the earlier documents should include more clearly

The final project brief requires a full software engineering lifecycle, not only a conceptual design. The following areas are essential and should be present more explicitly in the project documentation:

### 2.1 Verification and validation as a core lifecycle activity
The project must follow the Waterfall model with V-model validation. This means that every major stage should be linked to a verification activity.

Missing but necessary elements:
- requirements validation against stakeholders and project goals
- design review against the requirements
- implementation verification using compilation and unit tests
- integration testing for database and business logic
- system testing for role-based workflows and moderation rules
- documented acceptance checks for each main business process

### 2.2 Traceability across the lifecycle
The project should clearly trace each requirement to:
- a design component
- a class or module
- a test case
- an implementation check

This ensures that the system is not just built, but demonstrably satisfies the specification.

### 2.3 Quality assurance and static analysis
The assignment explicitly mentions static analysis, test frameworks, and quality practices. These should be included in the phase documentation or implementation notes.

Required additions:
- use of a static analyser such as `cppcheck` or `clang-tidy`
- use of a unit testing framework such as GoogleTest or Catch2
- a simple test strategy document
- automated build verification steps

### 2.4 Configuration management and change control
The final assignment expects repository-based configuration management, not just a final code submission.

The project should include:
- a Git repository with meaningful commit history
- versioned work products
- clear branch or issue tracking discipline
- a record of changes linked to requirements or design items

### 2.5 Security and audit requirements as design-for-implementation concerns
Although Phase 2 and Phase 3 address security, the project should go further and specify the implementation mechanism:
- password hashing rather than plaintext storage
- permission checks before sensitive actions
- append-only audit logging for mark changes
- transactional database updates and rollback behaviour

## 3. Correction to the earlier phases

### Phase 1 correction
Phase 1 is valid as a project management plan, but it should be strengthened by adding:
- a detailed week-by-week schedule
- explicit gates and acceptance criteria
- a quality assurance and testing plan
- a risk-driven implementation plan that prioritizes the Must requirements first
- a verification plan tied to the V-model

### Phase 2 correction
Phase 2 is strong and mostly complete. It should be expanded by adding:
- a requirements-to-test matrix
- an explicit validation procedure for each requirement
- a traceability table linking each requirement to design and test evidence
- the expected implementation constraints (C++17, SQLite, offline mode, security)

### Phase 3 correction
Phase 3 is well-structured and mostly correct. It should be improved by adding:
- testability considerations for each subsystem
- a component-to-test mapping
- more explicit database transaction rules
- deployment and runtime assumptions for the school environment
- a clearer statement of how the design supports the V-model verification process

## 4. Implemented system

The C++17 console implementation follows the layered design, with SQLite statements isolated in the data-access
layer and all runtime values bound through prepared statements. The implementation covers the Phase 2 Must
requirements for all four roles:

- Administrators create, edit, disable and list users; maintain departments and courses; assign lecturers; and
  enrol students.
- Lecturers manage assignments for assigned courses, grant individual extensions, view active submissions, and
  mark or revise work with feedback.
- Students see enrolled courses and assignments, submit or resubmit versioned work, and see only their own
  released marks.
- Heads of Department moderate only work in their department, optionally change marks with a reason, release
  moderated results, and view the department audit history.

An abstract polymorphic `User` hierarchy and `UserFactory` implement role creation. `StandardLatePenalty`
implements the Strategy pattern. `Database` encapsulates the Repository/DAO operations and SQLite persistence.
The late policy deducts 10% of the maximum mark per day for up to three days; later work receives zero unless
an individual extension changes the effective deadline. Resubmission is allowed only before that effective
deadline, and prior versions remain stored.

OpenSSL PBKDF2-HMAC-SHA-256 with a random salt is used for password storage. Startup upgrades older
unencoded password rows to salted hashes. Mark changes and audit inserts occur in the same transaction, and
database triggers prevent normal SQL updates or deletes of audit records. Coding and build conventions are in
[CODING_STANDARD.md](./CODING_STANDARD.md).

## 5. Verification

The CMake project defines the application and a CTest integration suite. The suite exercises credential hashing,
all four roles, role denial, course scoping, extensions, versioned resubmission, late-penalty boundaries,
zero-after-three-days behavior, hidden marks, moderation, release, audit records, and account disablement.
The C++17 sources and tests were also compiled with the available Windows GCC toolchain using `-Wall`,
`-Wextra`, `-Wpedantic`, and `-Werror`; the CMake build and CTest integration suite passed. The suite covers
role access, department-scoped HoD assignment/mark visibility, extensions, resubmission, late penalties,
moderation, release, audit, and document uploads. An end-to-end check against the existing project database
also repaired the demo HoD department assignment and verified visibility of its previously marked work.
Legacy password migration and console uploads of DOCX/PDF files were tested without exposing credentials.

## 6. Conclusion

The implementation now covers the Must workflows, applies the defined academic rules, persists records through
SQLite, and provides automated integration checks. Phase 1–3 traceability and broader stakeholder acceptance
evidence remain useful project documentation improvements, but they do not block the implemented core system.
