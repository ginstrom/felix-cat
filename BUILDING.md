# Building Felix (Visual Studio 2026)

This branch retargets the main **Felix** application from VS 2013 (`v120`) to
VS 2026 (`v145`, MSVC 14.50). The Word/Excel/PowerPoint add-ins and the test projects are
not migrated yet.

## 1. Install the toolchain

- **Visual Studio 2026** (Community is fine) with the
  **Desktop development with C++** workload, plus these individual components:
  - *C++ ATL for latest v145 build tools (x86 & x64)*
  - a Windows 10/11 SDK (installed by the workload)
- **Microsoft Office** (Excel and Word). Felix `#import`s the Office type
  libraries by their registry ID, so Office must be installed on the build machine.

## 2. Put the libraries in `third_party\`

By default, `Dependencies.props` looks in `third_party\` at the repo root. To use
other locations, copy `Dependencies.local.props.example` to
`Dependencies.local.props` and edit it. That file is git-ignored.

| Library  | Put it at | Notes |
|----------|-----------|-------|
| Boost    | `third_party\boost\` (folder containing `boost\version.hpp`) | Felix needs compiled **Boost.Test, Filesystem and Regex** for 32-bit. The easiest route is the prebuilt installer `boost_1_91_0-msvc-14.5-32.exe`, which creates `lib32-msvc-14.5\`. |
| WTL      | `third_party\wtl\Include\` | WTL 10 (SourceForge or the `wtl` NuGet package). |
| cpptempl | `third_party\cpptempl\` | **Already included in the repo.** Felix's HTML template engine, pinned to the last wide-string version (commit 4c5980c). See `third_party\cpptempl\SOURCE.txt`. |

> **Boost and long paths:** the Boost installer fails with "MoveFile failed; code 3"
> if its folder path is long, because some doc file names exceed the Windows
> 260-character limit. Install it to a short path such as `C:\local\boost_1_91_0`
> and set `BoostDir` and `BoostLibDir` in `Dependencies.local.props`.

Scintilla's headers are already in `common\`. The build copies `Felix\SciLexer.dll`
next to `Felix.exe`.

**No Office on the build machine?** Set `<FelixNoExcel>true</FelixNoExcel>` in
`Dependencies.local.props`. Felix then builds without the Excel type libraries,
and Excel export shows a "not available" message instead.

**Lua is no longer needed.** Lua was never actually used: Felix and Word Assist
only created an unused Lua VM at startup. That code, and the Lua, LuaBridge and
LuaWrapper sources, have been removed from both projects.

## 3. Build

1. Open `Felix\Felix.sln`. VS will offer to retarget the add-in and test
   projects. You can **cancel** that for now.
2. Choose the configuration `Unicode_Debug|Win32`. `Unicode_Release` treats
   warnings as errors. `Testing` builds the Boost.Test unit tests into the exe.
3. Build **Felix English** and **Felix Japanese** (the UI resource DLLs), then **Felix**.

Or from a Developer PowerShell:

```powershell
msbuild "Felix\EngResource\Felix English.vcxproj" /p:Configuration=Unicode_Debug /p:Platform=Win32
msbuild "Felix\JpnResource\Felix Japanese.vcxproj" /p:Configuration=Unicode_Debug /p:Platform=Win32
msbuild Felix\Felix.vcxproj /p:Configuration=Unicode_Debug /p:Platform=Win32 /m
```

The pre-build step copies `Felix\html\` to `%LOCALAPPDATA%\Felix\html\`, where
the UI templates are loaded from at runtime.

## 4. Run

Run `Felix\Unicode_Debug\Felix.exe`. It needs, next to it:

- `SciLexer.dll` (copied by the build)
- `lang\EngResource.dll` and `lang\JpnResource.dll` (from the two resource projects).
  Without them Felix stops with the assertion `localized_instance != 0`.

Felix logs to `%LOCALAPPDATA%\Felix\logs\felix.log`.

**COM registration.** Felix is also a COM server, which the Word, Excel and
PowerPoint add-ins use. Registering it writes to the machine-wide registry, so
it needs admin rights. The build's final step (`Felix.exe /RegServer`) fails
without them, and so does the automatic re-registration that Debug builds do
on every launch. That one is now only logged, and Felix runs normally. To
register for real, run `Felix.exe /RegServer` once from an Administrator terminal.

## 5. Unit tests

The `Testing` configuration compiles Felix's ~2300 Boost.Test unit tests into
`Felix\Testing\Felix.exe`. Running it executes the tests and exits; it shows no window.

1. Build `Testing|Win32`.
2. Copy `SciLexer.dll` and the `lang\` folder from `Felix\Unicode_Debug\` into
   `Felix\Testing\` (the Testing configuration has no copy step yet).
3. Run `Felix\Testing\Felix.exe`. The summary report goes to standard error,
   for example `Felix.exe 2> results.txt`. Individual failures go to
   `OutputDebugString`, so you see them in the VS Output window when debugging,
   or with a tool such as DebugView.

`Testing` treats warnings as errors, minus a listed set of warnings that newer
compilers added (see the comment in `Felix.vcxproj`).

**Crashes hide results.** After a crash (access violation, stack overflow),
Boost.Test reports every remaining test as "passed" without running it. A
single run of `Felix.exe` is trustworthy only while nothing crashes; check that
the summary has no "aborted" tests. `tools\run-test-suites.ps1` runs each
top-level suite in its own process, so a crash only spoils its own suite:

```powershell
powershell -ExecutionPolicy Bypass -File tools\run-test-suites.ps1
powershell -ExecutionPolicy Bypass -File tools\run-test-suites.ps1 -Only TestMemory,TestFelixModel
```

It writes a report per suite, with failure details in `<suite>.debug.txt`, plus
`summary.csv` to `Felix\Testing\test-results\`, prints the suites that failed,
and exits with 1 if any did.

**Tests must not depend on the machine.** Helpers for that:

- `common\test_paths.h`: `%LOCALAPPDATA%`, the Felix app data folder, the
  login name, and paths in the source tree (`repo_path`, worked out from the
  test exe's location). Also `requires_prog_id`, which skips a suite when a COM
  class isn't registered.
- `common\test_temp_dir.h`: a scratch folder under `%TEMP%`, deleted afterwards,
  for tests that need real files (raw bytes, UTF-16, or any code page).
- `Felix\test_data_files.h`: small Felix data files (a one-record memory,
  Japanese sample text).

**Last run (VS 2026, Boost 1.91): 2282 passed, 14 skipped, 0 failed** (5016
assertions). The 14 skipped are `test_memory_remote` and `test_record_remote`.
They need the fake COM servers `Felix.RemoteMemoryFake` / `Felix.RemoteRecordFake`
from `python_tools\MemoryEngineFake.py`, a Python 2 + pywin32 script that has
to be registered, and they run automatically when it is.

(An earlier version of this file said "2288 of 2294 passed". That was wrong: a
crash early in the run meant most tests never ran.)

What it took to get here:

- **Machine-specific tests**: about 45 tests hardcoded the original development
  machine (`C:\test\...`, `D:\dev\...`, `L:\dev\Test Files\...`,
  `D:\Users\Ryan\...`, the user name "Ryan", and the removed Armadillo
  `ArmAccess.DLL`). They now use the helpers above. `TestDegenerateEmpty` and
  `TestDegenerateBogus` used to pass only because their files were missing; they
  now get a real empty file and a real TMX file.
- **A real crash**: if an export file couldn't be opened (for example, a
  MultiTerm export to a folder that doesn't exist), the exporter's destructor
  dereferenced a null pointer. It now reports the error instead. Regression
  tests: `export_gloss_mt55_bad_folder_throws`, `export_gloss_mt6_bad_folder_throws`.
- **Source encoding** (11 tests): see "Source file encoding" below.
- The "stack overflow" in `TestComMemories/test_load` was in the test: when the
  load failed it called through a null pointer. It now stops at a `BOOST_REQUIRE`.

## Source file encoding

Source files are **UTF-8 with a byte order mark** (or, for three files,
UTF-16). Save new files the same way; Visual Studio's "Save with Encoding" →
"Unicode (UTF-8 with signature)" does it.

Many files used to be Shift-JIS. VS 2013 on a Japanese-locale Windows read
them correctly, but on any other locale MSVC reads them in the local code
page (1252 on English Windows) and silently garbles every Japanese literal.
That broke the Japanese-text tests and the Japanese labels in the file
dialogs (`common\ui.h`). All 30 Shift-JIS files, including the ones in the
add-ins, plus one Latin-1 file, were converted to UTF-8 with a BOM, so every
project reads them correctly whatever the locale.

The `.editorconfig` at the repo root makes Visual Studio save C++ files that way.

Felix also compiles with `/utf-8`, so a UTF-8 file without a BOM is read
correctly too. Every Japanese literal in Felix is a wide (`L"..."`) string, so
the narrow-string encoding this also sets doesn't change any existing string.

## What changed from the original VS 2013 setup

- `PlatformToolset` v120 → v145, Windows SDK 10.0, in Felix and its two resource DLL projects.
- The hardcoded `D:\dev\cpp\...` include and library paths were replaced by `Dependencies.props`.
  `common\` is on the include path *after* the system headers, because its
  `dbghelp.h` and `Parser.h` would otherwise hide the SDK's copies.
- The `#import`s of Office 2000 OLB files and `vbscript.tlb` now use
  `libid:` registry lookups (`ExcelInterfaceReal.h`, `common\WordAutomationObject.h`, `common\RegEx.h`).
