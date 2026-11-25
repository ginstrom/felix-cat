# Design: Office add-in fixes for 3.0.1 (issues #1, #2, #3)

## 1. Purpose and status

Three bugs in the Office add-ins, reported against Felix 3.0.0 with 64-bit Microsoft 365:

| Issue | Symptom | Root cause (confidence) |
|---|---|---|
| [#1](https://github.com/ginstrom/felix-cat/issues/1) P2 | Word **Find Concordance** finds 0 matches when the selection includes the paragraph mark | The add-in sends the Word paragraph mark (`\r`) to Felix as part of the query, and Felix's concordance is a substring search. (High) |
| [#2](https://github.com/ginstrom/felix-cat/issues/2) P2 | **Alt+N** in Excel's "Want to save your changes" prompt runs Felix's Next Translation and relaunches Felix | The add-in's keyboard hook runs for every keystroke on Excel's UI thread, including keys typed into Excel's own modal dialogs, and nothing checks where the key is going. (High) |
| [#3](https://github.com/ginstrom/felix-cat/issues/3) P3 | After **Get Translation**, the cell shows the translation but the formula bar keeps the old text until the cell is reselected | The add-in writes through `Selection.Characters.Text`, and Excel doesn't refresh the formula bar after a `Characters` write. (High, reproduced) |

**Status:** ready for planning. Three decisions are listed in section 8. Each has a recommendation, and the design below assumes the recommendation is accepted. None of them blocks starting work.

Each fix is local to one add-in, except the #2 helper, which goes in `common/` so the other hooks can use it. Felix.exe doesn't change.

## 2. Requirements and scope

### Requirements

- **R1 (#1).** Word **Find Concordance** and **Translation Concordance** send the query without Word structural marks at its ends. The marks are paragraph mark `\r` (0x0D), line feed (0x0A), end-of-cell/row mark (0x07), manual line break (0x0B), page/section break (0x0C) and column break (0x0E). They are removed from both ends of the query. Characters inside the query are kept.
- **R2 (#1).** Spaces, tabs, non-breaking spaces and all other characters at the ends are kept. A trailing space can be an intentional part of a concordance query, for example `lantern ` to skip `lanterns`.
- **R3 (#1).** R1 works for both the formatted (HTML) query and the plain-text query, so it must look past HTML tags at either end (e.g. `<font face="Calibri">blue lantern\r</font>`). It never removes or changes a tag.
- **R4 (#1).** If nothing is left after trimming (the selection was only a paragraph mark), **Find Concordance** sends nothing, as it already does for an empty selection. **Translation Concordance** passes the empty string to Felix, as it does today; Felix then shows its "empty query" feedback.
- **R5 (#2).** Excel Assist dispatches a keyboard shortcut, including the hard-coded Ctrl+Alt+F9 toggle, only if both of these hold when the key arrives:
  - The thread's focus window is inside an Excel main window (top-level ancestor of window class `XLMAIN`).
  - That main window is enabled.

  Otherwise the hook passes the key on (`CallNextHookEx`) without acting on it.
- **R6 (#2).** The same guard applies to the Word Assist hook (main class `OpusApp`) and the PowerPoint Assist hook (main class `PPTFrameClass`) (decision D2).
- **R7 (#2).** Pressing a shortcut in the worksheet or document behaves as before, including starting Felix if it isn't running (decision D3).
- **R8 (#3).** After **Get Translation** or a glossary-entry insertion (Alt+0..9) writes to a cell, Excel's formula bar shows the new text immediately.
- **R9 (#3).** Text is still written as a literal string: `001`, `1/2` and `=1+1` stay text and are not converted to a number, a date or a formula. Writing to a selected text box keeps working. A multi-cell selection keeps its extent and its active cell.

### Non-goals

- Trimming text for other Word commands (Look up, Set translation, Register glossary, auto-translate), for PowerPoint or Excel concordance, or inside Felix (decision D1).
- Changing whether commands start Felix (decision D3).
- Fixing the formula bar after Auto Translate (section 9).
- Anything for 32-bit Office beyond building it. Only 64-bit Office is available to test.

### Acceptance criteria

- **AC1 (R1, R3).** Reproduce #1 in Word with formatting on and then off (plain-text mode: Shift with the command): with `blue lantern¶` selected, Felix reports `Found 1 concordances for string [blue lantern]`.
- **AC2 (R1).** In a Word table, select a whole cell's text including the end-of-cell mark and run Find Concordance: the match is found.
- **AC3 (R2).** Selecting `blue lantern ` (trailing space, no ¶) sends `blue lantern ` unchanged. Felix reports the string with the space.
- **AC4 (R1, R4).** Trans mode: **Translation Concordance** on `lanterne bleue¶` finds the entry. Selecting only an empty paragraph's ¶ and running Find Concordance sends nothing and raises no error.
- **AC5 (R5).** Reproduce #2 as written: Alt+N in Excel's save prompt picks **Don't Save** (Excel's own accelerator), Excel closes, and Felix doesn't start.
- **AC6 (R5, R7).** In a worksheet with Felix running, Alt+N, Alt+P, Alt+Down, Alt+0..9 and Ctrl+Alt+F9 behave as before. With Felix closed, Alt+L in the worksheet starts Felix as before.
- **AC7 (R5).** With Excel's Format Cells dialog (Ctrl+1) or Felix's own Excel properties dialog open, Alt+N and Alt+L reach the dialog and Felix does nothing.
- **AC8 (R6).** The same checks as AC5 and AC6 pass in Word and in PowerPoint.
- **AC9 (R8).** Reproduce #3 as written: A1 and the formula bar both show the translation straight after Get Translation. The same holds after Alt+1 (glossary entry).
- **AC10 (R9).** With a memory entry whose translation is `001`, Get Translation stores the text `001`, and the formula bar shows `001`, not `1`. With A1:B3 selected and B2 active, Get Translation writes B2, the selection stays A1:B3 and B2 stays active. With a worksheet text box selected, Get Translation writes the text into the text box and the text box stays selected.
- **AC11.** The existing Test_WordAssist, Test_Excel and Test_PPT suites still pass, plus the new tests in section 7.

## 3. Current system (inspected)

### #1 Word concordance

- Every route to Find Concordance (the toolbar's [`CConnect::OnConcordance`](../../WordAssist/WordAssist/Connect.cpp), the Alt+M,C shortcut through `key_mapper_word.h`, and `WordInterface.h`) calls `WordController::OnConcordanceAction` ([WordController.cpp:793](../../WordAssist/WordAssist/WordController.cpp)). In trans mode, or when the classic interface is off, it forwards to `OnTransConcordanceAction` (line 1404).
- Both call `WordController::get_selection_text(as_plaintext)` (line 1664). That builds a `WordParser` from **`WordAssist/WordAssist/WordParser.cpp`**, not `common/WordParser.cpp`, which Word Assist doesn't compile. Note that this file is UTF-16. It returns `segment_as_html()` or `segment_as_plaintext()`, and both end in `parse_range_text`, which copies `range.get_text()` verbatim. Nothing removes `\r`. Characters are HTML-escaped (`&`, `<`, `>`) on both paths, so any `<`/`>` left in the result is a tag.
- With Shift+End from the start of a paragraph, Word's selection text is `blue lantern\r`. A table cell's text ends in `\r\a`.
- On the Felix side, `CApp::Concordance` → `MemoryWindowFrame::get_concordances` → `search_query_params::set_source` (`strip_tags` only) → `memory_local::perform_search`, a substring test. `blue lantern\r` isn't a substring of `The blue lantern is on.`, hence 0 hits. Felix's feedback shows the `\r` as the trailing space in `[blue lantern ]`.
- **Look up** uses the same `get_selection_text`, so it also sends `\r`, but Felix's lookup is a fuzzy match, so a stray `\r` costs one character of score rather than every result. This is inferred, not measured.
- Auto-selected segments (Look up Next, auto-translate) never include the ¶: `WordParser::select_next_sentence` stops before `\r` and `eat_whitespace` skips it. So the ¶ only gets in through a manual selection.
- **PowerPoint** `CPowerPointInterface::OnConcordanceAction` ([PowerPointInterface.cpp:351](../../PowerPointAssist/PowerPointAssist/PowerPointInterface.cpp)) sends `textRange->Text` unmodified, and PowerPoint separates paragraphs with `\r`, so it probably has the same defect. Not verified, and out of scope (D1).

### #2 Keyboard hooks

- Excel's `KeyboardProc` ([Hooker.cpp:23](../../ExcelAssist/ExcelAssist/Hooker.cpp)) is a thread-local `WH_KEYBOARD` hook on Excel's UI thread. Its only context check is `KF_DLGMODE | KF_MENUMODE | KF_UP` in the high word of `lParam`. `KF_DLGMODE`/`KF_MENUMODE` are reserved bits for `WH_KEYBOARD` and aren't set for keys typed into Office's modern dialogs. Anything else with Alt or Ctrl goes to `KeyboardShortcuts::receive_keypress` ([keyboard_shortcuts.cpp:62](../../common/keyboard_shortcuts.cpp)). Alt+N maps to `CmdNext` ([keyboard_shortcuts_excel.cpp:52](../../ExcelAssist/ExcelAssist/keyboard_shortcuts_excel.cpp)) → `CFelixExcelIF::OnNext` → `getAssistant()`. `realGetAssistant` (FelixExcelIF.cpp:531) finds the cached `Felix.App` dead, calls `CreateInstance(L"Felix.App")` again and makes it visible. That's the relaunch. The hook returns 1, so the prompt never gets the key.
- **Probe (run for this design, 64-bit Microsoft 365):** Excel, Word and PowerPoint were started through COM, a document was dirtied, `WM_CLOSE` was posted to the main window, and `GetGUIThreadInfo` was read on the UI thread:

  | App | Normal: focus → top-level | Save prompt: focus → top-level | Main window enabled during prompt |
  |---|---|---|---|
  | Excel | `EXCEL7` → `XLMAIN` | `NetUIHWND` → `NUIDialog` (owner `XLMAIN`) | No |
  | Word | (not focused in probe) → `OpusApp` | `NetUIHWND` → `NUIDialog` | No |
  | PowerPoint | `mdiClass` → `PPTFrameClass` | `NetUIHWND` → `NUIDialog` | No |

  The prompt runs on the same UI thread, so it gets the hook too, and its focus is in a top-level window that isn't the app's main window.
- The Word hook ([hooker.cpp:21](../../WordAssist/WordAssist/hooker.cpp)) and the PowerPoint hook ([Hooker.cpp:21](../../PowerPointAssist/PowerPointAssist/Hooker.cpp)) have the same structure and the same flag-only check. Both map Alt+N to Next (`keyboard_shortcuts_word.cpp:55`, `keyboard_shortcuts_ppt.cpp:57`), and in both the save prompt's **Don't Save** button uses N. So they have the same defect.
- Every command starts Felix if it isn't running. That's `realGetAssistant`, and it's the same for toolbar buttons and shortcuts.

### #3 Excel cell write

- `CFelixExcelIF::OnGet` and `OnEntry` call `set_active_cell_text` ([FelixExcelIF.cpp:1030](../../ExcelAssist/ExcelAssist/FelixExcelIF.cpp)). It writes `m_excel_app->GetSelectedChars()->SetText(...)`, which is `Selection.Characters.Text`. If that throws, it falls back to `set_cell_text(ActiveCell)`, which does `Range.Characters.Text`, then `FormulaR1C1`.
- `Selection.Characters` is used because the selection can be a **text box** (see `get_selection_text`'s comment and log text). It also writes literal text.
- **Probe (run for this design; out-of-process COM; formula bar read through UI Automation, AutomationId `FormulaBar`):**
  - After `Selection.Characters.Text = "La lanterne…"` the cell shows the translation but the formula bar still shows the source. Reproduced.
  - After `Selection.Value2 = …` the formula bar updates.
  - Literal semantics: `Characters.Text` stores `001`, `1/2` and `=1+1` as text. `Value2` and `Formula` turn them into `1`, the date serial `46024` and a formula. So switching to `Value2` would corrupt translations.
  - Refresh options after a `Characters` write: `ActiveCell.Activate()`, `ActiveCell.Select()`, `Selection.Select()`, `Application.Goto(ActiveCell)` and toggling `DisplayFormulaBar` all refresh the formula bar. Toggling `ScreenUpdating` or `Interactive` doesn't.
  - Multi-cell selection (A3:B5, B4 active): `Selection.Characters.Text` throws ("Unable to set the Text property"), so the fallback writes the active cell. `ActiveCell.Activate()` keeps the selection and active cell. `Selection.Select()` moves the active cell to A3.
- The add-in doesn't handle `SelectionChange` (Connect.h sinks only SheetActivate, SheetDeactivate and WorkbookBeforeClose), so re-activating the cell triggers no Felix action.
- **Other write paths:**
  - Get & Next = `OnGet` + `OnLookupNext`. It gets the fix through `OnGet`, and the selection moves anyway.
  - Glossary entry (Alt+0..9) uses `set_active_cell_text`. Fixed.
  - Set and Set & Next only read.
  - Excel Assist has no Restore command.
  - Auto Translate (selection, sheet, workbook) writes through `set_cell_text` → `Range.Characters.Text`. It could leave the formula bar stale for the active cell. Out of scope (section 9).

## 4. Proposed behavior

- **#1.** Find Concordance and Translation Concordance in Word ignore Word boundary marks at the ends of the selection. `blue lantern¶` queries `blue lantern`. Everything else in the selection is sent as before.
- **#2.** In Excel, Word and PowerPoint, Felix shortcuts work only while the keyboard focus is in the application's own main window and that window isn't disabled by a modal dialog. In a save prompt, Excel's Format Cells dialog, Felix's own properties dialog or any other dialog, the keystroke goes to the dialog. If Felix isn't running, a shortcut in the sheet or document still starts it (D3).
- **#3.** After Get Translation or a glossary-entry insertion into a cell, the formula bar matches the cell straight away. The selection and active cell don't change. With a text box selected, nothing changes.
- **Failure behavior:** the #1 trimming can't fail. If the #2 guard can't tell where a key is going (focus is NULL), it treats the key as not for Felix and passes it on. If the #3 refresh fails, the failure is logged and ignored, because the write itself has already succeeded.

## 5. Technical design

### 5.1 #1: trim boundary marks from the Word concordance query

**New header (proposed) `common/word_boundary_marks.h`**, header-only and inline, so no add-in project needs a new source file. It lives in `common/` so PowerPoint can adopt it later with one line.

```cpp
// Removes Word structural marks (0x07, 0x0A-0x0E) from both ends of a selection
// string, looking past HTML tags at the ends. Never removes tags or other characters.
std::wstring trim_word_boundary_marks(const std::wstring &text);
```

Algorithm (trailing end; the leading end is the mirror image):

1. Let `end = text.size()`. Keep a list of tag spans to keep.
2. While `end > 0`:
   - If `text[end-1] == L'>'` and `text.rfind(L'<', end-1)` gives a position `p`, treat `[p, end)` as a tag. Keep it, set `end = p`, and continue.
   - Otherwise, if `text[end-1]` is one of `0x07, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E`, drop it, decrement `end`, and continue.
   - Otherwise stop.
3. The result is `text[0, end)` followed by the kept tags in their original order.

Leading end: skip `<…>` spans forward from the start (a span runs from `<` to the next `>`), drop mark characters, and stop at any other character. An empty `<b></b>` left behind is harmless: Felix's `strip_tags` removes it.

The rule that "any `<…>` at the end is a tag" holds because both parser paths escape literal `<`, `>` and `&`. The one exception is `segment_as_plaintext`'s `_com_error` fallback, which returns raw range text. There the worst case is that a mark next to a literal `<…>` gets removed, which is acceptable.

**Call sites (WordController.cpp):**

- `OnConcordanceAction`: `text = trim_word_boundary_marks(text)` straight after `get_selection_text`, before the existing `text.length() == 0` check (R4). Convert between `_bstr_t` and `wstring` with `BSTR2wstring` / `string2BSTR`, which the file already uses.
- `OnTransConcordanceAction`: the same, applied to the argument passed to `TransConcordance`.

`get_selection_text` itself doesn't change (D1). Neither does `WordParser.cpp`, so the engineer doesn't need to edit the UTF-16 file.

### 5.2 #2: only dispatch shortcuts aimed at the app's main window

**New function in `common/keyboard_shortcuts.h/.cpp`.** All three add-ins and all three test programs already compile `keyboard_shortcuts.cpp`, so no add-in project needs a new file.

```cpp
// True if keystrokes for `focus` should be treated as Felix shortcuts: `focus` is
// non-NULL, its top-level ancestor (GetAncestor(focus, GA_ROOT)) has window class
// `main_window_class` (case-insensitive), and that top-level window is enabled.
bool keystroke_target_is_main_window(HWND focus, LPCWSTR main_window_class);
```

Hook changes. Each is one early return, placed after the existing up-key and modifier-only checks and before the Ctrl+Alt+F9 toggle:

| File | Main window class |
|---|---|
| `ExcelAssist/ExcelAssist/Hooker.cpp` `KeyboardProc` | `L"XLMAIN"` |
| `WordAssist/WordAssist/hooker.cpp` `KeyboardProc` (before `new_keyboard_proc`) | `L"OpusApp"` |
| `PowerPointAssist/PowerPointAssist/Hooker.cpp` `KeyboardProc` | `L"PPTFrameClass"` |

```cpp
if ( ! keystroke_target_is_main_window( ::GetFocus(), L"XLMAIN" ) )
{
    return CallNextHookEx(hkb, nCode, wParam, lParam);
}
```

How the parts fit together:

- `GetFocus()` is right here because the hook is thread-local and runs on the thread that owns the focus window.
- A class name is used instead of `Application.Hwnd` for three reasons: Excel 2013+ and Word 2013+ create one main window per workbook or document (SDI); the check makes no COM call inside a hook while the app may be closing; and the class names are stable across Office 2007 to Microsoft 365.
- The "enabled" test covers modal states where focus hasn't left the main window.
- The existing `KF_DLGMODE | KF_MENUMODE` check stays.
- A two-key shortcut that is half typed (Alt+M, then the second key) isn't affected. If the second key goes to a dialog it never reaches `receive_keypress`, and the pending state clears on the next key that does.

The Felix launch policy doesn't change (D3).

### 5.3 #3: refresh the formula bar after writing the active cell

**`excel::application` ([excel_application.h](../../ExcelAssist/ExcelAssist/excel_application.h))** gets a new virtual method:

```cpp
// If the selection is a worksheet range, re-activates the active cell so Excel
// refreshes the formula bar. Keeps the selection and active cell. No-op otherwise.
virtual void RefreshActiveCell()
{
    Excel::RangePtr selected_range = m_pApplication->Selection ; // E_NOINTERFACE -> NULL for shapes
    if ( selected_range )
    {
        m_pApplication->ActiveCell->Activate() ;
    }
}
```

`Activate` is used rather than `Select`, because `Select` moves a multi-cell selection's active cell. A selected text box is left alone, because activating the cell would take the selection away from the text box.

**`set_active_cell_text` (FelixExcelIF.cpp).** The write logic stays as it is (Characters first, then the `set_cell_text` fallback). After either write succeeds:

```cpp
try { m_excel_app->RefreshActiveCell() ; }
catch ( ... ) { logging::log_warn("Failed to refresh the formula bar after writing the cell") ; }
```

To make the routing testable without CFelixExcelIF (which needs Felix.tlb), move the body into a free function in a new header (proposed) `ExcelAssist/ExcelAssist/excel_selection_writer.h`:

```cpp
// Writes literal text to the selection (text box or cell), falling back to the
// active cell, then refreshes the formula bar. Returns false if text is empty.
bool write_selection_text(excel::app_ptr app, _bstr_t text);
```

`set_active_cell_text` keeps its empty-text log message and calls this function. `set_cell_text` stays a free function in FelixExcelIF.cpp. The new header declares it or moves it, whichever needs fewer includes; that's the engineer's call.

**Fakes ([fake_excel.h](../../ExcelAssist/ExcelAssist/fake_excel.h)):**

- `FakeApplication` overrides `RefreshActiveCell` to record a call, with an option to throw.
- `FakeRange::SetText` already records the text.

## 6. Consequences

- **Compatibility:**
  - #1 changes only what Word concordance sends. #2 changes only when shortcuts fire.
  - Users who relied on shortcuts while a dialog had focus lose that, which is the point of the fix.
  - PowerPoint shortcuts stop firing in Slide Show, whose window class is `screenClass`. That's acceptable: no Felix command works on a slide show.
  - #3 keeps the literal-text write, so stored cell content is the same as before.
- **Risk:**
  - The hook guard is the change most likely to cause a regression. Shortcuts typed while focus is in a part of the main window, such as the in-cell editor, the formula bar or the ribbon, still pass the guard, because those windows sit under `XLMAIN`/`OpusApp`. Floating windows that aren't children of the main window would no longer get shortcuts. None is known.
  - `RefreshActiveCell` adds one COM round trip per Get.
- **Performance:** the guard costs two or three cheap user32 calls per keystroke that gets past the existing checks.
- **Rollback:** each fix is independent and can be reverted alone.
- **Release:** the add-ins ship in the 3.0.1 installer. Felix.exe is unchanged. Whether to note the fixes in `setup/release-notes-3.0.1.md` is already on the orchestrator's list for the user.

## 7. Verification

### Unit tests (Boost.Test, `Testing` configuration, x64)

- **Test_WordAssist.** New `WordAssist/WordAssist/word_boundary_marks_test.cpp` (or under `Test_WordAssist/Test_WordAssist/`), added to `Test_WordAssist.vcxproj`. Cases:
  - `blue lantern\r` → `blue lantern`
  - `cell\r\a` → `cell`
  - leading `\r` removed
  - `\v`, `\f` and `\n` at the ends removed
  - a mark in the middle kept
  - trailing space kept, and space followed by `\r` → space kept
  - `<font face="Calibri">blue lantern\r</font>` → `<font face="Calibri">blue lantern</font>`
  - `<b>x</b>\r` → `<b>x</b>`
  - `x<b>\r</b>` → `x<b></b>`
  - `\r` → empty
  - empty → empty
  - text without marks unchanged, including `&lt;` entities
- **Test_Excel.** New `Test_Excel/keystroke_target_test.cpp`. The test registers a private window class (e.g. `FelixTestMain`) and a second class in the test process, creates hidden windows and checks:
  - NULL → false
  - main window → true
  - child of the main window → true
  - grandchild → true
  - child after `EnableWindow(main, FALSE)` → false
  - owned top-level popup of another class → false
  - main class matched case-insensitively
  - wrong class name → false

  Destroy the windows and unregister the classes in a fixture. If D2 is accepted, compile the same file into Test_WordAssist and Test_PPT too. It only needs `keyboard_shortcuts.h`.
- **Test_Excel.** New `Test_Excel/excel_selection_writer_test.cpp` using `FakeApplication`:
  - the chars path writes and refreshes once
  - chars throw → the active cell's text is written and refreshed once
  - empty text → no write, no refresh, returns false
  - refresh throws → no exception escapes and the text is still written

  This needs `ExcelImport.h` (Excel `#import` by libid) in Test_Excel. If that turns out to be impractical in the test project, record it and rely on the manual check for #3; don't add Felix.tlb to Test_Excel.
- Run the three test programs and confirm the existing totals (134, 65, 49) plus the new cases pass.

### Manual Office checks (64-bit Microsoft 365, x64 Debug add-ins registered with `tools\register-felix.ps1`)

Run AC1–AC10. For #3 the formula bar can also be read without looking: UI Automation, element with AutomationId `FormulaBar` under the `XLMAIN` window, `ValuePattern` or `TextPattern`. That's how the root cause was confirmed. For #2, check before and after the fix with Task Manager or the Felix log that no `Felix.exe` starts while the prompt is open.

32-bit Office can't be tested here. The x86 add-ins only need to build.

## 8. Decisions and assumptions

**Decisions for the user. Each has a recommendation, and the design assumes it is accepted.**

- **D1. Where the #1 trimming applies.**
  - **Recommended:** Word Find Concordance and Translation Concordance only. Those are the reported paths, and the trimming is applied at those two call sites.
  - Alternative: trim in `WordController::get_selection_text`, so Look up, Set translation, Register glossary and correction also stop picking up a stray ¶. That's arguably better, but it changes stored translations and lookup scores, and the auto-translate loops treat an empty result as end-of-document, so it needs more testing.
  - Alternative: also apply it to PowerPoint concordance (one line each in `PowerPointInterface.cpp`) or in Felix (helps every client but has to handle HTML on the Felix side).
  - Either alternative can be a follow-up issue.
- **D2. Whether the #2 guard covers Word and PowerPoint in this change.**
  - **Recommended:** yes. Same shared helper, one line per hook. The probe showed the same modal structure, and both map Alt+N to Next, which collides with **Don't Save**.
  - Alternative: Excel only, with follow-up issues for the other two.
- **D3. Whether shortcuts may start Felix.**
  - **Recommended:** no change. Shortcuts, like toolbar buttons, start Felix when it isn't running. The guard removes the reported relaunch.
  - Alternative: make shortcuts use only a running Felix. That would change documented behavior for every command, and a user pressing Alt+L in a fresh session would see nothing happen.

**Assumptions:**

- Excel, Word and PowerPoint main windows have the classes `XLMAIN`, `OpusApp` and `PPTFrameClass` in every supported Office version. Observed on Microsoft 365 16.0. The first two are long-standing, documented class names.
- `ActiveCell.Activate()` refreshes the formula bar when called in-process from the add-in, as it does from out-of-process COM in the probe.
- Excel Assist commands can't run while a cell is in edit mode, so "partial character selection" inside a cell doesn't happen. `Selection.Characters` always covers the whole cell or text box.

## 9. Out of scope: problems noticed

- **Auto Translate** (selection, sheet, workbook) writes cells through `Range.Characters.Text` and may leave the formula bar stale for the active cell. A `RefreshActiveCell()` at the end of each auto-translate command would fix it.
- **Excel shortcuts in cell edit mode:** the hook fires and the command's COM calls fail with a "COM Error" message box. The guard doesn't change this, because the cell editor is under `XLMAIN`.
- `FelixExcelIF_test.cpp` and `Connect_test.cpp` are old easyunit tests that no project builds. `fake_excel.h` is only usable under `_DEBUG`.
- Word Look up with a trailing ¶ sends `\r` (see D1).
- Excel's `KeyboardProc` keeps a large commented-out legacy switch, which is dead code.
