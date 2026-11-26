# Implementation plan: Office add-in fixes for issues #8-#11

Source design: [../../designs/office-addin-issues-8-11.md](../../designs/office-addin-issues-8-11.md) (approved 2026-11-26; repo commit 4bd4f22). Feature record (decisions in force): [../../features/office-addin-issues-8-11.md](../../features/office-addin-issues-8-11.md). Branch: `fix/office-addin-issues-8-11` (base f1a6a4c). Log: [log.md](log.md).

Purpose: fix four Office add-in bugs found in user testing. Felix.exe does not change.

- #8 (P2): Word Look up Next / Previous Translation sometimes fail with COM error 0x80010005 ("illegal to call out while inside message filter"), because shortcut commands run inside the keyboard hook. Fix in shared code for all three add-ins.
- #9 (P3): Word Alt+1 opens the AutoSave prompt instead of inserting glossary entry 1 (stale focus-HWND check).
- #10 (P3): during Excel Auto Translate the formula bar shows the source text while the active cell shows the translation.
- #11 (P3): the PowerPoint menu advertises Alt+C for Find Concordance, but Alt+C runs Translate to Fuzzy.

## Decisions in force (all user-approved, 2026-11-26)
- D1 (#11): relabel the PowerPoint menu; key bindings do not change.
- D2 (#8): defer shortcut commands in all three add-ins (Word, Excel, PowerPoint).
- D3 (#10): refresh the formula bar as soon as the active cell is written.
- D4 (#9): fix now on the inferred cause, together with debug logging in the hooks.
- One line per fix goes into `setup/release-notes-3.0.1.md` (step 07).

## Shared constraints (every step)
- **Tests are Boost.Test, test first:** write the failing test, see it fail, then implement. Run as x64 Debug (only 64-bit Office is installed). Baselines at the start of this plan: Test_WordAssist 166, Test_Excel 65, Test_PPT 81 test cases.
- **Build with MSBuild called from PowerShell** (Git Bash mangles `/p:`):
  ```powershell
  & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" <project> /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
  ```
  Projects (relative to the repository root): `Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj`, `Test_Excel\Test_Excel.vcxproj`, `Test_PPT\Test_PPT.vcxproj`, `WordAssist\WordAssist\WordAssist.vcxproj`, `ExcelAssist\ExcelAssist\ExcelAssist.vcxproj`, `PowerPointAssist\PowerPointAssist\PowerPointAssist.vcxproj`. The add-ins build as `Debug|x64`. Felix itself builds as `Unicode_Debug|Win32` (`Felix\Felix.vcxproj`, see [BUILDING.md](../../../BUILDING.md) section 3) but does not change here; it is needed only for manual QA.
- **Test executables** (run each after building; the post-build step runs them with `--result_code=no`, so read the output): `Test_WordAssist\Test_WordAssist\x64\Debug\Test_WordAssist.exe`, `Test_Excel\x64\Debug\Test_Excel.exe`, `Test_PPT\x64\Debug\Test_PPT.exe`, each with `--report_level=short`.
- **Excel fakes are Debug-only.** `fake_excel.h` defines `excel::FakeApplication`, `FakeRange` etc. only under `_DEBUG`. Any Test_Excel test that uses them is guarded `#if defined(UNIT_TEST) && defined(_DEBUG)` (as `excel_selection_writer_test.cpp` is), or Test_Excel Release|x64 stops compiling.
- **`PowerPointAssist/PowerPointAssist/AddIn.rc` is Shift-JIS** (`#pragma code_page(932)`) and must stay Shift-JIS. Edit it byte-safely (for example with .NET `[IO.File]::ReadAllBytes`/`WriteAllBytes` and the 932 encoding, or a Shift-JIS-aware editor); never save it as UTF-8. Check `git diff --stat` shows only the intended lines changed.
- **New headers in `common/` must be added to every test project that compiles them** (`<ClInclude>` in the .vcxproj, plus the `.filters` file), and to the vcxproj of each add-in that includes them. A header that is only included but not listed still builds, but keep the project files accurate.
- Source files are UTF-8 with BOM (BUILDING.md "Source file encoding"). New files: UTF-8 with BOM. Never edit `WordAssist\WordAssist\WordParser.cpp` (UTF-16).
- Wrap long builds and test runs with `runlog` when available. Python is not installed here, so `runlog` is unavailable: run the commands directly and record the exact command and its exit code in the task file's execution record.
- Do not push, open PRs or comment on GitHub issues. Manual Office checks are QA's, not steps'; steps record "deferred to QA" for them.
- Commit per step. End each commit message with the line `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>` unless the executing role's brief names a different line.

## Contracts (fixed here; task files copy them unchanged)

| # | Contract | Produced by | Consumed by |
|---|---|---|---|
| C1 | `common/key_mapper.h`: `KeyMapper` gains `virtual bool accepts(const string &command) { command ; return true ; }`. Called from the keyboard hook: returns false if the keystroke for `command` should go to the Office app instead. Must make no cross-process COM calls; in-process Office object-model calls are allowed. | 01 | 01, 02, 03 |
| C2 | `common/keyboard_shortcuts.h`: `namespace shortcuts { struct scoped_shift_state { explicit scoped_shift_state(bool down); ~scoped_shift_state(); }; }` (non-copyable). While one is alive, the global `inline bool shift_key_is_pressed()` returns its `down` value; the innermost scope wins and the destructor restores the previous override. With no scope alive it returns `!!(::GetKeyState(VK_SHIFT) & 0x8000)` as today. The override is a function-local static (one UI thread per add-in). `scoped_shift_state` must be declared before `shift_key_is_pressed()` is defined (or the override accessor forward-declared). The hook path suspends the override: `shortcuts::scoped_physical_shift_state` (non-copyable) clears it for its lifetime, and `shortcuts::physical_shift_key_is_pressed()` always reads `GetKeyState`; hook procedures read Shift with the latter. | 01 | 01, 02, 03 |
| C3 | `common/deferred_key_mapper.h`, header-only, `namespace shortcuts`: `const UINT WM_FELIX_RUN_COMMANDS = WM_APP + 0x0F31;` and `class DeferredKeyMapper : public KeyMapper, public CWindowImpl<DeferredKeyMapper, CWindow, CNullTraits>` with `explicit DeferredKeyMapper(KeyMapper *inner); bool create(); void destroy(); bool map_command(string command); bool accepts(const string &command); void run_pending(); size_t pending_count() const;`. Behaviour: `create()` makes a message-only window (`HWND_MESSAGE`) on the calling thread; on failure it logs once and returns false. `map_command` runs under `scoped_physical_shift_state` (the hook path suspends any running command's Shift override): if `!inner->accepts(command)` log `declined <command>` and return false; if there is no window return `inner->map_command(command)` (R5); otherwise push `{command, physical Shift state}` to a FIFO queue, post `WM_FELIX_RUN_COMMANDS` only if none is outstanding (if the post fails, remove the item, log, and return false so the key goes to Office), log `queued <command>` at debug level and return true. The physical read is the public `boost::function<bool()> m_read_keyboard_shift` (default `physical_shift_key_is_pressed`; tests replace it). `run_pending()` (the message handler): clear the posted flag; return if already running (nested pump); else loop popping the queue, running each `inner->map_command` under `scoped_shift_state(item.shift)`, catching everything (log it) so the next command still runs; the running flag is reset by RAII, and the window's message handler also catches everything. `destroy()` destroys the window and drops queued commands. `accepts` forwards to the inner mapper. | 01 | 01, 03 |
| C4 | `KeyTargetWord::SelectionIsEmpty()` (in `WordAssist/WordAssist/key_target_word.h`): `virtual bool SelectionIsEmpty() = 0;` In-process only, catches everything, returns true (empty) on error. `WordController::SelectionIsEmpty()` returns `m_word_object.selection_is_empty()`. `KeyTargetPpt::HasSelectedText()` (`common/key_target_ppt.h`): `virtual bool HasSelectedText() = 0;` In-process only, returns false on error. `CPowerPointInterface::HasSelectedText()` returns `m_app->Active == msoTrue && m_textRangeParser.getSelectedText(true).length() > 0` (the first checks of `OnExtendLookupAction`). | 02 | 02 |
| C5 | Fake controls (names fixed so tests in different steps agree). `KeyMapperFake` (`common/key_mapper_fake.h`): `bool m_accepts_result = true;` (returned by `accepts`), `std::vector<bool> m_shift_values;` (value of `shift_key_is_pressed()` recorded on every `map_command`), `boost::function<void(const string &)> m_on_map_command;` (called inside `map_command` if set), `bool m_throw = false;` (throws `std::runtime_error` in `map_command` when set, after sensing). `KeyTargetWordFake`: `bool m_is_active = true; bool m_selection_is_empty = false;`. `KeyTargetPptFake`: `bool m_is_active = true; bool m_has_selected_text = true;`. `IsActive()` returns the member. | 01 (KeyMapperFake), 02 (target fakes) | 01, 02 |
| C6 | `ExcelAssist/ExcelAssist/excel_selection_writer.h`: `inline void refresh_formula_bar_if_active(excel::app_ptr app, excel::range_ptr written);` Compares `written->Row()`/`Column()` with `app->GetActiveCell()->Row()`/`Column()` inside a try; if equal, calls `refresh_formula_bar(app)`. Catches `_com_error` and `...`, logs at warn level, never throws. | 05 | 05 |
| C7 | Log text (debug level, via `logging::log_debug`). Hooks, only for keystrokes with Alt or Ctrl down, one line each when passing the key on: `Felix hook: passing on key (dialog guard)`, `Felix hook: passing on key (shortcuts off)`, `Felix hook: passing on key (not mapped or declined)`. `DeferredKeyMapper`: `Felix hook: queued <command>` and `Felix hook: declined <command>`. | 01 (deferred lines), 04 (hook lines) | 03, 04 (QA reads them) |

Existing code the contracts rely on (inspected): `shortcuts::KeyboardShortcuts::receive_keypress` in `common/keyboard_shortcuts.cpp` calls `m_mapper->map_command`; `KeyMapperWord`, `KeyMapperPpt`, `KeyMapperExcel` are in `WordAssist/WordAssist/key_mapper_word.h`, `common/key_mapper_ppt.h`, `ExcelAssist/ExcelAssist/key_mapper_excel.h`; the hooks are `WordAssist/WordAssist/hooker.cpp`, `ExcelAssist/ExcelAssist/Hooker.cpp`, `PowerPointAssist/PowerPointAssist/Hooker.cpp`.

## Tasks

| ID | Task file | Outcome | Depends on | Size | Status |
|---|---|---|---|---|---|
| 01 | [01-deferred-key-mapper-core.md](01-deferred-key-mapper-core.md) | #8 core: `accepts` in `KeyMapper`, `scoped_shift_state`, `DeferredKeyMapper`, fake extension, tests in Test_PPT | none | M, moderate (ATL window in a test project) | done: bafe885; review round 1 fixes 48694fe, 67a7718, 3585518, 3a7cf38 (and the record commit after them) |
| 02 | [02-per-app-accepts.md](02-per-app-accepts.md) | #8: `accepts` in the Word, PowerPoint and Excel mappers, new target methods, fakes, tests | 01 | M, moderate | done: 143fc28 |
| 03 | [03-wire-deferred-mapper.md](03-wire-deferred-mapper.md) | #8: wire the decorator into the three Connects, route Excel's Shift reads through `shift_key_is_pressed()` | 01, 02 | S-M, moderate (no unit coverage; builds only) | done: 4921cb9 |
| 04 | [04-word-is-active-and-hook-logging.md](04-word-is-active-and-hook-logging.md) | #9: drop the HWND comparison in `WordApplication::is_active`; R7 hook logging in all three hooks | none (AC7 check needs 03) | S, easy | done: ef06fa5 |
| 05 | [05-excel-auto-translate-refresh.md](05-excel-auto-translate-refresh.md) | #10: `refresh_formula_bar_if_active` and its call in `AutoTransCell`, tests | none | S, easy | pending |
| 06 | [06-powerpoint-menu-labels.md](06-powerpoint-menu-labels.md) | #11: relabel Find Concordance / Translate to Fuzzy in `Connect.cpp` and the Shift-JIS `AddIn.rc` | none | S, easy (encoding care) | pending |
| 07 | [07-release-notes-and-final-check.md](07-release-notes-and-final-check.md) | Release-note lines (#8-#11) and full regression in Debug and Release x64 | 01-06 | S, easy | pending |

## Execution order
01, 02, 03, 04, 05, 06, 07, sequentially. The four fixes are independent, so each fix's commits can be reviewed and reverted on its own: #8 = 01-03, #9 = 04, #10 = 05, #11 = 06. Steps 01, 02 and 03 share edits to the test projects and Connect files, so do not run them in parallel; 04, 05 and 06 touch disjoint files and could run in parallel with each other (04 and 06 both avoid the test projects; 05 edits `Test_Excel.vcxproj`, which 02 also edits, so keep 05 after 02). Integration checkpoint after 03: all three test programs green and all three add-ins build Debug|x64; then 04-06; then step 07.

## Requirement mapping
| Requirement / criterion | Step |
|---|---|
| R1 (no cross-process call in the hook), R3 (FIFO, no nesting), R4 (Shift captured), R5 (fallback if no window) | 01 (mechanism), 03 (wiring) |
| R2 (what the hook consumes) | 02 (per-app `accepts`), 01 (`DeferredKeyMapper` honours `accepts`) |
| R6 (#9, Word shortcut runs regardless of stale HWND) | 04 |
| R7 (#9, hook debug logging) | 04 (hook lines), 01 (queued/declined lines) |
| R8, R9 (#10) | 05 |
| R10 (#11) | 06 |
| AC1-AC10 (manual Office checks) | QA after step 07; each step's record notes "deferred to QA" |
| AC11 (all three test programs pass with the new tests) | 01, 02, 05 add tests; final run in 07 |
| Release notes (user decision, feature record) | 07 |

AC3 (Ctrl+Right still moves the caret) is covered by the unit tests of step 02 plus QA. AC4 (Shift captured) by the unit tests of step 01 plus the Excel `shift_key_is_pressed()` change in step 03 plus QA.

## Overall completion criteria
All seven steps done; Test_WordAssist, Test_Excel and Test_PPT pass in Debug and Release x64 with the new cases added to the baselines (166, 65, 81); ExcelAssist, WordAssist and PowerPointAssist build Debug|x64 (and Win32 where it built before); release-note lines added; each fix on its own commit(s); nothing pushed. Manual AC1-AC10 are QA's.

## Blockers and open questions
None blocking. Notes for implementers:
- `CWindowImpl` needs ATL window headers (`atlwin.h`) in each project's precompiled header. The add-ins already use ATL; Test_PPT's `stdafx.h` may not. If a test project lacks it, add `<atlwin.h>` to that project's `stdafx.h` and record it.
- In Release builds Test_Excel has fewer cases than in Debug (the fakes are Debug-only), so compare each configuration with its own baseline. Step 01 measures the baseline of all three test programs in Debug and Release x64 and records them in the log.
- Design residual risk (section 5.1): a command dispatched from inside Office's own message filter while no Felix command runs can still fail with 0x80010005. No retry is added. Do not add one.

## Instructions for implementers
Update the step's row in the Tasks table (status and commit hashes) here. Record evidence, deviations and blockers in the task file's Execution record. Put anything that accumulates across steps (baseline counts, probe findings) in [log.md](log.md).
