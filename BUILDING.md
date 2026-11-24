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

Scintilla's headers are already in `common\`. At runtime Felix loads
`SciLexer.dll`, which must sit next to `Felix.exe`.

**Lua is no longer needed.** Felix only created an unused Lua VM at startup,
so that code and the Lua, LuaBridge and LuaWrapper sources have been removed
from the project.

## 3. Build

1. Open `Felix\Felix.sln`. VS will offer to retarget the add-in and test
   projects. You can **cancel** that for now.
2. Choose the configuration `Unicode_Debug|Win32`. `Unicode_Release` treats
   warnings as errors. `Testing` builds the Boost.Test unit tests into the exe.
3. Right-click the **Felix** project and choose **Build**.

The build ends by running `Felix.exe /RegServer` to register its COM server.
That step writes to the machine-wide registry, so either run Visual Studio
**as Administrator** or ignore that one step failing while you work on compiling.

The pre-build step copies `Felix\html\` to `%LOCALAPPDATA%\Felix\html\`, where
the UI templates are loaded from at runtime.

## What changed from the original VS 2013 setup

- `PlatformToolset` v120 → v145, Windows SDK 10.0, in Felix and its two resource DLL projects.
- The hardcoded `D:\dev\cpp\...` include and library paths were replaced by `Dependencies.props`.
- The `#import`s of Office 2000 OLB files and `vbscript.tlb` now use
  `libid:` registry lookups (`ExcelInterfaceReal.h`, `common\WordAutomationObject.h`, `common\RegEx.h`).
- The Armadillo/SoftwarePassport copy-protection step was removed from the Release build.
- The dead Lua startup code was removed.

## Expected next problems

These aren't verified yet, because the project hasn't been compiled under VS 2026.

- Stricter conformance in the new compiler: `/permissive-` is the default for
  new projects but **not** for upgraded ones, so it stays off here.
- Newer Office type libraries can add names that clash with Windows macros.
  The fix is to add more `rename(...)` clauses to the `#import`.
- Boost removals since ~1.57, for example `boost/filesystem` v2-era calls.
