# Building Felix (Visual Studio 2026)

This branch retargets Felix from VS 2013 (`v120`) to VS 2026 (`v145`, MSVC 14.50):
the main **Felix** application (32-bit), and the **Word, Excel and PowerPoint
add-ins**, which now also build as 64-bit for 64-bit Office (section 6).

## 1. Install the toolchain

- **Visual Studio 2026** (Community is fine) with the
  **Desktop development with C++** workload, plus these individual components:
  - *C++ ATL for latest v145 build tools (x86 & x64)*
  - a Windows 10/11 SDK (installed by the workload)
- **Microsoft Office is not needed to build.** The C++ headers generated from
  Office's type libraries are committed in `common\office_typelibs\` (see its
  `README.md`, including how to regenerate them). You still need Office to
  run the add-ins and their tests.

## 2. Put the libraries in `third_party\`

By default, `Dependencies.props` looks in `third_party\` at the repo root. To use
other locations, copy `Dependencies.local.props.example` to
`Dependencies.local.props` and edit it. That file is git-ignored.

| Library  | Put it at | Notes |
|----------|-----------|-------|
| Boost    | `third_party\boost\` (folder containing `boost\version.hpp`) | Felix needs compiled **Boost.Test, Filesystem and Regex** for 32-bit. The easiest route is the prebuilt installer `boost_1_91_0-msvc-14.5-32.exe`, which creates `lib32-msvc-14.5\`. The 64-bit add-ins need 64-bit libs in `lib64-msvc-14.5\` (see section 6). |
| WTL      | `third_party\wtl\Include\` | WTL 10 (SourceForge or the `wtl` NuGet package). |
| cpptempl | `third_party\cpptempl\` | **Already included in the repo.** Felix's HTML template engine, pinned to the last wide-string version (commit 4c5980c). See `third_party\cpptempl\SOURCE.txt`. |

> **Boost and long paths:** the Boost installer fails with "MoveFile failed; code 3"
> if its folder path is long, because some doc file names exceed the Windows
> 260-character limit. Install it to a short path such as `C:\local\boost_1_91_0`
> and set `BoostDir` in `Dependencies.local.props`. (Leave `BoostLibDir` unset:
> `Dependencies.props` picks `lib32-msvc-14.5` or `lib64-msvc-14.5` per platform.)

Scintilla's headers are already in `common\`. The build copies `Felix\SciLexer.dll`
next to `Felix.exe`.

`BoostDir` and `WtlDir` can also come from environment variables of the same
name (MSBuild reads them as properties); the GitHub Actions build does that
(`.github\actions\setup-felix-deps`).

`<FelixNoExcel>true</FelixNoExcel>` in `Dependencies.local.props` still builds
Felix without Excel export (it shows a "not available" message instead), but
it's no longer needed for building without Office.

**Lua is no longer needed.** Lua was never actually used: Felix and Word Assist
only created an unused Lua VM at startup. That code, and the Lua, LuaBridge and
LuaWrapper sources, have been removed from both projects.

## 3. Build

1. Open `Felix\Felix.sln`. (The `.sln` files in the add-in folders are
   obsolete VS 2005/2008 leftovers; use this one.)
2. Choose the configuration: `Unicode_Debug` for development,
   `Unicode_Release` for an optimized build, or `Testing`, which builds the
   Boost.Test unit tests (Felix's are compiled into the exe; Felix's is the
   only build that treats all warnings as errors).
3. Choose the platform: **x64** for 64-bit Office, **Win32** for 32-bit Office.
   It only affects the add-ins; Felix itself is always built 32-bit.
4. Build the solution. Felix and its resource DLLs build first, then the add-ins.

Or from a Developer PowerShell:

```powershell
msbuild "Felix\EngResource\Felix English.vcxproj" /p:Configuration=Unicode_Debug /p:Platform=Win32
msbuild "Felix\JpnResource\Felix Japanese.vcxproj" /p:Configuration=Unicode_Debug /p:Platform=Win32
msbuild Felix\Felix.vcxproj /p:Configuration=Unicode_Debug /p:Platform=Win32 /m
```

For Release, use `/p:Configuration=Unicode_Release` in all three commands. The
output goes to `Felix\Unicode_Release\`.

Every configuration copies `SciLexer.dll` and the UI templates (`Felix\html\`)
next to the exe. Felix looks for each template in `%LOCALAPPDATA%\Felix\html\`
first, where users can customize them, then falls back to the copy next to the
exe. The Debug build also refreshes the `%LOCALAPPDATA%` copy.

## 4. Run

Run `Felix\Unicode_Debug\Felix.exe` (or `Felix\Unicode_Release\Felix.exe`). It needs, next to it:

- `SciLexer.dll` (copied by the build)
- `lang\EngResource.dll` and `lang\JpnResource.dll` (from the two resource projects).
  Without them Felix stops with the assertion `localized_instance != 0`.

**COM registration.** Felix is also a COM server, which the Word, Excel and
PowerPoint add-ins use. Registering it writes to the machine-wide registry, so
it needs admin rights, so the build doesn't do it. Register with
`tools\register-felix.ps1` (section 6), which asks for admin rights, or run
`Felix.exe /RegServer` from an Administrator terminal. Debug builds also try to
re-register on every launch; without admin rights that fails and is only logged.

Felix's log is `%LOCALAPPDATA%\Felix\logs\felix.log`. It's rewritten on every
launch, and the `Testing` build writes to the same file.

## 5. Unit tests

The `Testing` configuration compiles Felix's ~2300 Boost.Test unit tests into
`Felix\Testing\Felix.exe`. Running it executes the tests and exits; it shows no window.

1. Build the two resource DLLs and Felix in `Unicode_Debug` once (section 3).
   The Testing build copies `SciLexer.dll`, plus `lang\EngResource.dll` and
   `lang\JpnResource.dll` from `Felix\Unicode_Debug\lang\`, next to its exe.
2. Build `Testing|Win32`.
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

## 6. Office add-ins (Word Assist, Excel Assist, PowerPoint Assist)

The add-ins are classic COM add-ins (`IDTExtensibility2`, toolbar buttons that
Office 2007 and later shows on the ribbon's **Add-ins** tab). Current desktop
Office for Windows (Microsoft 365, Office 2024) still loads them. They run
inside Office, so **their bitness must match Office's**: build **x64** for
64-bit Office (the default for current Office) and **Win32** for 32-bit Office.
They talk to Felix.exe across processes, so Felix stays 32-bit either way.

**Office type libraries.** The add-ins use the Office type libraries (MSO, VBA
extensibility, Word, Excel, PowerPoint, Add-in Designer) through the headers
in `common\office_typelibs\`, generated from 64-bit Microsoft 365 (Office
16.0), so Office isn't needed to build them. They also `#import "Felix.tlb"`,
which the Felix build produces (`FelixTlbDir` in `Dependencies.props`).

