param(
    [ValidateSet('Debug', 'Release')] [string]$Configuration = 'Release',
    [ValidateSet('x64', 'Win32')] [string]$Platform = 'x64',
    [switch]$Unregister
)

# Registers Felix's COM server and the Word, Excel and PowerPoint add-ins, so
# Office loads the add-ins and they can reach Felix. Both write to HKLM, so run
# this as administrator; from a normal PowerShell it asks to elevate (UAC):
#
#   powershell -ExecutionPolicy Bypass -File tools\register-felix.ps1
#   powershell -ExecutionPolicy Bypass -File tools\register-felix.ps1 -Configuration Debug
#   powershell -ExecutionPolicy Bypass -File tools\register-felix.ps1 -Unregister
#
# -Platform must match Office: x64 for 64-bit Office, Win32 for 32-bit Office.
# Felix itself is always 32-bit; -Configuration picks Unicode_Release or Unicode_Debug.

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent

$admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin) {
    # Relaunch elevated with the same options. Windows shows a UAC prompt; the
    # elevated window stays open so you can read the result.
    'Registration writes to HKLM, so it needs administrator rights. Asking Windows to elevate (UAC prompt)...'
    $relaunch = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-NoExit', '-File', "`"$PSCommandPath`"",
                  '-Configuration', $Configuration, '-Platform', $Platform)
    if ($Unregister) { $relaunch += '-Unregister' }
    try { Start-Process powershell.exe -Verb RunAs -ArgumentList $relaunch }
    catch { throw 'Elevation was cancelled. Run this from an Administrator PowerShell instead.' }
    'Continuing in the new administrator window.'
    return
}

$felixDir = if ($Configuration -eq 'Release') { 'Unicode_Release' } else { 'Unicode_Debug' }
$felix = Join-Path $repo "Felix\$felixDir\Felix.exe"
$outDir = if ($Platform -eq 'x64') { "x64\$Configuration" } else { $Configuration }
$addins = 'WordAssist', 'ExcelAssist', 'PowerPointAssist' | ForEach-Object { Join-Path $repo "$_\$_\$outDir\$_.dll" }

foreach ($f in @($felix) + $addins) {
    if (-not (Test-Path $f)) { throw "Not built yet: $f" }
}

# Close Office first: a running Word/Excel/PowerPoint keeps the old add-in loaded.
$office = Get-Process WINWORD, EXCEL, POWERPNT -ErrorAction SilentlyContinue
if ($office) { Write-Warning ("Close Office before testing the add-ins: " + (($office.ProcessName | Sort-Object -Unique) -join ', ') + ' running.') }

$verb = if ($Unregister) { 'Unregistering' } else { 'Registering' }

"$verb Felix COM server: $felix"
$p = Start-Process $felix -ArgumentList $(if ($Unregister) { '/UnregServer' } else { '/RegServer' }) -Wait -PassThru
if ($p.ExitCode -ne 0) { Write-Warning "Felix.exe exited with $($p.ExitCode); see %LOCALAPPDATA%\Felix\logs\felix.log" }

# 64-bit regsvr32 for x64 DLLs; the 32-bit one for Win32 DLLs.
$regsvr = if ($Platform -eq 'x64') { "$env:WINDIR\System32\regsvr32.exe" } else { "$env:WINDIR\SysWOW64\regsvr32.exe" }
foreach ($dll in $addins) {
    "$verb add-in: $dll"
    $regArgs = @('/s') + $(if ($Unregister) { @('/u') } else { @() }) + @("`"$dll`"")
    $p = Start-Process $regsvr -ArgumentList $regArgs -Wait -PassThru
    if ($p.ExitCode -ne 0) { Write-Warning "regsvr32 exited with $($p.ExitCode) for $dll" }
}

if (-not $Unregister) {
    ''
    'Office add-in entries now registered (LoadBehavior 3 = load at startup):'
    foreach ($app in 'Word', 'Excel', 'PowerPoint') {
        Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Office\$app\Addins" -ErrorAction SilentlyContinue |
            Where-Object { $_.PSChildName -like '*Assist.Connect' } |
            ForEach-Object { "  {0,-11} {1}  LoadBehavior={2}" -f $app, $_.PSChildName, (Get-ItemProperty $_.PSPath).LoadBehavior }
    }
}
