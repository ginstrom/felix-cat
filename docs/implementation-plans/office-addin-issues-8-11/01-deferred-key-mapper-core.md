# 01 - #8 core: `accepts`, `scoped_shift_state` and `DeferredKeyMapper`

Index: [index.md](index.md). Design: [sections 5.1 and 7](../../designs/office-addin-issues-8-11.md). Depends on: none. Size: M.

## Objective
Add the shared mechanism that moves shortcut commands out of the keyboard hook: a synchronous `accepts` pre-check on `KeyMapper`, a Shift-state override, and a header-only `DeferredKeyMapper` decorator that queues commands and runs them one at a time from an add-in-owned message window. Nothing is wired into an add-in yet (step 03), so no add-in behaviour changes in this step.

## Requirements (local copy)
- R1: no hook procedure makes a cross-process COM call or runs a command; the hook decides synchronously, consumes the key, and queues the command to run later from a window procedure on the Office UI thread.
- R3: queued commands run one at a time in keystroke order; a command queued while another runs (including when Office pumps messages inside a running command) waits until it returns; commands never nest.
- R4: a command runs with the Shift state that was current when its key was pressed.
- R5: if the window cannot be created, behaviour is as today: the command runs synchronously, and the failure is logged once.
- R7 (queued/declined part): log `Felix hook: queued <command>` and `Felix hook: declined <command>` at debug level.
- Failure behaviour: a command that throws is caught and logged; the queue continues with the next command; no exception reaches Office.

## Dependencies and prerequisites
None. Existing code (inspected):
- `common/key_mapper.h`: `class KeyMapper { virtual ~KeyMapper(){} virtual bool map_command(string command) = 0 ; };`
- `common/key_mapper_fake.h`: `KeyMapperFake` with `DECLARE_SENSING_VAR`, `map_command` that senses the command and returns true. Used by Test_PPT, Test_Excel and Test_WordAssist (all three list it as a `ClInclude`).
- `common/keyboard_shortcuts.h`: global `inline bool shift_key_is_pressed()` (`GetKeyState(VK_SHIFT) & 0x8000`), `control_key_is_pressed`, `alt_key_is_pressed`, then `namespace shortcuts { ... KeyboardShortcuts ... }`. `common/keyboard_shortcuts.cpp` is compiled by all three test projects.
- Test_PPT already compiles `common\keyboard_shortcuts.cpp`, `common\key_mapper_ppt_test.cpp`, `common\keystroke_target_test.cpp` and `keyboard_shortcuts_test.cpp` (which tests chord handling with `KeyMapperFake`).
- `logging::log_debug(const string)` and `log_warn`/`log_error` in `common/logging.h`.

## Contracts produced (identical to the index: C1, C2, C3, C5 part, C7 part)
- **C1** `common/key_mapper.h`: add `virtual bool accepts(const string &command) { command ; return true ; }` to `KeyMapper`. Comment: called from the keyboard hook; returns false if the keystroke for `command` should go to the Office app instead; must make no cross-process COM calls; in-process Office object-model calls are allowed.
- **C2** `common/keyboard_shortcuts.h`: `namespace shortcuts { struct scoped_shift_state { explicit scoped_shift_state(bool down); ~scoped_shift_state(); }; }`, non-copyable. While alive, the global `shift_key_is_pressed()` returns its `down` value; the innermost scope wins; the destructor restores the previous override. No scope alive: `!!(::GetKeyState(VK_SHIFT) & 0x8000)` as today. The override is a function-local static (single UI thread per add-in). `scoped_shift_state` must be declared before `shift_key_is_pressed()` is defined (or the override accessor forward-declared).
- **C3** `common/deferred_key_mapper.h`, header-only, `namespace shortcuts`:
  ```cpp
  const UINT WM_FELIX_RUN_COMMANDS = WM_APP + 0x0F31 ;
  class DeferredKeyMapper : public KeyMapper,
                            public CWindowImpl<DeferredKeyMapper, CWindow, CNullTraits>
  {
  public:
      explicit DeferredKeyMapper(KeyMapper *inner) ;
      bool create() ;   // message-only window (HWND_MESSAGE) on the calling thread; logs and returns false on failure
      void destroy() ;  // destroys the window, drops queued commands
      bool map_command(string command) ;
      bool accepts(const string &command) ;  // forwards to inner
      void run_pending() ;                   // the message handler; public for tests
      size_t pending_count() const ;
  private:
      KeyMapper *m_inner ;
      std::deque<std::pair<string, bool> > m_queue ;
      bool m_posted ;
      bool m_running ;
  } ;
  ```
  `map_command`: if `! m_inner->accepts(command)` log `Felix hook: declined <command>` and return false. If there is no window, return `m_inner->map_command(command)` (R5). Otherwise push `{command, shift_key_is_pressed()}`, `PostMessage(WM_FELIX_RUN_COMMANDS)` only if `! m_posted` (then set it), log `Felix hook: queued <command>` (debug) and return true. `run_pending()`: `m_posted = false`; if `m_running` return; `m_running = true`; while the queue is not empty pop the front, run `m_inner->map_command(item.first)` under `shortcuts::scoped_shift_state shift(item.second)` inside try/catch (log the error, continue); `m_running = false`. The window handles `WM_FELIX_RUN_COMMANDS` by calling `run_pending()`. Invariants: FIFO; at most one message outstanding; never nested.
