# winget manifest for Felix

This folder holds the draft [Windows Package Manager](https://learn.microsoft.com/windows/package-manager/)
manifest for Felix, in the multi-file format that the community repository
[microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs) requires
(ManifestVersion 1.10.0):

    packaging/winget/manifests/g/Ginstrom/Felix/<version>/
        Ginstrom.Felix.yaml                  version file
        Ginstrom.Felix.installer.yaml        installer URL, hash, silent switches
        Ginstrom.Felix.locale.en-US.yaml     default locale (en-US)
        Ginstrom.Felix.locale.ja-JP.yaml     Japanese description

The package identifier is `Ginstrom.Felix`. **The identifier is permanent once
it is submitted to winget-pkgs**, so be sure before you submit.

Nothing here is submitted automatically, and no workflow touches winget.
Submitting is a manual step for the maintainer.

**The manifest cannot be submitted yet.** `InstallerSha256` is a placeholder
(64 zeros) because the hash is only known once the release exists. Replace it
(step 2 below) before you validate for submission.

The Japanese text in `Ginstrom.Felix.locale.ja-JP.yaml` is a short draft. Have
a Japanese speaker check the wording before submitting.

## Submitting a version

1. **Publish the release first.** Push the version tag, let the release
   workflow create the draft release, and publish it. The installer URL in the
   manifest must be publicly downloadable.

2. **Set the installer hash.** Open the published release's `SHA256SUMS.txt`
   and copy the 64-character hash on the `Felix_Setup_<version>.exe` line.
   Paste it, in uppercase or lowercase, as `InstallerSha256` in
   `Ginstrom.Felix.installer.yaml`. To compute it yourself instead, download
   the installer and run either of these in PowerShell, and check that it
   matches `SHA256SUMS.txt`:

       Get-FileHash .\Felix_Setup_<version>.exe -Algorithm SHA256
       winget hash .\Felix_Setup_<version>.exe

   For versions after the first one has been merged into winget-pkgs,
   `wingetcreate update Ginstrom.Felix --version <version> --urls <installer URL>`
   computes the hash for you. It only works for a package that already exists
   in winget-pkgs, so it cannot be used for the first submission.

3. **For a later version**, copy the newest version folder to a new folder
   named for the new version, then in all four files set `PackageVersion`. In
   the installer file also set `InstallerUrl` (tag and file name) and
   `InstallerSha256`. In the en-US file set `ReleaseNotesUrl`. The
   `ProductCode` stays the same as long as `AppId` in `setup/FelixSetup.iss`
   does not change.

4. **Validate** from the repository root:

       winget validate --manifest packaging\winget\manifests\g\Ginstrom\Felix\<version>

   It should print `Manifest validation succeeded.`

5. **Test the install yourself** (winget cannot be tested before the release
   exists, and this is why it is a manual step). Local manifest installs are
   disabled by default. Test in a VM that has winget, not on your main PC.
   Windows Sandbox has no winget by default; to use it, run winget-pkgs'
   `Tools\SandboxTest.ps1` (from a clone of your fork), which sets up winget in
   the sandbox and installs from a manifest folder. In the VM, open
   PowerShell **as administrator** and enable local manifests:

       winget settings --enable LocalManifestFiles

   Then install from the manifest folder (it downloads the real installer and
   checks the hash):

       winget install --manifest packaging\winget\manifests\g\Ginstrom\Felix\<version>

   Launch Felix, check that it works, then remove it:

       winget uninstall --id Ginstrom.Felix

   When you are done, turn the setting off again (also as administrator):

       winget settings --disable LocalManifestFiles

6. **Submit by pull request.** Fork `microsoft/winget-pkgs`, copy the version
   folder to the same path in your fork
   (`manifests/g/Ginstrom/Felix/<version>/`), commit, and open a pull request.
   When the bot asks, sign the Microsoft Contributor License Agreement. Then
   answer anything the validation pipeline or the moderators raise on the
   pull request. The automated checks
   may flag a new unsigned installer (Defender or SmartScreen), so a moderator
   may need to review it.

7. **Alternative: `wingetcreate`.** Instead of forking by hand you can use
   [`wingetcreate`](https://github.com/microsoft/winget-create):

       wingetcreate submit packaging\winget\manifests\g\Ginstrom\Felix\<version>

   It needs a GitHub personal access token that you provide yourself, and it
   opens the pull request for you.
