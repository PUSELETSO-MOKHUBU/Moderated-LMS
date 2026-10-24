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
  both the brief and submitted-file locations for authorised users.

The sample HoD is assigned to the General department. Existing demo databases are repaired on startup if
that demo account was created without a department. Newly created HoD users must be assigned a department;
use the administrator's edit-user menu to correct an existing account's department. The login aliases
`lecture` and `head` are accepted for the sample Lecturer and HoD accounts.

Date/time input is local time in `YYYY-MM-DD HH:MM` format. Submission and brief file paths may contain
spaces; enter the full path on one line. SQLite stores the application-managed upload path and timestamp.