**64-bit Boost.** The x64 builds need Boost.Filesystem and Boost.Test built for
x64 in `<BoostDir>\lib64-msvc-14.5\`. Either install the prebuilt
`boost_1_91_0-msvc-14.5-64.exe`, or build just those two from the Boost source
in a "x64 Native Tools" prompt:

```bat
cd C:\local\boost_1_91_0
set NoDefaultCurrentDirectoryInExePath=
bootstrap.bat vc145
b2 -j8 toolset=msvc-14.5 address-model=64 link=static runtime-link=static,shared variant=debug,release threading=multi --with-filesystem --with-test --build-dir=C:\local\b2tmp64 --stagedir=stage64 stage
move stage64\lib lib64-msvc-14.5
```

(`set NoDefaultCurrentDirectoryInExePath=` matters only if that variable is set
in your environment; it stops `cmd` finding Boost's own batch files.)

**Registration.** Office finds the add-ins through
`HKLM\Software\Microsoft\Office\<App>\Addins`, and the add-ins find Felix
through its COM registration. Both need an Administrator prompt. Each add-in
tries to register itself after it builds; without admin rights the build
finishes with the warning `FELIXREG: Could not register ...`. To register
Felix and all three add-ins in one go, run the following. If the terminal
isn't elevated, the script asks Windows for administrator rights (a UAC
prompt), waits, and shows the result in the same window:

```powershell
powershell -ExecutionPolicy Bypass -File tools\register-felix.ps1 -Configuration Debug
```

`-Configuration` picks what Office and the add-ins should use: `Debug` or
`Release` (this repo's builds; the default is `Release`), or `Installed` (the
installed Felix in `C:\Program Files\Felix`, or `-InstallDir`), which switches
back after testing without reinstalling. Add `-Platform Win32` for 32-bit
Office, or `-Unregister` to remove a build's registration. Close Office first.

To see what is registered now, without admin rights:

```powershell
powershell -ExecutionPolicy Bypass -File tools\register-felix.ps1 -Status
```

It shows where Felix and each add-in point (repo Debug, repo Release,
installed, or other). It warns about per-user registrations that override
the machine-wide ones, and about a Felix add-in that Office has disabled.

In Word, Excel or PowerPoint, the add-in appears as **Felix Addin** (File > Options > Add-ins, "COM Add-ins"), and its buttons on the
**Add-ins** ribbon tab.

**An older installed Felix** (e.g. 2.0 under `C:\Program Files (x86)\Assistant
Suite\Felix\`) registers the same `Felix.App` class, so the add-ins would start
that Felix instead. Uninstall it *before* running the registration script: its
uninstaller doesn't unregister `Felix.App`, and it deletes
`%LOCALAPPDATA%\Felix`, which the new Felix shares (HTML templates, logs,
preferences). Afterwards, restore the templates by building Debug or with
`xcopy /E /I /Y Felix\html "%LOCALAPPDATA%\Felix\html\"`. To check what's
registered, run the script with `-Status`.

