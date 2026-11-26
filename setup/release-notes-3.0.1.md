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
- **Word concordance searches work with a whole paragraph selected** ([#1](https://github.com/ginstrom/felix-cat/issues/1)). Find Concordance and Translation Concordance no longer come back empty when your selection includes the paragraph mark.
- **Felix shortcuts no longer fire inside dialogs** ([#2](https://github.com/ginstrom/felix-cat/issues/2)). In Excel, Word and PowerPoint, pressing Alt+N in the "save changes?" prompt (or any other dialog) no longer launches Felix.
- **Excel's formula bar updates right away** ([#3](https://github.com/ginstrom/felix-cat/issues/3)). After Get Translation or inserting a glossary entry, the formula bar shows the new text instead of the old. The same goes for Auto Translate.
- **Two-key shortcuts no longer type a stray letter.** If you press Alt+M, C (Find Concordance) with nothing selected, the "c" is no longer typed into your document.
- **PowerPoint concordance searches work with a whole paragraph selected**, as in Word.
- **Shortcuts no longer fail with a COM error when you type fast** ([#8](https://github.com/ginstrom/felix-cat/issues/8)). In Word, Excel and PowerPoint, Felix shortcuts (for example Look up Next and Previous Translation) are queued and run in order instead of sometimes failing with "illegal to call out while inside message filter".
- **Word's Alt+1 to Alt+9 glossary shortcuts work after switching back to Word from another window** ([#9](https://github.com/ginstrom/felix-cat/issues/9)). Alt+1 no longer opens the AutoSave prompt.
<!-- TODO: QA must confirm the #9 fix (Alt+1 after switching back to Word) before release; reword or drop this line if not confirmed. -->
- **Excel's formula bar follows Auto Translate** ([#10](https://github.com/ginstrom/felix-cat/issues/10)). The formula bar shows the translation for the active cell as soon as it is written, instead of the source text until the command ends.
- **PowerPoint's menu shows the right shortcuts** ([#11](https://github.com/ginstrom/felix-cat/issues/11)). Translate to Fuzzy shows Alt+C, and Find Concordance no longer advertises a shortcut.
- **A two-key shortcut for Toggle Shortcuts now works.** If you bound Toggle Shortcuts to a two-key shortcut, it didn't turn Felix shortcuts off or on. It does now. (The default Ctrl+Alt+F9 was not affected.)

## Known issues

Unchanged from 3.0: **Help > Check for Updates** doesn't work (check the [releases page](https://github.com/ginstrom/felix-cat/releases) instead), the Excel and PowerPoint add-ins have had less hands-on testing than the Word add-in, and the installer includes only the 64-bit add-ins.

## Building from source

See [BUILDING.md](https://github.com/ginstrom/felix-cat/blob/v3.0.1/BUILDING.md).

SHA-256 of `Felix_Setup_3.0.1.exe`:
`<fill in>`
