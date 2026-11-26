# Log

Plan: [index.md](index.md). Accumulates across steps: baseline test totals, environment notes, step outcomes, review and QA records.

## Baseline test totals (Debug and Release x64)
Measured by step 01 at f7bf9b2, before any change (all passing, exit code 0):

| Program | Debug\|x64 | Release\|x64 |
|---|---|---|
| Test_PPT | 81 | 81 |
| Test_Excel | 65 | 58 (Excel fakes are Debug-only) |
| Test_WordAssist | 166 | 166 |

After step 01: Test_PPT 99 / 99 (+18), Test_Excel 65 / 58, Test_WordAssist 166 / 166. After step 01 review round 1: Test_PPT 103 / 103 (+22 in all).

## Environment notes
- Python is not installed, so `runlog` is unavailable; commands are run directly and their exit codes recorded.
- Incremental MSBuild did not recompile all dependants after edits to `common/*.h` (step 01: stale objects gave a crash in `KeyboardShortcutsLoadTest/test_load_simple` and wrong `shift_key_is_pressed()` results; `/t:Rebuild` fixed both). After changing a shared header, build with `/t:Rebuild`.

## Step outcomes
- Step 01: tier opus; reviewer verdict clean after 2 review rounds (round 1 changes needed, round 2 clean on bafe885..ee8cf71); commits bafe885, 48694fe, 67a7718, 3585518, 3a7cf38, ee8cf71; Test_PPT 103/103.
- Step 02: commit 143fc28. Test_WordAssist 173 / 173, Test_Excel 67 / 60 (Debug / Release), Test_PPT 108 / 108 (both configurations).
- Step 02 review round 1: 65c302d; Test_WordAssist 174 / 174 (Debug and Release).
- Step 03: commit 4921cb9. Decorator wired in Word, Excel, PowerPoint. Test_WordAssist 174 / 174, Test_Excel 67 / 60, Test_PPT 108 / 108 (Debug / Release x64, unchanged); all three add-ins build Debug|x64 and Release|x64.
- Step 03: tier opus; reviewer verdict clean after 1 review round (84a5fa6..d74e460); commits 4921cb9, d74e460.
- Step 04: commit ef06fa5. Test_WordAssist 175 / 175 (Debug), Test_Excel 67, Test_PPT 108; WordAssist, ExcelAssist, PowerPointAssist build Debug|x64.
- Step 04: tier sonnet; reviewer verdict clean after 1 review round plus a follow-up change (hook pass-on logging Debug-only); commits ef06fa5, fad7aba, plus the follow-up commit.
- Step 05: commit 6f2415b. Test_Excel 71 / 60 (Debug / Release; +4 Debug-only), Test_WordAssist and Test_PPT not rerun (untouched). ExcelAssist builds Debug|x64 and Release|x64.
- Step 06: tier sonnet; commit recorded in git log (subject "Step 06 (#11)"). PowerPointAssist builds Debug|x64 and Release|x64; Test_PPT 108 / 108 (Debug).
- Step 05: tier sonnet; reviewer verdict clean after 1 review round; commits 6f2415b, dc95f42.
- Step 06: tier sonnet; reviewed by the orchestrator's diff check (text-only change), clean; commit 8a1ad2d.
- Orchestrator addition (found during step 07): `ToggleShortcuts` chord path in `common/keyboard_shortcuts.cpp` used `!! m_shortcuts_enabled` and never toggled; fixed test-first in 6224018 (two tests in `Test_PPT/keyboard_shortcuts_test.cpp`, Test_PPT 108 -> 110).
- Step 07: release notes (0342489) and final regression, all `/t:Rebuild`, all exit 0. Test_WordAssist 175 / 175 (Debug and Release), Test_Excel 71 / 71 Debug and 60 / 60 Release, Test_PPT 110 / 110 (Debug and Release). WordAssist, ExcelAssist, PowerPointAssist build Debug|x64, Release|x64 and Debug|Win32; Felix `Unicode_Debug|Win32` builds.
