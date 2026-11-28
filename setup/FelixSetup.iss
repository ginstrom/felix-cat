; Felix installer (Inno Setup 6)
;
; Installs Felix (32-bit) and its Word, Excel and PowerPoint add-ins (64-bit,
; for 64-bit Office), the Visual C++ runtimes they need, and registers Felix's
; COM server and the add-ins.
;
; Build the binaries first (tools\build-installer.ps1 does everything):
;   Felix\Felix.vcxproj                       Unicode_Release | Win32
;   Felix\EngResource, Felix\JpnResource      Unicode_Release | Win32
;   <App>Assist\<App>Assist\<App>Assist.vcxproj  Release | x64
; then compile this script:  ISCC setup\FelixSetup.iss
;
; The uninstaller removes only what this installer put in place. It leaves
; each user's %LOCALAPPDATA%\Felix (settings, customized templates, logs) and
; their memories and glossaries alone.

; Each of these can be set from the command line, e.g.
;   ISCC /DAppVersion=3.0.2 /DVcRedist=<folder> setup\FelixSetup.iss
; (tools\build-installer.ps1 and the GitHub Actions workflows do this.)
#ifndef AppVersion
  #define AppVersion "3.0.1"
#endif
; Repo root, which holds the built binaries.
#ifndef SourceDir
  #define SourceDir ".."
#endif
#define Root SourceDir
#define FelixBin Root + "\Felix\Unicode_Release"
; Folder with vc_redist.x86.exe and vc_redist.x64.exe (Microsoft-signed).
#ifndef VcRedist
  #define VcRedist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\v145"
#endif

