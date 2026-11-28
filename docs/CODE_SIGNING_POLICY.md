<!--
DRAFT for a future SignPath Foundation application. Not linked from anywhere.
Releases are currently unsigned.
-->

# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io), certificate by [SignPath Foundation](https://signpath.org)
**(pending approval: not in effect until SignPath Foundation accepts the project)**

This is a draft. Felix's releases are **not code-signed today**. If SignPath
Foundation accepts the project, releases built as described below will be
Authenticode-signed so that Windows can confirm where they came from and that
they haven't been changed, and Windows will show **SignPath Foundation** as the
publisher.

## Roles

| Role | Who |
|------|-----|
| Author (committer) | [Ryan Ginstrom (@ginstrom)](https://github.com/ginstrom) |
| Reviewer | [Ryan Ginstrom (@ginstrom)](https://github.com/ginstrom) |
| Approver | [Ryan Ginstrom (@ginstrom)](https://github.com/ginstrom) |

Authors may change the source code. Reviewers review changes. Approvers
approve each signing request. Everyone in these roles uses multi-factor
authentication for GitHub and SignPath.

## What will be signed

Only Felix's own code, built from this repository:

- `Felix.exe`
- `WordAssist.dll`, `ExcelAssist.dll` and `PowerPointAssist.dll` (the 64-bit Office add-ins that the installer ships)
- `lang\EngResource.dll` and `lang\JpnResource.dll` (Felix's English and Japanese user interface)
- the installer, `Felix_Setup_<version>.exe`

The 32-bit Office add-ins are built by CI as well, but are not distributed
at the moment. If they are added to a release, they will be signed too.

## What will not be signed, and why

| File | Why |
|------|-----|
| `SciLexer.dll` | Third-party: the [Scintilla](https://www.scintilla.org/) editing component (version 2.01, by Neil Hodgson), shipped as provided. Migrating to a current Scintilla build is tracked in [ginstrom/felix-cat#44](https://github.com/ginstrom/felix-cat/issues/44). |
| `vc_redist.x86.exe`, `vc_redist.x64.exe` | Third-party: Microsoft's Visual C++ runtime installers, already signed by Microsoft. The installer runs them and deletes them afterwards. |
| `unins000.exe` (the uninstaller) | Inno Setup writes the uninstaller inside its compiler, where SignPath can't be called. The installer that contains it will be signed. |
| `Felix.tlb`, HTML templates, license files | Not executable code. |

Other third-party open source code compiled into Felix (WTL, Boost, cpptempl
and others in this repository) becomes part of Felix's own binaries and is
covered by their signatures. It keeps its own license.

**Smart App Control.** Smart App Control checks every executable file, not
just downloads. Even after signing, unsigned files such as `SciLexer.dll` and
the uninstaller could still be blocked, unless Microsoft's service recognizes
them. Signing the uninstaller needs a certificate path that can sign through
Inno Setup's `SignTool` (Inno's `SignedUninstaller` setting). Signing alone may
therefore not be enough to get past Smart App Control.

## How releases are built

- Releases are built only from tagged commits (`v<version>`) on this public
  repository, by GitHub Actions
  ([`release.yml`](../.github/workflows/release.yml)), on a GitHub-hosted
  runner. Nothing built on a personal computer is signed.
- Felix's program files and then the installer are submitted for signing.
  Each signing request is approved by hand.
- The result goes into a draft GitHub release, which the maintainer reviews
  before publishing.

## Privacy

Felix doesn't collect usage data, has no telemetry and doesn't connect to the
internet on its own.

It opens web pages only when you ask. Help and FAQ in Felix, and the help
buttons in the add-ins, open the online manual (`ginstrom.github.io/felix-cat`)
in your browser.

Remote memories and glossaries are handled by a separately installed component
(`Felix.RemoteMemory`), not part of Felix's release. If you add one, the
address and any user name and password you enter go to that component, which
connects to the server you named.

Felix displays entries with Windows' built-in HTML engine. If an entry or a
template you customize refers to a web address (for example an image), Windows
may load it.

The Office add-ins talk only to Felix on the same computer.

Your memories, glossaries and settings stay on your computer.

[To be used only if the update checker (branch `feature/cpp-update-checker`)
merges: "Felix can check for new versions. It asks first; if you agree, about
every two weeks (and when you choose Help > Check Updates) it requests
`https://api.github.com/repos/ginstrom/felix-cat/releases/latest`. GitHub
receives your IP address and Felix's version number (in the User-Agent,
`Felix-update-check/<VERSION>`); nothing else is sent. The answer is stored in
`HKCU\Software\AssistantSuite\Felix\UPDATE_CHECK`."]
