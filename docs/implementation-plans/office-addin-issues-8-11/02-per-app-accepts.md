# 02 - #8: per-app `accepts` in the Word, PowerPoint and Excel mappers

Index: [index.md](index.md). Design: [sections 3 (declines), 5.1 table and 7](../../designs/office-addin-issues-8-11.md). Depends on: 01. Size: M.

## Objective
Each app's `KeyMapper` implements `accepts`, so that once the decorator is wired (step 03) the hook consumes a key exactly when today's behaviour would have, except that other declines now consume the key. The one deliberate pass-through is Ctrl+Right (Extend Lookup, Extend Trans Lookup) with Shift down or nothing selected.

## Requirements (local copy)
- R2: whether a keystroke is consumed is decided in the hook from shortcut tables and cheap in-process checks:
  - the Word and PowerPoint mappers' activation check (`IsActive()`) passes the key on when it fails;
  - Extend Lookup and Extend Trans Lookup (Word and PowerPoint; Ctrl+Right by default) pass the key on when Shift is down or nothing is selected, so Ctrl+Right and Ctrl+Shift+Right keep moving/extending the caret;
  - every other mapped shortcut is consumed even if its command later declines or fails;
  - Excel: `accepts` returns false only when `IsActive()` fails (it is always true today).
- `accepts` makes no cross-process COM calls. New target methods are in-process only, catch everything and return "empty"/false on error (so the key passes on).

## Dependencies and prerequisites (from step 01, copied)
- **C1** `common/key_mapper.h`: `KeyMapper::accepts(const string &command)` is virtual, default returns true.
- **C2** `shortcuts::scoped_shift_state(bool down)` in `common/keyboard_shortcuts.h`: while alive, `shift_key_is_pressed()` returns `down`. Use it in tests to control Shift.
- **C5 (KeyMapperFake)** exists; this step adds the target-fake controls below.

## Contracts produced (identical to the index: C4, C5 target-fake part)
- **C4** `WordAssist/WordAssist/key_target_word.h`: `virtual bool SelectionIsEmpty() = 0;` (in-process; catches everything; true on error). `WordController::SelectionIsEmpty()` returns `m_word_object.selection_is_empty()` (`common/WordAutomationObject.h:315`). The Html and Word controller subclasses (`WordControllerHtml.h`, `WordControllerWord.h`) forward it like they forward `IsActive`. `common/key_target_ppt.h`: `virtual bool HasSelectedText() = 0;` (in-process; false on error). `CPowerPointInterface::HasSelectedText()` returns `m_app->Active == msoTrue && m_textRangeParser.getSelectedText(true).length() > 0` inside a catch-all (the first checks of `OnExtendLookupAction`, PowerPointInterface.cpp ~384).
- **C5 (target fakes)**: `KeyTargetWordFake` (`Test_WordAssist/Test_WordAssist/key_target_word_fake.h`): `bool m_is_active = true; bool m_selection_is_empty = false;`, with `IsActive()` returning `m_is_active` and `SelectionIsEmpty()` returning `m_selection_is_empty`. `KeyTargetPptFake` (`common/key_target_ppt_fake.h`): `bool m_is_active = true; bool m_has_selected_text = true;`, `IsActive()` returns `m_is_active`, `HasSelectedText()` returns `m_has_selected_text`.
- `accepts` rules (design 5.1 table):

| Mapper | `accepts(command)` returns false when |
|---|---|
| `KeyMapperWord` (`WordAssist/WordAssist/key_mapper_word.h`) | `! m_target->IsActive()`; or command is `CmdExtendLookupAction` / `CmdExtendTransLookupAction` and (`shift_key_is_pressed()` or `m_target->SelectionIsEmpty()`) |
| `KeyMapperPpt` (`common/key_mapper_ppt.h`) | `! m_target->IsActive()`; or command is `CMD_EXTEND_LOOKUP` / `CMD_EXTEND_TRANS_LOOKUP` and (`shift_key_is_pressed()` or `! m_target->HasSelectedText()`) |
| `KeyMapperExcel` (`ExcelAssist/ExcelAssist/key_mapper_excel.h`) | `! m_target->IsActive()` |

