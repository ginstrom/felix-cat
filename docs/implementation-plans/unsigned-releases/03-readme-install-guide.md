# Step 03: README install guide and release notes

Plan: [index.md](index.md). Design: [../../designs/unsigned-releases.md](../../designs/unsigned-releases.md) sections 5.5 and 5.6 (last bullet) (gaps G5, part of G7).

## Objective
Add a README "Installing Felix" section that covers Phase 4 of the spec, remove every claim that releases are signed, add one paragraph to the English manual's getting-started page, and fix `setup/release-notes-3.0.1.md`.

## Requirements and context
- R4.1 / A4: the section covers every bullet below; no claim that releases are signed; every Windows or Office path matches the design's section 10 sources (reproduced in the text below).
- English only (Q4); the Japanese getting-started refresh is a follow-up, not part of this step.
- `README.rst` is reStructuredText. The SmartScreen screenshot is a commented placeholder only (RST comment `.. TODO: ...`); add no image file.
- The README links the signing policy today (`CODE_SIGNING_POLICY.md`, deleted in step 04); after this step the README links to no policy file (the new one is deliberately unlinked).

## Dependencies and prerequisites
Step 01 (the commands must match `.github/release-notes-install.md`). Copy these two command blocks into the README unchanged apart from a concrete example version (use `3.0.1` for `{{VERSION}}`):

    Get-FileHash .\Felix_Setup_{{VERSION}}.exe -Algorithm SHA256
    gh attestation verify .\Felix_Setup_{{VERSION}}.exe --repo ginstrom/felix-cat

Anchor and section title: **Installing Felix** (`#installing-felix`), used by the release-notes link `https://github.com/ginstrom/felix-cat#installing-felix`.

## Scope and implementation guidance
Inspect first: `README.rst` (current "Installing", "Building" and "Code signing" sections, heading levels and anchor style), `docs/manual/en/getting-started.html` section 2.2, `setup/release-notes-3.0.1.md`.

1. `README.rst`: replace "Installing" with **"Installing Felix"**, in this order:
   1. Download and requirements: keep the current text (latest-release link, 64-bit Windows 10/11 with admin rights, 64-bit Office for the add-ins, what gets installed, manual links, "Upgrading from Felix 2.0"), but delete "Releases up to 3.0.1 aren't code-signed ... Later releases are signed".
   2. Why Windows warns you: releases aren't code-signed yet; signing lets Windows show who published a program, and without it Windows treats the installer as from an unknown publisher; we plan to sign (see Code signing status). Each release is built by GitHub Actions from a tagged commit, with a checksum and a provenance attestation.
   3. SmartScreen: `.. TODO: screenshot of the "Windows protected your PC" dialog (docs/images/smartscreen.png)` as an RST comment; text: "Windows protected your PC ... Microsoft Defender SmartScreen prevented an unrecognized app from starting"; steps **More info**, check the app is `Felix_Setup_<version>.exe` (Publisher: Unknown publisher), then **Run anyway**. Alternative: right-click the downloaded file, **Properties**, under *Security* tick **Unblock**, **OK**, then run it (the checkbox appears only on files downloaded from the internet). Note: organization policy can remove **Run anyway**.
   4. Verifying the download: the two commands, with what each proves. Checksum: the file isn't corrupted or swapped, provided `SHA256SUMS.txt` came from the release page. Attestation: GitHub Actions built the file from this repository. Optional stricter check: add `--signer-workflow ginstrom/felix-cat/.github/workflows/release.yml`. `gh` needs `gh auth login` first.
   5. Smart App Control (Windows 11): when **On**, Windows blocks apps it can't vouch for, including unsigned installers, with no "Run anyway" and no per-app exception. How to tell: **Windows Security > App & browser control > Smart App Control settings** shows **On**, **Evaluation** or **Off** (Evaluation doesn't block); same route via Settings > Privacy & security > Windows Security > App & browser control. If it blocked Felix, the notification and **Virus & threat protection > Protection history** say so. A signed release is expected to fix this, because Smart App Control lets software run when signed with a certificate from a CA in Microsoft's Trusted Root Program. Turning it off lowers protection for the whole PC, so we don't recommend it just for Felix; recent Windows updates let you turn it back on without reinstalling.
   6. Office add-ins: they are unsigned COM add-ins and Office loads them by default. If your organization turns on **File > Options > Trust Center > Trust Center Settings > Add-ins > Require Application Add-ins to be signed by Trusted Publisher**, Office won't load them and the message bar says they were disabled; this usually comes from policy, so the option may be greyed out; even after releases are signed the organization must also trust the publisher. Re-enabling an add-in Office disabled (for example after a crash): File > Options > Add-ins > **Manage: Disabled Items** > **Go** > select it > **Enable**; then **Manage: COM Add-ins** > **Go** > tick **Felix Addin** (description "Felix interface for Word/Excel/PowerPoint") > **OK**. Link the manual's Troubleshooting page (find the existing URL in `docs/manual/en`).
   7. Code signing status: not signed yet; we intend to apply to the SignPath Foundation's free code signing program for open source projects; signed releases will show a verified publisher and won't be blocked by Smart App Control. Mention that `SciLexer.dll` may still need attention before signing (tracked in [ginstrom/felix-cat#44](https://github.com/ginstrom/felix-cat/issues/44)) only if it reads naturally; the policy file carries the detail.
   Also: delete the "Code signing" section; change "builds signed releases from version tags (`release.yml`)" in "Building" to "builds draft releases from version tags (`release.yml`), with SHA-256 checksums and build provenance attestations". Add no link to `docs/CODE_SIGNING_POLICY.md`.
2. `docs/manual/en/getting-started.html` section 2.2: add one paragraph (hand-edit the HTML, match the surrounding markup): "Felix's installer isn't code-signed yet, so Windows may warn before running it. See [Installing Felix](https://github.com/ginstrom/felix-cat#installing-felix) for what to do and how to verify the download."
3. `setup/release-notes-3.0.1.md`: remove both "code-signed" claims; add a line under "Download and install" pointing to the README's Installing Felix section; remove the trailing "SHA-256 of ... :" placeholder (`SHA256SUMS.txt` replaces it); fix the header comment (it says "Publish once the installer is signed" and gives a manual `gh release create`) to say that `release.yml` uses this file for the draft release's notes.
- Exclusions: Japanese manual, the policy file and BUILDING.md (step 04), workflows, screenshot binary.

## Verification
1. Render check: if `docutils` is unavailable (no Python), review the RST by eye for heading underline lengths and list/literal-block syntax; record that. If `gh` can render (`gh api markdown` doesn't do RST), skip.
2. `rg -i "signed|signpath" README.rst` shows only statements that releases are NOT signed or future-tense plans; `rg -i "code-signed|signed" setup/release-notes-3.0.1.md docs/manual/en/getting-started.html` likewise.
3. Command parity: the two commands in README equal those in `.github/release-notes-install.md` with `{{VERSION}}` = `3.0.1` (compare with a script, e.g. `Select-String`).
4. Every checkbox/menu path in the README appears in the list above; no extra Windows or Office claims.
5. `git grep -n CODE_SIGNING_POLICY -- README.rst` finds nothing.
Reviewer will also render the README on the pushed branch (step 06).

## Completion criteria
- [ ] README has the seven parts, SignPath claims and "Code signing" section removed, "Building" sentence changed.
- [ ] getting-started.html paragraph added; release-notes-3.0.1.md fixed.
- [ ] Checks 1-5 recorded; index status updated; one commit if the brief says so.

## Execution record
