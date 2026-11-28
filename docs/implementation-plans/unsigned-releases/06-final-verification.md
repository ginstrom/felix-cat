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
