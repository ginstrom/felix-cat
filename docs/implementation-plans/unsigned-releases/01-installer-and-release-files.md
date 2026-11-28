# Step 01: Installer script cleanup and release files tooling

Plan: [index.md](index.md). Design: [../../designs/unsigned-releases.md](../../designs/unsigned-releases.md) sections 5.3, 5.4, 7 (gaps G2, G3, G4).

## Objective
Make `setup/FelixSetup.iss` require `/DVcRedist`, delete the obsolete installer script, create `tools/make-release-files.ps1` and `.github/release-notes-install.md`, and have `build.yml` run the script and upload its outputs. The release workflow (step 02) and README (step 03) build on this.

## Requirements and context
- R2.2: the `.iss` takes `/DAppVersion=` and `/DSourceDir=` and has no absolute paths. Today the `VcRedist` default (around line 31) is `C:\Program Files\Microsoft Visual Studio\18\Community\...`. Replace it with a required define; `tools/build-installer.ps1` always passes `/DVcRedist`, so it is unaffected.
- R2.3: `SignTool=felixsign` and `SignedUninstaller=yes` stay behind `#ifdef SIGN` (only a local signing build defines `SIGN`; CI passes `-Unsigned`). Add one comment: "CI never defines SIGN; releases are unsigned (docs/CODE_SIGNING_POLICY.md)."
- G3: delete `setup/HTML FILE SETUP.iss` (obsolete 2009 script, nothing references it; check with `git grep -i "HTML FILE SETUP"` first and fix any reference).
- R3.2 and the notes half of R3.4: `SHA256SUMS.txt` and `release-notes.md` come from the script (contract C1 below), exercised by every CI build so a broken script shows up before a tag.
- Constraints: see index "Shared constraints". No changes to `tools/build-installer.ps1`.

## Dependencies and prerequisites
None. Existing: `setup/FelixSetup.iss`, `setup/release-notes-3.0.1.md` (exists; used to test the "notes file present" branch), `.github/workflows/build.yml` (its installer artifact step uploads `setup/Output/Felix_Setup_*.exe`), `tools/build-installer.ps1`. Inno Setup (`ISCC`) is installed locally if the A2 check can run; see Verification.

## Contracts to produce (copied from the index)

**C1. `tools/make-release-files.ps1`**
`tools\make-release-files.ps1 -Version <x.y.z> [-Signed] [-OutDir setup\Output]` (`-OutDir` default `setup\Output`, resolved against the repository root, not the current directory).
- Input `<OutDir>\Felix_Setup_<Version>.exe`; throws if missing.
- Writes `<OutDir>\SHA256SUMS.txt`: one line per `Felix_Setup_<Version>.exe` it was given, `<lowercase hex sha256><two spaces><file name>`, LF endings, trailing LF, UTF-8 without BOM.
- Writes `<OutDir>\release-notes.md`: content of `setup\release-notes-<Version>.md` if present, else `Felix <Version>`; then a blank line; then `.github/release-notes-install.md` with every `{{VERSION}}` replaced. With `-Signed`, the text from `<!-- unsigned -->` through `<!-- /unsigned -->` (markers included) is removed. LF endings, UTF-8 without BOM.

**C2. `.github/release-notes-install.md`**, exact content:

~~~markdown
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
~~~

**C3.** `build.yml` adds `setup/Output/SHA256SUMS.txt` and `setup/Output/release-notes.md` to the existing `felix-installer-<sha>` artifact.

## Scope and implementation guidance
1. `setup/FelixSetup.iss`: replace the `VcRedist` default with
   ```
   #ifndef VcRedist
     #error Define VcRedist: the folder with vc_redist.x86.exe and vc_redist.x64.exe (tools\build-installer.ps1 does this)
   #endif
   ```
   Keep the header comment's example command. Add the SIGN comment next to the `#ifdef SIGN` block. Leave everything else.
