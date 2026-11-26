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
- **Phase:** 3. Planning
- **Next action:** The planner writes the plan from the approved design; then implementation on the branch.
- **Links:**
  - design: `docs/designs/office-addin-issues-8-11.md`
  - plan: `docs/implementation-plans/office-addin-issues-8-11/`
  - branch: `fix/office-addin-issues-8-11` (base f1a6a4c)
- **Status thread:** none, because the `recursive_slack` tools aren't available. This State section is the status.
- **Environment:**
  - Tooling: VS 2026, Boost at `C:\local\boost_1_91_0`, MSBuild from PowerShell by full path, no Python (so no `runlog`).
  - Office: 64-bit Microsoft 365 only.
  - Registration: Office currently uses the installed Felix. QA needs `tools\register-felix.ps1 -Configuration Debug`, which the user runs (UAC).
- **Coming up for the user:** registering the Debug build for QA; push/PR.
- **Open questions:** none yet.
- **Tiers:** designer opus; planner sonnet.

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
