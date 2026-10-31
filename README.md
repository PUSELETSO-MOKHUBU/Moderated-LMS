# Moderated LMS

A C++17 console system for the CS4433 Moderated Learning Management System. It implements the four
operational roles, role-scoped workflows, assignment submission and resubmission, late penalties,
extensions, marking, HoD moderation, controlled release, and persistent mark auditing.

## Requirements and build

Dependencies:

- CMake 3.16 or newer
- SQLite 3
- OpenSSL 3 (or compatible OpenSSL providing PBKDF2-HMAC-SHA-256)
- A C++17 compiler

Build and test from this directory:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows, `build.bat` performs these steps and launches the console application after successful tests.

## Deployment and user guides

The [UML deployment diagram](docs/diagrams/DEPLOYMENT_DIAGRAM.puml) shows the v1.0 single-workstation architecture. Use the [technician installation guide](docs/INSTALLATION_GUIDE.md) to build, install, configure the writable data folder, and set up backups. Role-specific operating instructions are available for the [Administrator](docs/USER_MANUAL_ADMINISTRATOR.md), [Lecturer](docs/USER_MANUAL_LECTURER.md), [Head of Department](docs/USER_MANUAL_HOD.md), and [Student](docs/USER_MANUAL_STUDENT.md). See the [v1.0 release notes](docs/RELEASE_NOTES_v1.0.md).

## Verification and validation

The automated suite uses Catch2 and CTest. The [Phase 5 test plan](docs/PHASE_5_TEST_PLAN.md) maps unit,
integration, system, and acceptance tests to requirements and V-Model levels. The [test results](docs/PHASE_5_TEST_RESULTS.md),
[coverage report](docs/PHASE_5_COVERAGE.md), and [defect log](docs/PHASE_5_DEFECT_LOG.md) record verification evidence.
CI runs the GCC static analyzer, builds the project, executes the suite on pushes and pull requests, and enforces
at least 70% line coverage of business-logic sources (excluding the console presentation layer). See the
[Phase 6 configuration strategy](docs/PHASE_6_CONFIGURATION_MANAGEMENT.md), [static-analysis report](docs/PHASE_6_STATIC_ANALYSIS.md),
[code metrics](docs/PHASE_6_CODE_METRICS.md), and [self-review record](docs/PHASE_6_SELF_REVIEW.md).
Configure with `-DENABLE_COVERAGE=ON` to reproduce the coverage run locally; this option requires GCC or Clang
and `gcovr`. Configure with `-DENABLE_GCC_ANALYZER=ON` to run GCC's analyzer and promote warnings to errors.

## Architecture and security

The application uses a layered structure: console presentation, polymorphic role/domain types, a
late-penalty Strategy, and a SQLite data-access layer. Database statements are prepared and parameterised;
the UI and domain classes do not use SQLite directly. `User` is abstract, and `UserFactory` creates the
Administrator, Lecturer, HeadOfDepartment, and Student types from authenticated database rows.

Passwords are encoded with OpenSSL PBKDF2-HMAC-SHA-256 using a random 16-byte salt and 200,000
iterations. SQLite stores only the algorithm/work-factor/salt/derived-key string, never the submitted
password. When an older starter database is opened, any legacy unencoded password values are immediately
converted to salted hashes. The password hash design and coding conventions are documented in
[CODING_STANDARD.md](docs/CODING_STANDARD.md).

Mark updates, audit inserts, and moderation changes use transactions. Audit records include actor,
timestamp, previous and new final mark, and reason; database triggers prevent update/delete of audit rows.
Students can query only their own released results.

## First-run demonstration accounts

| Role | Username | Password |
|---|---|---|
| Administrator | `admin` | `admin123` |
| Lecturer | `lecturer` | `lecturer123` |
| Head of Department | `hod` | `hod123` |
| Student | `student` | `student123` |

The accounts are inserted only when absent and passwords are hashed before insertion. Change these demo
credentials before real use. The initial database includes a General department and a demonstration course
assigned to the sample lecturer and enrolled for the sample student.

## Supported workflows

- Administrators create/edit/disable users, departments and courses, assign lecturers, and enrol students.
- Lecturers create assignments, issue/update individual extensions, inspect assigned-course submissions,
  and mark or revise work with written feedback.
- Students see only their enrolled courses and assignments, submit and resubmit before the effective
  deadline, and view only their own results after release.
- HoDs review marked work in their department, approve or change marks with a reason, release moderated
  marks, and inspect the department audit history.
- The late-penalty Strategy deducts 10% of the maximum mark per day, capped at three days. Work over
  three days late receives zero unless an individual extension moves the effective deadline.
- Assignment briefs and student submissions support PDF, DOC/DOCX, ODT, RTF, TXT/Markdown, PPT/PPTX,
  XLS/XLSX, and CSV uploads. Files are copied into `uploads/assignments` or `uploads/submissions` below
  the application working directory, capped at 25 MB, and referenced by the database. The console displays
  both the brief and submitted-file locations for authorised users. On Windows, the lecturer and HoD can
  open a submission and brief before marking/moderating; the lecturer has a separate open-brief action,
  and students can open a brief or their own submitted versions. Windows opens each file in its associated
  application.

HoD workflow: **3. Moderate marked work** lists lecturer-marked submissions. Review/approve or enter a
revised mark and reason, then explicitly confirm whether to release the result immediately. If it is not
released at that point, choose **4. Release moderated work** later. Students cannot see marks while a
submission is only marked or moderated; they become visible after release.

The sample HoD is assigned to the General department. Existing demo databases are repaired on startup if
that demo account was created without a department. Newly created HoD users must be assigned a department;
use the administrator's edit-user menu to correct an existing account's department. The login aliases
`lecture` and `head` are accepted for the sample Lecturer and HoD accounts.

Date/time input is local time in `YYYY-MM-DD HH:MM` format. Submission and brief file paths may contain
spaces; enter the full path on one line. SQLite stores the application-managed upload path and timestamp.
Uploaded documents must be non-empty files in a supported format, under 25 MB. Opening a document
requires a Windows application registered for that file format.
