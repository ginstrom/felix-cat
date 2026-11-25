# Feature: Office add-in fixes for open GitHub issues

## State

- **Request (user):** "please work through the open Felix issues: https://github.com/ginstrom/felix-cat/issues"
- **Slug:** `office-addin-fixes-3.0.1`
- **Scope:** ginstrom/felix-cat issues #1 (P2, Word concordance misses a match when the selection includes the paragraph mark), #2 (P2, Excel save dialog Alt+N relaunches Felix), #3 (P3, Excel formula bar stays stale after Get Translation).
- **Path:** Small. No architecture step. One short design covering all three fixes, then a plan with one step per issue.
- **Phase:** 4. Implementation
- **Next action:** Step 01 (engineer).
- **Links:** design `docs/designs/office-addin-fixes-3.0.1.md` (approved 2026-11-25), plan `docs/implementation-plans/office-addin-fixes-3.0.1/index.md` (3f203c2), log `docs/implementation-plans/office-addin-fixes-3.0.1/log.md`, branch `fix/office-addin-issues-1-3`.
- **Step tiers (engineer/reviewer):** 01 sonnet/sonnet; 02 sonnet/sonnet; 03 sonnet/sonnet; 04 opus/opus (COM fakes and Excel refresh); 05 sonnet/sonnet; overall review opus. Escalations: none.
- **Manual checks:** Felix.App is registered to the installed `C:\Program Files\Felix\Felix.exe`. Engineers record manual Office checks as not run. At verification, ask the user to run `tools\register-felix.ps1 -Configuration Debug` (admin) and register the x64 Debug add-ins, then QA runs the manual ACs.
- **runlog:** unavailable, because it needs Python and Python isn't installed. Agents run commands directly and quote the result lines, so gates record the command and the result instead of a run ID.
- **Step progress:**
  - 01 done (e8ace76), review clean in 1 round.
  - 02 done (95aa876), clean in 2 rounds; the second round fixed an index bookkeeping error.
  - 03 done (f1ea03f), clean in 1 round.
  - Tests: Test_Excel 57, Test_PPT 73, Test_WordAssist 161.
  - 04 done (a323bec, plus fix 01b35c5), clean in 2 rounds (opus/opus).
  - 05 done (655a95d, 9b8bf2a), with release notes and regression: Word 161, Excel 61 (Release 57), PPT 73. All add-ins build for Debug x64 and Win32. Its step review is folded into the overall review.
- **Phase 5 (verification):**
  - The overall review (opus) is running on 5a13d86..9b8bf2a.
  - QA (sonnet) waits for the user to register the Debug Felix and add-ins; it runs the manual AC1–AC10.
  - Overall review: clean (opus, run by hand). Notes: a formatted only-¶ selection leaves a tags-only string; step 05 log line missing; no newline at end of `excel_selection_writer.h`.
  - QA round 1 (sonnet): FAIL.
    - Passed: AC1–AC3, AC4 except formatted only-¶, AC5–AC10 in Excel and Word.
    - Defect 1 (major): a formatted only-¶ selection sends a tags-only query to Felix, which matches every entry. Sent to the step 01 engineer to fix.
    - Defect 2: Office disabled the Debug PowerPointAssist (it is in PowerPoint's Disabled Items list), so every PowerPoint check is not run. The user must re-enable it.
    - Pre-existing, out of scope: an empty-selection concordance leaks the `C` keystroke into the document.
    - Debug Felix shows a `Felix.Utilities` assert at every launch. It is pre-existing and harmless; dismiss it with Ignore.
  - Simulated user: skipped. These are bug fixes with exact reproduction steps, and QA runs those reproductions in real Office.
- **Out-of-scope finding:** in `WordAssist/WordAssist/hooker.cpp`, the `catch(CException&)` in `KeyboardProc` falls off the end without returning (pre-existing).
- **Status thread:** none. The `recursive_slack` tools aren't available in this session, so this State section is the status.
- **Environment:** VS Community 2026 is installed. Boost is at `C:\local\boost_1_91_0`. Only 64-bit Office is installed, so the x64 add-ins are the ones that can be tested. Test exes are Test_Excel, Test_WordAssist and Felix `Testing`. Python isn't installed.
- **Open questions:** none.
- **Out of scope, noted by the designer:** Auto Translate probably leaves the formula bar stale; shortcuts fire in cell edit mode; PowerPoint concordance probably has the #1 defect; Word Look up sends the trailing ¶; dead easyunit tests; dead code in Excel's `KeyboardProc`.
- **Coming up for the user:** go-ahead to push, open a PR and comment on or close the issues.
- **Tiers:** designer opus; planner sonnet.
- **Verification gates:** none yet.

## Decision log

| Decision | By | Alternatives | Reason |
|---|---|---|---|
| Handle all three issues as one feature with one design and a step per issue | orchestrator | three separate features | Each fix is small and touches only the add-ins, and one design keeps the review cheap |
| Skip architecture | orchestrator | architect first | No new components, contracts or dependencies |
| Feature records live in `docs/features/` even though `docs/` is the Pages site | orchestrator | separate dev-docs folder | Default convention; the repository is public anyway |
| Approve the design as written | user | request changes | Root causes reproduced, and the fixes are minimal |
| D1: trim Word boundary marks only in the two concordance actions | user | the shared selection reader for all Word commands | Smallest change; leaves stored translations and the auto-translate loops alone |
| D2: apply the modal-dialog keyboard guard to Word and PowerPoint as well | user | Excel only | Same defect confirmed by probe; one shared helper |
| D3: shortcuts may still start Felix | user | only use a running Felix | The guard alone removes the reported relaunch |
| Work on branch `fix/office-addin-issues-1-3` | user | commit on main | Ready for a PR |
| Add the fixes to `setup/release-notes-3.0.1.md` | user | leave the notes alone | Ships in 3.0.1 |
| The user registers the debug Felix and add-ins (admin) before QA, so the manual Office checks run then | user | skip the manual checks | Proves the real Word/Excel paths |
