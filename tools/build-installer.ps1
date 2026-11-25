# Builds the Release binaries and compiles the Felix installer.
#
#   powershell -ExecutionPolicy Bypass -File tools\build-installer.ps1
#
# Output: setup\Output\Felix_Setup_<version>.exe
# Needs: Visual Studio 2026 (C++ workload, ATL), the libraries in BUILDING.md,
# Microsoft Office (the add-ins import its type libraries) and Inno Setup 6.
# Close Felix, Word, Excel and PowerPoint first: their files are rebuilt.

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent

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
foreach ($b in $builds) {
    "Building $($b[0]) ($($b[1])|$($b[2]))"
    & $msbuild (Join-Path $repo $b[0]) /p:Configuration=$($b[1]) /p:Platform=$($b[2]) /m /nologo /v:minimal
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $($b[0])" }
}

"Compiling the installer"
& $iscc /Q (Join-Path $repo 'setup\FelixSetup.iss')
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compile failed' }
Get-ChildItem (Join-Path $repo 'setup\Output') -Filter 'Felix_Setup_*.exe' |
    ForEach-Object { "Installer: $($_.FullName) ({0:N1} MB)" -f ($_.Length / 1MB) }
