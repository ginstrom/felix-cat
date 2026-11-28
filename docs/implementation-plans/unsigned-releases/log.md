- Step 01: engineer sonnet, reviewer sonnet, verdict clean after 1 review round (fix: ISCC example in FelixSetup.iss header now passes /DVcRedist). Commits 99f914e and the header fix.
- Step 02: engineer opus, reviewer opus, verdict clean after 1 review round (change taken: the attest step runs only on tag pushes, so dispatch dry runs don't mint attestations that gh attestation verify --repo would accept). Commits b36a9f9 and 28d47fe.
- Step 03: engineer sonnet, reviewer sonnet, verdict clean after 1 review round (fixes: Smart App Control re-enable softened to "in most cases"; add-in description text corrected to per-application wording after checking the AddIn.rgs files, FriendlyName "Felix Addin" confirmed). Commits ed4863b and the fix commit.
- Step 04: engineer sonnet, reviewer sonnet, verdict clean after 1 review round (nits taken: privacy text notes the release does not include or install Felix.Utilities; "Signing locally" paragraph rewrapped). Commits d5a3abe and the nits commit.
- Step 05: engineer sonnet, reviewer sonnet, verdict clean after 1 review round (fixes: README notes wingetcreate update only works after the first version is merged; Windows Sandbox has no winget, so use a VM or winget-pkgs Tools/SandboxTest.ps1; moderator may need to review a new unsigned installer). Commits fe1e9a8 and d134417.
- Step 06: engineer opus. Local checks 1-4 and CI checks 5-8 pass; CI run https://github.com/ginstrom/felix-cat/actions/runs/46160127626 (Build, push of ffd48a9) concluded success. Installer artifact verified (SHA256SUMS.txt matches Get-FileHash; release-notes.md has both commands with 3.0.1 and the unsigned notice; no BOM/CR). Coordinator review follow-ups 1-5 applied in one docs commit (README "Starting with 3.0.1", policy privacy text on Felix.Utilities.CheckUpdates, README rewrap, sign-file.ps1 comment, index/step 04 criterion widened).

### Hand-offs for the PR description (all: not done, needs user/VM)
- [ ] U1 Review and merge the PR.
- [ ] U2 After merge: Actions > Release > Run workflow on `main` (dry run), then `gh attestation verify <installer> --repo ginstrom/felix-cat`. (The attest step runs only on tag pushes, so a dispatch dry run mints no attestation; verify with the v3.0.1 release.)
- [ ] U3 Check `setup/release-notes-3.0.1.md`, push tag `v3.0.1`, review and publish the draft release.
- [ ] U4 Leave repository variable `SIGNING_ENABLED` unset.
- [ ] U5 SmartScreen screenshot `docs/images/smartscreen.png` (QA on VM "FelixTest" may capture it: add Mark of the Web with `Set-Content <file> -Stream Zone.Identifier -Value "[ZoneTransfer]`r`nZoneId=3"`, run from Explorer; otherwise the user), then uncomment the placeholder in the README.
- [ ] U6 winget submission steps in `packaging/winget/README.md`.
- [ ] U7 Test the add-ins in Office on the user's machine with the CI installer.
- [ ] U8 Later: SignPath reapplication and replacing the placeholders.
- [ ] A7 Clean-VM install and run of the CI installer, full and Felix-only (`felix-test.cmd full|main`); silent switches; uninstall key `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{98802423-6787-4122-BCEC-08CC2E12F18E}_is1` with `DisplayVersion` = 3.0.1.
- Follow-ups: Japanese install guide (feature record, Q4); Scintilla 5 migration (#44); third-party license notices; `PowerPointAssist`'s unused `SciLexer.dll` load (design section 9).
- Clean-VM test (A7), 2026-11-29: CI installer from run 46160127626 with /VERYSILENT /SUPPRESSMSGBOXES /NORESTART. Full install, uninstall and Felix-only install all pass (exit 0; registrations and cleanup as in PR #16). Results in C:\VMs\FelixTestesults.
