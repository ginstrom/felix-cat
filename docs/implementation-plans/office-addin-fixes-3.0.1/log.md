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