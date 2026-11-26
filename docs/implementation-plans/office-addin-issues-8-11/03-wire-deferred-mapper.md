# 03 - #8: wire the deferred mapper into Word, Excel and PowerPoint

Index: [index.md](index.md). Design: [sections 5.1 (Wiring, Shift capture, Ordering effects) and 6](../../designs/office-addin-issues-8-11.md). Depends on: 01, 02. Size: S-M.

## Objective
Each add-in's `KeyboardShortcuts` uses a `DeferredKeyMapper` wrapping its app mapper, so shortcut commands run from the add-in's message window instead of inside the keyboard hook. Excel's Auto Translate reads Shift through the shared helper so the captured Shift state applies.

## Requirements (local copy)
- R1, R3, R4, R5 as in step 01, now live in all three add-ins (D2).
- The three hook procedures and `KeyboardShortcuts::receive_keypress` do not change. Chord handling stays synchronous in the hook: the first chord key is consumed with no command; the second key's command is queued and its key is always consumed; `ToggleShortcuts` (table entry or the hard-coded Ctrl+Alt+F9) stays synchronous.
- Shutdown: commands still queued at disconnect are dropped with the window.
- Rollback property: reverting #8 is the constructor argument plus `create`/`destroy` in each Connect; `accepts` can stay.

## Dependencies and prerequisites (from steps 01 and 02, copied)
- **C3** `common/deferred_key_mapper.h`: `shortcuts::DeferredKeyMapper(KeyMapper *inner)`, `bool create()`, `void destroy()`, `map_command`, `accepts`, `run_pending()`, `pending_count()`; message-only window on the calling thread; with no window it runs the inner mapper synchronously.
- **C2** `shortcuts::scoped_shift_state`; global `shift_key_is_pressed()` honours the override.
- **C1/C4** `accepts` implemented in `KeyMapperWord`, `KeyMapperPpt`, `KeyMapperExcel` (step 02).

## Scope and implementation guidance
Existing code (inspected; line numbers are from f1a6a4c):
- Word: `WordAssist/WordAssist/Connect.h:324-325` (`KeyMapperWord m_mapper; shortcuts::KeyboardShortcuts m_keyboard_shortcuts;`), `Connect.cpp:2056` initializer `m_keyboard_shortcuts(&m_mapper)`, `OnConnection` ~199-200 (`m_mapper.m_target = m_controller; installhook(&m_keyboard_shortcuts);`), `OnDisconnection` ~422 (`uninstallhook(&m_keyboard_shortcuts)`). The controller swap (Connect.cpp ~316, ~2604) only changes `m_mapper.m_target`; the decorator holds `&m_mapper`, so nothing changes there.
- Excel: `ExcelAssist/ExcelAssist/Connect.h:156-157`, `Connect.cpp:46` initializer, `OnConnection` ~237-239, `OnDisconnection` ~336.
- PowerPoint: `PowerPointAssist/PowerPointAssist/Connect.h:126-127` and the initializer at `Connect.h:134` (`m_keyboard_shortcuts(&m_mapper)`), `OnConnection` ~84-85, `OnDisconnection` (find the `uninstallhook` call).

