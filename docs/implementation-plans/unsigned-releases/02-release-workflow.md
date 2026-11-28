# Step 02: Release workflow

Plan: [index.md](index.md). Design: [../../designs/unsigned-releases.md](../../designs/unsigned-releases.md) sections 4.1, 4.3, 5.2 (gap G1).

## Objective
Rewrite `.github/workflows/release.yml` so a `v*` tag builds everything, writes `SHA256SUMS.txt`, attests build provenance for the installer, and creates a draft release with two assets. SignPath is removed; signing is represented by disabled, clearly marked placeholders.

## Requirements and context
- R3.1: runs on `v*` tags, builds, builds the installer with the tag's version (existing).
- R3.2/R3.3/R3.4: `SHA256SUMS.txt`; `actions/attest@v4` provenance for the installer with `id-token: write` and `attestations: write`; a **draft** release with the installer and `SHA256SUMS.txt`, whose notes include the verification commands.
- R3.5: placeholders gated on `vars.SIGNING_ENABLED == 'true'`, in the order sign program files, build installer, sign installer, verify. If the variable is `'true'` before a real signer exists, the first placeholder fails the run on purpose.
- A3: no `signpath`/`SIGNPATH` text anywhere under `.github`; with the variable unset, a tag run produces a draft named `Felix X.Y.Z (unsigned)` with two assets.
- Edge cases: tag not `vX.Y.Z` or not equal to `AppVersion` fails at the first step (existing behavior). Re-running after a draft exists makes `gh release create` fail; that is documented in BUILDING.md (step 04), not handled here. Dispatch runs never create a release.
- Do not push, tag or dispatch anything.

## Dependencies and prerequisites
Step 01 must be done. Consumed from it (copied from the index):

**C1.** `tools\make-release-files.ps1 -Version <x.y.z> [-Signed] [-OutDir setup\Output]` writes `<OutDir>\SHA256SUMS.txt` and `<OutDir>\release-notes.md` from `<OutDir>\Felix_Setup_<Version>.exe`.
**C3.** Outputs are `setup/Output/Felix_Setup_<v>.exe`, `setup/Output/SHA256SUMS.txt`, `setup/Output/release-notes.md`; this workflow uploads artifact `felix-<v>-release` containing all three.
**C4.** Variable `SIGNING_ENABLED`; title `Felix <v>` plus ` (unsigned)` when not signing; policy file `docs/CODE_SIGNING_POLICY.md` (created in step 04; only a comment here, so a dangling reference is fine until then).

