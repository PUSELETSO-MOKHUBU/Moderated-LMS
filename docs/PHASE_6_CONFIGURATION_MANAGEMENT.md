# Phase 6: Configuration Management and Quality Assurance

## Repository

- **Repository:** [PUSELETSO-MOKHUBU/Moderated-LMS](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS)
- **Default development branch:** `main`
- **Phase 6 topic branch:** `feature/phase-6-qa`
- **Remote:** `origin` → `https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS.git`
- **Observed history:** before Phase 6, `main` contained three descriptive project commits not yet on `origin/main`. The published `feature/phase-6-qa` branch contains those project commits plus the Phase 5/6 work; its pull request will therefore bring the earlier commits into `main` as well. There are no release tags yet; keep `v1.0` for the Phase 7 release.

The GitHub workflow at `.github/workflows/ci.yml` runs for every push and pull request. It configures and builds the C++17 application and Catch2 test suite, enables GCC static analysis, executes CTest, enforces Phase 5's 70% business-logic line-coverage gate, and retains JUnit, analyzer, and coverage reports as a workflow artifact.

## Branching and review strategy

Use short-lived branches; do not develop directly on `main`:

- `feature/<issue>-<short-description>` for new behavior.
- `fix/<issue>-<short-description>` for a defect.
- `docs/<short-description>` for documentation-only changes.

Open a pull request from the topic branch to `main`. Keep each pull request focused and link it to an assignment requirement or issue. A pull request must describe its behavior change, testing, coverage impact, and any migration/release impact. The author completes the self-review checklist; a second reviewer should approve when available. Require the **C++ CI and Quality Gates** workflow to pass before merging. Prefer squash merging with an imperative, meaningful subject, for example `test: add LMS deadline boundary cases`; do not commit generated builds, coverage reports, credentials, or the user's local database.

GitHub currently reports `main` as **unprotected**. A repository maintainer should enable branch protection in GitHub settings: require pull requests, require the CI check, block force pushes and deletion, and dismiss stale approvals after new commits. This repository setting cannot be enabled from this checkout.

## Commit, tag, and release policy

Use small commits with an imperative subject and a conventional scope, such as `fix: reject submissions after an extension deadline` or `docs: record Phase 6 quality evidence`. Keep unrelated binaries and local data out of source commits. Releases use annotated semantic-version tags created on a tested, merged commit, for example:

```powershell
git switch main
git pull --ff-only
git tag -a v0.6.0 -m "Moderated LMS Phase 6 QA milestone"
git push origin v0.6.0
```

Create the `v0.6.0` Phase 6 milestone tag only after the PR is merged and the QA acceptance criteria pass. Reserve `v1.0` for the Phase 7 project release. No tag is created on the unmerged feature branch.

## CI evidence and limitations

The branch was pushed on 2026-10-31. The first hosted run failed because `Database.cpp` used Windows-only `localtime_s`; the code now selects the thread-safe Windows or POSIX local-time API as appropriate. The corrected workflow completed successfully:

- **Run:** [C++ CI and Quality Gates — run 3](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/actions/runs/41246448511)
- **Result:** build with GCC static analysis, all tests, and the 70% coverage gate passed.
- **Artifact:** [CI quality reports](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS/actions/runs/41246448511/artifacts/13059745090)

The topic branch is pushed but has not yet been merged; no pull request or release tag exists. Open a PR from `feature/phase-6-qa` to `main`, review the diff (which includes the three earlier unpushed project commits), and merge after review. Then create and push the annotated `v0.6.0` Phase 6 milestone tag using the command above. Reserve `v1.0` for the Phase 7 project release.
