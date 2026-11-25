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

#define AppVersion "3.0"
#define Root ".."
#define FelixBin Root + "\Felix\Unicode_Release"
#define VcRedist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\v145"

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
VersionInfoVersion={#AppVersion}.0.0
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
en.InstallingRuntime=Installing the Visual C++ runtime...
en.RegisteringFelix=Registering Felix...
jp.TypeFull=Felix と Office アドイン (すべて)
jp.TypeCustom=カスタム
jp.WordAddin=Word アドイン (64 ビット Office)
jp.ExcelAddin=Excel アドイン (64 ビット Office)
jp.PowerPointAddin=PowerPoint アドイン (64 ビット Office)
jp.InstallingRuntime=Visual C++ ランタイムをインストールしています...
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

; Visual C++ runtimes (v14): x86 for Felix, x64 for the add-ins.
Source: {#VcRedist}\vc_redist.x86.exe; DestDir: {tmp}; Flags: deleteafterinstall
Source: {#VcRedist}\vc_redist.x64.exe; DestDir: {tmp}; Flags: deleteafterinstall

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
Filename: {tmp}\vc_redist.x86.exe; Parameters: "/install /quiet /norestart"; StatusMsg: {cm:InstallingRuntime}; Flags: waituntilterminated
Filename: {tmp}\vc_redist.x64.exe; Parameters: "/install /quiet /norestart"; StatusMsg: {cm:InstallingRuntime}; Flags: waituntilterminated
; Felix's COM server (Felix.App), which the add-ins use to reach Felix.
Filename: {app}\Felix.exe; Parameters: /RegServer; StatusMsg: {cm:RegisteringFelix}; Flags: waituntilterminated
Filename: {app}\Felix.exe; Description: {cm:LaunchProgram,Felix}; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: {app}\Felix.exe; Parameters: /UnregServer; RunOnceId: UnregisterFelix; Flags: waituntilterminated
