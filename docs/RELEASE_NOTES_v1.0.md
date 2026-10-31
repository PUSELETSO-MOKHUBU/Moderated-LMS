# Moderated LMS v1.0 Release Notes

**Release tag:** `v1.0`
**Project version:** 1.0.0
**Release scope:** CS4433 Final Project, Phase 7 deployment

## Included

- Role-scoped console workflows for Administrator, Lecturer, Head of Department and Student.
- Persistent SQLite storage, salted PBKDF2-HMAC-SHA-256 password hashes, and managed assignment/submission document storage.
- Course assignments and enrolment, assignment creation, individual student deadline extensions, submissions/resubmissions, late penalties, marking, HoD moderation/release and mark audit history.
- Automated Catch2 tests and CI build/test/coverage/static-analysis checks. See [Phase 5 test results](PHASE_5_TEST_RESULTS.md) and [Phase 6 configuration evidence](PHASE_6_CONFIGURATION_MANAGEMENT.md).
- Technician installation and per-role operating guidance in this documentation set.

## Deployment and operating notes

This is a Windows-oriented C++17 console program using SQLite files in its current working directory. Use a writable, access-controlled local data directory and keep `lms.db` with the complete `uploads` directory. Back up these files together after closing the application. This release does not provide a hosted server, remote browser access, centralized identity, or a multi-workstation service.

The first-run demonstration accounts are for setup/testing only. Change or disable them before real use. Apply the school's access-control and backup policies.

## Verification

The Phase 7 release must be built and tested from the release commit before the `v1.0` tag is applied. The CI run and final commit/tag are recorded in the Phase 7 release evidence.
