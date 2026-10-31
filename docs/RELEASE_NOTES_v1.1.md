# Moderated LMS v1.1 Release Notes

**Release tag:** `v1.1`
**Project version:** 1.1.0
**Change request:** CR1, Phase 8 maintenance

## What changed

- Lecturers can grant or update a deadline extension for a whole assignment class, or continue to grant an individual extension.
- Class-wide extensions apply to all students enrolled in the assignment's course, including students who enroll later while the class-extension record exists.
- Student assignment lists show the effective deadline and label an extension when it differs from the original deadline.
- The effective deadline is the latest of the original, class-wide and individual deadlines. Submission/resubmission acceptance, submission status refresh and late-mark calculations use the same value.
- Class extensions cannot shorten a previously granted class deadline or change an assignment after an active result has been marked, moderated or released.
- Added the [CR1 impact analysis](PHASE_8_CR1_ANALYSIS.md), [updated UML design sources](diagrams/CR1_CLASS_DIAGRAM.puml), and AC-10 regression assertions in the Phase 5 test plan.
- See the [Phase 8 regression results](PHASE_8_REGRESSION_TEST_RESULTS.md) for test outcomes and release CI evidence.

## Upgrade and compatibility

The v1.1 schema adds `class_extensions` through an idempotent `CREATE TABLE IF NOT EXISTS` migration; existing v1.0 assignments, individual extensions, submissions and marks are retained. Before upgrading, close the application and back up `lms.db` and the complete `uploads` directory together. Run only v1.1 against the upgraded live data folder: v1.0 does not enforce or display class-wide extensions. The system remains a single-workstation local SQLite application.

## Verification and release evidence

The release is eligible for the `v1.1` tag only after the full CTest regression suite, GCC analyzer build, coverage gate and hosted GitHub Actions workflow pass on the merged commit. The final run, commit and tag are linked from the repository's Phase 8 release record.