Changes, in each of the three Connects:
1. `#include "deferred_key_mapper.h"` (path as the other `common` includes in that project).
2. Add `shortcuts::DeferredKeyMapper m_deferred_mapper;` declared **after** `m_mapper` and **before** `m_keyboard_shortcuts` (member init order matters), constructed with `&m_mapper` in the constructor's initializer list.
3. Construct `m_keyboard_shortcuts(&m_deferred_mapper)` instead of `&m_mapper`.
4. `m_deferred_mapper.create();` in `OnConnection` next to `installhook` (before it, so the window exists when the hook can fire); the call must run on the Office UI thread, as `installhook` does.
5. `m_deferred_mapper.destroy();` in `OnDisconnection` after `uninstallhook`. `destroy()` (and the destructor, which calls it) must run on the Office UI thread that created the window: `DestroyWindow` fails from another thread. (Implementation note, step 03: `destroy()` goes in `OnBeginShutdown` right after `uninstallhook`, because that is where Word and Excel remove the hook; PowerPoint, which never uninstalls its hook (pre-existing), calls it as the first statement of `OnBeginShutdown`'s try block; on an unload from the COM Add-ins dialog the destructor covers it.)
6. List `common\deferred_key_mapper.h` as a `<ClInclude>` in `WordAssist.vcxproj`, `ExcelAssist.vcxproj` and `PowerPointAssist.vcxproj` (and filters).

PowerPoint hook Shift read (added after the step 01 review): `PowerPointAssist/PowerPointAssist/Hooker.cpp:60` (`control_key_pressed && shift_key_is_pressed()`) runs in the hook, possibly while a deferred command's Shift override is active. Change it to `shortcuts::physical_shift_key_is_pressed()`. This one-line hook edit is the only exception to "no change to `Hooker.cpp`" below. Word's and Excel's hooks do not read Shift.

Excel Shift reads (R4): in `ExcelAssist/ExcelAssist/FelixExcelIF.cpp`, `OnAutoTransSel`, `OnAutoTransSheet` and `OnAutoTransWorkbook` read `::GetKeyState(VK_SHIFT)` directly (lines ~476, ~563, ~582). Change each to `shift_key_is_pressed()` (include `keyboard_shortcuts.h` if needed), keeping the same boolean meaning (cell colouring when Shift is down). Menu handlers are unaffected because no override is alive when they run.

If `create()` returns false (window not created), nothing else is needed: the decorator falls back to synchronous dispatch and has logged the failure.

There is no unit test for the Connect wiring (it needs Office). Tests in this step: none new; the existing suites must stay green. Do not add tests that need Office. Record manual AC1-AC5 as deferred to QA, and tell QA to look for `Felix hook: queued` lines in the add-in log (`%LOCALAPPDATA%\Felix\logs\`).

Exclusions: no hook edits (step 04), no change to `is_active` (step 04), no retry on COM errors, no Felix.exe change.

## Verification
```powershell
$ms = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
& $ms WordAssist\WordAssist\WordAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms ExcelAssist\ExcelAssist\ExcelAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms PowerPointAssist\PowerPointAssist\PowerPointAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_WordAssist\Test_WordAssist\x64\Debug\Test_WordAssist.exe --report_level=short
& $ms Test_Excel\Test_Excel.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_Excel\x64\Debug\Test_Excel.exe --report_level=short
& $ms Test_PPT\Test_PPT.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_PPT\x64\Debug\Test_PPT.exe --report_level=short
```
- All three add-ins build `Debug|x64` with exit 0; the three test programs pass with the counts from step 02.
- Also build the three add-ins `Debug|Win32` if they built before this branch (record the outcome; a pre-existing Win32 failure is a note, not a blocker).
- Inspect: `git diff` shows the same two-line pattern (member, constructor argument) plus `create`/`destroy` in each Connect, and no change to `Hooker.cpp`/`hooker.cpp`.

## Completion criteria
- [ ] Decorator wired in all three Connects with correct member order, `create()` and `destroy()` placement.
- [ ] Excel's three Shift reads go through `shift_key_is_pressed()`.
- [ ] Project files list the header; all builds and test programs green, recorded in the execution record.
- [ ] Manual AC1-AC5 recorded as deferred to QA.
- [ ] Committed; index row 03 updated.

## Execution record

### Engineer, 2026-11-26 (opus; base 84a5fa6)

Changes:
- Word, Excel, PowerPoint `Connect.h`: `#include "deferred_key_mapper.h"`; `shortcuts::DeferredKeyMapper m_deferred_mapper` declared between `m_mapper` and `m_keyboard_shortcuts`; constructors initialise `m_deferred_mapper(&m_mapper)` and `m_keyboard_shortcuts(&m_deferred_mapper)`.
- `OnConnection` (all three): `m_deferred_mapper.create()` immediately before `installhook(&m_keyboard_shortcuts)`, on the thread Office calls `OnConnection` on (the UI thread).
- Shutdown: `m_deferred_mapper.destroy()` (see deviations for placement). The destructor also calls `destroy()`; `CConnect` is released by Office on the UI thread.
- `PowerPointAssist/PowerPointAssist/Hooker.cpp`: the Ctrl+Shift guard now reads `shortcuts::physical_shift_key_is_pressed()` (the one allowed hook edit). No other change to any `Hooker.cpp`/`hooker.cpp`.
- `ExcelAssist/ExcelAssist/FelixExcelIF.cpp`: `OnAutoTransSel`, `OnAutoTransSheet`, `OnAutoTransWorkbook` set `m_shouldColor = shift_key_is_pressed()`; added `#include "keyboard_shortcuts.h"` (the first Debug build failed with C3861 without it).
- `common\deferred_key_mapper.h` listed as `<ClInclude>` (Header Files filter) in `WordAssist.vcxproj`, `ExcelAssist.vcxproj`, `PowerPointAssist.vcxproj` and their `.filters`.

Deviations:
- `destroy()` placement. The task says `OnDisconnection` after `uninstallhook`, but in Word and Excel `uninstallhook` is in `OnBeginShutdown`, not `OnDisconnection`, so `destroy()` sits right after it there. PowerPoint never calls `uninstallhook` (pre-existing); its `destroy()` is the first statement of `OnBeginShutdown`'s try block, so a throwing `DispEventUnadvise` cannot skip it. With the window gone, a later shortcut runs synchronously (R5 fallback, the pre-#8 behaviour). If the add-in is unloaded through the COM Add-ins dialog without `OnBeginShutdown`, the window lives until the `CConnect` destructor (UI thread).
- Win32 builds were not attempted: the brief scopes builds to Debug|x64 and Release|x64 (only 64-bit Office is installed).

Verification (run from PowerShell in the repository root; `$ms` = MSBuild 18 path; `runlog` unavailable, run directly):

| Command | Exit | Result |
|---|---|---|
| `& $ms WordAssist\WordAssist\WordAssist.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo` | 0 | 0 errors |
| same, ExcelAssist Debug\|x64 (first run, before the include) | 1 | 3 x C3861 `shift_key_is_pressed` in FelixExcelIF.cpp |
| same, ExcelAssist Debug\|x64 (after the include) | 0 | 0 errors |
| same, PowerPointAssist Debug\|x64 | 0 | 0 errors |
| same, WordAssist / ExcelAssist / PowerPointAssist Release\|x64 | 0 / 0 / 0 | 0 errors |
| `& $ms Test_WordAssist\...\Test_WordAssist.vcxproj /t:Rebuild` Debug / Release x64, then `Test_WordAssist.exe --report_level=short` | 0 / 0 | 174 of 174 passed (both) |
| `& $ms Test_Excel\Test_Excel.vcxproj /t:Rebuild` Debug / Release x64, then `Test_Excel.exe --report_level=short` | 0 / 0 | 67 of 67 (Debug), 60 of 60 (Release) |
| `& $ms Test_PPT\Test_PPT.vcxproj /t:Rebuild` Debug / Release x64, then `Test_PPT.exe --report_level=short` | 0 / 0 | 108 of 108 passed (both) |

No warnings were reported in the touched files. The `WARN`/`ERROR` trace lines in the Test_Excel and Test_PPT output come from existing failure-path tests. No new tests (Connect wiring needs Office).

Deferred to QA (manual, in Office with this build registered):
- AC1-AC5: Word Look up Next / Previous Translation no longer fail with 0x80010005; Ctrl+Right etc. still reach Office when the mapper declines; Shift-modified shortcuts (and Excel Shift+Auto Translate cell colouring) keep their Shift meaning; rapid repeated shortcuts run in order; chords and Ctrl+Alt+F9 still work.
- With debug logging on, `%LOCALAPPDATA%\Felix\logs\` should show `Felix hook: queued <command>` for each shortcut and `Felix hook: declined <command>` when a key is passed to Office; there should be no `could not create the command window` line.
- PowerPoint: Ctrl+Shift+<key> is still passed to PowerPoint.
- Clean shutdown of Word, Excel and PowerPoint with Felix loaded (no crash or hang on exit).
