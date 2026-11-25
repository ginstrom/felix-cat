<!-- Draft notes for the v3.0.1 GitHub release. Publish once the installer is
     signed: tools\build-installer.ps1, then
     gh release create v3.0.1 setup\Output\Felix_Setup_3.0.1.exe --title "Felix 3.0.1" --notes-file setup\release-notes-3.0.1.md
     (delete this comment and fill in the SHA-256 first). -->
Felix 3.0.1 is a maintenance release of [Felix 3.0](https://github.com/ginstrom/felix-cat/releases/tag/v3.0). The installer is now **code-signed**.

## Download and install

Download **Felix_Setup_3.0.1.exe** below and run it. It needs:

- 64-bit Windows 10 or 11, and administrator rights to install;
- 64-bit Microsoft Office (Microsoft 365, Office 2021 or 2024) for the Word, Excel and PowerPoint add-ins.

**Upgrading from 3.0:** run the new installer; it updates Felix in place, keeping your settings and add-in choices.

**Upgrading from 2.0:** see the [3.0 release notes](https://github.com/ginstrom/felix-cat/releases/tag/v3.0#upgrading-from-felix-20). Uninstall 2.0 first, after backing up `%LOCALAPPDATA%\Felix` if you customized it.

## What's new since 3.0

- **The user manual is back online**, in [English](https://ginstrom.github.io/felix-cat/manual/en/) and [Japanese](https://ginstrom.github.io/felix-cat/manual/jp/).
- **Help links work again.** Felix's Help menu, the FAQ link and the Word, Excel and PowerPoint add-ins' help open the new online manual; they used to point at felix-cat.com, which is offline. The Japanese interface now opens the Japanese manual.
- **The installer and Felix's programs are code-signed**, so Windows shows the publisher's name instead of "unknown publisher".

## Known issues

Unchanged from 3.0: **Help > Check for Updates** doesn't work (check the [releases page](https://github.com/ginstrom/felix-cat/releases) instead), the Excel and PowerPoint add-ins have had less hands-on testing than the Word add-in, and the installer includes only the 64-bit add-ins.

## Building from source

See [BUILDING.md](https://github.com/ginstrom/felix-cat/blob/v3.0.1/BUILDING.md).

SHA-256 of `Felix_Setup_3.0.1.exe`:
`<fill in>`
