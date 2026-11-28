# Code-signs files for a Felix release, using the settings in signing.local.json.
#
#   powershell -ExecutionPolicy Bypass -File tools\sign-file.ps1 -Path Felix\Unicode_Release\Felix.exe
#
# tools\build-installer.ps1 calls this for Felix's binaries, and Inno Setup
# calls it for the installer and uninstaller. The settings file is not in git:
# copy tools\signing.example.json to signing.local.json at the repo root (or
# pass -Config) and fill in one method. See BUILDING.md, "Signing locally".

param(
    [Parameter(Mandatory = $true)] [string[]]$Path,
    [string]$Config = (Join-Path (Split-Path $PSScriptRoot -Parent) 'signing.local.json')
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Config)) { throw "No signing settings: $Config" }
$settings = Get-Content $Config -Raw | ConvertFrom-Json

function Find-SignTool {
    if ($settings.signtool) { return $settings.signtool }
    $kits = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
    $found = Get-ChildItem $kits -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^10\.' -and (Test-Path (Join-Path $_.FullName 'x64\signtool.exe')) } |
        Sort-Object { [version]$_.Name } | Select-Object -Last 1
    if (-not $found) { throw 'signtool.exe not found: install the Windows SDK (part of the VS C++ workload).' }
    return (Join-Path $found.FullName 'x64\signtool.exe')
}

$description = if ($settings.description) { $settings.description } else { 'Felix' }
$url = if ($settings.url) { $settings.url } else { 'https://github.com/ginstrom/felix-cat' }

foreach ($file in $Path) {
    $file = (Resolve-Path $file).Path
    switch ($settings.method) {
        'certificate' {
            # A certificate in the Windows certificate store (CurrentUser\My or
            # LocalMachine\My), including tokens and cloud signing services
            # that expose their certificate through Windows.
            if (-not $settings.certificate.thumbprint) { throw 'certificate.thumbprint is not set' }
            $ts = if ($settings.certificate.timestampUrl) { $settings.certificate.timestampUrl } else { 'http://timestamp.digicert.com' }
            $signArgs = @('sign', '/fd', 'SHA256', '/sha1', $settings.certificate.thumbprint,
                      '/tr', $ts, '/td', 'SHA256', '/d', $description, '/du', $url, $file)
            & (Find-SignTool) @signArgs
        }
        'artifact-signing' {
            # Microsoft Azure Artifact Signing (formerly Trusted Signing), through
            # its signtool plug-in (Azure.CodeSigning.Dlib.dll) and a metadata
            # file naming the endpoint, account and certificate profile.
            $a = $settings.artifactSigning
            if (-not $a.dlib -or -not $a.metadata) { throw 'artifactSigning.dlib and artifactSigning.metadata must be set' }
            $ts = if ($a.timestampUrl) { $a.timestampUrl } else { 'http://timestamp.acs.microsoft.com' }
            $signArgs = @('sign', '/fd', 'SHA256', '/tr', $ts, '/td', 'SHA256',
                      '/dlib', $a.dlib, '/dmdf', $a.metadata, '/d', $description, '/du', $url, $file)
            & (Find-SignTool) @signArgs
        }
        'command' {
            # Any other signing tool. {file} is replaced by the quoted file path.
            if (-not $settings.command) { throw 'command is not set' }
            $cmd = $settings.command.Replace('{file}', '"' + $file + '"')
            & cmd.exe /d /s /c "`"$cmd`""
        }
        default { throw "Unknown signing method '$($settings.method)' (use certificate, artifact-signing or command)" }
    }
    if ($LASTEXITCODE -ne 0) { throw "Signing failed ($LASTEXITCODE): $file" }

    # Check the signature chains to a trusted root (skipped for custom commands,
    # which may be dry runs or sign by other means).
    if ($settings.method -ne 'command' -and $settings.verify -ne $false) {
        & (Find-SignTool) verify /pa /q $file
        if ($LASTEXITCODE -ne 0) { throw "Signature check failed: $file" }
    }
    "Signed: $file"
}
