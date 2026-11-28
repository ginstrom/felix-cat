# Step 06: Final verification and hand-offs

Plan: [index.md](index.md). Design: [../../designs/unsigned-releases.md](../../designs/unsigned-releases.md) sections 2 (acceptance), 7, 8.2.

## Objective
Verify the whole branch against the acceptance criteria, including a CI run on the pushed branch, and record what only the user or a VM run can confirm. This step changes files only to fix defects found (and then re-runs the affected checks).

## Requirements and context
- A1-A6 are checked here; A7 (clean-VM install, Office add-ins) is a hand-off.
- **Pushing is the orchestrator's decision, not the implementer's.** If the branch has not been pushed when this step starts, stop after the local checks (1-4), record "CI run pending push" and report; the orchestrator pushes (or tells you to) and resumes the step at check 5.
- Existing PR #16 work (R1.1-R1.5, R2.1, R2.4, R2.5) only needs the CI result to confirm.
- Constraints: index "Shared constraints" (no tags, releases, dispatches, secrets or settings changes).

## Dependencies and prerequisites
Steps 01-05 done and committed. Contracts to verify (index C1-C5): script outputs, notes text, artifact names, `SIGNING_ENABLED` gating, README anchor, policy path, winget folder.

## Scope and implementation guidance (checks)
Local, no push needed:
1. Placeholders and leftovers: `rg -i signpath .github` empty; `rg -n "SIGNPATH_" .` outside `docs/designs|features|implementation-plans` empty; `rg -n CODE_SIGNING_POLICY` per the index completion criteria; `git ls-files CODE_SIGNING_POLICY.md "setup/HTML FILE SETUP.iss"` empty.
2. Re-run step 01's script checks on a dummy installer, and step 05's `winget validate`.
3. Parity: the two README commands equal `.github/release-notes-install.md` (version substituted); README anchor `Installing Felix` exists; no README text claims releases are signed; `docs/CODE_SIGNING_POLICY.md` is linked from nowhere (`rg -n "CODE_SIGNING_POLICY" README.rst BUILDING.md docs/index.html docs/manual` shows no hyperlink to it).
4. `git status` shows only intended files; no binaries added (`git diff --stat --diff-filter=A origin/main...HEAD` and check for non-text files).

CI (after the orchestrator pushes the branch; the Build workflow runs on every push):
5. `gh run list --branch ci/unsigned-releases --workflow Build --limit 3`, then `gh run watch <id> --exit-status`. Expect success (A1). Record the run URL in [log.md](log.md).
6. Download the artifact: `gh run download <id> -n felix-installer-<sha> -D <temp>`. Check: contains `Felix_Setup_<AppVersion>.exe`, `SHA256SUMS.txt`, `release-notes.md`; `SHA256SUMS.txt` hash equals `Get-FileHash` of the installer; `release-notes.md` has the version filled in, both commands, and the unsigned paragraph. Also confirm the binaries artifact still has both platforms' add-ins.
7. A2 in CI: the build succeeded with `/DVcRedist` passed by `build-installer.ps1`.
8. `release.yml` parse check on GitHub, if possible without running it: `gh workflow view release.yml --ref ci/unsigned-releases` (read-only). Do not dispatch it.

