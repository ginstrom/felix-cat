# Builds the Release binaries and compiles the Felix installer.
#
#   powershell -ExecutionPolicy Bypass -File tools\build-installer.ps1
#
# Output: setup\Output\Felix_Setup_<version>.exe
# Needs: Visual Studio 2026 (C++ workload, ATL), the libraries in BUILDING.md,
# Microsoft Office (the add-ins import its type libraries) and Inno Setup 6.
# Close Felix, Word, Excel and PowerPoint first: their files are rebuilt.
#
# Code signing: if signing.local.json exists at the repo root (or
# -SigningConfig names one), Felix's binaries, the installer and its
# uninstaller are signed with tools\sign-file.ps1. Otherwise the build is
# unsigned. -Unsigned forces an unsigned build; -SkipBuild reuses the
# existing binaries (e.g. to re-sign and repackage).

param(
    [string]$SigningConfig = '',
    [switch]$Unsigned,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (-not $SigningConfig) { $SigningConfig = Join-Path $repo 'signing.local.json' }
$sign = (-not $Unsigned) -and (Test-Path $SigningConfig)
$signScript = Join-Path $PSScriptRoot 'sign-file.ps1'
if ($sign) { "Signing with $SigningConfig" } else { "Not signing (no signing settings, or -Unsigned)" }

$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -products * -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild not found: install Visual Studio 2026 with the C++ workload.' }

$iscc = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
          "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw 'Inno Setup 6 not found: winget install --id JRSoftware.InnoSetup --source winget' }

$builds = @(
    @('Felix\EngResource\Felix English.vcxproj', 'Unicode_Release', 'Win32'),
    @('Felix\JpnResource\Felix Japanese.vcxproj', 'Unicode_Release', 'Win32'),
    @('Felix\Felix.vcxproj', 'Unicode_Release', 'Win32'),
    @('WordAssist\WordAssist\WordAssist.vcxproj', 'Release', 'x64'),
    @('ExcelAssist\ExcelAssist\ExcelAssist.vcxproj', 'Release', 'x64'),
    @('PowerPointAssist\PowerPointAssist\PowerPointAssist.vcxproj', 'Release', 'x64')
)
if (-not $SkipBuild) {
    foreach ($b in $builds) {
        "Building $($b[0]) ($($b[1])|$($b[2]))"
        & $msbuild (Join-Path $repo $b[0]) /p:Configuration=$($b[1]) /p:Platform=$($b[2]) /m /nologo /v:minimal
        if ($LASTEXITCODE -ne 0) { throw "Build failed: $($b[0])" }
    }
}

if ($sign) {
    # Our own binaries only; SciLexer.dll is Scintilla's and stays as shipped.
    $binaries = @(
        'Felix\Unicode_Release\Felix.exe',
        'Felix\Unicode_Release\lang\EngResource.dll',
        'Felix\Unicode_Release\lang\JpnResource.dll',
        'WordAssist\WordAssist\x64\Release\WordAssist.dll',
        'ExcelAssist\ExcelAssist\x64\Release\ExcelAssist.dll',
        'PowerPointAssist\PowerPointAssist\x64\Release\PowerPointAssist.dll'
    ) | ForEach-Object { Join-Path $repo $_ }
    "Signing Felix's binaries"
    # In-process, so the file list arrives as an array (powershell -File would
    # split it into separate arguments). sign-file.ps1 throws on failure.
    & $signScript -Config $SigningConfig -Path $binaries
}

"Compiling the installer"
$isccArgs = @('/Q')
if ($sign) {
    # Inno Setup runs this for the installer and the uninstaller; $f is the
    # quoted file name and $q a quote character.
    $isccArgs += '/DSIGN'
    $isccArgs += "/Sfelixsign=powershell.exe -NoProfile -ExecutionPolicy Bypass -File `$q$signScript`$q -Config `$q$SigningConfig`$q -Path `$f"
}
$isccArgs += (Join-Path $repo 'setup\FelixSetup.iss')
& $iscc @isccArgs
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compile failed' }
Get-ChildItem (Join-Path $repo 'setup\Output') -Filter 'Felix_Setup_*.exe' |
    ForEach-Object { "Installer: $($_.FullName) ({0:N1} MB)" -f ($_.Length / 1MB) }
