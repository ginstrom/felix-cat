# Implementation plan: Unsigned CI releases

Source design: [../../designs/unsigned-releases.md](../../designs/unsigned-releases.md) (approved 2026-11-29; repo base 807ccc0). Feature record (decisions in force): [../../features/unsigned-releases.md](../../features/unsigned-releases.md). Request: [../../features/unsigned-releases.request.md](../../features/unsigned-releases.request.md). Branch: `ci/unsigned-releases`. Log: [log.md](log.md).

Purpose: Felix ships **unsigned** releases for now. A tag builds the installer in GitHub Actions, writes `SHA256SUMS.txt`, attests build provenance, and creates a draft release. The README explains the SmartScreen warning, the signing policy becomes an unlinked draft, a winget manifest is drafted, and the pipeline keeps disabled signing placeholders. No C++ changes.

## Decisions in force
- Q1: keep Boost via b2. Q2: keep the checked-in `SciLexer.dll`; GitHub issue [ginstrom/felix-cat#44](https://github.com/ginstrom/felix-cat/issues/44) tracks the Scintilla 5 migration. Reference it where the design says "open an issue" (the draft policy's SciLexer note, BUILDING.md, README's code-signing-status text if it mentions SciLexer). Q3: `actions/attest@v4`. Q4: English install guide only; the Japanese refresh is a follow-up. Q5: winget ID `Ginstrom.Felix`.
- Assumptions A-1..A-7 of design 8.3 stand (README holds the guide; `SignTool` stays behind `#ifdef SIGN`; delete `setup/HTML FILE SETUP.iss`; policy lists the two language DLLs; keep `.signpath/`; dispatch dry run plus `-AllPlatforms`; `choco install innosetup` unpinned).

## Shared constraints (every step)
- **No pushing, tagging, releases, secrets, repository settings, or submissions to external services.** Do not change winget's `LocalManifestFiles` setting. Add no binaries (the SmartScreen screenshot stays a commented placeholder). No self-signed certificates.
- **No C++ or project-file changes.** Do not touch `Dependencies.props`, `.vcxproj` files, `.github/actions/setup-felix-deps`, `tools/build-installer.ps1` or `tools/sign-file.ps1`.
- Pin action majors as the existing workflows do (`checkout@v7`, `upload-artifact@v7`, `setup-msbuild@v3`; new: `actions/attest@v4`).
- Docs claim only what the design's section 10 sources support. No claim anywhere that releases are signed.
- Python is not installed and `runlog` is unavailable: run commands directly and record the exact command and result in the task file's execution record. Scripts must run under Windows PowerShell 5.1 (local) and `pwsh` (the runner); test with both if `pwsh` exists. `gh` is installed; `actionlint` is not.
- ISCC and MSBuild output can be long; use a generous timeout and read only the tail.
- Commit once per step, only when the executing brief says to. Commit message ends with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>` unless the brief names another line. Leave unrelated uncommitted files alone (the design, feature and request files under `docs/designs` and `docs/features` belong to the orchestrator).
- Things only the user can do (design 8.2, U1-U8) are not done by a step; step 06 records them for the PR description.

## Steps

| ID | Task file | Outcome | Depends on | Status |
|---|---|---|---|---|
| 01 | [01-installer-and-release-files.md](01-installer-and-release-files.md) | `VcRedist` required in the `.iss`; obsolete `.iss` deleted; `tools/make-release-files.ps1` and `.github/release-notes-install.md` created and verified; `build.yml` runs the script and uploads its outputs | none | done (see git log) |
| 02 | [02-release-workflow.md](02-release-workflow.md) | `release.yml` rewritten: no SignPath, gated signing placeholders, checksums, attestation, draft release with two assets, dispatch dry run | 01 | done (see git log) |
| 03 | [03-readme-install-guide.md](03-readme-install-guide.md) | README "Installing Felix" section, SignPath claims removed, getting-started.html paragraph, release-notes-3.0.1.md fixed | 01 | pending |
| 04 | [04-signing-policy-and-build-docs.md](04-signing-policy-and-build-docs.md) | Root policy deleted; `docs/CODE_SIGNING_POLICY.md` drafted (unlinked); `.signpath/` marked unused; BUILDING.md rewritten for the unsigned flow | 02 | pending |
| 05 | [05-winget-manifest.md](05-winget-manifest.md) | `packaging/winget/` manifests and README; `winget validate` passes | none | pending |
| 06 | [06-final-verification.md](06-final-verification.md) | Whole-branch checks, CI run on the pushed branch (orchestrator pushes), VM and user hand-offs recorded | 01-05 | pending |

Execution order: 01, then 02; 03, 04 and 05 touch disjoint files and can run in parallel after their dependencies (04 waits for 02 only because BUILDING.md describes the final workflow); then 06. Pushing and the CI run happen only in 06.

## Cross-step contracts (task files copy these unchanged)

**C1. `tools/make-release-files.ps1`** (produced by 01; consumed by 02 and `build.yml`)
`tools\make-release-files.ps1 -Version <x.y.z> [-Signed] [-OutDir setup\Output]` (`-OutDir` default `setup\Output`, resolved against the repository root, not the current directory).
- Input `<OutDir>\Felix_Setup_<Version>.exe`; throws if missing.
- Writes `<OutDir>\SHA256SUMS.txt`: one line per `Felix_Setup_<Version>.exe` it was given, `<lowercase hex sha256><two spaces><file name>`, LF endings, trailing LF, UTF-8 without BOM.
- Writes `<OutDir>\release-notes.md`: content of `setup\release-notes-<Version>.md` if present, else `Felix <Version>`; then a blank line; then `.github/release-notes-install.md` with every `{{VERSION}}` replaced. With `-Signed`, the text from `<!-- unsigned -->` through `<!-- /unsigned -->` (markers included) is removed. LF endings, UTF-8 without BOM.

**C2. `.github/release-notes-install.md`** (produced by 01; consumed by C1; README commands must match it, step 03). Exact content:

~~~markdown
## Installing and verifying

<!-- unsigned -->
This release is **not code-signed** yet, so Windows SmartScreen will say
"Windows protected your PC". Choose **More info**, then **Run anyway**. On PCs
with Smart App Control turned on, Windows blocks unsigned installers
completely. See [Installing Felix](https://github.com/ginstrom/felix-cat#installing-felix).
<!-- /unsigned -->

To check that your download is the file GitHub Actions built from this
release's tag, in PowerShell in your Downloads folder:

    Get-FileHash .\Felix_Setup_{{VERSION}}.exe -Algorithm SHA256

The hash must match the line for the installer in `SHA256SUMS.txt`. With the
[GitHub CLI](https://cli.github.com/), signed in:

    gh attestation verify .\Felix_Setup_{{VERSION}}.exe --repo ginstrom/felix-cat
~~~

**C3. Release outputs** (01, 02): `setup/Output/Felix_Setup_<v>.exe`, `setup/Output/SHA256SUMS.txt`, `setup/Output/release-notes.md`. `build.yml` adds the last two to the existing `felix-installer-<sha>` artifact; `release.yml` uploads artifact `felix-<v>-release` containing all three.

**C4. Names** (02, 03, 04): repository variable `SIGNING_ENABLED` (the string `'true'` enables the placeholders; leave unset); release title `Felix <v>` plus ` (unsigned)` when not signing; README section titled **Installing Felix** (anchor `#installing-felix`); policy file `docs/CODE_SIGNING_POLICY.md`; SciLexer issue `ginstrom/felix-cat#44`.

**C5. winget** (05): identifier `Ginstrom.Felix`, version `3.0.1`, ManifestVersion `1.10.0`, folder `packaging/winget/manifests/g/Ginstrom/Felix/3.0.1/`.

## Requirement-to-task mapping

| Requirement | Task |
|---|---|
| R1.1-R1.5, R2.1, R2.4, R2.5 (exist; checked only) | 06 |
| R2.2, R2.3 | 01 |
| R3.1 | 02 |
| R3.2, R3.4 (notes text) | 01 (script, notes), 02 (workflow) |
| R3.3, R3.5 | 02 |
| R4.1 | 03 |
| R5.1, R5.2 | 05 |
| R6.1, R6.2 | 04 |
| R7 (PR description) | 06 records the inventory and user actions; the PR itself is outside this plan |
| A1 | 01, 06 |
| A2 | 01 |
| A3 | 02, 06 |
| A4 | 03 |
| A5 | 05 |
| A6 | 04, 06 |
| A7 | 06 (VM and Office hand-offs) |

## Completion criteria (whole plan)
- All six steps `done`; A1-A6 met, with A3-live and A7 recorded as user/VM hand-offs.
- `rg -i signpath .github` finds nothing; `rg CODE_SIGNING_POLICY` finds only the policy file itself, workflow comments, and docs under `docs/designs`, `docs/features`, `docs/implementation-plans` (not the README).
- Build workflow green on the pushed branch; its artifact verifies per step 06.

## Unresolved questions / blockers
None. The SmartScreen screenshot `docs/images/smartscreen.png` is supplied later by QA or the user; it stays a commented placeholder.

## For implementers
Update your step's status and commit hash in the table above. Record evidence, deviations and blockers in the task file's Execution record. Anything the plan accumulates as a whole (CI run URLs, measurements) goes in [log.md](log.md). Keep this index short.
