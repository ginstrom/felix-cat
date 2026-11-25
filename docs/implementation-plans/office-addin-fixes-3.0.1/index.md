# Implementation plan: Office add-in fixes for 3.0.1 (issues #1, #2, #3)

Source design: [../../designs/office-addin-fixes-3.0.1.md](../../designs/office-addin-fixes-3.0.1.md) (approved 2026-11-25; repo commit 48b9bfd). Feature record (decisions in force): [../../features/office-addin-fixes-3.0.1.md](../../features/office-addin-fixes-3.0.1.md). Branch: `fix/office-addin-issues-1-3`. Log: [log.md](log.md).

Purpose: fix three Office add-in bugs in Felix 3.0.1. Felix.exe does not change.

- #1 (P2): Word Find Concordance / Translation Concordance send the trailing paragraph mark and find nothing.
- #2 (P2): Alt+N in Excel's "save changes?" prompt runs Felix's Next and relaunches Felix. Same defect in Word and PowerPoint.
- #3 (P3): Excel's formula bar stays stale after Get Translation.

## Decisions in force
- D1: the #1 trim applies only in `WordController::OnConcordanceAction` and `OnTransConcordanceAction`. `get_selection_text`, `WordParser.cpp` (a UTF-16 file) and PowerPoint are not touched.
- D2: the #2 guard goes into the Excel, Word and PowerPoint hooks through one shared helper.
- D3: no change to whether shortcuts may start Felix.
- Release notes get one line per fix in `setup/release-notes-3.0.1.md`.

## Shared constraints (every step)
- Tests are Boost.Test, test first: write the failing test, see it fail, then implement. Run as x64 Debug (only 64-bit Office is installed). Build with MSBuild from a VS 2026 Developer PowerShell; Boost is at `C:\local\boost_1_91_0` (`Dependencies.local.props`, git-ignored). See [BUILDING.md](../../../BUILDING.md) sections 3, 5, 6.
- The test projects are `Test_Excel\Test_Excel.vcxproj`, `Test_PPT\Test_PPT.vcxproj` and `Test_WordAssist\Test_WordAssist\Test_WordAssist.vcxproj`. Build `Debug|x64`; each runs its tests after build with `--result_code=no`, so read the output. Run by hand, for example `Test_Excel\x64\Debug\Test_Excel.exe --report_level=short`. Documented baseline totals: Word 134, PowerPoint 65, Excel 49 (re-measure at the start of step 01 and record them in the log).
- Wrap long builds and test runs with `C:\Users\softw\.claude\skills\orchestrator\scripts\runlog run --label "<step>" -- <command>`.
- Source files are UTF-8 with BOM (BUILDING.md "Source file encoding"). New files: UTF-8 with BOM. Never edit `WordAssist\WordAssist\WordParser.cpp` (UTF-16).
- Python is not installed. Use PowerShell and MSBuild. Do not push, open PRs or comment on GitHub issues.
- Manual Office checks need 64-bit Office, Debug x64 add-ins and Felix registered with `tools\register-felix.ps1 -Configuration Debug` (needs admin; it raises a UAC prompt). Close Office first. Felix must also be built (`Felix\Unicode_Debug`, Win32), and an older installed Felix must not be registered (BUILDING.md section 6). Probe scripts (keystroke injection, UI Automation) go in the scratchpad or another folder outside the repository.
- Commit per step. End each commit message with the line `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>` unless the executing role's brief names a different line.

## Contracts (fixed here; task files copy them unchanged)

| Contract | Produced by | Consumed by |
|---|---|---|
| `common/word_boundary_marks.h`: header-only, global namespace, `inline std::wstring trim_word_boundary_marks(const std::wstring &text)`. Removes 0x07, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E from both ends, looking past `<...>` tags, never removing tags or other characters. Does not throw. | 01 | 01 (call sites) |
| `common/keyboard_shortcuts.h/.cpp`, in `namespace shortcuts`: `bool keystroke_target_is_main_window(HWND focus, LPCWSTR main_window_class);` True iff `focus` is non-NULL, `GetAncestor(focus, GA_ROOT)` has window class equal (case-insensitive) to `main_window_class`, and that root window is enabled (`IsWindowEnabled`). | 02 | 03 |
| Hook guard (one early return after the up-key/modifier-only checks, before the Ctrl+Alt+F9 toggle): `if ( ! shortcuts::keystroke_target_is_main_window( ::GetFocus(), L"<class>" ) ) return CallNextHookEx(hkb, nCode, wParam, lParam);` Classes: Excel `XLMAIN`, Word `OpusApp`, PowerPoint `PPTFrameClass`. | 03 | none |
| `excel::application::RefreshActiveCell()`: `virtual void` in `ExcelAssist/ExcelAssist/excel_application.h`. If `Selection` is a worksheet range (`Excel::RangePtr`), calls `ActiveCell->Activate()`; otherwise no-op. May throw `_com_error`. | 04 | 04 |
| `ExcelAssist/ExcelAssist/excel_selection_writer.h`: `bool write_selection_text(excel::app_ptr app, _bstr_t text);` Empty text: returns false, no write, no refresh. Otherwise writes via `app->GetSelectedChars()->SetText(...)`, falling back to `set_cell_text(app->GetActiveCell(), text)`, then calls `app->RefreshActiveCell()` inside try/catch (log a warning, never throw). Returns true. | 04 | 04 (`CFelixExcelIF::set_active_cell_text`) |
| `FakeApplication::RefreshActiveCell` override in `fake_excel.h` records a call count and can be set to throw. | 04 | 04 (tests) |

