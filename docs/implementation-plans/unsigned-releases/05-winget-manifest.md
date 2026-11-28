# Step 05: winget manifest

Plan: [index.md](index.md). Design: [../../designs/unsigned-releases.md](../../designs/unsigned-releases.md) sections 5.7, 7 (winget), 8.2 U6 (gap G9).

## Objective
Draft a winget manifest for Felix 3.0.1 under `packaging/winget/`, validate it locally, and document how the user submits it.

## Requirements and context
- R5.1: manifest under `packaging/winget/` pointing at the GitHub release URL and using the Inno silent switches. R5.2 / A5: `winget validate` passes locally; `packaging/winget/README.md` lists the submission steps.
- Decided: identifier `Ginstrom.Felix` (Q5). Folder `packaging/winget/manifests/g/Ginstrom/Felix/3.0.1/`, ManifestVersion `1.10.0`, multi-file format (the singleton format is deprecated in the community repository).
- **Do not run `winget settings --enable LocalManifestFiles` or change any winget setting.** `winget install --manifest` can't be tested before a public release exists (needs that setting and a publicly downloadable installer matching the hash); it is the user's step U6.
- Local client is winget v1.7.10661. It validated 1.10.0 manifests of this shape in the design's trial; it rejects unsupported versions (e.g. 9.9.9) and bad enum values.

## Dependencies and prerequisites
None. Facts used (from the repo): installer `Felix_Setup_<v>.exe`, Inno `AppId` `{98802423-6787-4122-BCEC-08CC2E12F18E}` so the ProductCode is `{98802423-6787-4122-BCEC-08CC2E12F18E}_is1`; requires admin and 64-bit Windows 10 or later; license MIT (`LICENSE.txt`); repo `https://github.com/ginstrom/felix-cat`. Confirm the file extensions (`ftm`, `fgloss`, `fprefs`) and `AppPublisher` against `setup/FelixSetup.iss`; fix the manifest if the script says otherwise.

## Scope and implementation guidance
Create (each file starts with `# yaml-language-server: $schema=https://aka.ms/winget-manifest.<type>.1.10.0.schema.json`, `<type>` being `version`, `installer`, `defaultLocale`, `locale`):
- `Ginstrom.Felix.yaml` (`ManifestType: version`, `PackageIdentifier`, `PackageVersion: 3.0.1`, `DefaultLocale: en-US`).
- `Ginstrom.Felix.installer.yaml`:
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
    InstallerSha256: 0000000000000000000000000000000000000000000000000000000000000000
  ManifestType: installer
  ManifestVersion: 1.10.0
  ```
  with a comment above the hash: placeholder, replace with the installer's hash from the published release's `SHA256SUMS.txt` before submitting.
- `Ginstrom.Felix.locale.en-US.yaml` (`ManifestType: defaultLocale`): Publisher, PublisherUrl, PackageName `Felix`, PackageUrl, License `MIT`, LicenseUrl (`https://github.com/ginstrom/felix-cat/blob/main/LICENSE.txt`), ShortDescription (translation memory for Windows with Word, Excel and PowerPoint add-ins; take wording from README), Tags (`translation`, `translation-memory`, `cat`, `office-addin`), ReleaseNotesUrl (`.../releases/tag/v3.0.1`).
- `Ginstrom.Felix.locale.ja-JP.yaml` (`ManifestType: locale`): optional but included; short Japanese description. Keep it minimal and mark in the README that wording should be checked by a Japanese speaker. If validation of the ja-JP file causes trouble on the old client, report it instead of dropping it silently.
- `packaging/winget/README.md`: what the folder is; the U6 steps (set `InstallerSha256`, and for later versions `PackageVersion`, `InstallerUrl`, `ReleaseNotesUrl`, new version folder; `winget validate --manifest packaging\winget\manifests\g\Ginstrom\Felix\<v>`; as admin `winget settings --enable LocalManifestFiles`, then in Windows Sandbox or a VM `winget install --manifest <dir>`, launch Felix, `winget uninstall --id Ginstrom.Felix`; fork `microsoft/winget-pkgs` and copy the folder to the same path; open a PR and sign the Microsoft CLA when the bot asks; answer the validation pipeline; `wingetcreate submit` as an alternative to forking). State that the identifier is permanent once submitted and that the placeholder hash makes the manifest unsubmittable until replaced.
- Exclusions: no workflow automation of winget; no submission.

## Verification
1. `winget validate --manifest packaging\winget\manifests\g\Ginstrom\Felix\3.0.1` prints "Manifest validation succeeded." (run from the repo root; quote the output).
2. Negative check (confirms validation is real): copy the folder to a temp dir, change `InstallerType: inno` to `InstallerType: bogus`, confirm validation fails; do not leave that copy in the repo.
3. Cross-check the silent switches against `setup/FelixSetup.iss` behavior (admin, `ShowLanguageDialog`) and the ProductCode against the `.iss` `AppId`.
4. `winget settings export` still shows `LocalManifestFiles` false (unchanged).
Not testable here: `winget install --manifest` (record this).

## Completion criteria
- [x] Five manifest/doc files exist at the paths above; validation passes.
- [x] Negative check done; settings unchanged.
- [x] Index status updated; one commit if the brief says so.

## Execution record

### Engineer record (2026-11-29)

Done: created `packaging/winget/manifests/g/Ginstrom/Felix/3.0.1/` with `Ginstrom.Felix.yaml`, `.installer.yaml` (as specified; placeholder hash with a comment inside the `Installers` entry), `.locale.en-US.yaml`, `.locale.ja-JP.yaml`, and `packaging/winget/README.md` (submission steps U6: hash from `SHA256SUMS.txt`, `Get-FileHash`/`winget hash`, `wingetcreate update`; validate; admin `winget settings --enable LocalManifestFiles` then `winget install --manifest` in a sandbox/VM and `winget uninstall --id Ginstrom.Felix`; fork + PR + CLA; `wingetcreate submit`; identifier permanence; placeholder makes it unsubmittable; ja-JP wording check).

Verification (worktree root, winget v1.7.10661):
1. `winget validate --manifest packaging\winget\manifests\g\Ginstrom\Felix\3.0.1` -> `Manifest validation succeeded.` (exit 0). The ja-JP file validated without trouble.
2. Negative: temp copy with `InstallerType: bogus` -> `Manifest validation failed.` / `Schema Error ... [InstallerType] Failed to match against any enum values` / `Invalid field value. [InstallerType]` (exit -1978335191). Temp copy deleted.
3. Cross-check against `setup/FelixSetup.iss`: `AppId={{98802423-6787-4122-BCEC-08CC2E12F18E}` (so ProductCode `{...}_is1`), `AppPublisher=Ryan Ginstrom`, `PrivilegesRequired=admin`, `MinVersion=10.0`, `ArchitecturesInstallIn64BitMode=x64compatible`, `ShowLanguageDialog=yes` (suppressed by `/VERYSILENT`), registry entries for `.ftm`, `.fgloss`, `.fprefs`. `OutputBaseFilename=Felix_Setup_{#AppVersion}`. All match the manifest; no fixes needed.
4. `winget settings export` -> `"LocalManifestFiles":false` (unchanged; no winget setting touched).
Not testable here: `winget install --manifest` (needs the setting and a public installer matching the hash); left to the user (README step 5).

Deviations: none. Additions: README also names `winget hash` and `wingetcreate update`, and tells the user to turn `LocalManifestFiles` off again afterwards.
