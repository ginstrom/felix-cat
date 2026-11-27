# 04 - #9: Word activation check and hook diagnostics

Index: [index.md](index.md). Design: [sections 3 (#9), 5.2, 8 D4](../../designs/office-addin-issues-8-11.md). Depends on: none (the AC7 `queued` log line comes from step 01's decorator once step 03 wires it). Size: S.

## Objective
Word shortcuts such as Alt+1 run whenever focus is in an enabled Word main window, even straight after switching back from Felix; and each hook leaves a debug log line saying why it passed an Alt/Ctrl keystroke on, so a recurrence of #9 can be diagnosed.

## Requirements (local copy)
- R6: in Word, a mapped shortcut whose key is aimed at a Word main window (the #2 guard, `keystroke_target_is_main_window(GetFocus(), L"OpusApp")`) runs regardless of which child HWND had focus at the last activation event. `WordApplication::is_active` keeps its "Word object exists and a document window is active" meaning but no longer compares HWNDs.
- R7: each add-in's hook logs, at debug level, why it passed on an Alt/Ctrl keystroke (dialog guard, shortcuts disabled, not mapped or mapper declined). Keys without Alt or Ctrl are not logged (keeps the log small).
- D4: fix now on the inferred cause. If QA still fails Alt+1 and the log shows no line for it, Office is taking Alt+digit before the hook; that is a new decision, not part of this step.

## Prerequisites and existing code (inspected)
- `common/WordAutomationObject.cpp`, `WordApplication::is_active()` (~line 38): returns false unless `m_active` is set and `::GetFocus() == m_active_window` (~lines 53-57); falls through `WORD_TRY`/`WORD_CATCH`, returning true from the catch path. `m_active_window` is captured by `activate()` (`::GetFocus()`) on WindowActivate/DocumentChange/NewDocument/DocumentOpen/WindowSelectionChange events (`WordAssist/WordAssist/Connect.cpp` ~2434-2545) and cleared by WindowDeactivate. `m_active_window`, `activate()` and `active_window_has_focus()` stay; `new_window_has_focus()` still uses them.
- `WordController::IsActive` (`WordController.cpp` ~2228) calls `is_active`; it is also used by `OnMenuPreferences` and the segmentation swap, which benefit.
- Hooks: Word `WordAssist/WordAssist/hooker.cpp` (`KeyboardProc` -> `new_keyboard_proc(wParam, flags)` returning bool; the `#2` guard is in `KeyboardProc` before it), Excel `ExcelAssist/ExcelAssist/Hooker.cpp`, PowerPoint `PowerPointAssist/PowerPointAssist/Hooker.cpp`. Each returns `CallNextHookEx` (or false) at: the `KF_DLGMODE | KF_UP | KF_MENUMODE` flag check / the #2 guard failing, `! shortcuts_enabled`, and when `hook_keys->receive_keypress(...)` returns false. PowerPoint also has `if (control_key_pressed && shift_key_is_pressed()) return CallNextHookEx(...)` (a Ctrl+Shift pass-on; log it as `not mapped or declined`).
- Existing tests: `Test_WordAssist` compiles `common\WordAutomationObject.cpp` and `WordAssist/WordAssist/WordAutomationObject_test.cpp` (suite `WordApplicationTest`, a case `BOOST_CHECK_NO_THROW(WordApplication())` at ~line 106). PowerPoint has `hooker_test.cpp` (trivial).

## Contracts
- Produced: **C7 hook lines**, debug level via `logging::log_debug`, one per pass-on of a key that had Alt or Ctrl down:
  - `Felix hook: passing on key (dialog guard)`: the flag check or the #2 guard decided to pass the key.
  - `Felix hook: passing on key (shortcuts off)`: `shortcuts_enabled` is false.
  - `Felix hook: passing on key (not mapped or declined)`: `receive_keypress` returned false.
  - Include the virtual-key code (hex) in the text after the reason, e.g. `... (dialog guard) vk=0x31`.
- Consumed (from step 01, for QA reference only): `Felix hook: queued <command>` / `Felix hook: declined <command>`.
- `WordApplication::is_active` becomes:
  ```cpp
  bool WordApplication::is_active()
  {
  	WORD_TRY
  		if ( ! is_instantiated() ) { deactivate() ; return false ; }
  		return m_active ;
  	WORD_CATCH
  	return true ;
  }
  ```
  (Match the file's existing macro and return conventions; the point is only that the `GetFocus() == m_active_window` comparison is removed.)

## Scope and implementation guidance
Test first:
1. `WordAutomationObject_test.cpp`: `WordApplication::is_active()` returns false when Word is not instantiated (a default-constructed `WordApplication`). The active path cannot be faked without Word and is covered by manual AC6 (QA). Confirm the test passes before and after (it documents the retained meaning) and add a second check that `is_active()` never touches `GetFocus()`-dependent state only if that can be tested without Word; otherwise do not invent one.
2. Make the `is_active` change.
3. Add the hook log lines in the three hooks. Keep each addition a one-line `logging::log_debug(...)` on the existing pass-on path; do not restructure the hooks. Guard the logging with a cheap `control_key_pressed || alt_key_pressed` test. A hook log call must not throw or call COM; wrap nothing else. If `logging` is not yet included in a hook file, include `logging.h`.
4. Update the comment above `is_active`/`active_window_has_focus` if it states the HWND rule.

Exclusions: no change to the #2 guard logic or class names, no change to key bindings, no new decorator behaviour, no logging for keys without Alt/Ctrl.

## Verification
```powershell
$ms = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
& $ms Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_WordAssist\Test_WordAssist\x64\Debug\Test_WordAssist.exe --report_level=short
& $ms WordAssist\WordAssist\WordAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms ExcelAssist\ExcelAssist\ExcelAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms PowerPointAssist\PowerPointAssist\PowerPointAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms Test_PPT\Test_PPT.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_PPT\x64\Debug\Test_PPT.exe --report_level=short
& $ms Test_Excel\Test_Excel.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_Excel\x64\Debug\Test_Excel.exe --report_level=short
```
- Totals: Test_WordAssist, Test_PPT, Test_Excel equal the counts after step 03 plus any case added here; all pass; the three add-ins build Debug|x64.
- Manual (QA, not this step): AC6 (Alt+1 inserts `Ouvrez la porte de service.` with no AutoSave prompt, also after clicking into Felix and returning by title bar and by Alt+Tab) and AC7 (one hook log line per Alt+digit press saying the command was queued). Record as deferred.

## Completion criteria
- [x] `is_active` no longer compares HWNDs; test for the not-instantiated case passes.
- [x] Hook log lines in Word, Excel and PowerPoint hooks per C7; no behaviour change otherwise.
- [x] Builds and test programs green, recorded.
- [x] Manual AC6/AC7 recorded as deferred to QA.
- [x] Committed; index row 04 updated.

## Execution record

Commit ef06fa5. Engineer: Sonnet 5.5.

- Test first: added `WordApplicationTest/is_active_false_when_not_instantiated` (documents the retained meaning; it passes before and after, since the active path needs Word). No test was added for "never touches GetFocus": not testable without Word.
- `common/WordAutomationObject.cpp`: removed the `GetFocus() == m_active_window` comparison from `is_active()`. `m_active_window`, `activate()` and `active_window_has_focus()` are unchanged (`new_window_has_focus()` still uses them). No comment in the file stated the HWND rule, so only a short note in `is_active` was added.
- New `common/hook_log.h` (`shortcuts::log_hook_pass_on(reason, vk, key_message)`): one place for the C7 text `Felix hook: passing on key (<reason>) vk=0x..`. It returns at once for key-up messages and for keys without Ctrl or Alt (two `GetKeyState`/flag reads, no allocation), makes no COM calls and swallows exceptions. Listed in the three add-in vcxprojs.
- Hooks: dialog guard logged at the flag check and the #2 guard (all three); shortcuts off (all three); not mapped or declined after `receive_keypress` returns false (all three; PowerPoint's Ctrl+Shift pass-on also logs "not mapped or declined"). In the Word hook `new_keyboard_proc` logs both of its own pass-on reasons, so `KeyboardProc` adds no second line. The Ctrl+Alt+F9 toggle and key bindings are unchanged.
- Deviation: the helper checks `KF_UP` so a key release with Ctrl/Alt held does not produce a second line per press (the task only says "one line per pass-on"; the key-up event is not a keystroke to diagnose). Modifier-only keys (VK_MENU, VK_CONTROL) pass on silently, as before.
- Commands (PowerShell, x64 Debug, exit 0 each): MSBuild `/t:Rebuild` of Test_WordAssist, WordAssist, ExcelAssist, PowerPointAssist, Test_PPT, Test_Excel. Test_WordAssist.exe: 175 of 175 passed (174 + 1); Test_PPT.exe: 108 of 108; Test_Excel.exe: 67 of 67. Release not rebuilt (no Release-specific change).
- Deferred to QA: AC6 (Alt+1 inserts the glossary entry with no AutoSave prompt, also after clicking into Felix and returning by title bar and by Alt+Tab) and AC7 (one hook log line per Alt+digit press saying `queued`, and a pass-on line when a key is passed on).
- Follow-up (coordinator, review): `log_hook_pass_on` is compiled only under `_DEBUG` (empty inline in Release). The add-ins never set a logging level, so Release logs at debug level too and every Ctrl/Alt key (Ctrl+C etc.) would otherwise cost a log write on the UI thread. Pass-on lines are therefore Debug-only; QA uses the Debug build. The `queued`/`declined` lines are unchanged.