- Boost.Test no longer links itself automatically, so the project names its
  library explicitly. `boost_test_wchar_shim.cpp` covers a gap caused by Felix's
  non-native `wchar_t` (`/Zc:wchar_t-`).
- `Felix.rc` includes `winres.h` instead of MFC's `afxres.h`.
- cpptempl is compiled through `Felix\cpptempl_build.cpp` so it can use the precompiled header.
- The Armadillo/SoftwarePassport copy-protection step was removed from the Release build.
- The unused Lua startup code was removed from Felix and Word Assist.
- Source files converted from Shift-JIS to UTF-8 with a BOM, and Felix compiles with `/utf-8` (see "Source file encoding").
- Test fixes for newer Boost/compiler: `BOOST_MESSAGE` → `BOOST_TEST_MESSAGE`,
  an `int`→`short` fill in `array_test.cpp`, and `GetVersionEx` replaced by
  `IsWindowsVistaOrGreater()`.

## Not done yet

- `Unicode_Release` hasn't been built.
- The 14 remote-memory tests are skipped unless the fake COM server in `python_tools\MemoryEngineFake.py` (Python 2 + pywin32) is registered. See section 5.
- The Word/Excel/PowerPoint add-ins and their test projects are still on VS 2013 settings.
  (Word Assist's Lua has been removed already.)
- Excel export needs Office installed; Word Assist will need Word's type library too.
- Felix still uses the non-standard `/Zc:wchar_t-`. Switching to a built-in
  `wchar_t` is the long-term fix, but it touches all string and COM code.
