# Design: Unsigned CI releases

## 1. Purpose and status

Felix can't get a code-signing certificate yet: SignPath Foundation turned the project down, and the Certum application is stalled. Releases will ship **unsigned** until then, and nothing is self-signed. Every release must be built by GitHub Actions from a tag and carry SHA-256 checksums and GitHub build provenance. The README must explain how to install despite the SmartScreen warning, and the pipeline must make a signing step easy to add later. The user's full spec is in [`docs/features/unsigned-releases.request.md`](../features/unsigned-releases.request.md).

PR [#16](https://github.com/ginstrom/felix-cat/pull/16) (merged as `807ccc0`) already built most of Phases 1 and 2, but its release workflow, README section and code signing policy assume SignPath signing. This design is a **gap analysis**: for each part of the spec, what exists, what is missing or contradicts it, and the exact change.

**Status:** ready for planning once the user answers the questions in section 8.1 (Q1–Q5). Each one has a recommendation, and the design is written around those recommendations, so accepting them needs no rework. None of the questions block Phases 3–5 or the docs. Q1 (Boost) and Q2 (SciLexer) only decide whether some existing work is redone.

## 2. Requirements and scope

### Requirements

IDs follow the spec's phases. **Exists** means PR #16 already satisfies the requirement; such requirements need only a check (section 7).

| ID | Requirement (from the spec) | State |
|---|---|---|
| R1.1 | `build.yml` runs on push and PR on `windows-latest` with `microsoft/setup-msbuild`, and builds Felix (x86) plus the three add-ins (x86 and x64) in Release | Exists |
| R1.2 | Office type libraries are committed as `.tlh`/`.tli` with relative includes, `#import` is replaced by `#include`, there's a README on regenerating them, and no `.olb`/`.dll` is committed | Exists |
| R1.3 | WTL 10 comes from the `wtl` NuGet package | Exists |
| R1.4 | Boost: compiled libraries come from a pinned source, are cached with `actions/cache`, and no project file hard-codes a path | Exists through b2, not vcpkg (Q1) |
| R1.5 | SciLexer.dll is built from pinned source, or the checked-in DLL is kept and the user is told | Checked-in DLL kept (Q2) |
| R2.1 | Inno Setup is installed with `choco install innosetup -y` | Exists |
| R2.2 | The `.iss` takes `/DAppVersion=` and `/DSourceDir=` and has no absolute paths | Partly: the `VcRedist` default is an absolute path (G2) |
| R2.3 | `SignTool`/`SignedUninstaller` are removed or disabled | Disabled (`#ifdef SIGN`); keep (section 5.3) |
| R2.4 | The VC++ runtimes come from a known source, downloaded from Microsoft in CI if they're downloaded by hand today | Exists: taken from the runner's Visual Studio, not downloaded by hand |
| R2.5 | The installer is uploaded as a workflow artifact | Exists |
| R3.1 | `release.yml` runs on `v*` tags, builds, and builds the installer with the tag's version | Exists |
| R3.2 | `SHA256SUMS.txt` covers the installer | **Missing** |
| R3.3 | A build provenance attestation is made for the installer (`id-token: write`, `attestations: write`) | **Missing** |
| R3.4 | A **draft** release is created with the installer and `SHA256SUMS.txt`, and its notes include the README's verification commands | Partly: draft exists, but no checksums or verification text |
| R3.5 | A disabled, clearly marked signing placeholder is gated on `vars.SIGNING_ENABLED == 'true'`, in the order sign program files → build installer → sign installer | **Contradicts:** live SignPath steps gated on `vars.SIGNPATH_ORGANIZATION_ID` |
| R4.1 | An "Installing Felix" section covers why there's a warning, SmartScreen (with a screenshot placeholder), Unblock, verifying the download, Smart App Control, Office add-in policy and re-enabling, and code signing status | **Missing.** The README claims "Later releases are signed" and has a SignPath "Code signing" section |
| R5.1 | A winget manifest under `packaging/winget/` points at the GitHub release URL and uses the Inno silent switches | **Missing** |
| R5.2 | The manifest is checked with `winget validate` and tested with `winget install --manifest`; the user gets the steps to submit it | **Missing** (section 7 covers what can be tested before a release) |
| R6.1 | Every network connection Felix makes, and what it sends, is listed | Exists in the root `CODE_SIGNING_POLICY.md`; re-verified and extended (section 3.6) |
| R6.2 | `docs/CODE_SIGNING_POLICY.md` is drafted and linked from nowhere, with roles, what is and isn't signed, the privacy statement, and SignPath's line marked pending approval | **Missing.** The root `CODE_SIGNING_POLICY.md` claims signing and is linked from the README |
| R7 | The PR description reports the inventory, decisions, everything that needs the user, and open questions | For the PR step; section 3 supplies the inventory |

### Quality constraints

- Don't merge, push tags, publish releases, add secrets, change repository settings or submit anything to external services. Where one of these is needed, write down exactly what the user must do (section 8.2).
- Ask before committing any binary or any file whose license is unclear. This design adds no binaries. The SmartScreen screenshot, when it exists, is a binary the user supplies.
- Pin action majors as the existing workflows do (`@v7`, `@v6`, `@v3`).
- Docs make no claims beyond what Microsoft documents (sources are in section 10).

### Non-goals

- Any actual signing, SignPath or Certum setup, or self-signing.
- Changing what the installer ships. It keeps shipping only the 64-bit add-ins; the x86 add-ins are built in CI but not shipped.
- Upgrading Scintilla or switching Boost to vcpkg, unless the user chooses that in Q1 or Q2.
- Merging `feature/cpp-update-checker`. The privacy statement only covers what it would add (section 3.6).

### Acceptance criteria

- **A1 (R1, R2):** The Build workflow passes on the PR. Its artifacts contain `Felix_Setup_<AppVersion>.exe`, the binaries for both platforms, and (new, section 5.4) `SHA256SUMS.txt` plus the rendered release notes.
- **A2 (R2.2):** Compiling `setup\FelixSetup.iss` without `/DVcRedist` fails with a clear `#error`, and `tools\build-installer.ps1` still builds the installer locally and in CI.
- **A3 (R3):** `release.yml` defines no SignPath step and no `SIGNPATH_*` variable. Its signing steps are gated on `vars.SIGNING_ENABLED == 'true'` and appear in the order sign program files → build installer → sign installer → verify. With the variable unset, a tag run produces a draft release named `Felix X.Y.Z (unsigned)` with two assets, the installer and `SHA256SUMS.txt`. Its notes contain the `Get-FileHash` and `gh attestation verify` commands, and `gh attestation verify` succeeds on the downloaded installer. The tag run itself can only happen when the user pushes a tag. Before that, the `workflow_dispatch` dry run (section 5.4) covers everything except creating the release.
- **A4 (R4):** The README's "Installing Felix" section covers every bullet of Phase 4. It has no claim that releases are signed, and every Windows or Office path in it matches a source in section 10.
- **A5 (R5):** `winget validate --manifest packaging\winget\manifests\g\Ginstrom\Felix\<version>` prints "Manifest validation succeeded". `packaging/winget/README.md` lists the submission steps.
- **A6 (R6):** `docs/CODE_SIGNING_POLICY.md` exists, nothing links to it, and it contains SignPath's required line marked pending approval. The root `CODE_SIGNING_POLICY.md` and the README's SignPath section are gone.
- **A7 (Phase 1–2 "done when"):** The CI installer installs and runs on the clean VM, with and without the add-ins. The add-ins load in Office on the user's machine. Both need people (section 8.2).

## 3. Current system (inspected at `807ccc0`)

### 3.1 Build (`.github/workflows/build.yml`, `.github/actions/setup-felix-deps/action.yml`)

- **Triggers:** `push` (all branches), `pull_request` and `workflow_dispatch`. Runs on `windows-latest`, which today resolves to the `windows-2025-vs2026` image (run [46119431328](https://github.com/ginstrom/felix-cat/actions/runs/46119431328): "Image: windows-2025-vs2026"; the image includes Visual Studio Enterprise 2026 18.12, InnoSetup 6.7.1 and 7-Zip).
- **Steps:** `actions/checkout@v7`, the composite action `setup-felix-deps`, `microsoft/setup-msbuild@v3`, and `choco install innosetup -y --no-progress`. The log of that run shows `InnoSetup v6.7.1 already installed.`, so the step is a no-op on today's image but keeps working if the image drops Inno Setup. Then `tools\build-installer.ps1 -Unsigned -AllPlatforms -NoRegister`, and uploads `felix-binaries-<sha>` and `felix-installer-<sha>` with `actions/upload-artifact@v7`.
- **Platforms:** `-AllPlatforms` builds the add-ins for Release|Win32 as well as Release|x64. Felix and the language DLLs are Unicode_Release|Win32.
- **setup-msbuild:** `build-installer.ps1` finds MSBuild through `vswhere` itself, so `setup-msbuild` has no effect on the build. It satisfies the spec's wording and is harmless; keep it.
- **WTL:** NuGet `wtl` 10.0.10320, downloaded from nuget.org and cached (`wtl-nuget-10.0.10320`).
- **Boost:** `boost_1_91_0.7z` from archives.boost.io, checked against a pinned SHA-256 (`9446c37f…`). b2 builds static Filesystem, Test and Regex for the shared CRT, 32- and 64-bit, into `lib32-msvc-14.5` and `lib64-msvc-14.5`. The result is cached (`boost-1.91.0-msvc-14.5-fs-test-regex-b1`, about 20 MB). A cold run takes about 6.5 minutes (07:03:46 → 07:10:04 in that run). The cache saved on `main` is readable from tag runs, because GitHub's cache scoping lets any ref read the default branch's caches.
- **MSBuild properties:** each `.vcxproj` imports `Dependencies.props`, which reads `Dependencies.local.props` (git-ignored) or environment variables (`BoostDir`, `WtlDir`), with defaults under `third_party\`. This is the spec's `Directory.Build.props` idea under another name, with an explicit import. **Nothing in the project files hard-codes a path.** `rg '[A-Za-z]:\\|Program Files'` over `*.vcxproj`, `*.props`, `*.targets`, `*.iss`, `*.ps1` and `*.yml` finds only:
  - `setup/FelixSetup.iss:31`, the `VcRedist` default `C:\Program Files\Microsoft Visual Studio\18\Community\…` (G2)
  - `setup/HTML FILE SETUP.iss`, an obsolete 2009-era script with six absolute paths that nothing references (G3)
  - `C:\deps\…` in the composite action (CI-only working folders)
  - comments and registry paths in `tools/*.ps1`

  There is also a `_DEBUG`-only `D:\Program Files (x86)\…\CheckUpdates.exe` in `Felix/MemoryWindowFrame.cpp:4485`. The update-checker branch removes it.

### 3.2 Import inventory (re-verified from PR #16)

`git grep '#import'` outside `common/office_typelibs`:

| `#import` | Where | Handling |
|---|---|---|
| `Felix.tlb` | `WordAssist/WordAssist/WordController.h`, `ExcelAssist/ExcelAssist/ta_application.h`, `PowerPointAssist/PowerPointAssist/PowerPointInterface.h` | Felix's own; MIDL builds it first, and `FelixTlbDir` in `Dependencies.props` points to it |
| `mshtml.tlb` | `Felix/stdafx.h:270` | Part of Windows; present on the runner |
| `libid:3F4DACA7-…` (VBScript RegExp) | `common/RegEx.h:15` | Part of Windows; present on the runner |
| MSO, VBE6EXT, MSWORD, EXCEL, MSPPT, MSADDNDR | formerly in `common\WordAutomationObject.h`, `Felix\ExcelInterfaceReal.h`, `ExcelAssist\…\ExcelImport.h`, `PowerPointAssist\…\PowerPointImports.h`, and the add-ins' `stdafx.h` | Replaced by `#include` of `common/office_typelibs/{msoffice_word,msoffice_excel,powerpoint,addin_designer}.h`. One set serves x86 and x64 (see that folder's `README.md`), with relative `.tli` includes. `tools/regenerate-office-typelibs.ps1` regenerates them. `.gitignore` only re-includes `*.tlh`/`*.tli` there; no `.olb` or `.dll` is committed |

### 3.3 Installer (`setup/FelixSetup.iss`, `tools/build-installer.ps1`)

- `AppVersion` (default `3.0.1`), `SourceDir` (default `..`, relative to the `.iss`) and `VcRedist` can all be overridden with `#ifndef`/`/D`.
- `SignTool=felixsign` and `SignedUninstaller=yes` sit inside `#ifdef SIGN`. Only `build-installer.ps1` defines `SIGN`, and only when `signing.local.json` exists and `-Unsigned` isn't passed. CI always passes `-Unsigned`.
- **VC++ runtimes:** `build-installer.ps1` finds `vc_redist.x86.exe` and `vc_redist.x64.exe` under `<VS>\VC\Redist\MSVC\` (preferring `v145`). In CI that is `C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Redist\MSVC\v145` (log of run 46119431328). These are Microsoft-signed redistributables installed with Visual Studio, not files downloaded by hand, so the spec's "download them in CI from Microsoft" condition doesn't apply. The installer runs them in `PrepareToInstall` (fix `d5f8e53`).
- The installer ships `Felix.exe`, `Felix.tlb`, the language DLLs, `SciLexer.dll`, the HTML templates, the licenses and the **x64** add-ins. `AppId` is `{98802423-6787-4122-BCEC-08CC2E12F18E}`, so the uninstall key is `{98802423-6787-4122-BCEC-08CC2E12F18E}_is1`. The installer needs admin rights and x64, and requires Windows 10 or later. `ShowLanguageDialog=yes`; PR #16's clean-VM runs used `/VERYSILENT /SUPPRESSMSGBOXES /NORESTART` successfully.

### 3.4 Release (`.github/workflows/release.yml`)

On `v*` tags, the workflow:

1. checks that the tag is `vX.Y.Z` and equals `AppVersion` in the `.iss`
2. builds the binaries (x64 add-ins only; no `-AllPlatforms`)
3. stages Felix's own files and uploads them, then signs them through **SignPath** if `vars.SIGNPATH_ORGANIZATION_ID` is set, and copies the signed files back
4. builds the installer with `-SkipBuild -AppVersion`, uploads it, and signs it through **SignPath**
5. runs `signtool verify /pa` on every file, including a silent test install
6. runs `gh release create --draft --verify-tag`, with notes from `setup/release-notes-<v>.md` and the title suffix "(UNSIGNED)" when unsigned

The job has `contents: write` and `actions: read` (for SignPath) and a 240-minute timeout (for approval waits). There are **no checksums and no attestation.**

### 3.5 Documentation

- **`README.rst`, "Installing":** the download link, requirements, and "Releases up to 3.0.1 aren't code-signed … **Later releases are signed**". The "Building" section says CI "builds signed releases from version tags". The "Code signing" section has SignPath's line and "Windows shows **SignPath Foundation** as the publisher", and links to `CODE_SIGNING_POLICY.md`.
- **Root `CODE_SIGNING_POLICY.md`:** written as if signing were in force (roles, signed files, not-signed table, process, privacy).
- **`BUILDING.md` §"Releases from GitHub Actions":** describes the SignPath flow and `SIGNPATH_ORGANIZATION_ID`. "Signing locally" (`signing.local.json`, `tools/sign-file.ps1`) supports a real certificate, Azure Artifact Signing or a custom command. Nothing in the repo creates a self-signed certificate (`git grep -i 'SelfSigned|makecert'` finds nothing).
- **`setup/release-notes-3.0.1.md`:** the draft notes for the first CI release (no `v3.0.1` tag or release exists yet; `gh release list` shows only `Felix 3.0`). They say "The installer is now **code-signed**" twice.
- **`.signpath/artifact-configurations/{program-files,installer}.xml`:** SignPath configurations to paste into SignPath.
- **`docs/`** is the GitHub Pages site (legacy build from `main:/docs`, with `.nojekyll`). It holds `index.html`, `manual/en` (refreshed for 3.0; `getting-started.html` covers download, install and uninstall with no SmartScreen note) and `manual/jp`. The Japanese `getting-started.html` is still the pre-3.0 text (trial version, license key, Control Panel). The design and feature folders under `docs/` are published too. Because of `.nojekyll`, a `docs/*.md` file is served as plain text at `https://ginstrom.github.io/felix-cat/<path>`. So `docs/CODE_SIGNING_POLICY.md` is "not linked", but it can still be reached on the web.
- The `manual/` folder at the root holds only `monitor_changes.py`.

### 3.6 Network connections (main at `807ccc0`)

Searched for `WinHttp`, `wininet`, `InternetOpen`, `URLDownloadToFile`, `XMLHTTP`, `ShellExecute`, `http(s)://`, `CLSIDFromProgID`/`CDispatchWrapper` ProgIDs and `socket`/`WSAStartup` across `Felix`, `common`, the three add-ins and the HTML templates. Felix links no HTTP library and opens no socket. Every way it can reach the network:

| # | What | Code | When | What is sent |
|---|---|---|---|---|
| N1 | **Automatic update check (dead)** | `MemoryWindowFrame::on_startup_checks` → `CDispatchWrapper(L"Felix.Utilities").method(L"CheckUpdates", langcode)` | Every launch after the first | Nothing. `Felix.Utilities` (the old Python tool) isn't installed, so `CLSIDFromProgID` fails and one line goes to `felix.log` |
| N2 | **Help > Check Updates (dead)** | `on_help_check_updates` → `ShellExecute(<app>\pytools\CheckUpdates.exe)` | When clicked | Nothing. The file isn't shipped, so an error message appears |
| N3 | **Help, FAQ** | `on_help`, `on_help_faq` → `ShellExecute(IDS_ONLINE_HELPFILE_URL / IDS_FAQ_PATH)` = `https://ginstrom.github.io/felix-cat/manual/{en,jp}/…` | When clicked | The default browser opens the page: an ordinary web request to GitHub Pages |
| N4 | **Add-in help buttons** | `WordController.cpp:1585`, `app_state.cpp:56`, `ExcelAssist/Connect.cpp:1426`, `PowerPointAssist/Connect.cpp:863` → `ShellExecute` of `…/manual/en/{5,6,7}.html` | When clicked | Same as N3 |
| N5 | **Remote memories and glossaries** | `memory_remote` → `CDispatchWrapper(L"Felix.RemoteMemory")`, `Connect(conn_str)` and `login(user, password)` | Only if the user adds a remote memory | The address, user name and password the user enters, and lookups, go to the separately installed `Felix.RemoteMemory` component, which talks to that server. Not shipped, so without it nothing connects |
| N6 | **Office add-ins ↔ Felix** | `Felix.App` COM (local server) | Always | Local cross-process COM only; no network |
| N7 | **HTML in entries (content-driven; inferred, not verified)** | Felix renders matches with MSHTML, inserting rich source and translation text (`get_source_rich()`) unescaped into its templates (`ManagerView.cpp:77` and the `html/` templates) | Only if a memory entry or a user-customized template contains a remote reference such as `<img src="https://…">` | MSHTML would fetch that URL as a browser would. The shipped templates reference nothing remote (`git grep` finds no `src`/`href` with `http` in `Felix/html`) |

The installer makes no network connections: the VC++ redistributables are the offline packages from Visual Studio.

**If `feature/cpp-update-checker` merges** (`git diff origin/main...feature/cpp-update-checker`: 11 files, +734/−39, new `Felix/update_check.{h,cpp}`), N1 and N2 are replaced by:

| # | What | When | What is sent |
|---|---|---|---|
| N8 | `updates::fetch_latest_release` (WinHTTP, `WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY`, TLS) sends `GET https://api.github.com/repos/ginstrom/felix-cat/releases/latest` with `User-Agent: Felix-update-check/<VERSION>` and `Accept: application/vnd.github+json`. No cookies, no body, no identifier. Timeouts are 10/10/10/15 s and the response is capped at 1 MB | Felix asks once, on the second launch ("Should Felix check online for new versions automatically? … about every two weeks"). If the user says yes, it checks every 14 days (`CHECK_INTERVAL_DAYS`). Help > Check Updates also checks on demand. The answer is stored in `HKCU\Software\AssistantSuite\Felix\UPDATE_CHECK` | GitHub sees the user's IP address, the time, and Felix's version (in the User-Agent). The response is only parsed for `tag_name`. If a newer version exists and the user agrees, the release page opens in the browser (as in N3) |

## 4. Proposed behavior

### 4.1 Maintainer release flow (after this change)

1. Bump the version (BUILDING.md "Version number"), write `setup/release-notes-<v>.md` (optional), merge, then push tag `v<v>`.
2. `release.yml` builds the binaries (Felix x86, add-ins x86 and x64). The signing placeholders are skipped, the installer is built with the tag's version, and `SHA256SUMS.txt` is generated. A provenance attestation is made for the installer, and both files are uploaded as a workflow artifact.
3. The workflow creates draft release `Felix <v> (unsigned)` with the two assets. The notes are the release notes file followed by a generated "Installing and verifying" section.
4. The maintainer reviews and publishes the draft.

If `vars.SIGNING_ENABLED` is `'true'` before a real signing step has been added, the first placeholder **fails the run** with a message saying where to add signing. A release is never silently labeled as signed when it isn't.

### 4.2 User install flow (what the README documents)

Download the installer and optionally verify it (`Get-FileHash` against `SHA256SUMS.txt`; `gh attestation verify`). Run it. SmartScreen shows "Windows protected your PC": choose **More info**, then **Run anyway**. Alternatively, clear the download's "Unblock" box under Properties before running it. If Smart App Control is **On**, Windows blocks the installer with no override: wait for a signed release. In Office, the add-ins load by default. Where an organization requires signed add-ins, they stay disabled. Re-enable add-ins Office has disabled through Disabled Items, and turn them on under COM Add-ins.

### 4.3 Edge cases

- **Tag doesn't match `AppVersion`, or isn't `vX.Y.Z`:** fails at the first step, as today.
- **Re-running a tag run after the draft exists:** `gh release create` fails. Delete the draft and re-run, as today; this goes in BUILDING.md. A re-run adds a second attestation for the same digest, which is harmless.
- **`setup/release-notes-<v>.md` missing:** the notes are a one-line title plus the generated section.
- **Draft release assets aren't publicly downloadable,** so winget and `gh attestation verify` by the public work only after the draft is published. The attestation itself exists from the moment the workflow runs.

## 5. Technical design

### 5.1 Gap summary

| Gap | Change | Files (proposed) |
|---|---|---|
| G1 | Replace the SignPath steps with gated placeholders; add checksums, attestation, assets and notes; build all platforms; add a dry-run dispatch | `.github/workflows/release.yml` |
| G2 | Make `VcRedist` required (no absolute default) | `setup/FelixSetup.iss` |
| G3 | Delete the obsolete installer script | `setup/HTML FILE SETUP.iss` |
| G4 | Add a script that writes `SHA256SUMS.txt` and the release notes; run it in `build.yml` too | `tools/make-release-files.ps1` (new), `.github/release-notes-install.md` (new), `.github/workflows/build.yml` |
| G5 | Add an "Installing Felix" README section; remove the SignPath claims | `README.rst` |
| G6 | Move the signing policy to a draft | delete `CODE_SIGNING_POLICY.md`; new `docs/CODE_SIGNING_POLICY.md` |
| G7 | Release docs | `BUILDING.md`, `setup/release-notes-3.0.1.md`, `docs/manual/en/getting-started.html` |
| G8 | Mark the SignPath configs as not in use | `.signpath/artifact-configurations/*.xml` (header comments), `.signpath/README.md` (new, 5 lines) |
| G9 | winget manifest, drafted | `packaging/winget/…` (new) |

Nothing changes in `Dependencies.props`, the project files, `setup-felix-deps`, `tools/build-installer.ps1` or `tools/sign-file.ps1` (unless the user chooses vcpkg or a Scintilla build in Q1/Q2; section 8.1).

### 5.2 `release.yml` (G1)

The new step list. Every `if:` is shown; steps without one always run.

```yaml
# Release build: a tag like v3.0.2 builds Felix and the add-ins, builds the
# installer, writes SHA256SUMS.txt, attests build provenance, and attaches
# both files to a DRAFT GitHub release for the maintainer to publish.
#
# Releases are UNSIGNED for now. The three "SIGNING PLACEHOLDER" steps mark
# where signing goes, in the order it must run: sign Felix's program files,
# build the installer from them, sign the installer. They run only when the
# repository variable SIGNING_ENABLED is 'true', and until a real signer is
# added they fail the run on purpose. What will be signed:
# docs/CODE_SIGNING_POLICY.md.
name: Release
on:
  push:
    tags: ['v*']
  workflow_dispatch:        # dry run: everything except creating the release
permissions:
  contents: read
concurrency: { group: release-${{ github.ref }}, cancel-in-progress: false }
env:
  SIGNING_ENABLED: ${{ vars.SIGNING_ENABLED == 'true' }}
  # True only for tag pushes: dispatch runs never create a release.
  PUBLISH: ${{ github.event_name == 'push' && startsWith(github.ref, 'refs/tags/v') }}
jobs:
  release:
    runs-on: windows-latest
    timeout-minutes: 90
    permissions:
      contents: write       # create the draft release
      id-token: write       # attestation: Sigstore certificate via OIDC
      attestations: write   # attestation: store it on GitHub
    steps:
      - checkout@v7
      - Version: on a tag, the existing tag-vs-AppVersion check. On dispatch,
        read AppVersion from FelixSetup.iss. Either way, set FELIX_VERSION.
      - setup-felix-deps; setup-msbuild@v3; choco install innosetup -y --no-progress
      - Build binaries: build-installer.ps1 -Unsigned -AllPlatforms -NoRegister -SkipInstaller
      - name: Sign program files (SIGNING PLACEHOLDER)
        if: env.SIGNING_ENABLED == 'true'
        # Stage Felix.exe, lang\*.dll and the three x64 add-in DLLs (list kept
        # from PR #16), sign them, and copy them back before the installer is built.
        run: throw 'SIGNING_ENABLED is true, but no signing step has been added here yet.'
      - Build installer: build-installer.ps1 -Unsigned -SkipBuild -AppVersion $env:FELIX_VERSION
      - name: Sign installer (SIGNING PLACEHOLDER)
        if: env.SIGNING_ENABLED == 'true'
        run: throw 'SIGNING_ENABLED is true, but no signing step has been added here yet.'
      - name: Verify signatures
        if: env.SIGNING_ENABLED == 'true'
        # The existing signtool verify /pa step, kept as is: it works with any
        # Authenticode signer, including the silent-install check.
      - Release files: tools\make-release-files.ps1 -Version $env:FELIX_VERSION -Signed:($env:SIGNING_ENABLED -eq 'true')
        # writes setup\Output\SHA256SUMS.txt and setup\Output\release-notes.md
      - uses: actions/attest@v4            # or attest-build-provenance@v4, see Q3
        with:
          subject-path: setup/Output/Felix_Setup_${{ env.FELIX_VERSION }}.exe
      - upload-artifact@v7: felix-<v>-release = installer + SHA256SUMS.txt + release-notes.md
      - name: Create draft release
        if: env.PUBLISH == 'true'
        run: gh release create <tag> <installer> setup\Output\SHA256SUMS.txt --draft --verify-tag --title <title> --notes-file setup\Output\release-notes.md
```

Contract details:

- **Title:** `Felix <v>`, plus ` (unsigned)` when `SIGNING_ENABLED` isn't `'true'`. This keeps the current "(UNSIGNED)" behavior; lower case matches the README wording.
- **Removed:** the `SIGN`, `SIGNPATH_PROJECT_SLUG` and `SIGNPATH_SIGNING_POLICY_SLUG` env entries, both `signpath/github-action-submit-signing-request@v2` steps, the program-files and installer "unsigned" artifact uploads that only existed to hand files to SignPath, "Put signed program files in place", and `actions: read`.
- **The placeholders stay in place rather than as comments,** so a future signer replaces the body of two steps and changes nothing else. That signer could be SignPath (whose action reads uploaded artifacts, so it would re-add an upload and `actions: read`), Certum through `tools/sign-file.ps1`, or Azure Artifact Signing.
- **The `-AllPlatforms` build** makes the release build the same set as `build.yml`. It adds only the Win32 add-in builds to the run time; nothing more ships.
- **Attestation:** `actions/attest@v4` is the current major. Its README says `actions/attest-build-provenance` v4 "is simply a wrapper on top of `actions/attest`" and that "new implementations should use `actions/attest` instead". The spec names attest-build-provenance (Q3). The README also lists `artifact-metadata: write`, but that permission is "necessary to create the artifact storage record", and storage records are only created with `push-to-registry` (container images). So it isn't needed for a file subject. Attestations work in public repositories on every plan; the repository is public.
- **`workflow_dispatch` dry run:** this lets the user test checksums, attestation and notes before tagging, from the Actions tab on `main` after merge. It never creates a release. Dispatch runs do create an attestation for a test build, which is harmless: it names the dispatch ref and run.

### 5.3 Installer script (G2, G3), and the `SignTool` decision

- **G2:** replace the `VcRedist` default with:

  ```
  #ifndef VcRedist
    #error Define VcRedist: the folder with vc_redist.x86.exe and vc_redist.x64.exe (tools\build-installer.ps1 does this)
  #endif
  ```

  The header comment's example command stays. `build-installer.ps1` always passes `/DVcRedist`, so builds through it don't change.
- **`SignTool`/`SignedUninstaller`: keep them behind `#ifdef SIGN`.** That satisfies "remove or disable" as *disabled*. CI passes `-Unsigned`, so `SIGN` is never defined there, and an unsigned build has no `SignTool` line at all. Removing them would break `BUILDING.md`'s "Signing locally" path, which is how a future Certum certificate (or Azure Artifact Signing) would sign the installer *and the uninstaller*. Inno Setup can only sign the uninstaller through `SignTool` during compilation (see section 6 on Smart App Control). Add one comment line: "CI never defines SIGN; releases are unsigned (docs/CODE_SIGNING_POLICY.md)."
- **G3:** delete `setup/HTML FILE SETUP.iss`. It's an obsolete script for a 2009 layout (`C:\dev\C++\Projects\Assistant Suite 1.6\…`, a Python MemoryEngine, an RTF EULA). Nothing references it, and it holds most of the repo's absolute paths. This is an assumption (A-3): the user can keep it.

### 5.4 Release files script (G4)

New file `tools/make-release-files.ps1 -Version <v> [-Signed] [-OutDir setup\Output]`:

- **Input:** `<OutDir>\Felix_Setup_<v>.exe`. Throws if it's missing.
- **`SHA256SUMS.txt`:** one line per file, `<lowercase hex sha256><two spaces><file name>`, LF line endings, UTF-8 without BOM. This is the GNU `sha256sum` format, so `sha256sum -c SHA256SUMS.txt` works on Linux/WSL, and `actions/attest`'s `subject-checksums` could read it if ever wanted. Today it holds one line (the installer); the script hashes every `Felix_Setup_<v>.exe` it's given.
- **`release-notes.md`:** the content of `setup\release-notes-<v>.md` if present, else `Felix <v>`. Then a blank line and `.github/release-notes-install.md` with `{{VERSION}}` replaced. When `-Signed` is passed, the "not code-signed yet" paragraph is left out; it is wrapped in `<!-- unsigned -->…<!-- /unsigned -->` markers.
- **`.github/release-notes-install.md`** (new). Its commands must stay identical to the README's (the PR reviewer checks this by eye; a CI grep would be overkill):

  ```markdown
  ## Installing and verifying

  <!-- unsigned -->
  This release is **not code-signed** yet, so Windows SmartScreen will say
  "Windows protected your PC". Choose **More info**, then **Run anyway**. On PCs
  with Smart App Control turned on, Windows blocks unsigned installers
  completely. See [Installing Felix](https://github.com/ginstrom/felix-cat#installing-felix).
  <!-- /unsigned -->

  To check that your download is the file GitHub Actions built from this
  release's tag, in PowerShell in your Downloads folder:

      Get-FileHash .\Felix_Setup_{{VERSION}}.exe -Algorithm SHA256

  The hash must match the line for the installer in `SHA256SUMS.txt`. With the
  [GitHub CLI](https://cli.github.com/), signed in:

      gh attestation verify .\Felix_Setup_{{VERSION}}.exe --repo ginstrom/felix-cat
  ```

- **`build.yml`:** add a step after "Build binaries and installer" that runs the script with the source `AppVersion`, so every CI run exercises it, and add `setup/Output/SHA256SUMS.txt` and `setup/Output/release-notes.md` to the installer artifact. No attestation in `build.yml`, so there's no `id-token` permission on PRs.

### 5.5 README "Installing Felix" (G5)

Replace the current "Installing" section with **"Installing Felix"** in `README.rst`, using the same anchor style. Recommendation: put it in the README rather than `docs/INSTALL.md`. The README is the repository's front page, which the download link and the release notes already point to. A `docs/*.md` file would be published as raw text on the Pages site (`.nojekyll`). Content and order:

1. **Download and requirements.** Keep the current text: the latest-release link, 64-bit Windows 10/11 with admin rights, 64-bit Office for the add-ins, what gets installed, the manual links, and "Upgrading from Felix 2.0".
2. **Why Windows warns you.** "Felix's releases aren't code-signed yet. Code signing lets Windows show who published a program; without it, Windows treats the installer as coming from an unknown publisher. We plan to sign releases (see Code signing status)." Each release is built by GitHub Actions from a tagged commit, with a checksum and a provenance attestation to verify.
3. **SmartScreen.**
   - Placeholder: `.. TODO: screenshot of the "Windows protected your PC" dialog (docs/images/smartscreen.png)`. RST comment syntax keeps it invisible until the image is added.
   - The text: "Windows protected your PC … Microsoft Defender SmartScreen prevented an unrecognized app from starting." Steps: **More info**, check that the app is `Felix_Setup_<version>.exe` (Publisher: Unknown publisher), then **Run anyway**.
   - Alternative: right-click the downloaded file → **Properties** → under *Security*, check **Unblock** → **OK**, then run it. The checkbox only appears on files downloaded from the internet.
   - Note: an organization's policy can remove **Run anyway**.
4. **Verifying the download.** The two commands from 5.4, with a sentence on what each proves. The checksum shows the file isn't corrupted or swapped, provided `SHA256SUMS.txt` came from the release page. The attestation shows GitHub Actions built the file from this repository. Optionally add `--signer-workflow ginstrom/felix-cat/.github/workflows/release.yml` for a stricter check. `gh` needs `gh auth login` first.
5. **Smart App Control (Windows 11).**
   - When it's **On**, Windows blocks apps it can't vouch for, including unsigned installers, and there's no "Run anyway" and no per-app exception.
   - How to tell: **Windows Security → App & browser control → Smart App Control settings** shows **On**, **Evaluation** or **Off**. Evaluation doesn't block. The Settings route is the same: Settings → Privacy & security → Windows Security → App & browser control.
   - If it blocked Felix, the notification and **Virus & threat protection → Protection history** say so.
   - "A signed release is expected to fix this", because Smart App Control lets software run when it's signed with a certificate from a CA in Microsoft's Trusted Root Program.
   - Turning Smart App Control off is possible but lowers protection for the whole PC, so we don't recommend it just for Felix. Recent Windows updates let you turn it back on without reinstalling.
6. **Office add-ins.**
   - Felix's add-ins are unsigned COM add-ins. Office loads them by default.
   - If your organization turns on **File → Options → Trust Center → Trust Center Settings → Add-ins → Require Application Add-ins to be signed by Trusted Publisher**, Office won't load them and the message bar says they were disabled. This usually comes from organization policy, so the option may be greyed out. Even after releases are signed, the organization would also have to trust the publisher.
   - **Re-enabling an add-in Office disabled** (for example after a crash): File → Options → Add-ins → **Manage: Disabled Items** → **Go** → select it → **Enable**. Then **Manage: COM Add-ins** → **Go** → tick **Felix Addin** (the description reads "Felix interface for Word/Excel/PowerPoint") → **OK**. Link to the manual's Troubleshooting page for more.
7. **Code signing status.** "Felix's releases are not code-signed yet. We intend to apply to the SignPath Foundation's free code signing program for open source projects; signed releases will show a verified publisher and won't be blocked by Smart App Control."

Also in `README.rst`: delete the "Code signing" section, and change "builds signed releases from version tags (`release.yml`)" to "builds draft releases from version tags (`release.yml`), with SHA-256 checksums and build provenance attestations".

`docs/manual/en/getting-started.html` §2.2 gets one paragraph: "Felix's installer isn't code-signed yet, so Windows may warn before running it. See [Installing Felix](https://github.com/ginstrom/felix-cat#installing-felix) for what to do and how to verify the download." The page is hand-edited Sphinx output, so edit the HTML directly. The Japanese manual is Q4.

### 5.6 Code signing policy (G6, G8)

- **Delete the root `CODE_SIGNING_POLICY.md`.** It describes signing that doesn't happen, uses SignPath's attribution without approval, and is linked from the README. git history keeps it.
- **New file `docs/CODE_SIGNING_POLICY.md`, linked from nowhere.** It starts with an HTML comment: "DRAFT for a future SignPath Foundation application. Not linked from anywhere. Releases are currently unsigned." Then:
  - **Required line:** `Free code signing provided by [SignPath.io](https://signpath.io), certificate by [SignPath Foundation](https://signpath.org)`, followed by `**(pending approval: not in effect until SignPath Foundation accepts the project)**`.
  - **Roles:** Ryan Ginstrom ([@ginstrom](https://github.com/ginstrom)) as author (committer), reviewer and approver, with MFA on GitHub and SignPath.
  - **What will be signed:** `Felix.exe`, the add-in DLLs (`WordAssist.dll`, `ExcelAssist.dll`, `PowerPointAssist.dll`; the 64-bit ones that ship), and the installer `Felix_Setup_<version>.exe`. Also `lang\EngResource.dll` and `lang\JpnResource.dll`, which are Felix's own code and which PR #16 already planned to sign. That goes one item beyond the spec's list (assumption A-4).
  - **What won't be signed, and why:**
    - `SciLexer.dll`: third-party, Scintilla 2.01, shipped as provided
    - `vc_redist.x86.exe`/`vc_redist.x64.exe`: Microsoft's, already Microsoft-signed
    - `unins000.exe`: Inno Setup writes it inside the compiler, where SignPath can't be called
    - `Felix.tlb`, templates and licenses: not executable

    Plus a note pointing to section 6's Smart App Control consequence.
  - **How releases are built:** tag → GitHub Actions on a GitHub-hosted runner → signing requests approved by hand → draft release reviewed before publishing. Nothing built on a personal computer is signed.
  - **Privacy statement:** the text below, followed by a bracketed paragraph for N8, to be used if the update checker merges.

    > Felix doesn't collect usage data, has no telemetry and doesn't connect to the internet on its own.
    >
    > It opens web pages only when you ask. Help and FAQ in Felix, and the help buttons in the add-ins, open the online manual (`ginstrom.github.io/felix-cat`) in your browser.
    >
    > Remote memories and glossaries are handled by a separately installed component (`Felix.RemoteMemory`), not part of Felix's release. If you add one, the address and any user name and password you enter go to that component, which connects to the server you named.
    >
    > Felix displays entries with Windows' built-in HTML engine. If an entry or a template you customize refers to a web address (for example an image), Windows may load it.
    >
    > The Office add-ins talk only to Felix on the same computer.
    >
    > Your memories, glossaries and settings stay on your computer.

    [If the update checker merges: "Felix can check for new versions. It asks first; if you agree, about every two weeks (and when you choose Help > Check Updates) it requests `https://api.github.com/repos/ginstrom/felix-cat/releases/latest`. GitHub receives your IP address and Felix's version number; nothing else is sent. You can turn this off …"]
- **`.signpath/`: keep it**, with a header line in each XML ("Not in use: releases are unsigned. Kept for a future SignPath application; see docs/CODE_SIGNING_POLICY.md") and a five-line `.signpath/README.md` saying the same. These configurations would be reused as they are on reapplication. Deleting them would only move them into git history.
- **`BUILDING.md`:** rewrite "Releases from GitHub Actions" to describe 4.1 (unsigned, `SHA256SUMS.txt`, attestation, draft, the dispatch dry run, the re-run edge case, `SIGNING_ENABLED` and its placeholders). Keep "Signing locally" unchanged, apart from one sentence: "Official releases are currently unsigned; don't sign them with a self-signed certificate, which gives users no benefit."
- **`setup/release-notes-3.0.1.md`:** remove both "code-signed" claims. Add a line under "Download and install" that points to the README's Installing Felix section. Remove the "SHA-256 of … :" placeholder at the end, since `SHA256SUMS.txt` replaces it. Fix the header comment, which says "Publish once the installer is signed" and gives a manual `gh release create`. It should say that `release.yml` uses this file.

### 5.7 winget (G9)

Proposed layout, mirroring winget-pkgs so the folder can be copied as it is:

```
packaging/winget/README.md
packaging/winget/manifests/g/Ginstrom/Felix/3.0.1/
    Ginstrom.Felix.yaml                  # ManifestType: version
    Ginstrom.Felix.installer.yaml        # ManifestType: installer
    Ginstrom.Felix.locale.en-US.yaml     # ManifestType: defaultLocale
    Ginstrom.Felix.locale.ja-JP.yaml     # ManifestType: locale (optional, recommended)
```

- **Format:** multi-file. The 1.28 schema docs say the singleton format "has been deprecated in the community repository". **`ManifestVersion: 1.10.0`**, with the matching `# yaml-language-server: $schema=https://aka.ms/winget-manifest.<type>.1.10.0.schema.json` header. That's what current winget-pkgs tooling writes (for example JRSoftware.InnoSetup 6.7.1's manifest), and it validates on today's clients. The newest schema folder in winget-pkgs is 1.28.0, but its README says "The Windows Package Manager 1.28 client does not support all fields in the 1.28 schema", and nothing here needs newer fields.
- **Installer manifest:**

  ```yaml
  PackageIdentifier: Ginstrom.Felix
  PackageVersion: 3.0.1
  MinimumOSVersion: 10.0.0.0
  InstallerType: inno
  Scope: machine
  ElevationRequirement: elevatesSelf
  InstallModes: [interactive, silent, silentWithProgress]
  InstallerSwitches:
    Silent: /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
    SilentWithProgress: /SILENT /SUPPRESSMSGBOXES /NORESTART
  UpgradeBehavior: install
  FileExtensions: [ftm, fgloss, fprefs]
  ProductCode: '{98802423-6787-4122-BCEC-08CC2E12F18E}_is1'
  AppsAndFeaturesEntries:
  - DisplayName: Felix
    Publisher: Ryan Ginstrom
    ProductCode: '{98802423-6787-4122-BCEC-08CC2E12F18E}_is1'
  Installers:
  - Architecture: x64
    InstallerUrl: https://github.com/ginstrom/felix-cat/releases/download/v3.0.1/Felix_Setup_3.0.1.exe
    InstallerSha256: <from SHA256SUMS.txt of the published release>
  ManifestType: installer
  ManifestVersion: 1.10.0
  ```

  `InstallerType: inno` already implies Inno's silent switches; they're written out because the spec asks for them. `UpgradeBehavior: install` matches the installer, which updates in place over the same `AppId`. `ElevationRequirement: elevatesSelf` matches `PrivilegesRequired=admin`.
- **Placeholder hash:** the committed `InstallerSha256` is a placeholder of 64 zeros with a comment, because the real value only exists once the release is built. `winget validate` accepts it (checked; section 7).
- **The `defaultLocale` manifest** has Publisher, PublisherUrl, PackageName `Felix`, PackageUrl, License `MIT`, LicenseUrl (`…/blob/main/LICENSE.txt`), a ShortDescription, Tags (`translation`, `translation-memory`, `cat`, `office-addin`) and a ReleaseNotesUrl for the release page.
- **`packaging/winget/README.md`** has the submission steps (section 8.2, item U6).

## 6. Consequences

- **Compatibility:** local builds don't change; `build-installer.ps1` always passes `VcRedist`. Running `ISCC setup\FelixSetup.iss` by hand without `/DVcRedist` now fails with a clear message instead of failing on a missing Community-edition path.
- **The update checker interacts with releases:** it reads `/releases/latest`, which ignores drafts, so a draft never prompts users. Tags must stay `vX.Y.Z`; `release.yml` already enforces that.
- **Security and provenance:** the attestation is signed through Sigstore (public-good instance, since the repository is public) and ties the installer's digest to `release.yml` at the tag's commit. The Boost and WTL caches restored into a tag run were made by runs on `main`, and tag runs can't read other branches' caches. Fork PRs can't write `main`'s cache. Building Boost fresh on every release (about +6.5 minutes) would remove even that dependency. Not proposed; mentioned for completeness.
- **Smart App Control limits what signing alone fixes. This matters for the spec's "a signed release will fix this".** Smart App Control checks *every* executable file, DLLs included, not just downloads. Microsoft: "Smart App Control signature checks apply to all executable files". It allows a file that Microsoft's app intelligence knows to be safe, or one signed by a Trusted Root Program CA. So in a future signed release:
  - the uninstaller would still be blocked unless signed, which needs `SignedUninstaller` (Certum or Azure path) or another Inno approach
  - **`SciLexer.dll` (unsigned) could still be blocked**, unless Microsoft's service recognizes that 2009 binary. Felix loads it at startup (`Felix/Felix.cpp:340`, `common/atlscintilla.h:60`), and its editors throw "You must load SciLexer.dll" without it.

  That is a reason to build Scintilla in CI before signing starts (Q2), so it can be signed too. Today it changes nothing, because unsigned releases are blocked by Smart App Control anyway. The README promises "expected to fix", and the draft policy lists the SciLexer caveat.
- **Office policy:** signing alone doesn't satisfy "Require Application Add-ins to be signed by Trusted Publisher". The publisher's certificate must also be in the PC's Trusted Publishers, usually pushed by IT. The README wording reflects this; it's a correction to the spec's phrasing.
- **Rollback:** every change is in files only. Reverting the PR restores the SignPath workflow.

## 7. Verification

| Check | How | Who/when |
|---|---|---|
| R1, R2, A1 | The Build workflow on the PR passes (`gh run watch`). Download the artifact; check that `SHA256SUMS.txt` matches `Get-FileHash` of the installer and that `release-notes.md` contains both commands with the version filled in | Engineer/QA, on the PR |
| A2 | Locally: `ISCC setup\FelixSetup.iss` without `/D` → `#error` text. `tools\build-installer.ps1 -Unsigned -NoRegister` → installer built | Engineer |
| `make-release-files.ps1` | Run it on a dummy `Felix_Setup_9.9.9.exe` in a temp folder: line format (regex `^[0-9a-f]{64}  Felix_Setup_9\.9\.9\.exe\n$`), no BOM, LF; notes with and without `-Signed`, with and without `setup\release-notes-9.9.9.md` | Engineer (no unit test framework for PowerShell in the repo; a scripted check in the PR) |
| A3, structure | Review `release.yml`: no `signpath` or `SIGNPATH` text (`rg -i signpath .github` finds nothing); placeholder order; permissions; `PUBLISH` gate | Reviewer |
| A3, live | After merge: **Actions → Release → Run workflow** on `main` (dry run) → artifact plus an attestation. Then `gh attestation verify <downloaded installer> --repo ginstrom/felix-cat` succeeds. The first real tag run produces the draft | **User** (dispatching a workflow or pushing a tag is the user's call) |
| A4 | Read `README.rst` rendered on the PR branch; check each Windows and Office path against section 10 | Reviewer; QA on the VM: Windows Security → App & browser control shows the Smart App Control state there |
| SmartScreen flow | On the VM "FelixTest": copy the CI installer in, add Mark of the Web (`Set-Content <file> -Stream Zone.Identifier -Value "[ZoneTransfer]`r`nZoneId=3"`), and run it from Explorer → "Windows protected your PC" → More info → Run anyway. Repeat with Properties → Unblock. This can also produce the screenshot | QA, if the VM's SmartScreen is on. Otherwise the user captures it |
| A5 | `winget validate --manifest packaging\winget\manifests\g\Ginstrom\Felix\3.0.1` | Engineer; already shown to work (below) |
| A6 | `rg CODE_SIGNING_POLICY` finds only `docs/CODE_SIGNING_POLICY.md` itself, workflow comments and designs; the README has no link | Reviewer |
| A7 | Clean VM install and run of the CI installer: full, and Felix only (PR #16's scripts `felix-test.cmd full|main`). Office add-ins loading on the user's machine | QA (VM); **user** (Office) |

**winget, checked on this machine:** `winget --version` → `v1.7.10661`. `winget validate --manifest <dir>` on a draft of the three files above printed "Manifest validation succeeded." for ManifestVersion 1.10.0, 1.7.0 and 1.6.0 (and 1.28.0). It rejected `9.9.9` ("Unsupported ManifestVersion") and a bad `InstallerType` ("Invalid field value. [InstallerType]"). So local validation works, although this old client checks newer schema versions against what it knows. `winget settings export` shows `"LocalManifestFiles": false`.

**What can't be tested before a release exists:** `winget install --manifest` needs:

1. the admin setting turned on: `winget settings --enable LocalManifestFiles`, run as administrator. That is the user's decision; nothing here changes it.
2. an `InstallerUrl` that downloads without authentication and matches `InstallerSha256`. CI artifacts and draft release assets both need sign-in.

So the full install test happens after the user publishes the first release. Before that, the parts winget would run can be tested directly: the silent switches (`Felix_Setup_<v>.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART` on the VM, as PR #16 did) and the uninstall entry (`HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{98802423-6787-4122-BCEC-08CC2E12F18E}_is1` exists after install, with DisplayVersion = the version).

## 8. Open questions, user actions, assumptions

### 8.1 Questions for the user

**Q1. Boost: keep b2, or switch to vcpkg manifest mode as the spec says?** Felix needs compiled Boost.Test (linked by name in Felix's Release and Testing builds), Filesystem and Regex (auto-linked).

- **A. Keep b2 (recommended).** It keeps the pinned 1.91.0 archive with its SHA-256 and the cache. It builds Boost's own versioned names (`libboost_*-vc145-mt-x32-1_91.lib`), which `Dependencies.props` (`BoostLibToolset`/`BoostLibVersion`/`BoostLibArch`) and Boost's auto-link expect. Local developers keep BUILDING.md's route (prebuilt installer or b2), and CI and local builds both go through `BoostDir`. It meets the spec's intent (pinned, cached, no hard-coded paths) but not its letter.
- **B. vcpkg manifest mode.** This needs:
  - a `vcpkg.json` (`boost-test`, `boost-filesystem`, `boost-regex`) with a `builtin-baseline`, plus an `overrides` entry to pin 1.91.0
  - the `x86-windows-static-md` and `x64-windows-static-md` triplets, to match the shared CRT with static Boost
  - `VcpkgEnableManifest` in the projects
  - removing the explicit `libboost_unit_test_framework…` link, since vcpkg's own library names and its MSBuild integration replace b2's names and auto-link
  - rewriting BUILDING.md §2/§6 so local developers use VS's bundled vcpkg (the first build compiles Boost, about 10+ minutes)
  - vcpkg binary caching in CI

  Leads to: a dependency route that matches the spec and is common in C++ projects, at the cost of touching every project file and every developer's setup, and of new risk to a build that passes today. Estimate: one plan step plus a local re-verification of all configurations, including Testing.

**Q2. SciLexer.dll: keep the checked-in DLL, or build Scintilla in CI?** Today the DLL is Scintilla 2.01 (2009), a PE32 x86 DLL (identical copies in `Felix/` and `PowerPointAssist/PowerPointAssist/`). The source is available: SourceForge `scintilla/2.01/scintilla201.zip` (1.1 MB, 2009-08-19), under Scintilla's permissive license, which requires the copyright notice in all copies. Felix uses lexer IDs (`SetLexer(SCLEX_HTML/XML/…)`), `SetStyleBits(7)` and a `SciLexer.dll` file name (`common/ScintillaEdit.cpp`, `common/atlscintilla.h`, `Felix/SourceAndHtmlEdit.cpp`).

- **A. Keep the DLL for now (recommended for this PR), and open an issue for B or C before signing starts.** No risk now. Unsigned releases are blocked by Smart App Control anyway, so SciLexer's lack of a signature only starts to matter once Felix is signed (section 6).
- **B. Build 2.01 from the pinned source archive in CI** (SHA-256 pinned, `nmake -f scintilla.mak` in `win32\`, x86). Same behavior, gains provenance, and can be signed later. Risk: 2009 code under the v145 compiler may need patches. This is unknown until tried, so budget an experiment of half a day or more.
- **C. Upgrade to Scintilla 5.x + Lexilla (current 5.6.9).** This is a real migration:
  - Lexers moved to Lexilla.
  - The Scintilla 5 migration guide describes `SCI_SETLEXER(SCLEX_*)` as the 4.x method; 5.x apps use Lexilla's `CreateLexer(name)` and `SCI_SETILEXER`.
  - The DLLs are now `Scintilla.dll` and `Lexilla.dll`.
  - `SetStyleBits` is gone.
  - Felix's `CScintillaIF` uses `long`-typed direct calls.

  It touches the editor wrapper, the HTML/source edit dialogs and their tests. A middle option, the last 4.x release (which still builds `SciLexer.dll` with lexer IDs), would be nearly a drop-in replacement but is still a behavior change that needs testing. Leads to: a separate feature.

**Q3. `actions/attest@v4` or `actions/attest-build-provenance@v4`?**

- **A. `actions/attest@v4` (recommended).** It's the upstream recommendation for new workflows; attest-build-provenance v4 is a wrapper around it. The output and `gh attestation verify` are the same.
- **B. `actions/attest-build-provenance@v4`,** as the spec names it. It works the same today but may be deprecated later.

**Q4. A Japanese version of the install guide?** Felix has a Japanese UI and manual. The Japanese `getting-started.html` is still the pre-3.0 text (trial version, license key, Control Panel), and Windows shows SmartScreen in Japanese.

- **A. English only in this PR; a follow-up issue to refresh the Japanese getting-started page with the install guide (recommended).** The Japanese UI labels (for example the localized "Windows protected your PC", "More info" and "Run anyway") should be taken from a Japanese Windows rather than guessed.
- **B. Add a Japanese section now** (README or `docs/manual/jp/getting-started.html`). Better for Japanese users at the first release, but the UI strings would be unverified unless the user checks them.
- **C. No Japanese version.**

**Q5. winget package identifier.** It's permanent once submitted.

- **A. `Ginstrom.Felix` (recommended).** It matches the GitHub owner and is short. Nothing in winget-pkgs uses `Ginstrom` today (`manifests/g` has no `Ginstrom` folder).
- **B. `RyanGinstrom.Felix`,** which matches the installer's `AppPublisher`.

### 8.2 What the user must do (for the PR description)

- **U1.** Review and merge the PR.
- **U2.** Optional: after merge, run **Actions → Release → Run workflow** on `main` (dry run) and check the attestation with `gh attestation verify`.
- **U3.** Push the first tag (`v3.0.1`, after checking `setup/release-notes-3.0.1.md`), review the draft release, and publish it.
- **U4.** Leave the repository variable `SIGNING_ENABLED` **unset** until a signing step is added. Setting it to `true` now fails releases on purpose. No secrets are needed.
- **U5.** Supply the SmartScreen screenshot (`docs/images/smartscreen.png`) if QA can't capture it on the VM. That's a binary file, the user's own.
- **U6.** winget, after the release is published:
  1. Take the installer's hash from `SHA256SUMS.txt` and set `InstallerSha256` (and `PackageVersion`, `InstallerUrl` and `ReleaseNotesUrl` for later versions).
  2. Run `winget validate --manifest packaging\winget\manifests\g\Ginstrom\Felix\<v>`.
  3. As admin, `winget settings --enable LocalManifestFiles`, then in Windows Sandbox or the VM: `winget install --manifest <dir>`, launch Felix, `winget uninstall --id Ginstrom.Felix` (or by ProductCode).
  4. Fork `microsoft/winget-pkgs` and copy `manifests/g/Ginstrom/Felix/<v>/` into the same path.
  5. Open a PR and sign the Microsoft CLA when the bot asks.
  6. Answer the validation pipeline.

  `wingetcreate submit` is an alternative to steps 4–5.
- **U7.** Test the add-ins in Office on your own machine with the CI-built installer (the VM has no Office).
- **U8.** Later, for SignPath: reapply with the repository and `docs/CODE_SIGNING_POLICY.md` (then link it from the README and remove "pending approval"), and replace the two placeholder steps.

### 8.3 Assumptions (non-blocking; override if wrong)

- **A-1.** The README, not `docs/INSTALL.md`, holds the install guide (reason in 5.5).
- **A-2.** `SignTool`/`SignedUninstaller` stay behind `#ifdef SIGN` (5.3).
- **A-3.** The obsolete `setup/HTML FILE SETUP.iss` is deleted.
- **A-4.** The draft policy also lists the two language DLLs as signed.
- **A-5.** `.signpath/` is kept, marked as not in use.
- **A-6.** `release.yml` gains a `workflow_dispatch` dry run and builds the add-ins for both platforms.
- **A-7.** `choco install innosetup` stays unpinned, as a no-op guard. Pinning `--version 6.7.1` would trade image drift for a maintenance chore.

## 9. Out of scope: problems noticed

- **Third-party license notices aren't shipped.** The installer copies only `LICENSE.txt` and `LICENSE_JP.txt`. Scintilla's license asks for its notice "in all copies", and cpptempl and the jQuery files under `html/` have their own notices. Recommend a follow-up that adds `THIRD_PARTY_NOTICES.txt` to the installer.
- **`PowerPointAssist` (x64) calls `LoadLibrary("SciLexer.dll")`** (`Connect.h:139`) but never uses the handle. Only a 32-bit `SciLexer.dll` is installed, so the load fails harmlessly. The constructor and the duplicate DLL in `PowerPointAssist/PowerPointAssist/` could go.
- **The `Dependencies.props` comment on `FelixNoExcel`** says it's for "a machine without Microsoft Office", which is out of date since the headers were committed. BUILDING.md already says so.
- **The VM test under the Japanese UI** (`ShowLanguageDialog=yes`) was only run silently. An interactive run on a Japanese Windows hasn't been done.
- **The orchestrator's memory note "SignPath signing status"** (application submitted 2026-11-28) is superseded by the request (SignPath declined).

## 10. Sources

- actions/attest README (v4; permissions; `subject-path`; public repositories; storage records): <https://github.com/actions/attest>
- actions/attest-build-provenance README (v4 is a wrapper; "new implementations should use `actions/attest`"): <https://github.com/actions/attest-build-provenance>
- `gh attestation verify` manual (`--repo`, `--signer-workflow`, `--bundle`): <https://cli.github.com/manual/gh_attestation_verify>
- Runner images (`windows-latest` → Windows Server 2025 / `windows-2025-vs2026`; InnoSetup 6.7.1; VS 2026 18.12): <https://github.com/actions/runner-images> and `images/windows/Windows2025-VS2026-Readme.md`
- SmartScreen reputation ("Windows protected your PC", "Run anyway", policy can block; self-signed is the same as unsigned; Smart App Control applies to all executables): <https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation>
- Smart App Control overview (evaluation and enforcement modes; On/Evaluation/Off under App & browser control; Trusted Root Program signatures allowed): <https://learn.microsoft.com/en-us/windows/apps/develop/smart-app-control/overview>
- Smart App Control FAQ (no per-app bypass; can be re-enabled without a clean install after recent updates; Protection history): <https://support.microsoft.com/windows/smart-app-control-faq-285ea03d-fa88-4d56-882e-6698afdb7003>
- "Smart App Control has blocked part of this app" (a DLL block breaks only the dependent part): <https://support.microsoft.com/en-US/Windows/Security/Threat-Malware-Protection/smart-app-control-has-blocked-part-of-this-app>
- Office add-ins and the Trust Center ("Require Application Add-ins to be signed by Trusted Publisher"; the Trust Bar notification; organization policy): <https://support.microsoft.com/en-us/office/view-manage-and-install-add-ins-in-microsoft-365-programs-16278816-1948-4028-91e5-76dca5380f8d>
- Re-enabling disabled add-ins (Manage: Disabled Items → Go → Enable; Manage: COM Add-ins → Go → check → OK): <https://learn.microsoft.com/en-us/visualstudio/vsto/how-to-re-enable-a-vsto-add-in-that-has-been-disabled>
- Inno Setup command line (`/SILENT`, `/VERYSILENT`, `/SUPPRESSMSGBOXES`, `/NORESTART`, `/LANG`): <https://jrsoftware.org/ishelp/topic_setupcmdline.htm>
- winget-pkgs schema folders (newest 1.28.0; singleton deprecated): <https://github.com/microsoft/winget-pkgs/tree/master/doc/manifest/schema>; an example 1.10.0 Inno manifest: `manifests/j/JRSoftware/InnoSetup/6.7.1/JRSoftware.InnoSetup.installer.yaml`
- Scintilla 2.01 source: <https://sourceforge.net/projects/scintilla/files/scintilla/2.01/>; license: <https://www.scintilla.org/License.txt>; Scintilla 5 migration: <https://scintilla.org/Scintilla5Migration.html>; current release: <https://www.scintilla.org/ScintillaDownload.html>
