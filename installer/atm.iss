#define MyAppName "ATM Transmittance"
#define MyAppVersion "1.0.0"
#define MyAppExeName "atm_gui_qt.exe"

[Setup]
AppId={{D6B8A4B5-AC49-4A7F-8C21-3E0C8D74F4A1}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\ATM
DefaultGroupName=ATM
OutputDir=.
OutputBaseFilename=ATM_Setup
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\qt_gui\build\deploy\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\ATM Transmittance"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\卸载 ATM"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "运行 ATM"; Flags: nowait postinstall skipifsilent
