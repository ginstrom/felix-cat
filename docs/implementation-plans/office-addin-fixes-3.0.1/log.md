# Log

## Baseline test totals (step 01, Debug|x64, 2026-11-25)
| Test project | Cases | Assertions | Result |
|---|---|---|---|
| Test_WordAssist | 134 | 443 | passed |
| Test_Excel | 49 | 140 | passed |
| Test_PPT | 65 | 181 | passed |

Matches the documented baseline (Word 134, PowerPoint 65, Excel 49).

## Environment notes
- `runlog` needs Python, which is not installed, so runs are made directly.
- `Felix.App` is registered to `C:\Program Files\Felix\Felix.exe`, not the repo's Debug build. Manual Office checks need `tools\register-felix.ps1 -Configuration Debug` (admin).
## Step outcomes
- Step 01: done, e8ace76 (record 53d0b8f). Engineer: sonnet. Reviewer: sonnet. Verdict: clean after 1 review round. Test_WordAssist 153/153 (reviewer rerun). Manual AC1-AC4 deferred to QA until Felix is registered.
- Step 02: done, 95aa876 (record 5bf701e). Engineer: sonnet. Reviewer: sonnet. Verdict: clean after 2 review rounds (round 1: index row 03 wrongly marked done, fixed). Test_Excel 57/57, Test_PPT 73/73, Test_WordAssist 161/161.
- Step 03: done, f1ea03f (record 3d77982). Engineer: sonnet. Reviewer: sonnet. Verdict: clean after 1 review round. Test_Excel 57/57, Test_PPT 73/73, Test_WordAssist 161/161 (reviewer rebuilt all three add-ins and test projects). Manual AC5-AC8 deferred to QA.
- Step 04: done, a323bec (record 3ec98fe; review fix 01b35c5). Engineer: opus. Reviewer: opus. Verdict: clean after 2 review rounds (round 1: Test_Excel Release|x64 did not compile the new test, fixed by a Debug-only guard). Test_Excel Debug|x64 61/61, Release|x64 57/57; ExcelAssist Debug x64 and Win32 build. Manual AC9-AC10 deferred to QA.

## Step 04 notes
- Test_Excel can take `ExcelImport.h` and `fake_excel.h`; it only needed `common\DispatchWrapper.cpp` added. No fallback used. Test_Excel 61/61 (57 + 4).
- COM probe (64-bit Microsoft 365): `ActiveCell.Activate()` after a `Characters` write refreshes the formula bar; `001` stays a string; A1:B3 with B2 active is kept. A selected TextBox object QIs to `Excel::RangePtr` as NULL without throwing. In an automated Excel instance a text-box selection reverts to a Range by itself right after `Shape.Select`, so AC10c needs an interactive check.
- Step 01 QA fix (AC4/R4): tags-only query after trim now treated as empty (`trim_concordance_query`). Test_WordAssist 166/166; WordAssist Debug|x64 rebuilt. Manual re-check left to QA.
## Verification (orchestrator, 2026-11-26)

- **Step 05:** done at 655a95d (record 9b8bf2a), engineer sonnet. Its review was folded into the overall review.
- **Overall review (opus), 5a13d86..9b8bf2a:** clean.
  - All add-ins build for Debug x64 and Win32.
  - Tests, Debug and Release x64: Test_WordAssist 161/161 and 161/161, Test_Excel 61/61 and 57/57, Test_PPT 73/73 and 73/73.
  - Non-blocking notes: a formatted only-¶ selection leaves a tags-only string, which became QA defect 1; no newline at the end of `excel_selection_writer.h`, not fixed because it is cosmetic.
- **QA round 1 (sonnet):** FAIL.
  - Defect 1 (major): a formatted only-¶ selection sent a tags-only query that matched every entry.
  - The PowerPoint checks were blocked because Office had disabled the Debug PowerPointAssist add-in.
- **QA fix c8e44ee (step 01 engineer, sonnet):** reviewed clean (sonnet). Test_WordAssist 166/166 in Debug and Release x64.
- **QA round 2 (sonnet) at c8e44ee:** PASS.
  - AC1–AC10 pass in Word, Excel and PowerPoint, with the Debug x64 add-ins and the Debug Felix registered.
  - Not covered: Felix's own Excel properties dialog, a pre-fix baseline, Auto Translate, 32-bit Office.
- **Pre-existing issues found during verification, not fixed:**
  - Concordance on an empty selection leaks the second key (the `C` of Alt+M, C) into the document.
  - Debug Felix asserts at launch because the `Felix.Utilities` ProgID isn't registered (DispatchWrapper.cpp:10).
  - Debug PowerPointAssist asserts at startup with "Logger not initialized!" (logging.cpp:46).
  - In Word's `KeyboardProc`, the `catch(CException&)` swallows the key.
  - Auto Translate probably leaves the formula bar stale.
  - PowerPoint concordance probably has the trailing-¶ defect.

## Follow-up: pre-existing issues (branch fix/office-addin-older-bugs, 2026-11-26)

All six were confirmed in the code and fixed, test-first where the logic could be isolated.

| Issue | Cause | Fix | Test |
|---|---|---|---|
| Concordance key leak (Alt+M, C on empty selection) | `KeyboardShortcuts::receive_keypress` returned the command's result for a chord's second key, so a declining command (empty query) passed the key on to Word | The second key of a recognised chord is always consumed; single-key shortcuts unchanged | Test_PPT `test_second_key_consumed_when_command_declines`, `test_single_key_passed_on_when_command_declines` |
| Word hook swallows keys | `catch(CException&)` in Word's `KeyboardProc` fell through to `return 1` | Returns `CallNextHookEx` like the other handlers | None (Win32 hook callback); one line |
| Auto Translate formula bar | `AutoTransCell` writes through `set_cell_text` (Characters) and never refreshes | `formula_bar_refresher` (RAII) in `OnAutoTransSel`, `OnAutoTransSheet`, `OnAutoTransWorkbook` | Test_Excel `formula_bar_refresher_*` (3) |
| PowerPoint concordance trailing paragraph mark | `textRange->Text` sent raw in `OnConcordanceAction` and `OnTransConcordanceAction` | New `trim_plain_concordance_query` (plain text, no tag handling) | Test_PPT `TrimPlainConcordanceQueryTest` (5) |
| Debug "Logger not initialized!" at PowerPoint startup | `CPowerPointInterface`'s constructor (a `CConnect` member, built before `OnConnection` sets the logger) loads abbreviations, which logs "File does not exist." when `abbreviations.txt` is missing | Abbreviations load in `OnConnection` after `set_logger` | Test_PPT `constructor_does_not_log` |
| Debug assert in `DispatchWrapper.cpp:10` | `ATLVERIFY` on `CLSIDFromProgID`, then `CoCreateInstance` with an uninitialised CLSID | Throws `_com_error(hr)`, which callers already catch | Test_Excel `unregistered_progid_throws_com_error` |

Final, Debug|x64: Test_WordAssist 166/166, Test_Excel 65/65, Test_PPT 81/81. Add-ins build Debug x64 and Win32. Not checked by hand in Office (needs `tools\register-felix.ps1`, admin).