- **C5 (KeyMapperFake part)** in `common/key_mapper_fake.h`: `bool m_accepts_result = true;` (returned by `accepts`), `std::vector<bool> m_shift_values;` (value of `shift_key_is_pressed()` appended on every `map_command`), `boost::function<void(const string &)> m_on_map_command;` (called inside `map_command` if set), `bool m_throw = false;` (after sensing, throws `std::runtime_error` from `map_command`). Existing behaviour (senses the command, returns true) is unchanged when none is set. `key_mapper_fake.h` will need `keyboard_shortcuts.h` (for `shift_key_is_pressed`) or an equivalent include.

## Scope and implementation guidance
Test first, in `Test_PPT` (it already holds the shared tests):
1. New `Test_PPT/deferred_key_mapper_test.cpp` (add `<ClCompile>` to `Test_PPT.vcxproj` and `.filters`; add `..\common\deferred_key_mapper.h` as `<ClInclude>`). Pump messages on the test thread with `PeekMessage`/`DispatchMessage` for `HWND_MESSAGE` windows. Cases:
   - Without `create()`: `map_command` runs the inner mapper immediately and returns its result (R5).
   - After `create()`: `map_command` returns true and the inner mapper is not called until the message is pumped.
   - Three commands queued, one pump: all run in order, and only one message was posted (check `pending_count()` and that a second pump is a no-op).
   - `accepts` false: `map_command` returns false, nothing queued, inner not called.
   - Shift captured: queue under `scoped_shift_state(true)`, run (pump) outside it: inner `m_shift_values` records `true`; and the reverse.
   - Re-entrancy: the inner mapper's `m_on_map_command` queues another command and calls `run_pending()` itself; the second command runs after the first returns, not inside it.
   - An inner mapper with `m_throw = true` for the first command: no exception escapes, the next queued command still runs.
   - `destroy()` drops pending commands (pumping afterwards runs nothing).
   - `scoped_shift_state`: nesting restores the outer value; with no override, `shift_key_is_pressed()` equals `GetKeyState` (compare against `!!(::GetKeyState(VK_SHIFT) & 0x8000)`).
   - Through `KeyboardShortcuts` using the decorator as its mapper (`KeyboardShortcuts(&deferred)`): a chord's second key is consumed and queued (check `receive_keypress` returns true and the inner mapper only runs after the pump); `ToggleShortcuts` stays synchronous (not queued).
2. Implement C1, C2, C3 and the fake changes until the tests pass. Use `boost::function` / `std::deque` as the codebase does. If `CWindowImpl` does not compile in Test_PPT because `stdafx.h` lacks `<atlwin.h>`, add it to that project's `stdafx.h` and record it.
3. Measure and record in [log.md](log.md) the baseline totals before any change: all three test programs, Debug|x64 and Release|x64 (six runs).

Exclusions: no per-app `accepts` overrides (step 02), no wiring into Connect (step 03), no hook logging (step 04), no change to the three hook procedures. Do not add the new header to Test_WordAssist or Test_Excel unless those projects compile something that includes it (they do not yet).

