# Feature: Office add-in fixes for issues #8–#11

## State

- **Request (user):** "Great, please proceed with the open tickets" (ginstrom/felix-cat open issues, 2026-11-26)
- **Slug:** `office-addin-issues-8-11`
- **Scope:**
  - #8 (P2): Word lookup and Previous Translation sometimes fail with COM error 0x80010005, "illegal to call out while inside message filter".
  - #9 (P3): in Word, Alt+1 opens AutoSave help instead of inserting glossary entry 1.
  - #10 (P3): during Excel Auto Translate Selection, the formula bar briefly shows the source text.
  - #11 (P3): PowerPoint's menu advertises Alt+C for Find Concordance, but Alt+C runs another command.
  - All four were found in Codex user testing at `55079ee`.
- **Path:** Medium. No architecture step. One design covering all four, then a plan, implementation, verification.
- **Phase:** 5. Verification
- **Next action:** An opus engineer makes one bounded attempt at the #10 formula-bar repaint (fallback: revert the mid-run refresh) and fixes the overall-review findings. Then targeted re-QA of #10, then the PR.
- **Links:**
  - design: `docs/designs/office-addin-issues-8-11.md`
  - plan: `docs/implementation-plans/office-addin-issues-8-11/index.md`, log `log.md` beside it
  - branch: `fix/office-addin-issues-8-11` (base f1a6a4c)
- **Status thread:** none, because the `recursive_slack` tools aren't available. This State section is the status.
- **Environment:**
  - Tooling: VS 2026, Boost at `C:\local\boost_1_91_0`, MSBuild from PowerShell by full path, no Python (so no `runlog`).
  - Office: 64-bit Microsoft 365 only.
  - Registration: Office uses this repo's Debug build (the user registered it for QA on 2026-11-26).
- **Coming up for the user:** registering the Debug build for QA; push/PR.
- **Open questions:** none.
- **Verification gates:** steps 01–07 clean (see `log.md`). Regression at 5244adc: Word 175/175, Excel 71/71 (Release 60), PPT 110/110; add-ins Debug and Release x64 and Debug Win32 build; Felix Unicode_Debug|Win32 builds. Overall review (opus): clean at fe70b6b, with 7 non-blocking findings. Fixing after QA: the PowerPoint read-only probe, re-posting a lost run message, the release-note wording (toggle chord, Alt+0), the HRESULT name in a comment. Accepted: Ctrl+Right in a Word task pane goes to Felix (design §6). QA (sonnet) at 5244adc: FAIL on AC8 (#10: the formula bar stays stale until Auto Translate ends; the mid-run ActiveCell.Activate has no visible effect). AC1–AC7, AC9 and AC10 pass. Evidence in `.git/pr-review/fix/office-addin-issues-8-11/evidence/`.
- **Tiers:** designer opus; planner sonnet. Engineer and reviewer per step: 01 opus, 02 sonnet, 03 opus, 04 sonnet, 05 sonnet, 06 sonnet, 07 sonnet. Overall review opus; QA sonnet. Escalations: none.

## Decision log

| Decision | By | Alternatives | Reason |
|---|---|---|---|
| Handle #8–#11 as one feature with one design | orchestrator | four separate features | All four are small add-in fixes found in one test pass |
| Skip architecture | orchestrator | architect first | No new components or contracts expected |
| Approve the design as written (2026-11-26) | user | Word-only #8; request changes | Root causes traced in code; fixes are contained |
| D1 (#11): fix the PowerPoint labels, don't rebind | user | rebind Alt+C/Alt+Z like Word | Correct for existing shortcut files; no behavior change |
| D2 (#8): defer commands in all three add-ins | user (via approval) | Word only | Shared dispatch; same latent defect in Excel and PowerPoint |
| D3 (#10): refresh the formula bar when the active cell is written | user (via approval) | accept the transient | Cheap; stale for the whole run on large selections |
| D4 (#9): fix now on the inferred cause, with debug logging | user (via approval) | instrumented build first | If QA still fails, the log tells which follow-up applies |
| Work on branch `fix/office-addin-issues-8-11` | user | commit on main | Ready for a PR |
| Add the fixes to `setup/release-notes-3.0.1.md` | orchestrator | leave the notes alone | Same as #1–#3; 3.0.1 isn't released yet |
| Fix the two-key ToggleShortcuts chord (`!!` typo) on this branch | orchestrator | separate PR | One character in code this branch touches; found by the step 01 engineer |
| Make hook pass-on log lines Debug-only | orchestrator | log in all builds | Add-ins log at debug level in Release too; every Ctrl/Alt key Office handles would write and flush a line |
| Step 06 reviewed by the orchestrator's diff check | orchestrator | reviewer subagent | Text-only change; the diff was checked line by line, including the CP932 decode |
| #10: one bounded attempt at forcing the formula bar to repaint mid-run, falling back to removing the mid-run refresh | user | accept and close; leave #10 open | QA showed ActiveCell.Activate mid-command has no visible effect |
