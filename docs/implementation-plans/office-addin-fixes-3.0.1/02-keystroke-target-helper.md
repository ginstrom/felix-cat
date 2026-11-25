# 02 - Shared helper: keystroke_target_is_main_window (#2)

Index: [index.md](index.md). Design: [section 5.2](../../designs/office-addin-fixes-3.0.1.md). Depends on: none. Size: S-M.

## Objective
Add the shared guard function used by all three hooks (wired in step 03), with window-based unit tests compiled into Test_Excel, Test_WordAssist and Test_PPT.

## Requirements (local copy)
R5/R6: a shortcut may be dispatched only if the focus window is inside an application main window (top-level ancestor of the given class) and that main window is enabled. A NULL focus counts as "not for Felix". Class matching is case-insensitive. Rationale: modal dialogs (the save prompt has focus in `NetUIHWND` under top-level `NUIDialog`, and the main window is disabled) must receive their own keys.

## Contract produced
In `common/keyboard_shortcuts.h` and `.cpp`, inside the existing `namespace shortcuts`:
```cpp
// True if keystrokes for `focus` should be treated as Felix shortcuts: `focus` is
// non-NULL, its top-level ancestor (GetAncestor(focus, GA_ROOT)) has window class
// `main_window_class` (case-insensitive), and that top-level window is enabled.
bool keystroke_target_is_main_window(HWND focus, LPCWSTR main_window_class);
```
Implementation notes: `GetAncestor(focus, GA_ROOT)` (not `GA_ROOTOWNER`, so an owned popup of a different class is its own root and fails the class test), `GetClassNameW`, `_wcsicmp`, `IsWindowEnabled`. Handle a NULL root. No COM calls. `keyboard_shortcuts.cpp` is already compiled into all three test projects and all three add-ins, so no project needs a new source for the function. Check that `keyboard_shortcuts.h` has `HWND` available (it includes `config_file.h`, `key_mapper.h`; stdafx supplies windows.h).

## Scope
1. Tests first. One test file used by all three test programs: put it at `common/keystroke_target_test.cpp` (`common/` already holds shared tests such as `key_mapper_ppt_test.cpp`) and add it to `Test_Excel.vcxproj`, `Test_PPT.vcxproj` and `Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj` (and the `.filters` files if the others are listed there). It needs only `keyboard_shortcuts.h`. In the test, register two private window classes (main `FelixTestMain`, other `FelixTestOther`), create hidden windows on the test thread (no message loop needed), and destroy windows and unregister classes in a fixture. Cases: NULL -> false; main window -> true; child of main -> true; grandchild -> true; child after `EnableWindow(main, FALSE)` -> false; owned top-level popup (`WS_POPUP`, owner = main, class Other) -> false; main class passed in a different case (`felixtestmain`) -> true; wrong class name -> false. Mind the stdafx.h each project requires (include it first as the other tests do).
2. Run, see the build fail or tests fail, then implement the function and its header declaration.
Exclusions: no hook changes here (step 03).

## Verification
Build and run all three test programs `Debug|x64`. Expected: baselines (Excel 49, PPT 65, Word 134 plus whatever step 01 added) plus 8 new cases in each, all passing. Building the add-in projects is not needed in this step.

## Completion criteria
- [ ] Helper declared and defined exactly per the contract.
- [ ] 8 new cases pass in all three test programs; no regressions.
- [ ] Committed; index row 02 updated.

## Execution record
