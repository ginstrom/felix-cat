param(
    [ValidateSet('Debug', 'Release', 'Installed')] [string]$Configuration = 'Release',
    [ValidateSet('x64', 'Win32')] [string]$Platform = 'x64',
    [string]$InstallDir = (Join-Path $env:ProgramFiles 'Felix'),
    [switch]$Unregister,
    [switch]$Status,
    [string]$LogFile,  # internal: set when the script relaunches itself elevated
    [string]$UserSid   # internal: the account that started the script, whose per-user keys matter
)

# Points Felix's COM server and the Word, Excel and PowerPoint add-ins at one
# build, so Office loads those add-ins and they start that Felix.
#
#   tools\register-felix.ps1 -Status                    # what is registered now (no admin needed)
#   tools\register-felix.ps1 -Configuration Debug       # this repo's Debug build
#   tools\register-felix.ps1                            # this repo's Release build
#   tools\register-felix.ps1 -Configuration Installed   # back to the installed Felix (C:\Program Files\Felix)
#   tools\register-felix.ps1 -Unregister                # remove the registration of a build
#
# Run it with: powershell -ExecutionPolicy Bypass -File tools\register-felix.ps1 ...
# Registering writes to HKLM, so it needs administrator rights. From a normal
# PowerShell it asks Windows to elevate (UAC prompt), waits, and shows the
# result here. -Platform must match Office: x64 for 64-bit Office, Win32 for
# 32-bit Office. Felix itself is always 32-bit.

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$felixClsid = '{4948E973-2FE7-4E6b-914E-55417CCFDF22}'
$addinNames = 'WordAssist', 'ExcelAssist', 'PowerPointAssist'
$officeApps = [ordered]@{ Word = 'WordAssist'; Excel = 'ExcelAssist'; PowerPoint = 'PowerPointAssist' }
# The user's own hive. When UAC elevates to a different admin account, HKCU is
# that account's, so the relaunch passes the original user's SID.
$userRoot = if ($UserSid) { "Registry::HKEY_USERS\$UserSid" } else { 'HKCU:' }
$userClasses = "$userRoot\Software\Classes"

function Get-RegDefault([string]$path) {
    try { (Get-ItemProperty -LiteralPath $path -ErrorAction Stop).'(default)' } catch { $null }
}

# Names where a registered path lives: this repo's Debug or Release build, the installed copy, or something else.
function Get-Origin([string]$path) {
    if (-not $path) { return 'not registered' }
    $p = $path.Trim('"')
    if (-not (Test-Path -LiteralPath $p)) { $missing = ' (file missing)' } else { $missing = '' }
    if ($p -like "$repo\*") {
        if ($p -match '\\(Unicode_)?Debug\\') { return "repo Debug$missing" }
        if ($p -match '\\(Unicode_)?Release\\') { return "repo Release$missing" }
        return "repo$missing"
    }
    if ($p -like "$InstallDir\*") { return "installed$missing" }
    return "other$missing"
}

# Per-user COM keys (HKCU\Software\Classes) win over the machine-wide ones this
# script writes, so a stale per-user registration keeps Office on an old build.
# These are the keys Felix and its add-ins can leave there.
function Get-UserOverrideKeys {
    $keys = @()
    foreach ($progid in @('Felix.App', 'Felix.App.1') + ($addinNames | ForEach-Object { "$_.Connect"; "$_.Connect.1" })) {
        $keys += "$userClasses\$progid"
    }
    $keys += "$userClasses\CLSID\$felixClsid", "$userClasses\WOW6432Node\CLSID\$felixClsid"
    foreach ($name in $addinNames) {
        foreach ($root in 'HKLM:\SOFTWARE\Classes', $userClasses) {
            $clsid = Get-RegDefault "$root\$name.Connect\CLSID"
            if ($clsid) { $keys += "$userClasses\CLSID\$clsid", "$userClasses\WOW6432Node\CLSID\$clsid" }
        }
    }
    $keys | Sort-Object -Unique | Where-Object { Test-Path -LiteralPath $_ }
}

function Show-Status {
    ''
    'Felix registration now (what Windows uses for this user):'
    # Felix.exe is 32-bit; check the per-user view before the machine-wide one, as COM does.
    $felixPath = Get-RegDefault "$userClasses\WOW6432Node\CLSID\$felixClsid\LocalServer32"
    if (-not $felixPath) { $felixPath = Get-RegDefault "HKLM:\SOFTWARE\WOW6432Node\Classes\CLSID\$felixClsid\LocalServer32" }
    "  {0,-17} {1,-15} {2}" -f 'Felix.App', "[$(Get-Origin $felixPath)]", $felixPath
    foreach ($app in $officeApps.Keys) {
        $name = $officeApps[$app]
        $clsid = Get-RegDefault "Registry::HKEY_CLASSES_ROOT\$name.Connect\CLSID"
        $dll = $null
        if ($clsid) {
            # 64-bit view first, then the 32-bit view (32-bit Office).
            $dll = Get-RegDefault "Registry::HKEY_CLASSES_ROOT\CLSID\$clsid\InprocServer32"
            if (-not $dll) { $dll = Get-RegDefault "HKLM:\SOFTWARE\WOW6432Node\Classes\CLSID\$clsid\InprocServer32" }
        }
        $load = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Office\$app\Addins\$name.Connect" -ErrorAction SilentlyContinue).LoadBehavior
        $loadText = if ($null -ne $load) { "LoadBehavior=$load" } else { 'no Office entry' }
        "  {0,-17} {1,-15} {2}  ({3})" -f "$app add-in", "[$(Get-Origin $dll)]", $dll, $loadText
    }
    $overrides = @(Get-UserOverrideKeys)
    if ($overrides) {
        Write-Warning ("Per-user registrations override the machine-wide ones (registering again removes them):`n  " + ($overrides -join "`n  "))
    }
    # Office stops loading an add-in after it hangs or crashes, until you re-enable it.
    foreach ($app in $officeApps.Keys) {
        $key = "$userRoot\Software\Microsoft\Office\16.0\$app\Resiliency\DisabledItems"
        $item = Get-Item $key -ErrorAction SilentlyContinue
        if (-not $item) { continue }
        foreach ($v in $item.GetValueNames()) {
            $bytes = $item.GetValue($v)
            if ($bytes -is [byte[]] -and ([Text.Encoding]::Unicode.GetString($bytes) -match 'assist\.dll')) {
                Write-Warning "$app has disabled the Felix add-in. Re-enable it in $app : File > Options > Add-ins > Manage: Disabled Items > Go."
            }
        }
    }
    $office = Get-Process WINWORD, EXCEL, POWERPNT -ErrorAction SilentlyContinue
    if ($office) {
        Write-Warning ("Restart Office to pick up changes: " + (($office.ProcessName | Sort-Object -Unique) -join ', ') + ' running.')
    }
}