Existing and kept: the current `release.yml` (inspect it: version-from-tag step, `setup-felix-deps`, `setup-msbuild@v3`, Inno Setup install, the "Stage program files" file list, the `signtool verify /pa` step including the silent test install, the draft release step). Reuse the file list: `Felix\Unicode_Release\Felix.exe`, `lang\EngResource.dll`, `lang\JpnResource.dll`, and the x64 `WordAssist.dll`, `ExcelAssist.dll`, `PowerPointAssist.dll` under `<A>\<A>\x64\Release\`.

## Scope and implementation guidance
Edit only `.github/workflows/release.yml`. Target structure (keep existing step bodies where unchanged):
- Header comment as in design 5.2: unsigned for now, the three SIGNING PLACEHOLDER steps mark where signing goes in order, they run only when `SIGNING_ENABLED` is `'true'` and fail on purpose until a signer is added; what will be signed: `docs/CODE_SIGNING_POLICY.md`.
- `on: push: tags: ['v*']` plus `workflow_dispatch:` (dry run: everything except creating the release).
- Top-level `permissions: contents: read`; `concurrency` unchanged. Top-level env: `SIGNING_ENABLED: ${{ vars.SIGNING_ENABLED == 'true' }}`, `PUBLISH: ${{ github.event_name == 'push' && startsWith(github.ref, 'refs/tags/v') }}`. Remove `SIGN`, `SIGNPATH_*`.
- Job `release`: `runs-on: windows-latest`, `timeout-minutes: 90`, permissions `contents: write`, `id-token: write`, `attestations: write` (remove `actions: read`).
- Steps in order:
  1. `actions/checkout@v7`.
  2. Version: on a tag, the existing tag check (`vX.Y.Z`, equals `AppVersion` in `setup\FelixSetup.iss`); on dispatch, read `AppVersion` from the `.iss`. Either way write `FELIX_VERSION` to `$GITHUB_ENV`. Dispatch from a non-tag ref is allowed.
  3. `./.github/actions/setup-felix-deps`, `microsoft/setup-msbuild@v3`, `choco install innosetup -y --no-progress`.
  4. Build binaries: `tools\build-installer.ps1 -Unsigned -AllPlatforms -NoRegister -SkipInstaller` (same invocation style as the existing step, plus `-AllPlatforms`).
  5. `Sign program files (SIGNING PLACEHOLDER)`, `if: env.SIGNING_ENABLED == 'true'`: a comment block describing what a signer must do (stage the six files above, sign them, copy back before the installer build), then `throw 'SIGNING_ENABLED is true, but no signing step has been added here yet.'`. A comment notes a SignPath-style signer would re-add an artifact upload and `actions: read`.
  6. Build installer: `-Unsigned -SkipBuild -AppVersion $env:FELIX_VERSION` (existing).
  7. `Sign installer (SIGNING PLACEHOLDER)`, same `if` and `throw`.
  8. `Verify signatures`, `if: env.SIGNING_ENABLED == 'true'`: the existing `signtool verify /pa` step, minus the `Copy-Item` from `staging\installer-signed` (the installer is already in `setup\Output`); keep the file list and the silent-install check.
  9. Release files: `tools\make-release-files.ps1 -Version $env:FELIX_VERSION` with `-Signed` when `$env:SIGNING_ENABLED -eq 'true'`.
  10. `actions/attest@v4` with `subject-path: setup/Output/Felix_Setup_${{ env.FELIX_VERSION }}.exe`. Do not add `artifact-metadata: write` or `push-to-registry`.
  11. `actions/upload-artifact@v7`: name `felix-${{ env.FELIX_VERSION }}-release`, the three files, `if-no-files-found: error`.
  12. `Create draft release`, `if: env.PUBLISH == 'true'`, `GH_TOKEN: ${{ github.token }}`: `gh release create <tag> <installer> setup\Output\SHA256SUMS.txt --draft --verify-tag --title <title> --notes-file setup\Output\release-notes.md`; title `Felix <v>` plus ` (unsigned)` unless `$env:SIGNING_ENABLED -eq 'true'`. Use `${{ github.ref_name }}` as the tag.
- Note env values are strings: compare with `== 'true'` in `if:` and `-eq 'true'` in pwsh, as the existing workflow does.
- Exclusions: no secrets, no `.signpath/` edits (step 04), no BUILDING.md (step 04), no changes to `build.yml`.

## Verification
1. `rg -i signpath .github` finds nothing; `rg -n "SIGNING_ENABLED|PUBLISH" .github/workflows/release.yml` shows the gates as designed; the placeholder steps appear in the order 5, 6, 7, 8 above.
2. YAML parses: `gh` can't lint offline, so load the file with a YAML parser available locally (e.g. `pwsh`/`ConvertFrom-Yaml` if installed, or Node: `node -e "require('js-yaml')..."` only if already installed; do not install packages globally). If none is available, review by eye and record that. If the branch is already on GitHub, `gh workflow view release.yml --repo ginstrom/felix-cat` or `--ref` can confirm it parses; do not push to find out.
3. Dry-run the notes logic: run `tools\make-release-files.ps1 -Version 3.0.1 -OutDir <temp>` on a dummy installer (as step 01) and confirm the title expression in the workflow matches contract C4 by reading the script text.
4. Review against A3: permissions exactly `contents: write`, `id-token: write`, `attestations: write` at job level; `PUBLISH` gates only the release step.
The live run (dispatch/tag) is the user's, and the pushed-branch CI run is step 06; record both as not done here.

## Completion criteria
- [x] `release.yml` matches the structure above; checks 1-4 recorded.
- [x] No SignPath text remains under `.github`.
- [x] Index status updated; one commit if the brief says so.

## Execution record

### Run 2026-11-29 (engineer; commit with this record)
No `runlog` (no Python); commands run directly from the worktree in Git Bash. `pwsh`, `actionlint` and any YAML parser (no `ConvertFrom-Yaml`, no `js-yaml` for Node) are absent; nothing was installed.
- Change: `.github/workflows/release.yml` rewritten to the structure above. Only that file (plus this record and the index) changed.
- Baseline (red): before the rewrite, `grep -rn -i signpath .github | wc -l` gave 20.
- Check 1: after, `grep -rn -i signpath .github` prints nothing, exit 1. `grep -n "SIGNING_ENABLED\|PUBLISH" .github/workflows/release.yml`: env `SIGNING_ENABLED: ${{ vars.SIGNING_ENABLED == 'true' }}` and `PUBLISH: ${{ github.event_name == 'push' && startsWith(github.ref, 'refs/tags/v') }}`; `if: env.SIGNING_ENABLED == 'true'` on exactly "Sign program files (SIGNING PLACEHOLDER)", "Sign installer (SIGNING PLACEHOLDER)", "Verify signatures"; `if: env.PUBLISH == 'true'` only on "Create draft release". Step order (`grep -n "name:"`): Version, setup-felix-deps, setup-msbuild, Install Inno Setup, Build binaries, Sign program files (PLACEHOLDER), Build installer, Sign installer (PLACEHOLDER), Verify signatures, Release files, Attest build provenance, upload `felix-<v>-release`, Create draft release.
- Check 2: no YAML parser available; reviewed by eye (no tabs: `grep -c $'\t'` = 0; block scalars and step indentation match `build.yml`). The real parse is the pushed-branch run in step 06 (the workflow file is parsed on push even though it only triggers on tags/dispatch).
- Check 3: dummy `Felix_Setup_3.0.1.exe` in a scratch folder; ran the workflow's exact "Release files" line plus the release step's title logic under Windows PowerShell 5.1 with `$env:SIGNING_ENABLED` = `false` and `true`: both exit 0; SHA256SUMS.txt is `<64 hex>  Felix_Setup_3.0.1.exe`; notes contain the `Get-FileHash` and `gh attestation verify` lines with 3.0.1, no BOM, no CR; "not code-signed" present only for `false`. Titles: `Felix 3.0.1 (unsigned)` and `Felix 3.0.1` (C4). `-Signed:($env:SIGNING_ENABLED -eq 'true')` binds correctly in 5.1.
- Version step: extracted its `run:` body to a scratch .ps1 and ran it under 5.1 with `REF_TYPE`/`REF_NAME`: tag `v3.0.1` -> `FELIX_VERSION=3.0.1`; tag `v3.0.2` -> "Tag says 3.0.2 but setup\FelixSetup.iss says 3.0.1. Bump the version first.", exit 1; tag `v3.0` -> "Tag v3.0 is not vX.Y.Z", exit 1; branch `ci/unsigned-releases` -> `FELIX_VERSION=3.0.1`.
- Check 4: top-level `permissions: contents: read`; job `release` has exactly `contents: write`, `id-token: write`, `attestations: write` (`actions: read` removed); `concurrency` unchanged; `timeout-minutes: 90`.
- Not done here: the live tag run and the dispatch dry run (user, design 8.2) and the pushed-branch CI run (step 06).
- Deviations: (a) The Version step branches on `github.ref_type == 'tag'` rather than event name, so a dispatch from a tag also runs the tag check (it still never publishes; `PUBLISH` is false for dispatch). (b) `github.ref_name` reaches scripts through step `env` (`REF_NAME`, `TAG`) instead of `${{ }}` interpolation inside the script, so branch names can't inject PowerShell. (c) The header comment says "two SIGNING PLACEHOLDER steps ... then Verify signatures" instead of design 5.2's "three", because only two steps carry that label. (d) `gh release create` exit code is checked explicitly (native command failures don't stop pwsh). (e) The placeholder comment describes "a signing service that reads files from this run's artifacts" instead of naming SignPath, because check 1 forbids the name under `.github`.
- For later steps: step 04's BUILDING.md should describe the steps by these names; artifact `felix-<v>-release`; title `Felix <v> (unsigned)`; dispatch = dry run with no release; re-running a tag after the draft exists fails at `gh release create`.

### Review round 1 (2026-11-29)
- Found (reviewer; change taken by the orchestrator): a `workflow_dispatch` dry run from any branch minted a valid provenance attestation, and the documented `gh attestation verify <file> --repo ginstrom/felix-cat` (C2) accepts attestations from any ref, so it could not tell a branch build from a tag build.
- Fixed: "Attest build provenance" now has `if: env.PUBLISH == 'true'` (tag pushes only), with a comment giving the reason; the header comment, the `workflow_dispatch` comment and the `PUBLISH` comment now say dry runs neither attest nor create a release. C2's verify command is unchanged. This supersedes check 1's note above: `PUBLISH` now gates "Attest build provenance" and "Create draft release".
- Check: `grep -n "PUBLISH" .github/workflows/release.yml` shows the env definition, the Version step's log line, and `if: env.PUBLISH == 'true'` on exactly those two steps (lines 155 and 170). `grep -rn -i signpath .github` still finds nothing (exit 1). No YAML parser available; reviewed by eye.
- For later steps: BUILDING.md (step 04) should say a dispatch dry run produces the artifact but no attestation and no release.
