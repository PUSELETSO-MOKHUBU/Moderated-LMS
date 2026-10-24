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

## 4. Implementation start point

The project is now moving into implementation. The implementation will begin with a minimal but realistic C++17 foundation using SQLite and a layered structure consistent with the design:

- Domain models for User, Course, Assignment, Submission, and Audit records
- Data access layer for SQLite initialization and persistence
- Service layer for role checks, late penalty evaluation, and submission flow
- Console-based entry point for demonstration and verification

This implementation is intended to satisfy the most critical Must requirements first, while preserving the structure needed for later validation and extension.

## 5. Implementation priorities

1. Database schema initialization
2. User and role model creation
3. Assignment and submission domain objects
4. Role-based access checks
5. Late penalty and extension logic
6. Mark auditing and release workflow
7. Testable console interaction and validation

## 6. Conclusion

The earlier phases are fundamentally correct, but they need stronger lifecycle traceability, verification, and quality assurance content to match the project brief fully. The implementation phase begins from this corrected understanding and follows the design direction already established in Phase 2 and Phase 3.