Hand-offs to write into [log.md](log.md) as a checklist for the PR description (design 8.2), with status "not done, needs user/VM":
- U1 review and merge. U2 after merge, Actions > Release > Run workflow on `main` (dry run), then `gh attestation verify <installer> --repo ginstrom/felix-cat`. U3 push `v3.0.1` after checking `setup/release-notes-3.0.1.md`, review and publish the draft. U4 leave `SIGNING_ENABLED` unset. U5 SmartScreen screenshot `docs/images/smartscreen.png` (QA on the VM "FelixTest" may capture it: add Mark of the Web with `Set-Content <file> -Stream Zone.Identifier -Value "[ZoneTransfer]`r`nZoneId=3"`, run from Explorer; otherwise the user). U6 winget steps (`packaging/winget/README.md`). U7 test the add-ins in Office on the user's machine with the CI installer. U8 later SignPath reapplication and replacing the placeholders.
- A7: clean-VM install and run of the CI installer, full and Felix-only (VM scripts `felix-test.cmd full|main`, per the orchestrator's Felix test VM notes); silent switches and the uninstall registry key `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{98802423-6787-4122-BCEC-08CC2E12F18E}_is1` with `DisplayVersion` = the version. Not run by this step unless the brief says so.
- Follow-ups to list: Japanese install guide (feature record, Q4), Scintilla 5 migration (#44), third-party license notices, `PowerPointAssist`'s unused `SciLexer.dll` load (design section 9).

## Verification
The checks above are the verification; record each command and result.

## Completion criteria
- [ ] Checks 1-4 pass and are recorded; 5-8 pass (or are recorded as pending the orchestrator's push).
- [ ] [log.md](log.md) holds the CI run URL, the artifact check result and the hand-off checklist.
- [ ] Any defect found is fixed in the step that owns the file (note it here), or reported.
- [ ] Index status updated for all steps; nothing pushed by this step.

## Execution record

Local checks (worktree, Windows PowerShell 5.1 and Git Bash; no `pwsh`, no `runlog`):
1. `rg -i signpath .github` -> empty. `rg -n SIGNPATH_ .` outside `docs/designs|features|implementation-plans` -> empty. `rg -n CODE_SIGNING_POLICY` (same exclusions) -> `BUILDING.md:300` (plain backticked mention) and `setup\FelixSetup.iss:65` (comment); not README. `git ls-files CODE_SIGNING_POLICY.md "setup/HTML FILE SETUP.iss"` -> empty. (`.signpath/` mentions are plain text too; the index criterion is widened, see review follow-ups.)
2. `tools\make-release-files.ps1 -Version 3.0.1 -OutDir <scratch>` on a 200-byte dummy installer: wrote both files; SHA256SUMS line equals `Get-FileHash`; no BOM, no CR, ends with LF; unsigned notice present; no `{{`. With `-Signed`: unsigned paragraph gone (the remaining `<!--` is the HTML header comment of `release-notes-3.0.1.md`). Missing installer throws "Installer not found". `winget validate packaging\winget\manifests\g\Ginstrom\Felix\3.0.1` -> "Manifest validation succeeded."
3. Parity: README contains both commands from `.github/release-notes-install.md` with 3.0.1 substituted; anchor "Installing Felix" exists (README.rst:15); README claims only that releases are NOT signed; no hyperlink to `CODE_SIGNING_POLICY` from README.rst, BUILDING.md, docs/index.html or docs/manual (BUILDING.md has one plain-text mention).
4. `git status` clean before follow-ups; `git diff --numstat origin/main...HEAD` shows only text files added (20 files, all docs/yaml/ps1/md); no binaries.

CI: run https://github.com/ginstrom/felix-cat/actions/runs/46160127626 (workflow Build, push of ffd48a9, ~25 min) -> success, all steps green including "Release files" (A1). A2: `tools\build-installer.ps1:108` passes `/DVcRedist=$VcRedist` to ISCC and the installer built. Artifacts: `felix-installer-ffd48a9a...` (Felix_Setup_3.0.1.exe 28,705,801 bytes, SHA256SUMS.txt 88 bytes, release-notes.md 4,963 bytes) and `felix-binaries-ffd48a9a...` (addins-x64 and addins-x86 each with ExcelAssist, PowerPointAssist, WordAssist DLLs). Downloaded to the scratchpad `ci-artifacts` folder; installer not run. Installer SHA256 eb6e77b2de112576493289ad3367a69743393c51dd57f0a1a411e79eaa19953a equals the SHA256SUMS.txt line (`<hash>  Felix_Setup_3.0.1.exe`); both text files no BOM, no CR, end with LF; release-notes.md has 3.0.1 in both commands and the unsigned paragraph.
Check 8: `gh workflow view release.yml --ref ci/unsigned-releases --yaml` returns the new file (starts with the new header comment); `gh workflow list` shows Release active. GitHub has no read-only parse check short of running it, and no workflow-file error run exists for the branch; release.yml was not dispatched.

Review follow-ups from the coordinator, applied in one docs commit: (1) README "Starting with 3.0.1, each release ..." (latest release is still v3.0); (2) `docs/CODE_SIGNING_POLICY.md` privacy text now says Felix calls `Felix.Utilities.CheckUpdates` at startup (after first launch) if that old tool is registered, and that the release doesn't install it (Felix/MemoryWindowFrame.cpp:3079-3087; nothing claimed about what the tool contacts); (3) README "turn it back on" rewrapped; (4) `tools/sign-file.ps1` comment now points to BUILDING.md "Signing locally" (comment-only); (5) index completion criterion and step 04 task criterion widened to plain-text mentions in `.signpath/`, `BUILDING.md` and the `FelixSetup.iss` comment.

Deviations: none beyond the follow-ups. No tags, releases, dispatches or settings changes. Pushed ci/unsigned-releases (authorized) after the follow-up commit.
