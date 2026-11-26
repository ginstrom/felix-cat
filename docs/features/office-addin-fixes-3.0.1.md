# Feature: Office add-in fixes for open GitHub issues

## State

- **Request (user):** "please work through the open Felix issues: https://github.com/ginstrom/felix-cat/issues"
- **Slug:** `office-addin-fixes-3.0.1`
- **Scope:** ginstrom/felix-cat issues #1 (P2, Word concordance misses a match when the selection includes the paragraph mark), #2 (P2, Excel save dialog Alt+N relaunches Felix), #3 (P3, Excel formula bar stays stale after Get Translation).
- **Path:** Small. No architecture step. One design, then a 5-step plan.
- **Phase:** 6. Report (verified; nothing pushed)
- **Next action:** The user decides about pushing the branch, opening a PR and commenting on or closing issues #1–#3. If they want a PR, run `pr-reviewer` and then `pr-creator`. The QA evidence is in the session scratchpad `qa\evidence\`, which isn't durable.
- **Links:** design `docs/designs/office-addin-fixes-3.0.1.md` (approved 2026-11-25), plan `docs/implementation-plans/office-addin-fixes-3.0.1/index.md`, log `docs/implementation-plans/office-addin-fixes-3.0.1/log.md`, branch `fix/office-addin-issues-1-3` (base 5a13d86).
- **Status thread:** none, because the `recursive_slack` tools aren't available. This State section is the status.
- **Environment:** VS 2026, Boost at `C:\local\boost_1_91_0`, 64-bit Office only, no Python (so `runlog` is unavailable and gates record commands instead of run IDs). Felix.App and the add-ins are currently registered to this repo's Debug build. The user registered it for QA, and can restore the installed Felix with `tools\register-felix.ps1 -Unregister` and then the installer.
- **Open questions:** none.
- **Tiers:** designer opus; planner sonnet; engineers 01–03 and 05 sonnet, 04 opus; reviewers matched their engineers; overall review opus; QA sonnet. Escalations: none. Simulated user skipped because the issues have exact reproduction steps and QA ran them in real Office.
- **Verification gates:**
  - Overall review: clean at 9b8bf2a. Tests, Debug and Release x64: Word 161/161, Excel 61/61 and 57/57, PPT 73/73.
  - QA fix c8e44ee: review clean, Word 166/166.
  - QA round 2: PASS at c8e44ee. AC1–AC10 pass in Word, Excel and PowerPoint.
- **Out-of-scope findings (not fixed):**
  - Empty-selection concordance leaks the second key into the document.
  - Debug Felix asserts on the unregistered `Felix.Utilities`.
  - Debug PowerPointAssist asserts with "Logger not initialized" at startup.
  - The Word `KeyboardProc` catch swallows the key.
  - Auto Translate probably leaves the formula bar stale.
  - PowerPoint concordance probably has the trailing-¶ defect.
  - Excel `KeyboardProc` has dead code.
  - There are stale easyunit tests that nothing builds.

## Decision log

| Decision | By | Alternatives | Reason |
|---|---|---|---|
| Handle all three issues as one feature with one design and a step per issue | orchestrator | three separate features | Each fix is small and touches only the add-ins |
| Skip architecture | orchestrator | architect first | No new components, contracts or dependencies |
| Feature records live in `docs/features/` even though `docs/` is the Pages site | orchestrator | separate dev-docs folder | Default convention; the repository is public anyway |
| Approve the design as written | user | request changes | Root causes reproduced, and the fixes are minimal |
| D1: trim Word boundary marks only in the two concordance actions | user | the shared selection reader for all Word commands | Smallest change; leaves stored translations and the auto-translate loops alone |
| D2: apply the modal-dialog keyboard guard to Word and PowerPoint as well | user | Excel only | Same defect confirmed by probe; one shared helper |
| D3: shortcuts may still start Felix | user | only use a running Felix | The guard alone removes the reported relaunch |
| Work on branch `fix/office-addin-issues-1-3` | user | commit on main | Ready for a PR |
| Add the fixes to `setup/release-notes-3.0.1.md` | user | leave the notes alone | Ships in 3.0.1 |
| The user registers the Debug Felix and add-ins (admin) before QA | user | skip the manual checks | Proves the real Office paths |
| Fold step 05's review into the overall review | orchestrator | separate step review | Step 05 is release-note lines plus a regression run |
| An only-¶ formatted selection counts as empty (QA defect fix) | orchestrator | none | Required by approved R4/AC4 |
| Leave the missing newline in `excel_selection_writer.h` | orchestrator | fix it | Cosmetic; not worth another commit and review |
