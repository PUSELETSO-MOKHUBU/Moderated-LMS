# Phase 6: Configuration Management and Quality Assurance

## Repository

- **Repository:** [PUSELETSO-MOKHUBU/Moderated-LMS](https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS)
- **Default development branch:** `main`
- **Phase 6 topic branch:** `feature/phase-6-qa`
- **Remote:** `origin` → `https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS.git`
- **Observed history:** `main` contained three descriptive project commits ahead of `origin/main` before Phase 6 work. The Phase 6 changes are kept on the `feature/phase-6-qa` topic branch. There were no release tags yet; this is intentional because the assignment's v1.0 release belongs to Phase 7.

The GitHub workflow at `.github/workflows/ci.yml` runs for every push and pull request. It configures and builds the C++17 application and Catch2 test suite, enables GCC static analysis, executes CTest, enforces Phase 5's 70% business-logic line-coverage gate, and retains JUnit, analyzer, and coverage reports as a workflow artifact.

## Branching and review strategy

Use short-lived branches; do not develop directly on `main`:

- `feature/<issue>-<short-description>` for new behavior.
- `fix/<issue>-<short-description>` for a defect.
- `docs/<short-description>` for documentation-only changes.

Open a pull request from the topic branch to `main`. Keep each pull request focused and link it to an assignment requirement or issue. A pull request must describe its behavior change, testing, coverage impact, and any migration/release impact. The author completes the self-review checklist; a second reviewer should approve when available. Require the **C++ CI and Quality Gates** workflow to pass before merging. Prefer squash merging with an imperative, meaningful subject, for example `test: add LMS deadline boundary cases`; do not commit generated builds, coverage reports, credentials, or the user's local database.

Repository maintainers should enable branch protection for `main` in GitHub settings: require pull requests, require the CI check, block force pushes and deletion, and dismiss stale approvals after new commits. The local checkout does not have permission to verify or configure remote branch-protection settings, so this is an operational action, not claimed as already enabled.

## Commit, tag, and release policy

Use small commits with an imperative subject and a conventional scope, such as `fix: reject submissions after an extension deadline` or `docs: record Phase 6 quality evidence`. Keep unrelated binaries and local data out of source commits. Releases use annotated semantic-version tags created on a tested, merged commit, for example:

```powershell
git switch main
git pull --ff-only
git tag -a v1.0 -m "Moderated LMS v1.0"
git push origin v1.0
```

Do not tag a release before the applicable phase's acceptance criteria pass. No release tag is claimed for Phase 6: the assignment explicitly reserves tag `v1.0` for Phase 7.

## CI evidence and limitations

The pipeline definition is versioned in the repository and the same commands passed locally as recorded in `PHASE_5_TEST_RESULTS.md`, `PHASE_5_COVERAGE.md`, and `PHASE_6_STATIC_ANALYSIS.md`. A remote Actions run/artifact cannot be evidenced until the topic branch is pushed. Record the first green workflow run URL and retain its report artifact before claiming remote CI evidence. The repository link is supplied above; the topic branch is local, has not been pushed or merged, and has no release tag. Keep the v1.0 tag for the Phase 7 release.