if ($Status) { Show-Status; return }

$admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin) {
    # Relaunch elevated with the same options, wait for it, and show its output here.
    'Registration writes to HKLM, so it needs administrator rights. Asking Windows to elevate (UAC prompt)...'
    $log = Join-Path $env:TEMP ("register-felix-{0}.log" -f [guid]::NewGuid())
    $relaunch = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"",
                  '-Configuration', $Configuration, '-Platform', $Platform,
                  '-InstallDir', "`"$InstallDir`"", '-LogFile', "`"$log`"",
                  '-UserSid', [Security.Principal.WindowsIdentity]::GetCurrent().User.Value)
    if ($Unregister) { $relaunch += '-Unregister' }
    try { $p = Start-Process powershell.exe -Verb RunAs -ArgumentList $relaunch -WindowStyle Hidden -Wait -PassThru }
    catch { throw 'Elevation was cancelled, so nothing changed. Run this from an Administrator PowerShell instead.' }
    if (Test-Path $log) { Get-Content $log; Remove-Item $log -ErrorAction SilentlyContinue }
    if ($p.ExitCode -ne 0) { Write-Warning "The elevated registration failed (exit code $($p.ExitCode))." }
    Show-Status
    exit $p.ExitCode
}

# From here on we are elevated. Output goes through Write-Output (warnings
# prefixed by hand), so it can be captured to $LogFile for the caller to show.
function Write-Problem([string]$message) { "WARNING: $message"; $script:exitCode = 1 }

$script:exitCode = 0
function Invoke-Registration {
  try {
    if ($Configuration -eq 'Installed') {
        $felix = Join-Path $InstallDir 'Felix.exe'
        $addins = $addinNames | ForEach-Object { Join-Path $InstallDir "$_.dll" }
    } else {
        $felixDir = if ($Configuration -eq 'Release') { 'Unicode_Release' } else { 'Unicode_Debug' }
        $felix = Join-Path $repo "Felix\$felixDir\Felix.exe"
        $outDir = if ($Platform -eq 'x64') { "x64\$Configuration" } else { $Configuration }
        $addins = $addinNames | ForEach-Object { Join-Path $repo "$_\$_\$outDir\$_.dll" }
    }
    foreach ($f in @($felix) + $addins) {
        if (-not (Test-Path $f)) {
            if ($Configuration -eq 'Installed') { throw "Not found: $f. Is Felix installed? Pass -InstallDir if it is somewhere else." }
            throw "Not built yet: $f"
        }
    }

    if (Get-Process Felix -ErrorAction SilentlyContinue) {
        "WARNING: Felix is running. Close it so the next start uses the newly registered Felix.exe."
    }

    # Remove stale per-user registrations first, so the machine-wide one written below is the one Windows uses.
    foreach ($key in @(Get-UserOverrideKeys)) {
        "Removing per-user registration: $key"
        Remove-Item -LiteralPath $key -Recurse -Force
    }

    $verb = if ($Unregister) { 'Unregistering' } else { 'Registering' }

    "$verb Felix COM server: $felix"
    $p = Start-Process $felix -ArgumentList $(if ($Unregister) { '/UnregServer' } else { '/RegServer' }) -Wait -PassThru
    if ($p.ExitCode -ne 0) { Write-Problem "Felix.exe exited with $($p.ExitCode); see %LOCALAPPDATA%\Felix\logs\felix.log" }

    # 64-bit regsvr32 for x64 DLLs; the 32-bit one for Win32 DLLs.
    $regsvr = if ($Platform -eq 'x64') { "$env:WINDIR\System32\regsvr32.exe" } else { "$env:WINDIR\SysWOW64\regsvr32.exe" }
    foreach ($dll in $addins) {
        "$verb add-in: $dll"
        $regArgs = @('/s') + $(if ($Unregister) { @('/u') } else { @() }) + @("`"$dll`"")
        $p = Start-Process $regsvr -ArgumentList $regArgs -Wait -PassThru
        if ($p.ExitCode -ne 0) { Write-Problem "regsvr32 exited with $($p.ExitCode) for $dll" }
    }
  } catch {
    Write-Problem $_.Exception.Message
  }
}

if ($LogFile) {
    Invoke-Registration *>&1 | ForEach-Object { "$_" } | Out-File -FilePath $LogFile -Encoding utf8
} else {
    Invoke-Registration
    Show-Status
}
exit $script:exitCode