Command-name comparison is case-insensitive, as `map_command` does (`boost::iequals`). Everything else is accepted. `map_command` keeps its own checks unchanged (it still returns false when inactive or for Extend Lookup with Shift).

## Scope and implementation guidance
Test first:
1. **Test_WordAssist** (`keyboard_mapper_word_test.cpp`, suite `KeyMapperTest`): with `KeyTargetWordFake` and `KeyMapperWord`: Extend Lookup declined with Shift down (use `shortcuts::scoped_shift_state`), declined with `m_selection_is_empty`, accepted otherwise; same for Extend Trans Lookup; any command declined when `m_is_active` is false; `Entry 1` accepted when active; an unmapped command name is accepted (the decorator then runs `map_command`, which returns false). In `WordAutomationObject_test.cpp` (suite `WordApplicationTest`) nothing is added in this step.
2. **Test_PPT** (`common/key_mapper_ppt_test.cpp`): `KeyMapperPpt::accepts` with `KeyTargetPptFake` for `CMD_EXTEND_LOOKUP` and `CMD_EXTEND_TRANS_LOOKUP` (Shift down, no selected text, selected text), inactive target declines everything, other commands accepted.
3. **Test_Excel** (`key_mapper_excel_test.cpp`): `accepts` true when active, false when the target reports inactive (give `key_target_excel_fake.h` a `bool m_is_active = true` that `IsActive()` returns, if it does not have one). These tests use `KeyTargetExcelFake`, not `fake_excel.h`, so they need no `_DEBUG` guard; if you add any test that uses `excel::FakeApplication`/`FakeRange` from `fake_excel.h`, guard it `#if defined(UNIT_TEST) && defined(_DEBUG)` because those fakes are Debug-only.
4. Implement C4 and the `accepts` overrides, and the fake controls. Keep fakes' existing behaviour (default values reproduce it). Update every concrete implementer of `KeyTargetWord` and `KeyTargetPpt` so the build still links: `WordController`, `WordControllerHtml`, `WordControllerWord`, `CPowerPointInterface`, and the fakes; search for other subclasses (`: public KeyTargetWord`, `: public KeyTargetPpt`) before finishing.
5. `WordController::SelectionIsEmpty` and `CPowerPointInterface::HasSelectedText` have no unit test (they need Office); wrap them in a try/catch-all returning "empty"/false, as specified.

Exclusions: no wiring into Connect (step 03); no change to `map_command` behaviour; no hook changes; no change to `WordApplication::is_active` (step 04).

## Verification
```powershell
$ms = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
& $ms Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_WordAssist\Test_WordAssist\x64\Debug\Test_WordAssist.exe --report_level=short
& $ms Test_Excel\Test_Excel.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_Excel\x64\Debug\Test_Excel.exe --report_level=short
& $ms Test_PPT\Test_PPT.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
Test_PPT\x64\Debug\Test_PPT.exe --report_level=short
& $ms WordAssist\WordAssist\WordAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms ExcelAssist\ExcelAssist\ExcelAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $ms PowerPointAssist\PowerPointAssist\PowerPointAssist.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```
- Red first, then green. Totals: each program equals its count after step 01 plus the new cases; all pass.
- Also build Test_Excel Release|x64 and run it (Release must still compile).
- Manual (Ctrl+Right moves the caret, AC3): deferred to QA; record that.

## Completion criteria
- [ ] Tests written first and passing; counts recorded per program.
- [ ] `accepts` in the three mappers per the table; C4 methods in Word and PowerPoint targets and every implementer; fakes per C5.
- [ ] All three test programs pass Debug|x64; Test_Excel builds Release|x64; the three add-ins build Debug|x64.
- [ ] Committed; index row 02 updated.

## Execution record
