# Request: unsigned CI releases (user's words, 2026-11-28)

"Change of plans: We are going to go without code signing for now. Details pasted in."

# Task: Set up unsigned CI releases for Felix (ginstrom/felix-cat)

## Context
Felix is an MIT-licensed C++ translation memory tool for Windows, built with Visual Studio 2026 (toolset v145). It ships with Word, Excel and PowerPoint COM add-ins and an Inno Setup installer.

We can't get a code-signing certificate yet. SignPath Foundation turned down the project for lack of public visibility, and the Certum application is stalled. Until we can reapply to SignPath, releases will ship **unsigned**. Do NOT self-sign anything: a self-signed certificate gives users no benefit.

Goal: every release is built by GitHub Actions from a tagged commit. Each release includes SHA-256 checksums and GitHub build provenance attestations. The README explains clearly how to install despite the SmartScreen warning. The pipeline must make it easy to add a signing step later.

## Ground rules
- Work on a branch (e.g. `ci/unsigned-releases`) and open a PR. Do not merge, push tags or publish releases.
- Do not add secrets, change repo settings, or submit anything to external services (winget-pkgs, SignPath, Certum). When one of those is needed, stop and write down exactly what I need to do.
- Ask before committing any binary or any file whose license is unclear.

## Phase 1: CI build (`.github/workflows/build.yml`)
Runs on push and PR, on `windows-latest` (this image has VS 2026 and v145), using `microsoft/setup-msbuild`. Build Release for Felix (x86) and all three add-ins (x86 + x64).

Things a clean runner doesn't have:
1. **Office type libraries:** runners have no Office, so `#import` will fail. Generate the `.tlh`/`.tli` files locally, commit them (e.g. to `common/office_typelibs/`, split by platform if x86 and x64 differ), make the `.tli` include paths relative, and replace each `#import` with `#include`, keeping the same namespace and rename attributes. Add a README explaining how to regenerate them. Never commit Office's own `.olb`/`.dll` files.
2. **WTL 10:** use the `wtl` NuGet package.
3. **Boost:** if it's header-only, download a pinned release; if compiled libraries are needed, use vcpkg in manifest mode. Replace hard-coded paths with properties (e.g. `Directory.Build.props`) so local and CI builds take the same route. Cache dependencies with `actions/cache`.
4. **SciLexer.dll:** prefer building Scintilla from a pinned source release in the workflow. If that's hard, keep the checked-in DLL and tell me.

Done when: CI passes on a clean runner, and the built files work on my machine (Felix runs, each add-in loads in Office).

## Phase 2: Installer in CI
- Install Inno Setup with `choco install innosetup -y`.
- Make the `.iss` take `/DAppVersion=` and `/DSourceDir=` from the command line instead of absolute paths.
- Remove or disable any `SignTool` / `SignedUninstaller` lines.
- Find where the bundled VC++ runtime installers come from. If they're downloaded by hand today, download them in CI from Microsoft's official links.
- Upload the installer as a workflow artifact.

Done when: the CI installer installs and runs on a clean Windows VM or Windows Sandbox, with and without the add-ins.

## Phase 3: Release workflow (`.github/workflows/release.yml`)
Triggered by tags matching `v*`:
1. Build (Phase 1) and build the installer (Phase 2) with the version taken from the tag.
2. Generate `SHA256SUMS.txt` covering the installer.
3. Add build provenance with `actions/attest-build-provenance` for the installer (the job needs `id-token: write` and `attestations: write` permissions).
4. Create a **draft** GitHub release with the installer and `SHA256SUMS.txt` attached, plus release notes that include the verification commands from the README.
5. Leave a clearly marked, disabled placeholder where signing will go later. The order must stay: sign program files, then build the installer, then sign the installer, so that SignPath or Certum steps can be added later without restructuring. Gate it on a repository variable such as `vars.SIGNING_ENABLED == 'true'`.

## Phase 4: User-facing docs
Add an "Installing Felix" section to the README (or `docs/INSTALL.md`, linked from the README), covering:
- **Why there's a warning:** releases aren't code-signed yet; signing is planned.
- **SmartScreen:** what the "Windows protected your PC" dialog looks like (leave a screenshot placeholder), and the "More info → Run anyway" steps. Mention that right-clicking the file → Properties → Unblock also works.
- **Verifying the download:**
  - `Get-FileHash .\Felix_Setup_<version>.exe -Algorithm SHA256`, compared against `SHA256SUMS.txt`
  - `gh attestation verify .\Felix_Setup_<version>.exe --repo ginstrom/felix-cat`
- **Smart App Control:** on Windows 11 PCs where it's turned on, unsigned apps are blocked with no "Run anyway" option. Explain how users can tell, and that a signed release will fix this.
- **Office add-ins:** unsigned add-ins load by default. If an organization enforces "Require Application Add-ins to be signed by Trusted Publisher", the add-ins won't load until releases are signed. Include how to re-enable an add-in Office has disabled: File → Options → Add-ins → Manage: Disabled Items, and how to turn it back on under Manage: COM Add-ins.
- Short "Code signing status" note: not signed yet; we intend to apply to SignPath Foundation's OSS program.

## Phase 5: winget (prepare only)
- Draft a winget manifest under `packaging/winget/` that points at the GitHub release URL, with Inno Setup silent install switches (`/VERYSILENT /SUPPRESSMSGBOXES /NORESTART`).
- Validate it with `winget validate`, and test it locally with `winget install --manifest`.
- Write down the steps for me to submit it to `microsoft/winget-pkgs` after the first release. Don't submit it yourself.

## Also prepare for the future SignPath application
- Find every network connection Felix makes (update checks, telemetry, downloads) and list what each sends. SignPath's code signing policy will need a privacy statement.
- Draft `docs/CODE_SIGNING_POLICY.md` (not linked anywhere yet). Include:
  - roles: me (@ginstrom) as author, reviewer and approver
  - what will be signed: Felix.exe, the add-in DLLs, the installer
  - what won't be signed and why: SciLexer.dll, the VC++ runtimes, the Inno uninstaller
  - the privacy statement
  - SignPath's required line: "Free code signing provided by SignPath.io, certificate by SignPath Foundation" (marked as pending approval)

## Report back in the PR description
- An inventory of `#import`s, external paths and Boost usage, and how each was handled
- Decisions taken (SciLexer approach, Boost approach)
- Anything that needs me: settings, submissions, screenshots, testing on a clean machine
- Open questions