**Tests.** `Test_WordAssist`, `Test_Excel` and `Test_PPT` are console apps
built in the `Testing` configuration; each runs its tests after building.
Latest run, 64-bit Debug: Word Assist 134/134, PowerPoint Assist 65/65,
Excel Assist 49/49. Run one by hand with, for example:

```powershell
Test_Excel\x64\Debug\Test_Excel.exe --report_level=short
```

The post-build run uses `--result_code=no`, so a failing test doesn't fail the
build; read its output, or run the exe by hand.

What the migration changed:

- Toolset v145 and SDK 10.0; x64 configurations next to Win32; `Dependencies.props`
  for WTL, Boost and `common\` (the projects used to rely on per-user VS
  include folders). Word Assist's empty `Release_64`, `Template` and
  `Testing_64` configurations were removed.
- Hardcoded `D:\dev\...` / `D:\Development\...` `#import` paths for the Office 2000
  type libraries and `MSADDNDR.DLL` now use `libid:` lookups, as in Felix.
- 64-bit fixes in shared code: `CFile::operator HFILE()` truncated the handle
  (removed; unused), and Word Assist's clipboard backup stored `GlobalSize` in an `int`.
- The add-ins exempt the same newer-compiler warnings as Felix's `Testing`
  build, but not the 64-bit truncation warnings, which stay errors.
- The test programs install a logger (`common\test_logging_fixture.h`); without
  one, any code that logged tripped an assertion. Their Release configurations
  now define `UNIT_TEST` (they had no tests before).
- `Felix.sln` was regenerated: `Unicode_Debug`, `Unicode_Release` and `Testing`,
  each for Win32 and x64, with the add-ins depending on Felix.

## 7. Installer and releases

