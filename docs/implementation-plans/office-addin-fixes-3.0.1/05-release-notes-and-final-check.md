# 05 - Release notes and final regression

Index: [index.md](index.md). Depends on: 01, 03, 04. Size: S.

## Objective
Record the fixes in the 3.0.1 release notes and confirm the whole change set is green.

## Requirements (local copy)
User decision: one line per fix in `setup/release-notes-3.0.1.md`. AC11: existing Test_WordAssist, Test_Excel and Test_PPT suites still pass plus the new tests. Felix.exe is unchanged. Only 64-bit Office is available, so 32-bit add-ins only need to build.

## Scope
1. Edit `setup/release-notes-3.0.1.md`, section "What's new since 3.0", adding three bullets in the existing style (bold lead-in, plain wording):
   - Word: Find Concordance and Translation Concordance no longer miss a match when the selection includes the paragraph mark.
   - Excel, Word and PowerPoint: Felix keyboard shortcuts no longer fire while a dialog has focus (for example Alt+N in the "save changes" prompt no longer launches Felix).
   - Excel: the formula bar shows the new text right after Get Translation or a glossary entry is inserted.
   Leave the HTML comment, headings, "Known issues" and the SHA placeholder alone. Do not cite issue numbers unless the other bullets do (they currently do not).
2. Final regression, `Debug|x64`: build and run the three test programs; record totals against the baselines in `log.md`. Build the three add-ins `Debug|x64`.
3. Try the Win32 builds of the three add-ins (`Debug|Win32`; BUILDING.md says these have not been built since the migration). Record pass or fail. A Win32 failure that predates this branch (check by building commit 48b9bfd in a scratch worktree outside the repository) is a note in the log, not a blocker; a failure caused by the new code is a defect to fix in the relevant step's files.
4. `git status` shows only intended files; nothing pushed.

## Verification
Test totals, build results and the release-notes diff in the Execution record.

## Completion criteria
- [ ] Three bullets added; rest of file unchanged.
- [ ] Final totals recorded: all passing, new cases included.
- [ ] Win32 build outcome recorded.
- [ ] Committed; index row 05 updated and the index's overall completion criteria checked off.

## Execution record
