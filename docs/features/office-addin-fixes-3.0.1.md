# Feature: Office add-in fixes for open GitHub issues

## State

- **Request (user):** "please work through the open Felix issues: https://github.com/ginstrom/felix-cat/issues"
- **Slug:** `office-addin-fixes-3.0.1`
- **Scope:** ginstrom/felix-cat issues #1 (P2, Word concordance misses a match when the selection includes the paragraph mark), #2 (P2, Excel save dialog Alt+N relaunches Felix), #3 (P3, Excel formula bar stays stale after Get Translation).
- **Path:** Small. No architecture step. One short design covering all three fixes, then a plan with one step per issue.
- **Phase:** 2. Design
- **Next action:** Design gate. Waiting on the user to approve the design and answer D1–D3, the branch question and the release-notes question.
- **Links:** design `docs/designs/office-addin-fixes-3.0.1.md` (written, uncommitted), plan (pending), branch (undecided; repo is on `main`).
- **Open questions (block planning):**
  - Design approval.
  - D1: where the #1 trim applies. Recommended: the two Word concordance actions only.
  - D2: apply the #2 modal guard to Word and PowerPoint too. Recommended: yes.
  - D3: should shortcuts stop starting Felix? Recommended: no change.
  - Branch: feature branch or `main`.
  - Release notes.
- **Out of scope, noted by the designer:** Auto Translate probably leaves the formula bar stale; shortcuts fire in cell edit mode; PowerPoint concordance probably has the #1 defect; Word Look up sends the trailing ¶; dead easyunit tests; dead code in Excel's `KeyboardProc`.
- **Status thread:** none. The `recursive_slack` tools aren't available in this session, so this State section is the status.
- **Environment:** VS Community 2026 is installed. Boost is at `C:\local\boost_1_91_0`. Only 64-bit Office is installed, so the x64 add-ins are the ones that can be tested. Test exes are Test_Excel, Test_WordAssist and Felix `Testing`.
- **Open questions:** none yet.
- **Coming up for the user:** design approval; feature branch or `main`; whether to add the fixes to `setup/release-notes-3.0.1.md`; go-ahead to push, open a PR and close or comment on the issues.
- **Tiers:** designer opus.
- **Verification gates:** none yet.

## Decision log

| Decision | By | Alternatives | Reason |
|---|---|---|---|
| Handle all three issues as one feature with one design and a step per issue | orchestrator | three separate features | Each fix is small and touches only the add-ins, and one design keeps the review cheap |
| Skip architecture | orchestrator | architect first | No new components, contracts or dependencies |
| Feature records live in `docs/features/` even though `docs/` is the Pages site | orchestrator | separate dev-docs folder | Default convention; the repository is public anyway |