The installer is an [Inno Setup 6](https://jrsoftware.org/isinfo.php) script,
`setup\FelixSetup.iss`. To build the Release binaries and compile it in one go
(close Felix and Office first):

```powershell
winget install --id JRSoftware.InnoSetup --source winget
powershell -ExecutionPolicy Bypass -File tools\build-installer.ps1
```

The result is `setup\Output\Felix_Setup_<version>.exe`. It requires 64-bit
Windows 10 or later, asks for admin rights, and:

- installs Felix (32-bit) to `C:\Program Files\Felix`, with its language DLLs,
  HTML templates and `Felix.tlb`;
- installs the Word, Excel and PowerPoint add-ins (64-bit, each optional) and
  registers them with Office;
- installs the Visual C++ runtimes (x86 for Felix, x64 for the add-ins), taken
  from the local Visual Studio (`tools\build-installer.ps1` finds them and
  passes `/DVcRedist=<folder>`; or use `-VcRedist <folder>`);
- registers Felix's COM server (`Felix.exe /RegServer`), the `.ftm`, `.fgloss`
  and `.fprefs` file types, and a Start menu shortcut.

Uninstalling removes all of that, but leaves each user's
`%LOCALAPPDATA%\Felix` (settings, customized templates, logs) and their
memories and glossaries alone. (The 2015 installer's uninstaller deleted
`%LOCALAPPDATA%\Felix`.)

**Releases from GitHub Actions.** Official releases are built and signed by
`.github\workflows\release.yml`, not on a personal machine. Bump the version
(below), merge, then push a tag such as `v3.0.2`. The workflow builds
everything on a clean runner, sends Felix's own program files to SignPath for
signing (the maintainer approves the request in SignPath), builds the
installer from the signed files, sends the installer for signing, checks all
signatures with `signtool verify /pa`, and attaches the installer to a
**draft** release. Review it, then publish. Until SignPath is set up (the
repository variable `SIGNPATH_ORGANIZATION_ID` is empty), it skips signing
and makes an unsigned draft. What is signed, and why: `CODE_SIGNING_POLICY.md`.
The SignPath artifact configurations are in `.signpath\artifact-configurations\`.
`.github\workflows\build.yml` builds the binaries (add-ins for both
platforms) and an unsigned installer on every push and pull request.

**Signing locally.** Unsigned installers make Windows SmartScreen warn about an
"unknown publisher". To sign a build yourself, get a code-signing certificate (an
OV/individual certificate from a certificate authority, or Microsoft's Azure
Artifact Signing service), then copy `tools\signing.example.json` to
`signing.local.json` at the repo root and fill in one method:

- `certificate`: a certificate in your Windows certificate store, by
  thumbprint. This also covers hardware tokens and cloud signing services
  that expose the certificate through Windows. List yours with
  `Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert`.
- `artifact-signing`: Azure Artifact Signing (formerly Trusted Signing), via
  its `signtool` plug-in DLL and a metadata JSON file (endpoint, account,
  certificate profile).
- `command`: any other signing tool; `{file}` becomes the quoted file path.

`signing.local.json` is git-ignored. When it exists, `tools\build-installer.ps1`
signs `Felix.exe`, the language DLLs and the add-ins (not `SciLexer.dll`,
which is Scintilla's), and Inno Setup signs the installer and the
uninstaller, all through `tools\sign-file.ps1` with SHA-256 and an RFC 3161
timestamp. Each signature is verified, and Inno Setup refuses to build if a
file comes back unsigned. Use `-Unsigned` to build without signing, or
`-SigningConfig <file>` to use settings stored elsewhere. Signing shows your
name as the publisher right away; SmartScreen's warning fades as signed
releases build up download reputation.

**Version number.** It's set in `Felix/stdafx.h` (`VERSION`, shown in the
status bar and written into saved memories), the `VERSIONINFO` blocks of
`Felix/Felix.rc`, `Felix/FelixENG.rc`, `Felix/FelixJPN.rc` and the three
add-ins' `AddIn.rc`, and `AppVersion` in `setup\FelixSetup.iss`.

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

- `Unicode_Release` builds and runs, but with about 220 compiler warnings
  (mostly the newer-compiler warnings that `Testing` exempts). Worth cleaning up.
- The 14 remote-memory tests are skipped unless the fake COM server in `python_tools\MemoryEngineFake.py` (Python 2 + pywin32) is registered. See section 5.
- The Word add-in has been tried in Word (lookup works end to end); the Excel
  and PowerPoint add-ins load-tested only through their test programs so far.
- Help > Check for Updates, and Felix's automatic update check at startup,
  relied on Python tools that aren't part of this repo. The automatic check
  fails quietly (one line in `felix.log`); the menu item doesn't work.
- The installer only offers the 64-bit add-ins; it could also offer the Win32
  ones for 32-bit Office.
- The Win32 add-in builds (for 32-bit Office) haven't been built since the
  migration; with 64-bit Office installed they can't be loaded here anyway.
- Felix still uses the non-standard `/Zc:wchar_t-`. Switching to a built-in
  `wchar_t` is the long-term fix, but it touches all string and COM code.
