# 01 - Word concordance: trim boundary marks (#1)

Index: [index.md](index.md). Design: [section 5.1](../../designs/office-addin-fixes-3.0.1.md). Depends on: none. Size: S.

## Objective
Word Find Concordance and Translation Concordance send the query without Word structural marks at its ends, so `blue lantern<CR>` finds `blue lantern`.

## Requirements (local copy)
- R1: remove paragraph mark 0x0D, LF 0x0A, end-of-cell/row 0x07, manual line break 0x0B, page/section break 0x0C and column break 0x0E from both ends. Interior characters are kept.
- R2: spaces, tabs, NBSP and all other end characters are kept (`lantern ` with a trailing space is intentional).
- R3: works for the HTML query and the plain-text query: look past tags at either end (`<font face="Calibri">blue lantern\r</font>` becomes `<font face="Calibri">blue lantern</font>`). Never remove or change a tag. Literal `<`, `>`, `&` are already escaped by the parsers, so any `<...>` is a tag.
- R4: if nothing is left, Find Concordance sends nothing (existing `length()==0` check); Translation Concordance passes the empty string to Felix as today.
- D1: only these two call sites. `get_selection_text` and `WordParser.cpp` do not change.

## Prerequisites
Existing code (inspected): `WordAssist/WordAssist/WordController.cpp` `OnConcordanceAction` (line ~793; forwards to `OnTransConcordanceAction` in trans mode or when the classic interface is off) and `OnTransConcordanceAction` (line ~1404), both calling `get_selection_text(as_plaintext)` (line ~1664). The file already uses `BSTR2wstring` and `string2BSTR` for `_bstr_t`/`wstring` conversion. Word tests live as `WordAssist/WordAssist/*_test.cpp` and are listed in `Test_WordAssist/Test_WordAssist/Test_WordAssist.vcxproj` (for example `..\..\WordAssist\WordAssist\html_formatter_test.cpp`).

## Contract produced
`common/word_boundary_marks.h` (new, header-only, UTF-8 BOM, global namespace):
```cpp
inline std::wstring trim_word_boundary_marks(const std::wstring &text);
```
Trailing end: `end = text.size()`; loop while `end > 0`: if `text[end-1]==L'>'` and `text.rfind(L'<', end-1)` finds `p`, keep span `[p,end)` as a tag and set `end = p`; else if `text[end-1]` is a mark character, drop it and `--end`; else stop. Result is `text[0,end)` followed by the kept tags in original order. Leading end is the mirror image: skip `<...>` spans (from `<` to the next `>`), drop marks, stop at anything else. When the two ends meet (a string of only marks and tags, or a lone `\r`) the function must not read out of range or duplicate characters. An empty `<b></b>` left behind is fine (Felix's `strip_tags` removes it). Does not throw.

## Scope
1. Test first: `WordAssist/WordAssist/word_boundary_marks_test.cpp`, added to `Test_WordAssist.vcxproj` (and `.filters` if the other tests are listed there). Cases: `blue lantern\r` -> `blue lantern`; `cell\r\a` -> `cell`; leading `\r` removed; `\v`, `\f`, `\n`, 0x0E at the ends removed; mark in the middle kept; trailing space kept; space followed by `\r` -> space kept; `<font face="Calibri">blue lantern\r</font>` -> `<font face="Calibri">blue lantern</font>`; `<b>x</b>\r` -> `<b>x</b>`; `x<b>\r</b>` -> `x<b></b>`; `\r` -> empty; empty -> empty; text without marks unchanged, including `&lt;` entities. Add the leading-end variants (`\r<b>x</b>`, `<b>\rx</b>` -> `<b>x</b>`) and a string of only tags and marks.
2. Run it and see it fail (header missing).
3. Implement the header. Include it from `WordController.cpp` (`common` is on the include path through `Dependencies.props`).
4. `OnConcordanceAction`: right after `get_selection_text`, convert to `wstring`, trim, convert back (`BSTR2wstring` / `string2BSTR`), before the existing `text.length() == 0` check. `OnTransConcordanceAction`: the same, on the argument passed to `TransConcordance`.
Exclusions: Look up, Set translation, Register glossary, auto-translate, PowerPoint, Felix, `WordParser.cpp`.

## Verification
- Automated: build `Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj` `Debug|x64` and run `Test_WordAssist\Test_WordAssist\x64\Debug\Test_WordAssist.exe --report_level=short` (confirm the actual output folder). Expected: baseline (documented 134) plus the new cases, all passing. Also build `WordAssist\WordAssist\WordAssist.vcxproj` `Debug|x64` so the call sites compile.
- Manual (64-bit Word, AC1-AC4), after `tools\register-felix.ps1 -Configuration Debug`:
  1. Start Felix with a memory containing `The blue lantern is on.` (and a trans-mode entry `lanterne bleue` for step 5). Create a document via COM: `$w = New-Object -ComObject Word.Application; $w.Visible=$true; $d=$w.Documents.Add(); $d.Content.Text="The blue lantern is on."`. Felix reports `Found N concordances for string [...]` in its concordance feedback; read it with UI Automation on the Felix window, or from `%LOCALAPPDATA%\Felix\logs\felix.log` if it logs the line (check first).
  2. AC1: use `$w.Selection.SetRange(start,end)` so the selection covers `blue lantern` plus the paragraph mark (confirm `$w.Selection.Text` ends in a carriage return). Trigger Find Concordance with the add-in's shortcut (Alt+M then C; see `WordAssist/WordAssist/keyboard_shortcuts_word.cpp`) sent as keystrokes to the foreground Word window (a scratch `Add-Type` P/Invoke of `SendInput`, with `SetForegroundWindow` first), or with the toolbar button on the Add-ins tab via UI Automation. Expect `Found 1 concordances for string [blue lantern]`. Repeat in plain-text mode (hold Shift with the command).
  3. AC2: in a one-cell table select the whole cell text including the end-of-cell mark; expect a hit.
  4. AC3: select `blue lantern ` (trailing space, no paragraph mark); expect the string reported with the trailing space.
  5. AC4: in trans mode, `lanterne bleue` + paragraph mark: Translation Concordance finds the entry. Select only an empty paragraph's mark and run Find Concordance: nothing is sent and no error appears.
  6. Reproduce AC1 first on the unfixed build to confirm 0 hits, and record it. If an automated route proves unworkable, record what was tried and mark the check not run; do not claim it passed.

## Completion criteria
- [ ] New unit tests written first, seen failing, then passing; Test_WordAssist total = baseline + new cases, no failures.
- [ ] Two call sites trim; `get_selection_text` and `WordParser.cpp` unchanged (`git diff --stat` shows neither).
- [ ] Manual AC1-AC4 results (or a stated reason) in the Execution record.
- [ ] Committed; index row 01 updated with status and hash.

## Execution record
