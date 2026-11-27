# 06 - #11: PowerPoint menu labels match the bindings

Index: [index.md](index.md). Design: [sections 3 (#11) and 5.4](../../designs/office-addin-issues-8-11.md). Depends on: none. Size: S (encoding care).

## Objective
In the PowerPoint Felix menu, English and Japanese, translation and review modes, **Find Concordance** shows no shortcut and **Translate to Fuzzy** shows Alt+C, matching the default bindings. Key bindings do not change (D1 = relabel).

## Requirements (local copy)
- R10: as above. Alt+C still runs Translate to Fuzzy (`keyboard_shortcuts_ppt.cpp`: Alt+C -> `Auto Trans to Fuzzy`; Ctrl+Alt+C -> `Trans Concordance`; there is no source-concordance command and Alt+Z is unbound). `key_mapper_ppt.h`, `keyboard_shortcuts_ppt.cpp`, tooltips and the manual do not change.
- `PowerPointAssist/PowerPointAssist/AddIn.rc` is **Shift-JIS** (`#pragma code_page(932)`, line ~20) and must stay Shift-JIS.

## Prerequisites and existing code (inspected)
Captions are hard-coded in `PowerPointAssist/PowerPointAssist/Connect.cpp` (line numbers from f1a6a4c; search for the literal text to confirm):

| Lines | Old caption | New caption |
|---|---|---|
| ~458 (`add_menu`), ~1467 (`set_english_trans_menu_text`), ~1620 (`set_english_review_menu_text`) | `Find C&oncordance (ALT + C)` | `Find C&oncordance` |
| ~415, ~1427, ~1580 | `Translate to Fu&zzy (ALT + Z)` | `Translate to Fu&zzy (ALT + C)` |

Japanese strings in `AddIn.rc` (~lines 340 and 343), used by Connect.cpp at ~1778/1818 (trans) and ~1927/1967 (review):
- `IDS_CONCORDANCE_J` ("...(&O) (ALT + C)"): drop ` (ALT + C)`.
- `IDS_AUTO_TRANS_FUZZY_J` ("...(&Z) (ALT + Z)"): change `(ALT + Z)` to `(ALT + C)`.
Tooltips: unchanged (English tooltips name no key; Japanese tooltips load the same string as the caption, so they follow automatically).

## Scope and implementation guidance
1. Edit the six `Connect.cpp` literals (`Connect.cpp` is UTF-8 with BOM; keep its encoding and line endings).
2. Edit the two `AddIn.rc` strings **byte-safely**, preserving Shift-JIS: for example in PowerShell read the bytes with `[IO.File]::ReadAllBytes`, decode with `[Text.Encoding]::GetEncoding(932)`, replace only the two ` (ALT + C)` / `(ALT + Z)` substrings, and write back with the same encoding and the original BOM/line-ending convention (check the first bytes and CRLF vs LF first). Do not use a tool that re-saves as UTF-8. Verify with `git diff --stat` (two lines changed in `AddIn.rc`) and `git diff --text` on those lines, and confirm the rest of the file is byte-identical (`git diff` shows no other hunks).
3. There is no unit test for menu captions (the menus need PowerPoint). Verification is a grep-level check: no remaining `Find C&oncordance (ALT + C)` or `Fu&zzy (ALT + Z)` in `Connect.cpp`; the `.rc` decodes (codepage 932) to the new text. Add no new tests. Optionally build the add-in so the resource compiler accepts the file.

Exclusions: no key binding changes, no `key_mapper_ppt.h` change, no manual change, no fix for the other mislabelled PowerPoint items (design section 9: Japanese review-mode captions, `IDS_GLOSS_N_J`, swapped Set/Get-and-Next English review captions, unused `IDS_TRANS_CONCORDANCE_J`).

## Verification
```powershell
$ms = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
& $ms PowerPointAssist\PowerPointAssist\PowerPointAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms Test_PPT\Test_PPT.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_PPT\x64\Debug\Test_PPT.exe --report_level=short
```
- The add-in builds `Debug|x64` with exit 0 (the resource compiler accepted the edited `.rc`). Test_PPT unchanged from the previous step's count, passing.
- `git diff` for `AddIn.rc` shows only the two intended lines; the file's bytes elsewhere are unchanged. Record the check in the execution record.
- Manual (QA): AC10, English and Japanese menus, translation and review modes: Find Concordance shows no shortcut; Translate to Fuzzy shows Alt+C; Alt+C still runs Translate to Fuzzy. Record as deferred.

## Completion criteria
- [x] Six English literals and two Japanese strings changed; nothing else.
- [x] `AddIn.rc` is still Shift-JIS, only two lines differ.
- [x] Add-in builds; Test_PPT green.
- [x] Manual AC10 recorded as deferred to QA.
- [x] Committed; index row 06 updated.

## Execution record

Engineer: sonnet. Date 2026-11-26. Baseline Test_PPT 108/108 (step 05 left it unchanged).

- `Connect.cpp`: six literals changed with `sed` (file has no BOM at HEAD or now; line endings untouched, `git diff --numstat` 6/6). Find Concordance `Find C&oncordance (ALT + C)` -> `Find C&oncordance` (lines 463, 1472, 1625). Translate to Fuzzy `Translate to Fu&zzy (ALT + Z)` -> `Translate to Fu&zzy (ALT + C)` (lines 420, 1432, 1585).
- `AddIn.rc`: read as bytes, decoded with codepage 932, only the lines starting `IDS_AUTO_TRANS_FUZZY_J` and `IDS_CONCORDANCE_J` edited, re-encoded with 932 and written with `WriteAllBytes` (no BOM; file starts `// M`, 519 CRLF, 0 bare LF, preserved). File shrank by exactly 10 bytes (` (ALT + C)`). `git diff --numstat` 2/2; `git diff --text | iconv -f CP932` shows only:
  - `IDS_AUTO_TRANS_FUZZY_J`: `ファジーマッチまで自動翻訳(&Z) (ALT + Z)` -> `ファジーマッチまで自動翻訳(&Z) (ALT + C)`
  - `IDS_CONCORDANCE_J`: `コンコーダンス(一致)検索(&O) (ALT + C)` -> `コンコーダンス(一致)検索(&O)`
- Untouched on purpose: `IDS_TRANS_CONCORDANCE_J` (line 383, unused, design section 9).
- Grep: no `Find C&oncordance (ALT + C)` or `Fu&zzy (ALT + Z)` left in `Connect.cpp`.
- Builds (PowerShell, MSBuild): PowerPointAssist Debug|x64 exit 0; Release|x64 exit 0 (resource compiler accepted the .rc; only the usual FELIXREG registration warning, needs admin). Test_PPT Debug|x64 build exit 0; `Test_PPT.exe --report_level=short` exit 0: "108 test cases out of 108 passed, 340 assertions out of 340 passed".
- No deviations. No tests added (menus need PowerPoint).
- Deferred to QA: AC10, English and Japanese, translation and review modes: Find Concordance shows no shortcut; Translate to Fuzzy shows Alt+C; Alt+C runs Translate to Fuzzy; Japanese tooltips follow the caption string.
