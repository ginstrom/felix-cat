<#
.SYNOPSIS
  Writes SHA256SUMS.txt and release-notes.md next to the built installer.

.DESCRIPTION
  Input:  <OutDir>\Felix_Setup_<Version>.exe (throws if missing).
  Output: <OutDir>\SHA256SUMS.txt   "<sha256>  <file name>" per installer, LF, UTF-8 no BOM
          <OutDir>\release-notes.md setup\release-notes-<Version>.md (or "Felix <Version>"),
                                    then .github\release-notes-install.md with {{VERSION}} filled in.
  -Signed removes the <!-- unsigned --> ... <!-- /unsigned --> block from the notes.
  A relative -OutDir is resolved against the repository root.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Version,
    [switch]$Signed,
    [string]$OutDir = 'setup\Output'
)
$ErrorActionPreference = 'Stop'

if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw "Version must look like 1.2.3, got '$Version'." }

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not [IO.Path]::IsPathRooted($OutDir)) { $OutDir = Join-Path $root $OutDir }

$installerName = "Felix_Setup_$Version.exe"
$installer = Join-Path $OutDir $installerName
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) { throw "Installer not found: $installer" }

$utf8 = New-Object Text.UTF8Encoding $false
function Write-LfFile([string]$path, [string]$text) {
    [IO.File]::WriteAllText($path, ($text -replace "`r`n", "`n"), $utf8)
}
function Read-LfFile([string]$path) {
    ([IO.File]::ReadAllText($path, $utf8)) -replace "`r`n", "`n"
}

# Everything is read before anything is written, so a failure leaves no partial output.
$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()

$templatePath = Join-Path $root '.github\release-notes-install.md'
if (-not (Test-Path -LiteralPath $templatePath -PathType Leaf)) { throw "Template not found: $templatePath" }
$install = (Read-LfFile $templatePath).Replace('{{VERSION}}', $Version)
if ($Signed) {
    $install = [regex]::Replace($install, '(?s)<!-- unsigned -->.*?<!-- /unsigned -->\n*', '')
}

$notesPath = Join-Path $root "setup\release-notes-$Version.md"
if (Test-Path -LiteralPath $notesPath -PathType Leaf) {
    $notes = (Read-LfFile $notesPath).TrimEnd("`n")
} else {
    $notes = "Felix $Version"
}

$sumsPath = Join-Path $OutDir 'SHA256SUMS.txt'
$releaseNotesPath = Join-Path $OutDir 'release-notes.md'
Write-LfFile $sumsPath "$hash  $installerName`n"
Write-LfFile $releaseNotesPath ($notes + "`n`n" + $install.TrimStart("`n"))

Write-Host "Wrote $sumsPath"
Write-Host "Wrote $releaseNotesPath"
