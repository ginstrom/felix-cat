# 03 - Wire the guard into the Excel, Word and PowerPoint hooks (#2)

Index: [index.md](index.md). Design: [section 5.2](../../designs/office-addin-fixes-3.0.1.md). Depends on: 02. Size: S for the code, M for manual verification.

## Objective
Shortcuts are dispatched only while keyboard focus is in the app's enabled main window. Decision D2: all three hooks. Decision D3: shortcuts still start Felix when it is not running.

## Requirements (local copy)
- R5/R6 as in step 02.
- R7: pressing a shortcut in the worksheet or document behaves as before, including starting Felix if it is not running.
- If focus is NULL the key is passed on. The existing `KF_DLGMODE | KF_MENUMODE | KF_UP` check stays. A half-typed two-key shortcut is unaffected.
- Accepted consequence: PowerPoint shortcuts stop in Slide Show (`screenClass`).

## Prerequisites
From step 02 (must exist when this step runs): `bool shortcuts::keystroke_target_is_main_window(HWND focus, LPCWSTR main_window_class);` in `common/keyboard_shortcuts.h`. Existing code (inspected): `ExcelAssist/ExcelAssist/Hooker.cpp` `KeyboardProc` (line 23: stop-flags check, then the VK_MENU/VK_CONTROL check, then the Ctrl+Alt+F9 toggle); `WordAssist/WordAssist/hooker.cpp` `KeyboardProc` (line 21; calls `new_keyboard_proc`, which holds the F9 toggle at line ~121); `PowerPointAssist/PowerPointAssist/Hooker.cpp` `KeyboardProc` (line 21; toggle at line ~47).

## Contract (identical in all three)
```cpp
if ( ! shortcuts::keystroke_target_is_main_window( ::GetFocus(), L"<class>" ) )
{
    return CallNextHookEx(hkb, nCode, wParam, lParam);
}
```
Placed after the up-key and modifier-only (VK_MENU / VK_CONTROL) checks and before the Ctrl+Alt+F9 toggle (in Word: before the call to `new_keyboard_proc`; in Word the guard sits inside the existing `try`). Classes: Excel `XLMAIN`, Word `OpusApp`, PowerPoint `PPTFrameClass`. Include `keyboard_shortcuts.h` in the hook source if it is not already reachable.

## Scope
Edit only those three `KeyboardProc` functions. Leave Excel's commented-out legacy switch alone. Do not change Felix launch behavior (`realGetAssistant`).

## Verification
- Automated: build `ExcelAssist`, `WordAssist` and `PowerPointAssist` `Debug|x64`; re-run the three test programs (Test_PPT compiles `Hooker.cpp` and `hooker_test.cpp`; Test_WordAssist compiles `hooker.cpp`). Totals unchanged from step 02, all passing.
- Manual (64-bit Office, Debug x64 add-ins registered with `tools\register-felix.ps1 -Configuration Debug`). Drive Office through COM from PowerShell and inject keys with a scratch `SendInput` P/Invoke into the foreground window (`SetForegroundWindow` first). Detect a Felix launch with `Get-Process Felix` and `%LOCALAPPDATA%\Felix\logs\felix.log`.
  1. AC5 (Excel): start Excel (`New-Object -ComObject Excel.Application`, `Visible=$true`), dirty a workbook, make sure Felix is not running, `PostMessage(WM_CLOSE)` to `$xl.Hwnd` so the "Want to save your changes" prompt appears, send Alt+N. Expect: Don't Save is chosen, Excel closes, no `Felix.exe` starts. First reproduce the failure on the unfixed build (Felix starts) and record it.
  2. AC6 (Excel): with Felix running and a worksheet cell focused, Alt+N, Alt+P, Alt+Down, Alt+0..9 and Ctrl+Alt+F9 behave as before (Felix reacts; the F9 toggle beeps and turns shortcuts off and on). With Felix closed, Alt+L in the worksheet starts Felix.
  3. AC7: with Format Cells open (Ctrl+1) or Felix's own Excel properties dialog open, Alt+N and Alt+L reach the dialog and Felix does nothing.
  4. AC8: repeat 1 and 2 in Word (dirty a document, `PostMessage(WM_CLOSE)` to `$w.ActiveWindow.Hwnd`) and in PowerPoint (`$ppt.Windows(1).HWND`; ensure a normal editing window, not Slide Show).
  If a probe cannot be automated reliably, record what was tried and mark that check not run; do not claim a pass.

## Completion criteria
- [x] Three guards added, identical in shape, correct class per app.
- [x] Builds green, three test programs pass, totals unchanged from step 02.
- [x] Manual AC5-AC8 outcomes in the Execution record (or stated reasons): deferred to QA.
- [x] Committed; index row 03 updated.

## Execution record
- Red/green: the guard lives in the hook procs (`KeyboardProc`), which the test projects do not call (no hook is installed under test), so there is no unit test for the wiring; the helper itself is covered by step 02's 8 tests. Verification check defined first: grep for the guard in the three hooks (absent before, present after) plus clean builds.
- Edits: one guard inserted per contract in `ExcelAssist/ExcelAssist/Hooker.cpp` (XLMAIN, after the VK_MENU/VK_CONTROL check, before the Ctrl+Alt+F9 toggle), `PowerPointAssist/PowerPointAssist/Hooker.cpp` (PPTFrameClass, same place), `WordAssist/WordAssist/hooker.cpp` (OpusApp, inside the existing try, before `new_keyboard_proc`). CRLF preserved; all three headers already include keyboard_shortcuts.h.
- Builds (PowerShell, MSBuild by full path, /p:Configuration=Debug /p:Platform=x64): ExcelAssist.dll, WordAssist.dll, PowerPointAssist.dll built, no errors. Test_Excel, Test_PPT, Test_WordAssist built, no errors. Runlog not used (needs Python).
- Tests (<exe> --report_level=short, exit 0): Test_Excel 57/57 cases, 160 assertions; Test_PPT 73/73, 201 assertions; Test_WordAssist 161/161, 492 assertions. Unchanged from step 02.
- Manual AC5-AC8: not run, deferred to QA (needs the debug Felix and add-ins registered, which needs admin/UAC; Office not launched). Nothing was claimed as passed. Probe plan is in Verification above.
- Deviations: none. Note for QA: PowerPoint shortcuts now do nothing in Slide Show (accepted consequence).