## Verification
Commands (PowerShell, from the repository root):
```powershell
$ms = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
& $ms Test_PPT\Test_PPT.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_PPT\x64\Debug\Test_PPT.exe --report_level=short
& $ms Test_Excel\Test_Excel.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_Excel\x64\Debug\Test_Excel.exe --report_level=short
& $ms Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_WordAssist\Test_WordAssist\x64\Debug\Test_WordAssist.exe --report_level=short
```
- Red first: the new tests fail (or do not compile against stubs) before the implementation.
- After: Test_PPT is 81 plus the new cases, all passing; Test_Excel stays 65 and Test_WordAssist stays 166 (they compile `key_mapper_fake.h` and `keyboard_shortcuts.h`, so they must still build). Also build and run Release|x64 for all three (`/p:Configuration=Release`, exe under `x64\Release`).
- The three add-ins still build `Debug|x64` (they include `key_mapper.h` and `keyboard_shortcuts.h`).

## Completion criteria
- [x] New tests written first, seen failing, then passing; counts recorded.
- [x] C1, C2, C3 and the C5 fake part implemented exactly as in the index; header listed in `Test_PPT.vcxproj` (+ filters); UTF-8 BOM on new files.
- [x] Baselines (six runs) recorded in `log.md`; the three test programs pass in Debug|x64 and Release|x64; the three add-ins build Debug|x64.
- [x] Committed; index row 01 updated.

## Execution record
Engineer, 2026-11-26, branch `fix/office-addin-issues-8-11` from f7bf9b2. `runlog` unavailable (no Python); commands run directly from PowerShell at the repository root with `$ms` = MSBuild 18.

**Baseline** (before any change; six runs, all exit 0): Test_PPT 81 / 81, Test_Excel 65 / 58, Test_WordAssist 166 / 166 (Debug / Release x64). Recorded in [log.md](log.md).

**Red:** `& $ms Test_PPT\Test_PPT.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo` with only the new test file added: exit 1, `error C1083: Cannot open include file: 'deferred_key_mapper.h'`.

**Green:** after C1, C2, C3 and the C5 fake part, an incremental build ran 99 cases with 4 failures and an access violation in the existing `KeyboardShortcutsLoadTest/test_load_simple`. The cause was stale object files (MSBuild did not recompile all dependants of the edited `common/*.h`): `& $ms Test_PPT\Test_PPT.vcxproj /t:Rebuild ...` then gave exit 0 and `Test_PPT.exe --report_level=short`: exit 0, "99 test cases out of 99 passed".

**Verification** (all with `/t:Rebuild`, x64):

| Command | Exit | Result |
|---|---|---|
| Test_PPT Debug build + `Test_PPT\x64\Debug\Test_PPT.exe --report_level=short` | 0 / 0 | 99 test cases out of 99 passed |
| Test_Excel Debug build + run | 0 / 0 | 65 test cases out of 65 passed |
| Test_WordAssist Debug build + run | 0 / 0 | 166 test cases out of 166 passed |
| Test_PPT Release build + run | 0 / 0 | 99 test cases out of 99 passed |
| Test_Excel Release build + run | 0 / 0 | 58 test cases out of 58 passed |
| Test_WordAssist Release build + run | 0 / 0 | 166 test cases out of 166 passed |
| `& $ms WordAssist\WordAssist\WordAssist.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64` | 0 | built |
| `& $ms ExcelAssist\ExcelAssist\ExcelAssist.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64` | 0 | built |
| `& $ms PowerPointAssist\PowerPointAssist\PowerPointAssist.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64` | 0 | built |

New tests (18, `Test_PPT/deferred_key_mapper_test.cpp`): suite `ScopedShiftStateTest` (3: no override reads the keyboard; override down/up; nesting restores the outer value) and `DeferredKeyMapperTest` (15: no window runs the inner mapper at once and returns its result (2); queued until pumped; three commands, one message, FIFO, second pump a no-op; declined with and without a window (2); `accepts` forwards; Shift captured down and up (2); command queued and nested `run_pending` inside a running command waits; throwing commands are caught and the queue continues; `destroy()` drops pending commands and later commands run synchronously; `KeyboardShortcuts` chord second key and single-key shortcut consumed and queued (2); `ToggleShortcuts` stays synchronous).