## Tasks

| ID | Task file | Outcome | Depends on | Size | Status |
|---|---|---|---|---|---|
| 01 | [01-word-concordance-trim.md](01-word-concordance-trim.md) | #1: trim helper plus two call sites; AC1-AC4 | none | S, easy | done, e8ace76 (manual AC1-AC4 not run: needs Felix registration) |
| 02 | [02-keystroke-target-helper.md](02-keystroke-target-helper.md) | #2: shared `keystroke_target_is_main_window` with tests in three test projects | none | S-M, moderate (Win32 window tests, three vcxproj edits) | done, 95aa876 |
| 03 | [03-hook-guards.md](03-hook-guards.md) | #2: guard in Excel, Word, PowerPoint hooks; AC5-AC8 | 02 | S code, M manual verification (keystroke probes) | done (manual AC5-AC8 not run: deferred to QA), f1ea03f |
| 04 | [04-excel-formula-bar-refresh.md](04-excel-formula-bar-refresh.md) | #3: `RefreshActiveCell`, `write_selection_text`, fakes, tests; AC9-AC10 | none | M, moderate (Excel `#import` in Test_Excel) | done (manual AC9-AC10 not run: deferred to QA), a323bec |
| 05 | [05-release-notes-and-final-check.md](05-release-notes-and-final-check.md) | Release-note lines, full regression, Win32 build check | 01, 03, 04 | S, easy | done (manual AC1-AC10 deferred to QA), 655a95d |

## Execution order
01, 02, 03, 04, 05, sequentially. 01, 02 and 04 are logically independent, but 01 and 02 both edit `Test_WordAssist.vcxproj` and 02 and 04 both edit `Test_Excel.vcxproj`, so run them one after another rather than in parallel. Integration checkpoint after 04: all three test programs green, then step 05.

## Requirement mapping
| Requirement / criterion | Step |
|---|---|
| R1, R2, R3, R4; AC1-AC4 | 01 |
| R5 (helper) | 02 |
| R5, R6, R7; AC5-AC8 | 03 (helper from 02) |
| R8, R9; AC9, AC10 | 04 |
| AC11 (existing + new tests pass) | 01-05; final run in 05 |
| Release notes (user decision) | 05 |

## Overall completion criteria
Status 2026-11-26: met, except the manual AC1-AC10 checks, which are deferred to QA (they need Felix registered, which raises a UAC prompt). Win32 builds of the three add-ins succeeded. All five steps done; Test_WordAssist, Test_Excel and Test_PPT pass with the new cases added to the baseline totals; manual AC1-AC10 recorded in the task files' execution records (or a stated reason why one could not be run); Win32 builds of the three add-ins attempted and the outcome recorded; release-notes lines added; nothing pushed.

## Blockers and open questions
None blocking. Notes for implementers:
- The design says tests run in the "`Testing` configuration, x64". The three test projects only have `Debug` and `Release` configurations for Win32 and x64 (the `Testing_64` folders are stale output). Use `Debug|x64`. All of them define `UNIT_TEST`.
- `fake_excel.h` is only usable under `_DEBUG`, and Test_Excel currently compiles none of the Excel-wrapper sources and has no `ExcelImport.h` include. Step 04 may find the `#import` impractical in Test_Excel; the design allows falling back to the manual check for #3 (do not add Felix.tlb to Test_Excel). Record that in the log if it happens.
- The Excel hook's existing flag check (`KF_DLGMODE | KF_UP | KF_MENUMODE`) stays.

## Instructions for implementers
Update the step's row in the Tasks table (status and commit hashes) here. Record evidence, deviations and blockers in the task file's Execution record. Put anything that accumulates across steps (baseline counts, probe findings) in [log.md](log.md).
