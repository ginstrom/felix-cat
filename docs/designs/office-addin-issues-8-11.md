# Design: Office add-in fixes for issues #8–#11

## 1. Purpose and status

Four bugs in the Office add-ins, found in user testing at `55079ee` with 64-bit Microsoft 365 (16.0.20625.20142):

| Issue | Symptom | Root cause (confidence) |
|---|---|---|
| [#8](https://github.com/ginstrom/felix-cat/issues/8) P2 | Word **Look up Next** (Alt+Right after Alt+Up) or **Previous Translation** (Alt+P) sometimes shows "COM Error … Failed to connect to Felix window … 0x80010005 It is illegal to call out while inside message filter" | Every shortcut runs its command synchronously inside the `WH_KEYBOARD` hook. When the hook is invoked from inside Word's COM message filter (Word pumping input while an earlier call to Felix is still outstanding), the add-in's cross-process call to `Felix.App` is refused with `RPC_E_CANTCALLOUT_INEXTERNALCALL`. Excel and PowerPoint dispatch the same way. (High for the mechanism; medium for the exact pumping path, which is inside Office.) |
| [#9](https://github.com/ginstrom/felix-cat/issues/9) P3 | Word **Alt+1** opens "How do I turn on AutoSave?" (Quick Access Toolbar item 1) instead of inserting glossary entry 1 | Alt+1 is mapped, and the command consumes the key when it runs. The Word mapper first checks `IsActive()`, which requires the focus window to be the exact HWND captured at the last Word activation or selection-change event. That HWND goes stale when focus returns to Word (for example from Felix's window) without a selection change, so the mapper declines and the hook passes Alt+1 to Word. Excel and PowerPoint have no such check. (Medium: inferred from code; not reproduced.) |
| [#10](https://github.com/ginstrom/felix-cat/issues/10) P3 | During Excel **Auto Translate Selection** the formula bar briefly shows the source while the active cell shows the translation | Auto Translate writes cells through `Characters.Text`, which doesn't refresh the formula bar, and PR #7's `formula_bar_refresher` refreshes only when the whole command ends. Between the active cell's write and the end, the bar is stale. (High) |
| [#11](https://github.com/ginstrom/felix-cat/issues/11) P3 | PowerPoint's menu says **Find Concordance (ALT + C)**, but Alt+C runs Translate to Fuzzy | The default PowerPoint shortcut table binds Alt+C to `Auto Trans to Fuzzy` and has no source-concordance command at all, while the menu captions are hard-coded strings. The same menu also labels Translate to Fuzzy "(ALT + Z)", which is unbound. (High) |

**Status:** approved by the user on 2026-11-26, with every recommendation in section 8 accepted (D1 relabel, D2 all three add-ins, D3 refresh, D4 fix now).

Felix.exe doesn't change. The #8 mechanism is shared code in `common/` used by all three add-ins; #9 is Word-only; #10 is Excel-only; #11 is PowerPoint-only.

## 2. Requirements and scope

### Requirements

- **R1 (#8).** No keyboard hook procedure (Word, Excel, PowerPoint) makes a cross-process COM call or runs a command. The hook decides synchronously whether the keystroke is a Felix shortcut, consumes it if so, and queues the command. The command runs later from a window procedure owned by the add-in on the Office UI thread, dispatched by Office's message loop.
- **R2 (#8).** Whether a keystroke is consumed is decided in the hook, from the shortcut tables and cheap in-process checks only, as follows:
  - Same as today: up-keys, modifier-only keys, keys aimed at a dialog (the #2 guard), unmapped keys and disabled shortcuts pass on.
  - The Word mapper's activation check (`IsActive`, as changed by R6) passes the key on when it fails.
  - **Extend Lookup** and **Extend Trans Lookup** (Word and PowerPoint; Ctrl+Right by default) pass the key on when Shift is down or when nothing is selected, so Ctrl+Right and Ctrl+Shift+Right keep moving and extending the caret as today.
  - Every other mapped shortcut is consumed even if its command later declines or fails.
- **R3 (#8).** Queued commands run one at a time in keystroke order. A command queued while another is running (including when Office pumps messages inside a running command, e.g. during its call to Felix, a message box or a dialog) waits until the running command returns. Commands never nest.
- **R4 (#8).** A command runs with the Shift state that was current when its key was pressed (Shift selects plain-text mode or, in Excel Auto Translate, cell colouring).
- **R5 (#8).** If the add-in's window can't be created, shortcuts behave as today (synchronous in the hook) and the failure is logged once.
- **R6 (#9).** In Word, a mapped shortcut whose key is aimed at a Word main window (the #2 guard) runs regardless of which child HWND had focus at the last activation event. Word's `IsActive` keeps its "Word object exists and a document window is active" meaning but no longer compares HWNDs.
- **R7 (#9).** Each add-in's hook logs, at debug level, why it passed on an Alt/Ctrl keystroke (dialog guard, shortcuts disabled, not mapped, mapper declined) or that it queued a command, so a recurrence of #9 can be diagnosed from the add-in log.
- **R8 (#10).** During Auto Translate (Selection, Sheet, Workbook), when a cell that is the active cell is written, the formula bar is refreshed straight away. The end-of-command refresh from PR #7 stays.
- **R9 (#10).** Writes keep literal-text semantics (`001` stays text). The refresh never changes the selection or the active cell, and a refresh failure is logged and ignored.
- **R10 (#11).** In the PowerPoint Felix menu, English and Japanese, translation and review modes, **Find Concordance** shows no shortcut and **Translate to Fuzzy** shows Alt+C, matching the default bindings (decision D1). Key bindings don't change.

### Non-goals

- Changing any default key binding (unless D1 is decided the other way).
- Fixing other mislabelled PowerPoint menu items (section 9).
- Retrying commands that fail with a COM error, and any change to Office's or Felix's message filters.
- Felix.exe changes. 32-bit Office beyond building.

### Acceptance criteria

- **AC1 (R1, R3; #8).** Word, Felix running with a memory that matches several lines: Alt+Right, Alt+Up, Alt+Right as fast as possible for 10 lines, and Alt+N/Alt+P alternated rapidly 20 times with a multi-match query. No COM error dialog. Each lookup happens in order; no sentence is skipped or looked up twice.
- **AC2 (R1, R3).** Hold Alt+Right for 2 seconds (auto-repeat) in Word, Excel and PowerPoint: no COM error; lookups proceed one after another and stop after the queued ones finish.
- **AC3 (R2).** Word and PowerPoint: Ctrl+Right with an insertion point (nothing selected) moves the caret one word; Ctrl+Shift+Right extends the selection by a word; after a lookup (sentence selected), Ctrl+Right extends the Felix lookup.
- **AC4 (R4).** Word: Shift+Alt+Down inserts the translation as plain text. Excel: Shift+Alt+Z (Auto Translate Selection) colours cells without a perfect match.
- **AC5 (R2).** Word Alt+M then C runs Find Concordance; Ctrl+Alt+F9 toggles shortcuts (beep) in all three apps; the #2 checks (AC5–AC8 of the previous design: Alt+N in the save prompt picks Don't Save, Felix doesn't start) still pass.
- **AC6 (R6; #9).** Reproduce #9 as written, in an unsaved document with AutoSave on the QAT: Alt+1 inserts `Ouvrez la porte de service.` and no AutoSave prompt appears. Repeat after clicking into Felix's window and returning to Word by clicking Word's title bar, and again by Alt+Tab.
- **AC7 (R7).** With the add-in log at debug level, each Alt+digit press in AC6 leaves one hook log line saying the command was queued.
- **AC8 (R8; #10).** Reproduce #10 with A10:A60 all `Serial label.` (A10 active): sampling the formula bar through UI Automation (AutomationId `FormulaBar`) while Auto Translate Selection runs, it shows `001` from the first sample at which A10 shows `001`. After the command, the selection is still A10:A60 with A10 active.
- **AC9 (R8, R9).** Auto Translate Sheet and Workbook: when the command ends the active cell and the formula bar agree, the active cell holds the text `001` (not the number 1), and the active cell hasn't moved.
- **AC10 (R10; #11).** PowerPoint, English and Japanese menus, translation and review modes: the Find Concordance item shows no shortcut, the Translate to Fuzzy item shows Alt+C. Alt+C still runs Translate to Fuzzy.
- **AC11.** Test_WordAssist, Test_Excel and Test_PPT pass with the new tests in section 7 (baseline 166, 65, 81).

## 3. Current system (inspected)

### #8 Hook dispatch and the COM error

- The three hooks are thread-local `WH_KEYBOARD` hooks on the Office UI thread: Word [hooker.cpp:21](../../WordAssist/WordAssist/hooker.cpp) (→ `new_keyboard_proc` line 104 → `receive_keypress` line 139), Excel [Hooker.cpp:27](../../ExcelAssist/ExcelAssist/Hooker.cpp) (→ `receive_keypress` line 69), PowerPoint [Hooker.cpp:21](../../PowerPointAssist/PowerPointAssist/Hooker.cpp) (→ line 71). Each returns 1 (consumes the key) iff `receive_keypress` returns true.
- `KeyboardShortcuts::receive_keypress` ([keyboard_shortcuts.cpp:85](../../common/keyboard_shortcuts.cpp)) calls `m_mapper->map_command(command)` synchronously (line 108 for a chord's second key, which is always consumed since #7; line 169 for a single key, which is consumed iff `map_command` returns true).
- `map_command` runs the whole command: `KeyMapperWord` ([key_mapper_word.h:63](../../WordAssist/WordAssist/key_mapper_word.h)), `KeyMapperExcel` ([key_mapper_excel.h:35](../../ExcelAssist/ExcelAssist/key_mapper_excel.h)), `KeyMapperPpt` ([key_mapper_ppt.h:41](../../common/key_mapper_ppt.h)).
- The #8 paths in Word: Alt+Right → `WordController::OnLookupNextAction` ([WordController.cpp:770](../../WordAssist/WordAssist/WordController.cpp)) → `select_next_sentence()` (in-process Word object model, line 780) → `OnLookupAction` → `getAssistant()` (line 720). Alt+P → `OnPrevAction` (line 955) → `getAssistant()` (line 960). `getAssistant` (line 2195) does `CreateInstance(L"Felix.App")` wrapped in `COM_ENFORCE(..., "Failed to connect to Felix window")` (line 2200), then `Visible` and optionally `Raise()`. On a `_com_error`, `handle_exception(CComException&)` (line 2250) shows "COM Error\r\nCall to Felix.WordInterface on automation server failed." with the action name from `CATCH_ALL` ("Action: Looking up current selection", line 761; "Action: Navigate to previous match in Felix", line 964). This matches both screenshots exactly.
- `0x80010005` is `RPC_E_CANTCALLOUT_INEXTERNALCALL`. COM returns it, without contacting the server, when an STA thread tries an outgoing call while COM is calling that thread's `IMessageFilter` (Word registers one). COM calls Word's `IMessageFilter::MessagePending` when a message arrives while the thread is waiting for an outgoing call to return.
- The add-in doesn't register a message filter (no `CoRegisterMessageFilter`/`IMessageFilter` in WordAssist, ExcelAssist, PowerPointAssist or `common/`), so the filter context is Word's. Inference for the sequence:
  1. Alt+Up: Word's loop retrieves the key, the hook runs `OnSetAction` synchronously, which calls Felix and waits. Felix's handlers do UI work on its own thread (re-rendering the match view), so the wait can be noticeable.
  2. The user presses Alt+Right during the wait. COM calls Word's `MessagePending`; Word removes the keyboard message there (a `PM_REMOVE` peek is what invokes a `WH_KEYBOARD` hook with `HC_ACTION`; the hook ignores `HC_NOREMOVE`).
  3. The hook runs `OnLookupNextAction` nested inside the filter: the in-process `select_next_sentence` succeeds, and `CreateInstance` fails with 0x80010005.
  - This explains the reporter's notes that "the next sentence can be selected in Word even when the lookup in Felix fails", that it is intermittent (it depends on typing speed vs. Felix's response time), and that the menu works (menu clicks run from Word's normal dispatch, never inside the hook).
- The only part not visible in our code is exactly what Word's `MessagePending` does. Any outgoing call Word itself makes (for example AutoSave or cloud services) would open the same window for a keystroke; it doesn't change the fix.
- **Excel and PowerPoint** carry the same latent defect: `CFelixExcelIF::realGetAssistant` ([FelixExcelIF.cpp:518](../../ExcelAssist/ExcelAssist/FelixExcelIF.cpp)) and `CPowerPointInterface::getAssistant` ([PowerPointInterface.cpp:1456](../../PowerPointAssist/PowerPointAssist/PowerPointInterface.cpp)) make the same cross-process calls from inside the same kind of hook. Whether their message filters pump input is up to Excel and PowerPoint; not observed.
- **Commands that decline today and so let the key through** (relevant to R2):
  - Word: `map_command` returns false when `IsActive()` fails (line 65) and for Extend Lookup with Shift (line 109). `OnExtendLookupAction` (line 824) and `OnExtendTransLookupAction` (line 1426) return false on an empty selection, so Ctrl+Right with an insertion point reaches Word and moves the caret. Other `false` returns follow a user cancelling a prompt or an error dialog (e.g. `OnSetAction` "long translation" prompt, line 588); passing the key after that is incidental.
  - PowerPoint: `map_command` returns `SUCCEEDED(hr)`. `OnExtendLookupAction` ([PowerPointInterface.cpp:384](../../PowerPointAssist/PowerPointAssist/PowerPointInterface.cpp)) returns `E_FAIL` when PowerPoint isn't active, Shift is down or no text is selected; `OnExtendTransLookupAction` (line 696) when no text is selected. Other `E_FAIL`s are error paths.
  - Excel: `map_command` returns true for every known command (`IsActive()` is always true, [FelixExcelIF.h:84](../../ExcelAssist/ExcelAssist/FelixExcelIF.h)).
- **Shift:** commands read Shift when they run: `shift_key_is_pressed()` ([keyboard_shortcuts.h:16](../../common/keyboard_shortcuts.h), `GetKeyState`) in the mappers, and `::GetKeyState(VK_SHIFT)` directly in Excel's `OnAutoTransSel`, `OnAutoTransSheet` and `OnAutoTransWorkbook` (FelixExcelIF.cpp lines 476, 563, 582).
- PowerPoint's notes-page navigation injects keystrokes (`CKeyboardDriver`, `clearAltKey` checks `GetKeyState(VK_MENU)`; PowerPointInterface.cpp ~998, ~1037). Injected input is queued and processed after the command returns, today and after the change.
- There is no deferral mechanism in any add-in today (no `PostMessage`, message windows or timers in the three add-ins).

### #9 Word Alt+1

- Alt+1 is bound in Word's default table ([keyboard_shortcuts_word.cpp:64](../../WordAssist/WordAssist/keyboard_shortcuts_word.cpp), `"1"` → `L'1'` = `VK_1`, matching `wParam`), mapped to `OnEntry1Action` (key_mapper_word.h:146). `OnEntry1Action` (WordController.cpp:1019) returns true unless it throws, and a throw would have shown a COM error dialog, which the reporter didn't see. So if the mapper had run the command, the key would have been consumed and the entry inserted. The key reaching Word means the hook passed it on before the command ran.
- The hook passes Alt+digit on only when one of these holds: the `KF_DLGMODE|KF_MENUMODE` flags are set; the #2 guard fails (focus not under an enabled `OpusApp`); shortcuts are toggled off; or `KeyMapperWord::map_command` returns false at `if (! m_target->IsActive())` (key_mapper_word.h:65).
- `WordController::IsActive` (line 2228) → `WordApplication::is_active` ([WordAutomationObject.cpp:38](../../common/WordAutomationObject.cpp)): false unless `m_active` is set and `::GetFocus() == m_active_window` (lines 53–57). `m_active_window` is captured by `activate()` (line 102: `m_active_window = ::GetFocus()`) from the `WindowActivate`, `DocumentChange`, `NewDocument`, `DocumentOpen` and `WindowSelectionChange` events ([Connect.cpp:2434–2545](../../WordAssist/WordAssist/Connect.cpp)), and cleared by `WindowDeactivate`.
  - So the shortcut fails whenever focus is on any HWND other than the one that happened to have focus during the last of those events. For example, `WindowActivate` fires during activation, possibly before Word moves focus to the document window, and returning to Word by its title bar or Alt+Tab changes no selection, so nothing re-captures the HWND.
  - The reporter's steps include running the command from the menu and checking Felix's glossary window before pressing Alt+1, which is that kind of round trip.
- Supporting evidence:
  - Word menu commands don't check `IsActive` (e.g. `CConnect::OnEntry1`, Connect.cpp:2156), which is why the menu worked.
  - Excel and PowerPoint `IsActive()` return true unconditionally, which is why Excel Alt+1 worked.
  - Word Alt+0..9 passed QA for the #2 fix (AC6/AC8 of the previous design), so Word doesn't always take Alt+digit before the hook.
- Alternative not ruled out by code: Office could handle the QAT's Alt+digit before Felix's hook sees the key (e.g. its own hook installed later). R7's logging distinguishes the two: under this alternative the hook logs nothing for the keystroke.

### #10 Excel Auto Translate

- `OnAutoTransSel` ([FelixExcelIF.cpp:469](../../ExcelAssist/ExcelAssist/FelixExcelIF.cpp)) creates `formula_bar_refresher refresh_on_exit` (line 472), then loops over `Selection.Item(i)` calling `AutoTransCell`. `AutoTransCell` (line 765) queries Felix per cell (`assistant->SetQuery`, a cross-process call) and, on a perfect match, `set_cell_text(activeCell, ...)` ([excel_selection_writer.h:12](../../ExcelAssist/ExcelAssist/excel_selection_writer.h)) through `Range.Characters.Text`.
- PR #7 proved that a `Characters` write leaves the formula bar stale until `ActiveCell.Activate()` (`excel::application::RefreshActiveCell`, [excel_application.h:86](../../ExcelAssist/ExcelAssist/excel_application.h)). The refresher runs only on scope exit (excel_selection_writer.h:91–105).
- So the stale period is from the active cell's write to the end of the command, i.e. every later cell's Felix round trip. With two cells it is one lookup, which is what the screenshot caught; with a large selection it lasts the whole run. That fits the report: transient, and correct after completion.
- `OnAutoTransSheet` (line 559, via `PerformSheetAutoTrans`, which also re-selects a row-1 cell every `ROW_SELECTION_INTERVAL` rows) and `OnAutoTransWorkbook` (line 576) have the same shape.
- The fakes support a test: `FakeApplication` counts `RefreshActiveCell` calls and returns a configurable `active_cell`; `FakeRange` has settable `row`/`col` ([fake_excel.h](../../ExcelAssist/ExcelAssist/fake_excel.h)).

### #11 PowerPoint bindings and labels

- Default bindings ([keyboard_shortcuts_ppt.cpp:33–80](../../PowerPointAssist/PowerPointAssist/keyboard_shortcuts_ppt.cpp)):
  - Alt+C → `Auto Trans to Fuzzy` (line 61).
  - Ctrl+Alt+C → `Trans Concordance` (line 48).
  - No source-concordance command: `KeyMapperPpt` ([key_mapper_ppt.h](../../common/key_mapper_ppt.h)) has no `Concordance` command, and `KeyTargetPpt` has no `OnConcordanceAction`.
  - Alt+Z unbound.
- The user's bindings live in `shortcuts_ppt.xml` in the config folder. It is written from the defaults on first run and never updated (`get_shortcuts_text`, keyboard_shortcuts_ppt.cpp:6). Changing a default therefore reaches new users only.
- Menu captions are hard-coded, not generated from the bindings ([Connect.cpp](../../PowerPointAssist/PowerPointAssist/Connect.cpp)):
  - English: `"Find C&oncordance (ALT + C)"` at lines 458 (`add_menu`), 1467 (`set_english_trans_menu_text`) and 1620 (`set_english_review_menu_text`). `"Translate to Fu&zzy (ALT + Z)"` at lines 415, 1427 and 1580.
  - Japanese: `IDS_CONCORDANCE_J` ("…(&O) (ALT + C)") and `IDS_AUTO_TRANS_FUZZY_J` ("…(&Z) (ALT + Z)") in [AddIn.rc](../../PowerPointAssist/PowerPointAssist/AddIn.rc) lines 340 and 343, used at Connect.cpp 1778/1818 (trans) and 1927/1967 (review).
  - AddIn.rc is Shift-JIS (`#pragma code_page(932)`, line 20) and must stay so.
- Pressing Alt+C on a selection runs Translate to Fuzzy, which walks forward and reports "End of Slide Reached". That's what the reporter saw.
- Other apps:
  - Word: concordance on the chord Alt+M, C (keyboard_shortcuts_word.cpp:79), fuzzy on Alt+Z (line 59).
  - Excel: no concordance shortcut; Alt+Z is Auto Translate Selection (keyboard_shortcuts_excel.cpp:56).
- The PowerPoint manual ([ppt/trans-mode.html](../manual/en/ppt/trans-mode.html)) names no shortcut for concordance or Translate to Fuzzy, so no manual change is needed.

## 4. Proposed behavior

- **#8.** Felix shortcuts behave as now for the user, except:
  - No COM error, however fast keys are pressed.
  - A shortcut pressed while a command is still running (for example while Felix is busy) runs after that command, in order.
  - A Felix shortcut key never reaches Office, even if its command declines. The exception is Ctrl+Right, which still moves or extends the caret when there's nothing to extend.
- **#9.** Word shortcuts work whenever the keyboard focus is in the document (the #2 guard), including straight after switching back from Felix. Alt+1 inserts glossary entry 1.
- **#10.** The formula bar follows the active cell during Auto Translate. Other cells are unaffected; the selection and active cell don't change.
- **#11.** The PowerPoint menu shows the bindings that actually work: Find Concordance (no shortcut), Translate to Fuzzy (Alt+C).
- **Failure behavior:**
  - A command that throws while running from the queue is caught and logged; the queue goes on with the next command, and the command's existing error dialogs still appear.
  - If the window can't be created, shortcuts run synchronously as today (R5).
  - A #10 refresh failure is logged and ignored.

## 5. Technical design

### 5.1 #8: run shortcut commands from the add-in's window, not the hook

**Seam.** `KeyboardShortcuts` calls `KeyMapper::map_command`. A decorator `KeyMapper` sits between the shortcut table and each app's mapper, so the three hook procedures and `receive_keypress` don't change.

**`common/key_mapper.h`** gets a synchronous pre-check with a default:

```cpp
class KeyMapper
{
public:
	virtual ~KeyMapper(){}
	virtual bool map_command(string command) = 0 ;
	// Called from the keyboard hook. Returns false if the keystroke for
	// `command` should go to the Office app instead. Must make no
	// cross-process COM calls; in-process Office object-model calls are allowed.
	virtual bool accepts(const string &command) { command ; return true ; }
};
```

**New header (proposed) `common/deferred_key_mapper.h`**, header-only (ATL `CWindowImpl`), so no project needs a new .cpp:

```cpp
namespace shortcuts
{
	const UINT WM_FELIX_RUN_COMMANDS = WM_APP + 0x0F31 ;

	class DeferredKeyMapper : public KeyMapper,
	                          public CWindowImpl<DeferredKeyMapper, CWindow, CNullTraits>
	{
	public:
		explicit DeferredKeyMapper(KeyMapper *inner) ;
		bool create() ;    // message-only window (HWND_MESSAGE) on the calling (Office UI) thread; logs and returns false on failure
		void destroy() ;   // destroys the window, drops queued commands
		// From the hook (via receive_keypress):
		//   if ! inner->accepts(command)          -> return false (key passes on)
		//   if no window                          -> return inner->map_command(command) (R5, today's behavior)
		//   push {command, shift_key_is_pressed()} to m_queue
		//   if ! m_posted: PostMessage(WM_FELIX_RUN_COMMANDS); m_posted = true
		//   return true
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
}
```

`run_pending()`:

```
m_posted = false
if m_running: return          // nested pump inside a running command; the outer call drains the queue (R3)
m_running = true
while m_queue not empty:
    item = pop_front()
    scoped_shift_state shift(item.second)     // R4
    try { m_inner->map_command(item.first) } catch (...) { log_error; }
m_running = false
```

Invariants:

- The hook path makes no cross-process COM call (`accepts` contract, R1).
- Commands never nest (`m_running`).
- FIFO order.
- At most one `WM_FELIX_RUN_COMMANDS` is outstanding (`m_posted`), so a burst of keys costs one message.
- The window proc catches everything; no exception reaches Office.

**Shift capture (R4).** In `common/keyboard_shortcuts.h`, `shift_key_is_pressed()` consults an override set by an RAII helper:

```cpp
namespace shortcuts { struct scoped_shift_state { explicit scoped_shift_state(bool down); ~scoped_shift_state(); } ; }
inline bool shift_key_is_pressed() ; // returns the innermost active override if any, else GetKeyState(VK_SHIFT)
```

The override is a function-local static (single UI thread per add-in instance) and is restored by the destructor, so nested scopes are safe. The hook path suspends the override: Office can pump messages inside a running command, so `DeferredKeyMapper::map_command` (accepts and the capture) and the hook procedures read the physical key (`shortcuts::scoped_physical_shift_state`, `shortcuts::physical_shift_key_is_pressed()`). Excel's three direct `::GetKeyState(VK_SHIFT)` reads (FelixExcelIF.cpp 476, 563, 582) change to `shift_key_is_pressed()`. Menu handlers are unaffected: no override is active when they run.

**Per-app `accepts` (R2):**

| Mapper | `accepts(command)` returns false when |
|---|---|
| `KeyMapperWord` (key_mapper_word.h) | `! m_target->IsActive()`; or command is Extend Lookup / Extend Trans Lookup and (`shift_key_is_pressed()` or `m_target->SelectionIsEmpty()`) |
| `KeyMapperPpt` (key_mapper_ppt.h) | `! m_target->IsActive()`; or command is `CMD_EXTEND_LOOKUP` / `CMD_EXTEND_TRANS_LOOKUP` and (`shift_key_is_pressed()` or `! m_target->HasSelectedText()`) |
| `KeyMapperExcel` (key_mapper_excel.h) | `! m_target->IsActive()` (always true today) |

New target methods (in-process only):

- `KeyTargetWord::SelectionIsEmpty()`. `WordController` implements it as `m_word_object.selection_is_empty()`.
- `KeyTargetPpt::HasSelectedText()`. `CPowerPointInterface` implements it as `m_app->Active == msoTrue` and `m_textRangeParser.getSelectedText(true).length() > 0`, which are the first checks of `OnExtendLookupAction`.

Both catch everything and return "empty"/false on error, so the key passes on. Test fakes (`key_target_*_fake.h`, `key_mapper_fake.h`) get controllable versions.

**Wiring (each `Connect.h`/`Connect.cpp`):**

- Add `shortcuts::DeferredKeyMapper m_deferred_mapper` constructed with `&m_mapper`, declared after `m_mapper` and before `m_keyboard_shortcuts`.
- Construct `m_keyboard_shortcuts(&m_deferred_mapper)` instead of `&m_mapper`. Word Connect.cpp:2056, Excel Connect.cpp:46, PowerPoint Connect.h:134.
- `m_deferred_mapper.create()` in `OnConnection` next to `installhook`.
- `m_deferred_mapper.destroy()` in `OnDisconnection` after `uninstallhook`. (Implementation note, step 03: `destroy()` goes in `OnBeginShutdown` right after `uninstallhook`, because that is where Word and Excel remove the hook; PowerPoint, which never uninstalls its hook (pre-existing), calls it as the first statement of `OnBeginShutdown`'s try block; on an unload from the COM Add-ins dialog the destructor covers it.)
- Word's controller swap (Connect.cpp:316, 2604) only changes `m_mapper.m_target`; the decorator holds `&m_mapper`, so nothing else changes.

**Ordering effects:**

- *The keystroke itself.* It is consumed in the hook exactly as now (`return 1`), so Word, Excel or PowerPoint never sees it. Only the command moves later. `GetMessage` returns posted messages before input messages, so a command posted from the hook runs before any keystroke typed after it is processed. For example, after Alt+Down followed by typing, the translation is inserted before the typed text, as today.
- *During a running command.* If Office pumps input while a command waits on Felix, a further shortcut is queued, not run. If Office also dispatches the posted message inside that pump, `run_pending` sees `m_running` and returns. The outer `run_pending` then runs it after the current command. Ordinary typing that Office processes during the wait behaves as today.
- *Chords.* `receive_keypress` keeps the chord state machine synchronous. The first key (Alt+M) is consumed with no command. The second key's command is queued like a single key and its key is always consumed (#7). `ToggleShortcuts` (table entry or the hard-coded Ctrl+Alt+F9) stays synchronous in the hook; it makes no COM call.
- *Auto-repeat.* Each repeated keydown queues one command, as each repeated keydown runs one command today. The difference is that a backlog runs after the key is released rather than failing.
- *Modal UI.* A command that opens a dialog or message box runs from the window proc, like a menu click. Keys typed into that dialog go to it (the #2 guard sees the disabled main window). Queued shortcuts wait until the dialog closes and the command returns.
- *Declines.* Apart from the R2 pass-through cases, a key whose command declines is consumed (today it was passed to Office, e.g. Alt+Up after answering No to the "long translation" prompt).
- *Shutdown.* Commands still queued when the add-in disconnects are dropped with the window.

**Residual risk.** If Office ever dispatches the posted message from inside a message filter while *no* Felix command is running (i.e., Office's own outgoing call), that command can still fail with 0x80010005. This is no worse than today and much rarer. No retry is added: compound commands such as Look up Next have already changed the document before they call Felix, so a retry would skip a sentence.

### 5.2 #9: Word activation check

`WordApplication::is_active` ([WordAutomationObject.cpp:38](../../common/WordAutomationObject.cpp)) drops the HWND comparison (lines 53–57):

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

Reasons:

- The #2 guard in the hook (`keystroke_target_is_main_window(GetFocus(), L"OpusApp")`) already checks, on every keystroke, that focus is in an enabled Word main window. That is the robust form of what the HWND comparison approximated.
- `m_active` still turns shortcuts off between `WindowDeactivate` and the next activation event.

`m_active_window`, `activate()` and `active_window_has_focus()` stay; `new_window_has_focus()` still uses them. `IsActive` is also used by `OnMenuPreferences` (Connect.cpp:1969) and the segmentation swap (line 2601), which both benefit.

**Diagnostics (R7).** In each hook, at the points that return `CallNextHookEx` for a key that had Alt or Ctrl down, log one `logging::log_debug` line naming the reason (`dialog guard`, `shortcuts off`, `not mapped or declined`). `DeferredKeyMapper::map_command` logs `queued <command>` and, when `accepts` is false, `declined <command>`. Keys without Alt/Ctrl aren't logged, to keep the log small. These pass-on lines are compiled in Debug builds only, because the add-ins log at debug level in Release too; QA uses the Debug build.

If AC6 still fails after this change and the log shows no line for Alt+1, the cause is the alternative in section 3. See D4 for what to do then.

### 5.3 #10: refresh when the active cell is written

**`excel_selection_writer.h`** gets:

```cpp
// If `written` is the application's active cell (same row and column),
// refreshes the formula bar now. Never throws; failures are logged.
inline void refresh_formula_bar_if_active(excel::app_ptr app, excel::range_ptr written) ;
```

Implementation:

- Inside a try, compare `written->Row()`/`Column()` with `app->GetActiveCell()->Row()`/`Column()`; if equal, call `refresh_formula_bar(app)` and then `reload_formula_bar(app)`.
- Catch `_com_error` and `...` and log at warn level, like `refresh_formula_bar`.
- Comparing row and column only can fire for the same address on a non-active sheet during Auto Translate Workbook. That is harmless: `RefreshActiveCell` re-activates the active cell, which keeps the selection and active cell, and the reload below runs once more.

QA found that `Activate` alone does not update the formula bar mid-command: Excel refreshes it only when the add-in command returns (`ac8_formulabar_samples_menu.txt`, `_keyboard.txt`). Repainting the formula-bar window (`EXCEL<`) or `XLMAIN` with `RedrawWindow(RDW_UPDATENOW)`, re-reading `ActiveCell.Formula`, toggling `ScreenUpdating`, and setting `FormulaBarHeight` to its own value had no effect; hiding and showing the formula bar does work, so **`reload_formula_bar(app)`** (never throws) sets `Application.DisplayFormulaBar` to false and back to true when it is shown (new `excel::application::FormulaBarShown`/`ShowFormulaBar`), leaves a hidden formula bar alone, and retries the show once if it fails so the user's setting is kept (`ac8_formulabar_samples_fixed.txt`: the bar shows the translation in the same sample as the cell). No message pumping is involved.

**`CFelixExcelIF::AutoTransCell`** (FelixExcelIF.cpp:821): straight after `set_cell_text(activeCell, assistant->GetTrans())`, call `refresh_formula_bar_if_active(m_excel_app, activeCell)`.

The three `formula_bar_refresher`s stay, because the active cell can change during Sheet auto-translate (row re-selection) and the final state must be right.

Cost: three in-process Excel calls per translated cell (no Felix call), and one `Activate` plus one formula-bar hide/show for the active cell only.

### 5.4 #11: PowerPoint labels (D1 = relabel)

In [PowerPointAssist/PowerPointAssist/Connect.cpp](../../PowerPointAssist/PowerPointAssist/Connect.cpp):

| Lines | Old caption | New caption |
|---|---|---|
| 458, 1467, 1620 | `Find C&oncordance (ALT + C)` | `Find C&oncordance` |
| 415, 1427, 1580 | `Translate to Fu&zzy (ALT + Z)` | `Translate to Fu&zzy (ALT + C)` |

In [AddIn.rc](../../PowerPointAssist/PowerPointAssist/AddIn.rc), saved as Shift-JIS:

- `IDS_CONCORDANCE_J` drops ` (ALT + C)`.
- `IDS_AUTO_TRANS_FUZZY_J` changes `(ALT + Z)` to `(ALT + C)`.

Tooltips are unchanged: the English tooltips name no key, and the Japanese tooltips load the same string as the caption. Bindings, `key_mapper_ppt.h` and the manual don't change.

If D1 is decided as "rebind" instead, see D1 for the extra work.

## 6. Consequences

- **Compatibility:**
  - #8 changes when commands run, not what they do.
  - A Felix key whose command declines no longer leaks to Office, except Ctrl+Right (R2). Users relying on that leak, e.g. Alt+Up reaching Word after a cancelled prompt, lose it; none of those leaks did anything useful.
  - #9 makes Word shortcuts work in more focus states; the #2 guard still excludes dialogs.
  - #11 changes only text.
- **Risk:**
  - The deferral is the largest change. It touches every shortcut in three add-ins.
  - The `accepts` pre-checks must match today's pass-through behavior for Ctrl+Right, or Ctrl+Right stops moving the caret. AC3 covers it.
  - `shift_key_is_pressed()` gains a hidden override. It is scoped to the running command, and the hook path suspends it (`DeferredKeyMapper::map_command` reads the physical key, as do hook procedures).
  - Dropping the HWND check could let a shortcut run while focus is in an `OpusApp` child that isn't the document (task pane search box, ribbon). That was already possible for Excel and PowerPoint. A command there acts on Word's current selection, as the menu does.
- **Performance:** one `PostMessage` per shortcut, one message per burst. #10 adds three in-process COM calls per translated cell.
- **Rollback:** the four fixes are independent. Reverting #8 is the constructor argument plus `create`/`destroy` in each `Connect`; `accepts` can stay.
- **Release:** add-ins only, in the 3.0.1 installer. Release-note lines are the orchestrator's call (`setup/release-notes-3.0.1.md`).

## 7. Verification

### Unit tests (Boost.Test, Debug x64; baseline Test_WordAssist 166, Test_Excel 65, Test_PPT 81)

- **Test_PPT `deferred_key_mapper_test.cpp`** (new; Test_PPT already holds the shared `keyboard_shortcuts_test.cpp`). Use `KeyMapperFake`, extended with a settable `accepts` result, a recorded shift value per call, and an optional hook that calls back into the decorator. Cases:
  - Without `create()`, `map_command` runs the inner mapper immediately and returns its result (R5).
  - After `create()`:
    - `map_command` returns true and the inner mapper isn't called until the window processes the posted message. Pump with `PeekMessage`/`DispatchMessage` on the test thread.
    - Three commands queued, then one pump: all three run in order, and only one message was posted.
    - `accepts` false: returns false, nothing queued.
    - Shift captured: queue with Shift reported down (through `scoped_shift_state` in the test), run with it up; the inner mapper observes "down".
    - Re-entrancy: the inner mapper queues another command and calls `run_pending()` itself; the second command runs after the first returns, not inside it.
    - An inner mapper that throws: the next queued command still runs.
    - `destroy()` drops pending commands.
  - `scoped_shift_state`: nesting restores the outer value, and with no override the result is `GetKeyState`.
  - Through `KeyboardShortcuts` with the decorator: a chord's second key is consumed and queued, and `ToggleShortcuts` stays synchronous.
- **Test_WordAssist.** Mapper `accepts` with a fake `KeyTargetWord`:
  - Extend Lookup is declined when Shift is down or the selection is empty, and accepted otherwise.
  - Any command is declined when `IsActive` is false.
  - Entry 1 is accepted when active.
  - `WordApplication::is_active` returns false when not instantiated. The active path can't be faked without Word; covered by AC6.
- **Test_PPT.** `KeyMapperPpt::accepts` with the fake target for `CMD_EXTEND_LOOKUP` and `CMD_EXTEND_TRANS_LOOKUP` (Shift, no text, text); other commands are accepted.
- **Test_Excel `excel_selection_writer_test.cpp`.** `refresh_formula_bar_if_active`:
  - Same row/column: refreshed once.
  - Different row or column: not refreshed.
  - `GetActiveCell` throws (fake with no active cell): no throw, no refresh.
  - `RefreshActiveCell` throws: no throw.
- Run all three programs; existing totals plus the new cases pass.

### Manual Office checks (64-bit Microsoft 365, Debug x64 add-ins and Debug Felix, registered with `tools\register-felix.ps1 -Configuration Debug`, which the user runs)

- Run AC1–AC10.
- For #8, check the add-in log for `queued` lines and the absence of 0x80010005.
- AC1 is most reliable with a large memory loaded, so Felix's responses take long enough for keys to arrive mid-call. Before the fix, reproduce once on the unfixed build to confirm the setup triggers it.
- AC8 samples the formula bar with UI Automation, as in PR #7.
- 32-bit Office can't be tested; the Win32 add-ins only need to build.

## 8. Decisions and assumptions

**Decisions for the user. Each has a recommendation, and the design assumes it is accepted.**

- **D1 (#11). Relabel the menu or rebind the key.**
  - **Recommended: relabel** (section 5.4). Six caption literals and two Japanese strings change, and nothing else. Existing users' `shortcuts_ppt.xml` already has Alt+C → Translate to Fuzzy, so relabelling is right for every default configuration, old and new. Users keep their muscle memory.
  - Alternative: rebind defaults to match the labels and Word: Alt+Z → Translate to Fuzzy, Alt+C → Find Concordance. Needs:
    - a new `Concordance` command in `key_mapper_ppt.h`;
    - `KeyTargetPpt::OnConcordanceAction(bool)`;
    - a new default;
    - a migration of existing `shortcuts_ppt.xml` files (rewrite Alt+C → fuzzy to the new pair when the file still has the old default), or existing users keep the old binding and the label stays wrong for them.
    - It also changes what Alt+C does for current users.
  - Alternative: relabel now, and add a concordance shortcut (e.g. Alt+M, C as in Word) for new installs in a follow-up.
- **D2 (#8). Defer in all three add-ins or only Word.**
  - **Recommended: all three.** The defect is in shared dispatch, and Excel and PowerPoint make the same cross-process calls from the same kind of hook. The decorator is wired with two lines per add-in.
  - Alternative: Word only, with follow-up issues for Excel and PowerPoint.
- **D3 (#10). Refresh during Auto Translate or accept the transient.**
  - **Recommended: refresh when the active cell is written** (section 5.3). It is cheap, and on a large selection the "transient" lasts the whole run.
  - Alternative: accept it and close #10 as expected behavior, since the bar is correct when the command ends.
- **D4 (#9). Fix on the inferred cause or diagnose first.**
  - **Recommended: fix now** (section 5.2) together with the R7 logging, and verify with AC6. The change is small and safe given the #2 guard, and it explains every observation.
  - If AC6 still fails and the log shows the hook never saw Alt+1, Office is taking Alt+digit before the hook. Options then: leave the Word glossary keys as they are and document Alt+digit as unreliable when the QAT has items; or add a second Word binding for glossary entries (e.g. Ctrl+Alt+digit) in new defaults. That would be a new decision.
  - Alternative: ship an instrumented build first and have the user reproduce, then fix.

**Assumptions:**

- Office's message loop dispatches messages posted to an add-in's message-only window normally. This is standard Win32 behavior; COM add-ins commonly rely on it.
- `GetMessage` priority (posted before input) holds for Office's loops, so a deferred command runs before later keystrokes. If an Office loop filters by message range, a command could run slightly later; AC1 and AC4 would show it.
- In-process Office object-model calls from the hook (`SelectionIsEmpty`, `HasSelectedText`) succeed in the message-filter context. #8 shows `select_next_sentence` succeeding there.
- Word's `WindowActivate` can fire before focus reaches the document window (#9). Not observed directly.
- The add-in log is at `%LOCALAPPDATA%\Felix\logs\` and debug-level lines are written in Debug builds (`LOG_LEVEL` is verbose in [logging.h](../../common/logging.h)).

## 9. Out of scope: problems noticed

- **Other PowerPoint menu labels don't match the bindings or the item:**
  - The Japanese review-mode captions show Alt where the binding is Ctrl+Alt: `IDS_LOOKUP_TRANS_J` (ALT + L), `IDS_LOOKUP_NEXT_TRANS_J` (ALT + →), `IDS_CORRECT_TRANS_J` (ALT + ↑).
  - `IDS_GLOSS_N_J` says ALT + G, which is Get and Next.
  - `set_english_review_menu_text` puts the Set caption on `m_menu_get_and_next` and the Get and Next caption on `m_menu_set` (Connect.cpp ~1590–1602).
  - Captions could be generated from the loaded shortcut table to prevent this class of bug.
- `IDS_TRANS_CONCORDANCE_J` is defined but unused.
- Word's `catch(CException&)` path and every command's own `CATCH_ALL` show modal error dialogs. With deferral they come from the window proc, like menu errors.
- PowerPoint notes-page navigation simulates keystrokes with `keybd_event` (`CKeyboardDriver`). It's fragile and unrelated to this change.
- Word, Excel and PowerPoint all create or re-check `Felix.App` on every command (`getAssistant`), which adds cross-process round trips per keystroke and lengthens the window in which #8 could happen.
