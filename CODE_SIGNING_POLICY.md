# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io), certificate by [SignPath Foundation](https://signpath.org).

Felix's Windows releases are Authenticode-signed so that Windows can confirm
where they came from and that they haven't been changed. Under this program,
Windows shows **SignPath Foundation** as the publisher.

## Roles

| Role | Who |
|------|-----|
| Committers and reviewers | [Ryan Ginstrom (@ginstrom)](https://github.com/ginstrom) |
| Approvers | [Ryan Ginstrom (@ginstrom)](https://github.com/ginstrom) |

Committers and reviewers may change the source code. Approvers approve each
signing request. Everyone in these roles uses multi-factor authentication for
GitHub and SignPath.

## What is signed

Only Felix's own code, built from this repository:

- `Felix.exe`
- `lang\EngResource.dll` and `lang\JpnResource.dll` (Felix's English and Japanese user interface)
- `WordAssist.dll`, `ExcelAssist.dll` and `PowerPointAssist.dll` (the 64-bit Office add-ins that the installer ships)
- the installer, `Felix_Setup_<version>.exe`

The 32-bit Office add-ins are built by CI as well, but are not distributed
at the moment. If they are added to a release, they will be signed too.

## What is not signed with Felix's certificate

| File | Why |
|------|-----|
| `SciLexer.dll` | Third-party: the [Scintilla](https://www.scintilla.org/) editing component (version 2.01, by Neil Hodgson), shipped as provided. |
| `vc_redist.x86.exe`, `vc_redist.x64.exe` | Third-party: Microsoft's Visual C++ runtime installers, already signed by Microsoft. The installer runs them and deletes them afterwards. |
| `unins000.exe` (the uninstaller) | Inno Setup writes the uninstaller while it compiles the installer, and SignPath can't be called from inside that step. The installer that contains it is signed. |
| `Felix.tlb`, HTML templates, license files | Not executable code. |

Other third-party open source code compiled into Felix (WTL, Boost, cpptempl
and others in this repository) is covered by Felix's signature as part of
Felix's binaries. It keeps its own license.

## How releases are built and signed

- Releases are built only from tagged commits (`v<version>`) on this public
  repository, by GitHub Actions ([`release.yml`](.github/workflows/release.yml)),
  on a clean GitHub-hosted runner. Nothing built on a personal computer is signed.
- The build compiles Felix and its add-ins, submits Felix's own program files
  to SignPath, builds the installer from the signed files, and submits the
  installer.
- SignPath checks that each file came from a GitHub Actions run of this repository.
- An approver reviews and approves each signing request by hand.
- The workflow verifies every signature, then attaches the installer to a
  draft GitHub release, which the maintainer reviews before publishing.

## Privacy

Felix does not send information to other computers unless you ask it to:

- **No telemetry, no automatic update check.** Felix collects no usage data.
  (Felix 3's code still has an automatic update check from the original
  distribution, but it used a separate update tool that is no longer
  installed, so it does nothing; Help > Check for Updates doesn't work either.)
- **Help links.** Felix's Help menu and the add-ins' help buttons open the
  online manual (`ginstrom.github.io/felix-cat`) in your web browser, when
  you click them.
- **Remote memories and glossaries.** If you add a remote memory or glossary,
  Felix hands the address you enter to a separately installed remote-memory
  component (`Felix.RemoteMemory`), which connects to that server. That
  component is not part of Felix's release, so without it nothing connects.
- **Office.** The Word, Excel and PowerPoint add-ins talk to Felix running
  on the same computer. They don't connect to the internet.

Your memories, glossaries and settings stay on your computer
(`%LOCALAPPDATA%\Felix` and wherever you save your files).