[Setup]
AppId={{98802423-6787-4122-BCEC-08CC2E12F18E}
AppName=Felix
AppVersion={#AppVersion}
AppVerName=Felix {#AppVersion}
AppPublisher=Ryan Ginstrom
AppPublisherURL=https://github.com/ginstrom/felix-cat
AppSupportURL=https://github.com/ginstrom/felix-cat/issues
AppUpdatesURL=https://github.com/ginstrom/felix-cat/releases
AppCopyright=Copyright (c) 1999-2026 Ryan Ginstrom
VersionInfoVersion={#AppVersion}.0
VersionInfoDescription=Felix Setup

; Felix is 32-bit, but the Office add-ins are 64-bit, so this installs in
; 64-bit mode and requires 64-bit Windows.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

DefaultDirName={autopf}\Felix
DefaultGroupName=Felix
UninstallDisplayName=Felix
UninstallDisplayIcon={app}\Felix.exe
PrivilegesRequired=admin
ShowLanguageDialog=yes
; Close Felix and Office if they're running, so files can be replaced.
CloseApplications=yes

OutputDir=Output
OutputBaseFilename=Felix_Setup_{#AppVersion}

#ifdef SIGN
; Code signing: tools\build-installer.ps1 defines SIGN and the "felixsign"
; sign tool (tools\sign-file.ps1) when signing.local.json exists. Inno Setup
; then signs the installer and the uninstaller.
SignTool=felixsign
SignedUninstaller=yes
#endif
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: en; MessagesFile: compiler:Default.isl; LicenseFile: {#Root}\LICENSE.txt
Name: jp; MessagesFile: compiler:Languages\Japanese.isl; LicenseFile: {#Root}\LICENSE_JP.txt

[Types]
Name: full; Description: {cm:TypeFull}
Name: custom; Description: {cm:TypeCustom}; Flags: iscustom

[Components]
Name: main; Description: Felix; Types: full custom; Flags: fixed
Name: word; Description: {cm:WordAddin}; Types: full
Name: excel; Description: {cm:ExcelAddin}; Types: full
Name: powerpoint; Description: {cm:PowerPointAddin}; Types: full

[CustomMessages]
en.TypeFull=Felix and all Office add-ins
en.TypeCustom=Custom
en.WordAddin=Word add-in (64-bit Office)
en.ExcelAddin=Excel add-in (64-bit Office)
en.PowerPointAddin=PowerPoint add-in (64-bit Office)
en.RuntimeFailed=Could not install the Visual C++ runtime (%1): %2
en.RegisteringFelix=Registering Felix...
jp.TypeFull=Felix と Office アドイン (すべて)
jp.TypeCustom=カスタム
jp.WordAddin=Word アドイン (64 ビット Office)
jp.ExcelAddin=Excel アドイン (64 ビット Office)
jp.PowerPointAddin=PowerPoint アドイン (64 ビット Office)
jp.RuntimeFailed=Visual C++ ランタイム (%1) をインストールできませんでした: %2
jp.RegisteringFelix=Felix を登録しています...

[Files]
; Felix (32-bit)
Source: {#FelixBin}\Felix.exe; DestDir: {app}; Components: main; Flags: ignoreversion 32bit
; Felix's type library: Felix /RegServer registers this file, next to the exe.
Source: {#FelixBin}\Felix.tlb; DestDir: {app}; Components: main; Flags: ignoreversion
Source: {#FelixBin}\lang\EngResource.dll; DestDir: {app}\lang; Components: main; Flags: ignoreversion 32bit
Source: {#FelixBin}\lang\JpnResource.dll; DestDir: {app}\lang; Components: main; Flags: ignoreversion 32bit
Source: {#FelixBin}\SciLexer.dll; DestDir: {app}; Components: main; Flags: ignoreversion 32bit
; UI templates. Felix uses %LOCALAPPDATA%\Felix\html first (for customized
; templates) and falls back to this copy.
Source: {#FelixBin}\html\*; DestDir: {app}\html; Components: main; Flags: ignoreversion recursesubdirs; Excludes: "Thumbs.db,*.komodoproject,*.kpf"
Source: {#Root}\LICENSE.txt; DestDir: {app}; Components: main; Flags: ignoreversion
Source: {#Root}\LICENSE_JP.txt; DestDir: {app}; Components: main; Flags: ignoreversion

; Office add-ins (64-bit). regserver registers them with the 64-bit regsvr32
; and unregisters them on uninstall.
Source: {#Root}\WordAssist\WordAssist\x64\Release\WordAssist.dll; DestDir: {app}; Components: word; Flags: ignoreversion regserver 64bit
Source: {#Root}\ExcelAssist\ExcelAssist\x64\Release\ExcelAssist.dll; DestDir: {app}; Components: excel; Flags: ignoreversion regserver 64bit
Source: {#Root}\PowerPointAssist\PowerPointAssist\x64\Release\PowerPointAssist.dll; DestDir: {app}; Components: powerpoint; Flags: ignoreversion regserver 64bit

; Visual C++ runtimes (v14): x86 for Felix, x64 for the add-ins. Not copied
; with the other files: PrepareToInstall ([Code]) installs them first.
Source: {#VcRedist}\vc_redist.x86.exe; Flags: dontcopy
Source: {#VcRedist}\vc_redist.x64.exe; Flags: dontcopy

[Registry]
; File types: Felix memories, glossaries and preferences.
Root: HKA; Subkey: Software\Classes\.ftm; ValueType: string; ValueData: FelixTMFile; Flags: uninsdeletevalue
Root: HKA; Subkey: Software\Classes\FelixTMFile; ValueType: string; ValueData: Felix Translation Memory; Flags: uninsdeletekey
Root: HKA; Subkey: Software\Classes\FelixTMFile\DefaultIcon; ValueType: string; ValueData: """{app}\Felix.exe"",0"
Root: HKA; Subkey: Software\Classes\FelixTMFile\shell\open\command; ValueType: string; ValueData: """{app}\Felix.exe"" ""%1"""
Root: HKA; Subkey: Software\Classes\.fgloss; ValueType: string; ValueData: FelixGlossaryFile; Flags: uninsdeletevalue
Root: HKA; Subkey: Software\Classes\FelixGlossaryFile; ValueType: string; ValueData: Felix Glossary; Flags: uninsdeletekey
Root: HKA; Subkey: Software\Classes\FelixGlossaryFile\DefaultIcon; ValueType: string; ValueData: """{app}\Felix.exe"",0"
Root: HKA; Subkey: Software\Classes\FelixGlossaryFile\shell\open\command; ValueType: string; ValueData: """{app}\Felix.exe"" ""%1"""
Root: HKA; Subkey: Software\Classes\.fprefs; ValueType: string; ValueData: FelixPreferenceFile; Flags: uninsdeletevalue
Root: HKA; Subkey: Software\Classes\FelixPreferenceFile; ValueType: string; ValueData: Felix Preferences; Flags: uninsdeletekey
Root: HKA; Subkey: Software\Classes\FelixPreferenceFile\shell\open\command; ValueType: string; ValueData: """{app}\Felix.exe"" ""%1"""

[Icons]
Name: {group}\Felix; Filename: {app}\Felix.exe
Name: {group}\{cm:UninstallProgram,Felix}; Filename: {uninstallexe}

[Run]
; Felix's COM server (Felix.App), which the add-ins use to reach Felix.
Filename: {app}\Felix.exe; Parameters: /RegServer; StatusMsg: {cm:RegisteringFelix}; Flags: waituntilterminated
Filename: {app}\Felix.exe; Description: {cm:LaunchProgram,Felix}; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: {app}\Felix.exe; Parameters: /UnregServer; RunOnceId: UnregisterFelix; Flags: waituntilterminated

[Code]
// Runs one Visual C++ runtime installer from the setup's embedded files.
// Returns '' on success, or an error message.
function InstallVcRedist(const FileName: String): String;
var
  ResultCode: Integer;
begin
  Result := '';
  ExtractTemporaryFile(FileName);
  if not Exec(ExpandConstant('{tmp}\') + FileName, '/install /quiet /norestart', '',
              SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    Result := FmtMessage(CustomMessage('RuntimeFailed'), [FileName, SysErrorMessage(ResultCode)])
  // 0 = installed, 1638 = a newer version is already installed,
  // 3010 = installed, restart pending (the files are usable now).
  else if (ResultCode <> 0) and (ResultCode <> 1638) and (ResultCode <> 3010) then
    Result := FmtMessage(CustomMessage('RuntimeFailed'), [FileName, IntToStr(ResultCode)]);
end;

// The Office add-ins are registered (regsvr32) as soon as their files are
// copied, before [Run], and they can't load without the Visual C++ runtime:
// on a PC without it, registration failed and setup rolled everything back.
// So install both runtimes before any file is copied.
function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := InstallVcRedist('vc_redist.x86.exe');
  if Result = '' then
    Result := InstallVcRedist('vc_redist.x64.exe');
end;
