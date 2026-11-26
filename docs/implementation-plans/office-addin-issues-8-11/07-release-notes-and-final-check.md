# 07 - Release notes and final regression

Index: [index.md](index.md). Design: [section 6 (Release)](../../designs/office-addin-issues-8-11.md). Depends on: 01-06. Size: S.

## Objective
Record the four fixes in the 3.0.1 release notes and confirm the whole change set is green in Debug and Release x64.

## Requirements (local copy)
- Release notes (decision in the feature record): one line per issue in `setup/release-notes-3.0.1.md`, in the style of the existing lines.
- AC11: Test_WordAssist, Test_Excel and Test_PPT pass with the new tests (baselines 166, 65, 81 in Debug|x64).
- Felix.exe is unchanged. Only 64-bit Office is available, so 32-bit add-ins only need to build.

## Prerequisites
Steps 01-06 done. Existing file: `setup/release-notes-3.0.1.md`, section "What's new since 3.0". Existing bullets use a bold lead-in, plain wording and a linked issue, for example:
`- **Felix shortcuts no longer fire inside dialogs** ([#2](https://github.com/ginstrom/felix-cat/issues/2)). In Excel, Word and PowerPoint, pressing Alt+N ...`
The file is LF without BOM.

## Scope and implementation guidance
1. Add four bullets under "What's new since 3.0", after the existing bullets and before "Known issues", with the same style and issue links (`https://github.com/ginstrom/felix-cat/issues/8` ... `/11`):
   - #8: **Shortcuts no longer fail with a COM error when you type fast** ([#8](...)). In Word, Excel and PowerPoint, Felix shortcuts (for example Look up Next and Previous Translation) are queued and run in order instead of sometimes failing with "illegal to call out while inside message filter".
   - #9: **Word's Alt+1 to Alt+9 glossary shortcuts work after switching back from Felix** ([#9](...)). Alt+1 no longer opens the AutoSave prompt.
   - #10: **Excel's formula bar follows Auto Translate** ([#10](...)). The formula bar shows the translation for the active cell as soon as it is written, instead of the source text until the command ends.
   - #11: **PowerPoint's menu shows the right shortcuts** ([#11](...)). Translate to Fuzzy shows Alt+C, and Find Concordance no longer advertises a shortcut.
   Leave the HTML comment, headings, "Known issues" and the SHA placeholder alone. Adjust wording to read naturally next to the existing bullets; do not claim behaviour QA has not seen (for #9, if QA has not confirmed it, say only what the change does).
2. Final regression, `Debug|x64` and `Release|x64`: build and run the three test programs; record every total against the baselines in [log.md](log.md). Build the three add-ins `Debug|x64`. Try `Debug|Win32` for the three add-ins as step 03 did; a Win32 failure that predates this branch is a note in the log, not a blocker; a failure caused by the new code is a defect to fix in the relevant step's files (reopen that step).
3. `git status` shows only intended files; nothing pushed.

## Verification
```powershell
$ms = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
foreach ($cfg in "Debug","Release") {
  & $ms Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj /p:Configuration=$cfg /p:Platform=x64 /m /v:minimal /nologo
  & ".\Test_WordAssist\Test_WordAssist\x64\$cfg\Test_WordAssist.exe" --report_level=short
  & $ms Test_Excel\Test_Excel.vcxproj /p:Configuration=$cfg /p:Platform=x64 /m /v:minimal /nologo
  & ".\Test_Excel\x64\$cfg\Test_Excel.exe" --report_level=short
  & $ms Test_PPT\Test_PPT.vcxproj /p:Configuration=$cfg /p:Platform=x64 /m /v:minimal /nologo
  & ".\Test_PPT\x64\$cfg\Test_PPT.exe" --report_level=short
}
& $ms WordAssist\WordAssist\WordAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms ExcelAssist\ExcelAssist\ExcelAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms PowerPointAssist\PowerPointAssist\PowerPointAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```
Record each command's exit code and each test program's "N test cases out of N" line. Expected: all pass; Debug counts are baselines plus the new cases from steps 01, 02, 04 and 05 (Test_Excel's Release count is lower than Debug, because the fakes are Debug-only). Record the release-notes diff.

## Completion criteria
- [ ] Four bullets added in the existing style; rest of the file unchanged.
- [ ] All six test runs (three projects x Debug and Release) pass; totals in the execution record and `log.md`.
- [ ] Add-in builds (Debug|x64, and the Win32 outcome) recorded.
- [ ] `git status` clean apart from intended files; nothing pushed.
- [ ] Manual AC1-AC10 listed as deferred to QA.
- [ ] Committed; index row 07 updated and the index's overall completion criteria checked off.

## Execution record