2. `git rm "setup/HTML FILE SETUP.iss"`.
3. Create `.github/release-notes-install.md` with exactly the C2 content (LF, trailing newline).
4. Create `tools/make-release-files.ps1`: `[CmdletBinding()]`, `$ErrorActionPreference='Stop'`, repository root from `$PSScriptRoot\..`. Write files with `[IO.File]::WriteAllText(path, text, (New-Object Text.UTF8Encoding $false))` after normalizing to LF (`-replace "`r`n","`n"`). Hash with `Get-FileHash -Algorithm SHA256`, lowercased. Must work in Windows PowerShell 5.1 and pwsh. Validate `-Version` matches `^\d+\.\d+\.\d+$`. Read the template and the optional notes file as UTF-8. Print the paths it wrote.
5. `build.yml`: after "Build binaries and installer", add a `pwsh` step "Release files" that reads `AppVersion` from `setup\FelixSetup.iss` (regex `#define AppVersion "([^"]+)"`, as release.yml does) and runs `tools\make-release-files.ps1 -Version <it>`. Change the installer artifact `path:` to a multi-line list: `setup/Output/Felix_Setup_*.exe`, `setup/Output/SHA256SUMS.txt`, `setup/Output/release-notes.md`. Add no permissions and no attestation here.
- Exclusions: `release.yml` (step 02), docs.

## Verification
Record each command and result in the Execution record.
1. Script on a dummy file, in a temp folder outside the repo (use `$env:TEMP`): create `Felix_Setup_9.9.9.exe` with some bytes; run `powershell -NoProfile -File tools\make-release-files.ps1 -Version 9.9.9 -OutDir <temp>` (and `pwsh` if present). Check: `SHA256SUMS.txt` bytes match `^[0-9a-f]{64}  Felix_Setup_9\.9\.9\.exe\n$`, the hash equals `Get-FileHash`, no BOM (first bytes are not EF BB BF), no CR.
2. `release-notes.md`, four cases: without a `setup\release-notes-9.9.9.md` (begins `Felix 9.9.9`), and with one (temporarily create it, then delete it; or use `-Version 3.0.1` with a dummy `Felix_Setup_3.0.1.exe` in the temp OutDir to use the real notes file); each with and without `-Signed`. Check: no `{{VERSION}}` left, both commands contain `9.9.9`, the "not code-signed" paragraph and its markers are present only without `-Signed`, no CR, no BOM.
3. Missing installer: the script throws a clear error and writes nothing.
4. A2: `ISCC setup\FelixSetup.iss` (without `/DVcRedist`) fails with the `#error` text. Find ISCC under `C:\Program Files (x86)\Inno Setup 6\` or via `choco`. If Inno Setup isn't installed locally, say so and leave it for step 06's CI run. If it is, also confirm `powershell -NoProfile -ExecutionPolicy Bypass -File tools\build-installer.ps1 -Unsigned -NoRegister -SkipBuild` still builds the installer when binaries exist; skip (and say so) if no Release binaries are present.
5. `build.yml` is valid YAML: `gh workflow view build.yml` is not reliable offline, so check by loading it with PowerShell (e.g. `ConvertFrom-Yaml` if available) or by careful review; `actionlint` isn't installed. The real check is step 06's CI run.
6. `git grep -n "C:\\\\Program Files" -- setup/FelixSetup.iss` shows no absolute path left.

## Completion criteria
- [ ] `.iss` requires `VcRedist`, has the SIGN comment, `HTML FILE SETUP.iss` is deleted, nothing references it.
- [ ] Script and template exist and match C1/C2; checks 1-3 pass (record results).
- [ ] `build.yml` runs the script and uploads the three files (C3).
- [ ] Check 4 done or explicitly deferred to step 06 with the reason.
- [ ] Index status updated; one commit made if the brief says so.

## Execution record

### Run 2026-11-29 (engineer; commit with this record)
No `runlog` (no Python); commands run directly. `pwsh` is not installed, so only Windows PowerShell 5.1 was tested.
- Changes: `setup/FelixSetup.iss` (VcRedist default replaced by `#error`; SIGN comment added; BOM and CRLF preserved), `setup/HTML FILE SETUP.iss` removed (`git grep -i "HTML FILE SETUP"` outside `docs/` found nothing; docs hits are the plan/design text only), `.github/release-notes-install.md` (C2 verbatim), `tools/make-release-files.ps1` (C1), `.github/workflows/build.yml` ("Release files" pwsh step after "Build binaries and installer"; installer artifact path is now the 3-line list; CRLF kept).
- Checks 1-2: dummy installers `Felix_Setup_9.9.9.exe` and `Felix_Setup_3.0.1.exe` in `$env:TEMP\mrf`; `powershell -NoProfile -File tools\make-release-files.ps1 -Version <v> -OutDir <tmp> [-Signed]` for 9.9.9 (no notes file) and 3.0.1 (real notes file), each with and without `-Signed`. All four: SHA256SUMS.txt matches `^[0-9a-f]{64}  Felix_Setup_<v>\.exe\n$` and starts with the `Get-FileHash` value; no BOM; no CR in either file; no `{{VERSION}}`; version appears in both commands; "not code-signed" text and `<!-- unsigned -->` markers present only without `-Signed`; 9.9.9 notes begin `Felix 9.9.9`, 3.0.1 notes begin with the real file's first line.
- Check 3: empty OutDir, `-Version 1.2.3`: error "Installer not found: ...\Felix_Setup_1.2.3.exe", exit 1, OutDir still empty. `-Version 1.2`: "Version must look like 1.2.3".
- Check 4: ISCC found at `%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe`. `ISCC setup\FelixSetup.iss` fails: "Error on line 31 ...: Define VcRedist: the folder with vc_redist.x86.exe and vc_redist.x64.exe (tools\build-installer.ps1 does this)", exit 2. The `build-installer.ps1 -SkipBuild` run was skipped: `Felix\Unicode_Release\Felix.exe` does not exist here (no Release binaries).
- Check 5: no YAML parser available (`ConvertFrom-Yaml` absent); reviewed the `git diff` by hand (indentation, block scalar). Real check is step 06's CI run.
- Check 6: `git grep -n 'C:\\Program Files' -- setup/FelixSetup.iss` returns nothing.
- Deviations: (a) with `-Signed` the removal also swallows the newlines after `<!-- /unsigned -->` so no extra blank lines are left; the removed text still runs from the opening to the closing marker inclusive. (b) Only Windows PowerShell 5.1 tested (no `pwsh` locally); the script uses nothing 5.1-incompatible, but CI will be the first pwsh run.
- For later steps: `make-release-files.ps1` takes `-Version`, `-Signed`, `-OutDir` as in C1; relative `-OutDir` resolves against the repo root.
