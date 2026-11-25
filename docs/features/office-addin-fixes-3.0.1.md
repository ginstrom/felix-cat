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
- **Manual checks:** if a check needs Felix COM registration (UAC/admin), the engineer stops and reports, and the user runs `tools\register-felix.ps1`.
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
