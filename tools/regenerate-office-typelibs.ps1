# Regenerates the Office type library headers in common\office_typelibs\.
#
#   powershell -ExecutionPolicy Bypass -File tools\regenerate-office-typelibs.ps1
#
# Felix and the Office add-ins used to #import the Office type libraries
# straight from the registry, so Office had to be installed to build them.
# They now #include headers that this script generated once from those same
# #import directives, so the build (and CI) needs no Office.
#
# Needs: Visual Studio 2026 (C++ workload) and Microsoft Office (Word, Excel,
# PowerPoint). Run it after changing one of the #import directives below, or
# to pick up a newer Office. The #import lines here must match the attributes
# the code was written against: they decide the namespaces and renamed names.
#
# The compiler writes a .tlh (declarations) and a .tli (inline wrapper
# methods) per type library. Current compilers link the two with
# "#pragma start_map_region" and an absolute path; the script removes that
# mapping and makes the .tlh #include its .tli by relative path instead.

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$outRoot = Join-Path $repo 'common\office_typelibs'

$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
if (-not $vs) { throw 'Visual Studio not found.' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'

# Office's own libraries (MSO.DLL, VBE6EXT.OLB, MSWORD.OLB, EXCEL.EXE,
# MSPPT.OLB) and the Add-in Designer (MSADDNDR.DLL). Each set is compiled as
# one translation unit; a set's headers go to <platform>\<set>\.
$sets = [ordered]@{
    # Felix (Word and Excel export), Word Assist and Excel Assist.
    'msoffice' = @'
#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" \
	rename_namespace("MSOffice") rename("RGB", "OfficeRGB") rename( "SearchPath", "OfficeSearchPath" ) rename("DocumentProperties", "OfficeDocumentProperties") named_guids
#import "libid:0002E157-0000-0000-C000-000000000046" \
	rename_namespace("VBE6")
#import "libid:00020905-0000-0000-C000-000000000046" \
	rename_namespace("MSWord") rename("FindText", "WordFindText") rename("ExitWindows","MSWordExitWindows") \
	rename( "RGB", "WordRBG" ) rename( "ReplaceText", "WordReplaceText" )
#import "libid:00020813-0000-0000-C000-000000000046" \
	exclude( "IFont" ) exclude( "IPicture" ) \
	rename("DialogBox", "DialogBoxForExcel") rename( "RGB", "RGBForExcel") \
	rename( "CopyFile", "CopyFileForExcel" ) rename( "ReplaceText", "ReplaceTextForExcel" )
'@
    # PowerPoint Assist.
    'powerpoint' = @'
#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" \
	rename_namespace("Office") rename("RGB", "OfficeRGB") \
	rename( "DocumentProperties", "OfficeDocumentProperties" ) rename( "SearchPath", "OfficeSearchPath" )
#import "libid:0002E157-0000-0000-C000-000000000046" \
	rename_namespace("VBE6")
#import "libid:91493440-5A91-11CF-8700-00AA0060263B" \
	rename_namespace("PowerPoint"), rename( "RGB", "PowerPointRGB")
#import "libid:00020813-0000-0000-C000-000000000046" \
	rename_namespace("Excel") rename("Range","ExcelRange") rename("DialogBox","ExcelDialogBox") rename("RGB","ExcelRGB") \
	exclude("IFont","IPicture") rename ("CopyFile", "ExcelCopyFile" ) rename ("ReplaceText", "ExcelReplaceText" )
'@
    # IDTExtensibility2, which every add-in implements.
    'addin' = @'
#import "libid:AC0714F2-3D04-11D1-AE7D-00A0C90F26F4" version("1.0") lcid("0") raw_interfaces_only named_guids
'@
}

# One set of headers serves both platforms. A 64-bit MSBuild build reads the
# same 32-bit ("Win32") type library registrations as a 32-bit one, so these
# are the headers the x64 add-ins were always built against. (A 64-bit
# command-line cl reads the win64 registrations instead, which give different
# Word and Add-in Designer headers that the add-in code doesn't compile with.)
$vcvarsArch = 'x86'

$utf8Bom = New-Object System.Text.UTF8Encoding($true)
$work = Join-Path ([IO.Path]::GetTempPath()) ("felix-typelibs-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $work | Out-Null

function Convert-Header([string]$file, [string]$label) {
    $lines = [IO.File]::ReadAllLines($file)
    $out = New-Object System.Collections.Generic.List[string]
    $inMap = $false
    foreach ($l in $lines) {
        if ($l -match '^// Created by Microsoft') { continue }
        # The generated file's own absolute path.
        if ($l -match '^// [A-Za-z]:\\') { $out.Add("// common\office_typelibs\$label"); continue }
        if ($l -match '^// Function implementation mapping') { $inMap = $true; continue }
        if ($inMap) { if ($l -match '^#pragma stop_map_region') { $inMap = $false }; continue }
        if ($l -match '^#pragma implementation_key\(') { continue }
        # Some libraries (VBE6EXT) get the older layout: the .tlh includes its
        # .tli by absolute path, inside the namespace. Make that relative.
        if ($l -match '^#include "[A-Za-z]:\\.*\\([^\\"]+\.tli)"$') { $out.Add("#include `"$($Matches[1])`""); continue }
        $out.Add($l)
    }
    # Drop the trailing blank lines left where the mapping was.
    while ($out.Count -and $out[$out.Count - 1].Trim() -eq '' ) { $out.RemoveAt($out.Count - 1) }
    return ,$out   # the comma keeps PowerShell from unrolling the list
}

try {
    foreach ($set in $sets.Keys) {
        $dir = Join-Path $work $set
        New-Item -ItemType Directory $dir | Out-Null
        $cpp = Join-Path $dir 'typelibs.cpp'
        [IO.File]::WriteAllText($cpp, "#include <comdef.h>`r`n" + $sets[$set] + "`r`n")
        "Generating $set"
        # Merge stderr inside cmd: Windows PowerShell turns a native
        # command's stderr into errors, which $ErrorActionPreference stops on.
        $log = & cmd /c "`"$vcvars`" $vcvarsArch >nul 2>&1 && cl /nologo /c /EHsc /Zc:wchar_t- /Fo`"$dir\\`" `"$cpp`" 2>&1"
        if ($LASTEXITCODE -ne 0) { $log; throw "cl failed for $set" }

        $dest = Join-Path $outRoot $set
        if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
        New-Item -ItemType Directory $dest | Out-Null
        foreach ($tlh in Get-ChildItem $dir -Filter *.tlh) {
            $base = $tlh.BaseName
            $lines = Convert-Header $tlh.FullName "$set\$base.tlh"
            $tli = Join-Path $dir "$base.tli"
            if (Test-Path $tli) {
                $tliInclude = "#include `"$base.tli`""
                if (-not $lines.Contains($tliInclude)) {
                    $lines.Add('')
                    $lines.Add('//')
                    $lines.Add('// Wrapper method implementations')
                    $lines.Add('//')
                    $lines.Add('')
                    $lines.Add($tliInclude)
                }
                $tliLines = Convert-Header $tli "$set\$base.tli"
                [IO.File]::WriteAllText((Join-Path $dest "$base.tli"), (($tliLines -join "`r`n") + "`r`n"), $utf8Bom)
            }
            [IO.File]::WriteAllText((Join-Path $dest "$base.tlh"), (($lines -join "`r`n") + "`r`n"), $utf8Bom)
        }
    }
    "Done: $outRoot"
}
finally {
    Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
}
