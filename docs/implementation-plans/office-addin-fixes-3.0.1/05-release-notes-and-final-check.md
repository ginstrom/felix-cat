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
- [x] Three bullets added; rest of file unchanged.
- [x] Final totals recorded: all passing, new cases included.
- [x] Win32 build outcome recorded.
- [x] Committed; index row 05 updated and the index's overall completion criteria checked off.

## Execution record
`runlog` is unavailable (needs Python), so commands were run directly from PowerShell: `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe <proj> /p:Configuration=Debug /p:Platform=<x64|Win32> /m /v:m /nologo`.

**Release notes.** `setup/release-notes-3.0.1.md`: three bullets added under "What's new since 3.0" (#1 Word concordance, #2 shortcuts in dialogs, #3 Excel formula bar), each linking its issue. The orchestrator's brief asked for issue references, so this departs from the task text ("do not cite issue numbers"). The HTML comment, headings, Known issues and the SHA placeholder are untouched. The file stays LF without BOM, as before.

**Final regression, Debug|x64** (MSBuild exit 0 for each, then `<exe> --report_level=short`):

| Test project | Cases | Assertions | Result |
|---|---|---|---|
| Test_WordAssist | 161 (baseline 134, +27) | 492 | all passed |
| Test_Excel | 61 (baseline 49, +12; Release x64 has 57 because the new case is Debug-only) | 174 | all passed |
| Test_PPT | 73 (baseline 65, +8) | 201 | all passed |

**Add-in builds, MSBuild exit 0 for all six:** ExcelAssist, WordAssist and PowerPointAssist at Debug|x64 and Debug|Win32. There was no Win32 failure, so no comparison build at 5a13d86 was needed. ExcelAssist Win32 was already up to date from step 04's review.

**Manual AC1-AC10:** not run. They need Felix registered, which raises a UAC prompt. Deferred to QA, as recorded in steps 01, 03 and 04.

**git status** before the commit showed only `setup/release-notes-3.0.1.md` modified. Nothing was pushed.
