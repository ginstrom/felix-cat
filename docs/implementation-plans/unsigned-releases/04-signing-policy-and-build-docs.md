# Step 04: Signing policy draft and build docs

Plan: [index.md](index.md). Design: [../../designs/unsigned-releases.md](../../designs/unsigned-releases.md) sections 3.6, 5.6, 6 (gaps G6, G7 BUILDING.md, G8).

## Objective
Replace the root `CODE_SIGNING_POLICY.md` (which describes signing that doesn't happen) with an unlinked draft at `docs/CODE_SIGNING_POLICY.md`, mark `.signpath/` as not in use, and rewrite BUILDING.md's release section for the unsigned flow.

## Requirements and context
- R6.1: the policy's privacy statement lists every network connection Felix makes (design 3.6, N1-N8).
- R6.2 / A6: `docs/CODE_SIGNING_POLICY.md` exists, is linked from nowhere (README, workflows' comments excepted), has roles, what is and isn't signed, how releases are built, the privacy statement, and SignPath's required line marked pending approval. The root file and README link are gone (README part done in step 03).
- It is still reachable on GitHub Pages as plain text (`.nojekyll`); that is accepted, so the draft opens with the DRAFT comment and "pending" marker.
- SciLexer: GitHub issue [ginstrom/felix-cat#44](https://github.com/ginstrom/felix-cat/issues/44) tracks the Scintilla 5 migration (decided, user, 2026-11-29).

## Dependencies and prerequisites
Step 02 (BUILDING.md describes the final `release.yml`: dispatch dry run, `SIGNING_ENABLED`, placeholders, `SHA256SUMS.txt`, attestation, draft titled `Felix <v> (unsigned)`; contract names in the index C4). Inspect current files first: root `CODE_SIGNING_POLICY.md` (reuse its structure and good text), `BUILDING.md` ("Releases from GitHub Actions", "Signing locally"), `.signpath/artifact-configurations/{program-files,installer}.xml`.

## Scope and implementation guidance
1. `git rm CODE_SIGNING_POLICY.md`.
2. Create `docs/CODE_SIGNING_POLICY.md`, starting with an HTML comment: "DRAFT for a future SignPath Foundation application. Not linked from anywhere. Releases are currently unsigned." Then:
   - Required line: `Free code signing provided by [SignPath.io](https://signpath.io), certificate by [SignPath Foundation](https://signpath.org)` followed by `**(pending approval: not in effect until SignPath Foundation accepts the project)**`.
   - Roles: Ryan Ginstrom ([@ginstrom](https://github.com/ginstrom)) as author (committer), reviewer and approver, with MFA on GitHub and SignPath.
   - Will be signed: `Felix.exe`; the 64-bit add-in DLLs `WordAssist.dll`, `ExcelAssist.dll`, `PowerPointAssist.dll`; `lang\EngResource.dll` and `lang\JpnResource.dll`; the installer `Felix_Setup_<version>.exe`.
   - Won't be signed and why: `SciLexer.dll` (third-party, Scintilla 2.01, shipped as provided; migration tracked in #44); `vc_redist.x86.exe`/`vc_redist.x64.exe` (Microsoft-signed); `unins000.exe` (Inno Setup writes it inside the compiler, where SignPath can't be called); `Felix.tlb`, templates, licenses (not executable). Add a note on the Smart App Control consequence: Smart App Control checks every executable file, so unsigned DLLs such as `SciLexer.dll` and the uninstaller could still be blocked after signing, unless Microsoft's service recognizes them (`SignedUninstaller` needs a certificate path that can sign through Inno's `SignTool`).
   - How releases are built: tag, GitHub Actions on a GitHub-hosted runner, signing requests approved by hand, draft release reviewed before publishing; nothing built on a personal computer is signed.
   - Privacy statement (text from the design, verbatim in substance): Felix doesn't collect usage data, has no telemetry and doesn't connect to the internet on its own. It opens web pages only when you ask (Help and FAQ, add-in help buttons open the online manual at `ginstrom.github.io/felix-cat` in the browser). Remote memories and glossaries are handled by a separately installed component (`Felix.RemoteMemory`), not part of the release; address and any user name and password you enter go to that component, which connects to the server you named. Felix displays entries with Windows' built-in HTML engine; if an entry or customized template refers to a web address (for example an image), Windows may load it. The Office add-ins talk only to Felix on the same computer. Memories, glossaries and settings stay on your computer. Then a bracketed paragraph for the update checker, "to be used only if `feature/cpp-update-checker` merges": Felix asks first; if you agree, about every two weeks (and on Help > Check Updates) it requests `https://api.github.com/repos/ginstrom/felix-cat/releases/latest`; GitHub receives your IP address and Felix's version number (User-Agent `Felix-update-check/<VERSION>`); nothing else is sent; the answer is stored in `HKCU\Software\AssistantSuite\Felix\UPDATE_CHECK`.
3. `.signpath/artifact-configurations/program-files.xml` and `installer.xml`: add a leading XML comment in each: "Not in use: releases are unsigned. Kept for a future SignPath application; see docs/CODE_SIGNING_POLICY.md". Keep the XML well-formed (comment goes after the XML declaration if there is one). Create `.signpath/README.md` (about five lines) saying the same.
4. `BUILDING.md`: rewrite "Releases from GitHub Actions" for the flow in design 4.1: bump version and write `setup/release-notes-<v>.md` (optional), merge, push tag `v<v>`; the workflow builds, writes `SHA256SUMS.txt`, attests provenance, uploads artifact `felix-<v>-release` and creates draft `Felix <v> (unsigned)` with two assets, notes = release notes file plus the generated "Installing and verifying" section (from `.github/release-notes-install.md`); maintainer reviews and publishes. Also document: the `workflow_dispatch` dry run (Actions > Release > Run workflow; never creates a release; makes a test attestation); the repository variable `SIGNING_ENABLED` must stay unset, setting it to `true` fails the run on purpose until the placeholder steps get a signer; re-running a tag run after the draft exists fails at `gh release create`, so delete the draft and re-run; no secrets are needed. Remove the SignPath description and `SIGNPATH_*`. Keep "Signing locally" unchanged apart from adding: "Official releases are currently unsigned; don't sign them with a self-signed certificate, which gives users no benefit." Mention the SciLexer migration issue (#44) wherever BUILDING.md discusses SciLexer/Scintilla, or in one line in the release section. Remove "builds signed" wording elsewhere in BUILDING.md (`rg -n -i "sign" BUILDING.md` to review each hit).
- Exclusions: README and manual (step 03); workflows (step 02); no new signing mechanism.

## Verification
1. `git ls-files CODE_SIGNING_POLICY.md` is empty; `rg -n CODE_SIGNING_POLICY` outside `docs/designs`, `docs/features`, `docs/implementation-plans` finds only `docs/CODE_SIGNING_POLICY.md` itself, comments in `.github/workflows/release.yml` and `.signpath` files (not README, not BUILDING.md's links as hyperlinks to the policy; a plain mention is fine).
2. The required SignPath line and "pending approval" text are present: `rg -n "pending approval|Free code signing provided by" docs/CODE_SIGNING_POLICY.md`.
3. XML well-formedness of both `.signpath` files: `[xml](Get-Content -Raw <file>)` in PowerShell returns without error.
4. `rg -n -i "signpath" BUILDING.md` finds nothing (or only the policy-draft reference); `rg -n "SIGNPATH_" .` outside plan/design docs finds nothing.
5. Each network connection N1-N8 in design 3.6 is accounted for in the privacy text (N1/N2 dead paths need no user-facing mention beyond "doesn't connect on its own").

## Completion criteria
- [ ] Root policy deleted; new draft exists, unlinked, with all sections.
- [ ] `.signpath/` marked not in use; BUILDING.md rewritten.
- [ ] Checks 1-5 recorded; index status updated; one commit if the brief says so.

## Execution record
