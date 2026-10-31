# Phase 6: Configuration Management and Quality Assurance

## Repository

- **Repository:** [PUSELETSO-MOKHUBU/Moderated-LMS](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS)
- **Default development branch:** `main`
- **Phase 6 topic branch:** `feature/phase-6-qa`
- **Remote:** `origin` → `https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS.git`
- **Phase 6 integration:** [PR #1](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/pull/1) was squash-merged on 2026-11-01 as commit [`fe7f265`](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/commit/fe7f2655c6413a3abd09e9d119707c780a31e31d). The PR included the three project commits that were ahead of `origin/main` before this work, as well as Phase 5/6 deliverables.

The GitHub workflow at `.github/workflows/ci.yml` runs for every push and pull request. It configures and builds the C++17 application and Catch2 test suite, enables GCC static analysis, executes CTest, enforces Phase 5's 70% business-logic line-coverage gate, and retains JUnit, analyzer, and coverage reports as a workflow artifact.

## Branching and review strategy

Use short-lived branches; do not develop directly on `main`:

- `feature/<issue>-<short-description>` for new behavior.
- `fix/<issue>-<short-description>` for a defect.
- `docs/<short-description>` for documentation-only changes.

Open a pull request from the topic branch to `main`. Keep each pull request focused and link it to an assignment requirement or issue. A pull request must describe its behavior change, testing, coverage impact, and any migration/release impact. The author completes the self-review checklist; a second reviewer should approve when available. Require the **C++ CI and Quality Gates** workflow to pass before merging. Prefer squash merging with an imperative, meaningful subject, for example `test: add LMS deadline boundary cases`; do not commit generated builds, coverage reports, credentials, or the user's local database.

GitHub branch protection is enabled for `main`: merges must go through a pull request, the `tests-and-coverage` status check must pass, and force pushes and branch deletion are disabled. The repository does not enforce a minimum approval count, since this is an individual assignment; request a second reviewer when one is available and resolve all review comments before merging.

## Commit, tag, and release policy

Use small commits with an imperative subject and a conventional scope, such as `fix: reject submissions after an extension deadline` or `docs: record Phase 6 quality evidence`. Keep unrelated binaries and local data out of source commits. Releases use annotated semantic-version tags created on a tested, merged commit, for example:

```powershell
git switch main
git pull --ff-only
git tag -a v0.6.0 -m "Moderated LMS Phase 6 QA milestone"
git push origin v0.6.0
```

The annotated [`v0.6.0` Phase 6 QA milestone tag](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/releases/tag/v0.6.0) was created and pushed after PR #1 was merged and its required CI passed. It points to merged commit `fe7f265`. Reserve `v1.0` for the Phase 7 project release; do not create a release tag on an unmerged feature branch.

## CI evidence and limitations

The first hosted run failed because `Database.cpp` used Windows-only `localtime_s`; the code now selects the thread-safe Windows or POSIX local-time API as appropriate. The corrected workflow passed on both the PR and the merged `main` commit:

- **PR check:** [C++ CI and Quality Gates — run 7](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/actions/runs/41249062754) passed the analyzer build, tests, and coverage gate before merge.
- **Main check:** [C++ CI and Quality Gates — run 8](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/actions/runs/41249555427) passed after the squash merge.
- **Artifact:** [CI quality reports](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/actions/runs/41249555427/artifacts/13061110059)

PR #1 is merged and `main` is protected by required pull requests and the `tests-and-coverage` check; force pushes and branch deletion are disabled. The repository does not require a separate approval count for this individual assignment, but the self-review record is completed. Phase 6 is tagged `v0.6.0`; keep `v1.0` for the Phase 7 project release.