Decisions and deviations:
- `stdafx.h` of Test_PPT already includes `<atlwin.h>`; no change needed.
- `DeferredKeyMapper` catches `_com_error`, `except::CException`, `std::exception` and `...` separately so each is logged with its message (`logging::log_exception`); the header therefore includes `Exceptions.h` and `logging.h`. Projects that include `deferred_key_mapper.h` (step 03) need `common/Exceptions.h` on their include path; all three add-in projects already compile `common\Exceptions.cpp`.
- `create()` returns true without creating a second window if one already exists. If `PostMessage` fails, `map_command` logs a warning and leaves the posted flag clear so the next keystroke retries; the command stays queued (contract silent on this).
- The destructor calls `destroy()`.
- The throw case uses `m_throw = true` for both queued commands (the fake has one flag): both are sensed, so the second ran after the first threw.
- The `scoped_shift_state` override lives in `shortcuts::detail::current_shift_override()` (function-local static), declared before the global `shift_key_is_pressed()`.

For later steps: rebuild with `/t:Rebuild` after editing shared headers (see log.md). Manual Office checks deferred to QA.

### Review round 1 (reviewer verdict: changes needed on f7bf9b2..bafe885)

| # | Finding | Outcome |
|---|---|---|
| 1 | The Shift override leaked into hook-time decisions made while a deferred command runs (Office pumps messages inside commands) | Fixed, 48694fe. Following the orchestrator's decision, `map_command` runs under the new `shortcuts::scoped_physical_shift_state` (it clears the override for its lifetime), so `accepts`, the capture and a synchronous run see the real keyboard. There is a new `shortcuts::physical_shift_key_is_pressed()`, and the capture goes through the public `boost::function<bool()> m_read_keyboard_shift` (default `physical_shift_key_is_pressed`). That member is a test seam the contract did not have: the R4 tests can no longer simulate the key with `scoped_shift_state`, so they set it instead (deliberate change of those two tests' setup). New tests: `test_keystroke_during_running_command_ignores_its_shift_override` (red first: captured and `accepts` Shift values were the override), `test_without_window_runs_with_physical_shift` (red first), and `ScopedShiftStateTest/test_physical_scope_suspends_and_restores_override`. C2/C3 updated in index.md, design section 5.1 (two sentences) updated, and step 03 told to change `PowerPointAssist/PowerPointAssist/Hooker.cpp:60` to `shortcuts::physical_shift_key_is_pressed()`. |
| 2 | `m_running` not exception-safe; handler could let an exception reach Office | Fixed, 67a7718: `running_scope` RAII and `catch (...)` in `OnRunCommands`. No unit test: `run_one` catches everything, so no exception can be made to escape `run_pending` through the public interface. |
| 3 | Tests: nested case bypassed the message path; throw case did not check a later command | Fixed, 3585518. `NestedQueuer` now calls `map_command` then `pump_messages()` inside First and checks Second is still pending there and runs afterwards. Checked by mutation: with the `m_running` guard disabled the test fails (`1 != 2`, `1 != 0`). The throw test now also queues and pumps Third. |
| 4 | A failed `PostMessage` stranded the command | Fixed, 3a7cf38. The item just queued is removed, a warning is logged, and `map_command` returns false so Office gets the key. This supersedes the round 0 note "the command stays queued". Test `test_failed_post_returns_key_to_office` fills the thread message queue (red first: returned true, 1 pending). |
| 5 | Notes: `accepts` must not throw; `destroy()` on the UI thread | Records only: lines added to 02-per-app-accepts.md (shared constraints, plus a C2 note that the decorator suspends the override) and 03-wire-deferred-mapper.md (item 5). |
| 6 | Commit hashes in index row 01 | Done in this record commit. |

Verification after the round (all `/t:Rebuild`, x64, exit 0): Test_PPT 103 / 103, Test_Excel 65 / 58, Test_WordAssist 166 / 166 (Debug / Release). WordAssist, ExcelAssist and PowerPointAssist built Debug|x64.
