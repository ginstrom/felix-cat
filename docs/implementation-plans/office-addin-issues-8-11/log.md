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
