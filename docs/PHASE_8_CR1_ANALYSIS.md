# Phase 8 - CR1 Maintenance Analysis

## Change request

> A lecturer should be able to extend the deadline for a whole class, not just an individual student, and extensions should be shown on the student's assignment list.

## Classification and justification

CR1 is **perfective maintenance**: it adds a requested capability and improves deadline visibility in an existing workflow. It is not corrective maintenance because the existing individual-extension behavior is not a defect, and it is not adaptive maintenance because no external platform or operating-environment change prompted it.

## Impact analysis

The original Phase 2 requirements and Phase 3 diagram files are not checked into the repository. Requirement IDs below therefore use the traceability references already recorded in [the Phase 5 test plan](PHASE_5_TEST_PLAN.md); the UML files linked below provide the maintained v1.1 design views for this change.

| Area | Affected artifact | Impact |
|---|---|---|
| Requirements | FR-06–FR-07 | Expand the lecturer extension capability from an individual-only grant to individual or class-wide grant/update. Preserve lecturer/course authorization and the later-than-original-deadline rule. |
| Requirements | FR-08–FR-12, BR-02, BR-04 | Student assignment viewing, submission/resubmission deadline checks and late-penalty calculation must all use the effective deadline, not just the original deadline. |
| Requirements | FR-19 | A class-wide deadline change can transition an active submission between `LATE` and `SUBMITTED`; existing status/queue expectations remain valid. |
| Requirements | FR-20 | The assignment list remains enrolment-scoped; an extension must not expose an assignment to an unenrolled student. |
| Use case | UC-01: Submit assignment | Student sees the effective deadline; lecturer can choose class-wide or individual scope. |
| UML | [CR1 class diagram](diagrams/CR1_CLASS_DIAGRAM.puml) | Shows the new database operation and class-extension record alongside the preserved individual extension. |
| UML | [CR1 data model](diagrams/CR1_DATA_MODEL.puml) | Adds the one-per-assignment class-extension table and its relationships. |
| UML | [CR1 sequence diagram](diagrams/CR1_EXTENSION_WORKFLOW.puml) | Shows validation, persistence, status refresh, and student effective-deadline lookup. |
| UML | [v1.0 deployment diagram](diagrams/DEPLOYMENT_DIAGRAM.puml) | Physical topology is unchanged: the new table is in the existing local SQLite database. No deployment-node change is required. |
| Classes/interfaces | `Database` | Add `grantClassExtension`; update individual extension refresh, student-list, submit and mark queries to apply the effective deadline consistently. |
| Classes/interfaces | `AssignmentRecord` | Existing `deadline` and `effectiveDeadline` fields are sufficient; no data-transfer shape change is needed. |
| Classes/interfaces | `runLecturer` and `printAssignments` in `src/main.cpp` | Add an explicit class/individual choice and show when an effective extension is applied. |
| Classes/interfaces | `Lecturer`, `Student`, `FileStorage`, `LatePenaltyStrategy` | No interface or responsibility change. Role checks and existing late-penalty policy are reused. |
| Database | `class_extensions` (new) | One row per assignment stores the class deadline, reason and granting lecturer. Schema creation is additive and idempotent. |
| Database | `assignments`, `enrolments` | Supply the assignment's original deadline and the students to whom the class-wide deadline is visible. Current and future enrollees see it while the class-extension record exists. |
| Database | `extensions` | Existing per-student records remain intact. The effective deadline is the later of original, class-wide and individual deadlines. |
| Database | `submissions` | Active `SUBMITTED`/`LATE` status is refreshed after a class deadline is granted or updated. Marked/released work is not reclassified. |
| Database | `marks` | No schema change. A class extension is rejected once any active result for that assignment has been marked/moderated/released, preserving the existing protection against changing a marked result's deadline. |
| Tests | `tests/test_lms.cpp` | Extend the database-backed role workflow with scope, authorization, enrollment, precedence, no-shortening, marked-result, status-refresh and mark-calculation assertions. |
| Tests/docs | `docs/PHASE_5_TEST_PLAN.md` | Add AC-10 and trace the changed requirements and regression behavior to executable checks. |

## Behavior and data decisions

- A class extension is associated with an assignment, not a snapshot of students. It applies to every student currently enrolled in the assignment's course and to students enrolled later while that class-extension record exists.
- `effectiveDeadline = max(original assignment deadline, class-wide deadline if present, individual deadline if present)`. An individual extension can therefore give one student more time than the class.
- Only a lecturer assigned to the assignment's course may grant/update its class extension. The deadline must be later than the original. Updating a class extension cannot shorten the existing class-wide deadline.
- To preserve the existing marked-result rule for a whole class, a class extension is rejected if the assignment has an active marked, moderated or released submission. Invalid or unauthorized requests report an error and do not persist a change.
- A new `class_extensions` table is created with `CREATE TABLE IF NOT EXISTS`. Existing v1.0 `extensions` rows, submissions and marks are not rewritten during schema initialization.

## Migration, release and rollback

The change is a backward-compatible additive schema migration for normal upgrade: launch v1.1 against a copy of the v1.0 data directory, verify existing individual extensions and assignments, then use the v1.1 executable. Back up `lms.db` and the complete `uploads` folder together before upgrading. Do not operate the same live data folder with both v1.0 and v1.1 binaries: v1.0 cannot display or enforce class-wide deadlines. To roll back, close the application and restore the pre-upgrade database and uploads backup as a matched set.

## Verification

The CR1 integration assertions are part of the existing Catch2/CTest workflow, so the complete prior suite also runs as regression coverage. Release evidence, hosted CI and the final v1.1 tag/commit are recorded with the v1.1 release notes after merge.
