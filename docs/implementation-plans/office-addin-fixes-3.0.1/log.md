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

## Step 04 notes
- Test_Excel can take `ExcelImport.h` and `fake_excel.h`; it only needed `common\DispatchWrapper.cpp` added. No fallback used. Test_Excel 61/61 (57 + 4).
- COM probe (64-bit Microsoft 365): `ActiveCell.Activate()` after a `Characters` write refreshes the formula bar; `001` stays a string; A1:B3 with B2 active is kept. A selected TextBox object QIs to `Excel::RangePtr` as NULL without throwing. In an automated Excel instance a text-box selection reverts to a Range by itself right after `Shape.Select`, so AC10c needs an interactive check.
