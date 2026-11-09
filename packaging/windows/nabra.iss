; Inno Setup script for Nabra (built by CI: iscc /DAppVersion=x.y.z /DSrcDir=... nabra.iss)
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef SrcDir
  #define SrcDir "..\..\build\Nabra_artefacts\Release"
#endif

[Setup]
AppId={{6E3B1F2A-8C4D-4E7B-9A51-4E61B7A0C0DE}
AppName=Nabra
AppVersion={#AppVersion}
AppPublisher=Tapestry
AppPublisherURL=https://github.com/femifleming/nabra
AppSupportURL=https://github.com/femifleming/nabra/issues
DefaultDirName={autopf}\Nabra
DefaultGroupName=Nabra
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputBaseFilename=Nabra-{#AppVersion}-Windows-Setup
SetupIconFile=nabra.ico
UninstallDisplayIcon={app}\Nabra.exe
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Shortcuts:"
Name: "vst3"; Description: "Install the VST3 plug-in (C:\Program Files\Common Files\VST3)"; GroupDescription: "Plug-ins:"

[Files]
Source: "{#SrcDir}\Standalone\Nabra.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcDir}\VST3\Nabra.vst3\*"; DestDir: "{commoncf64}\VST3\Nabra.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: vst3
Source: "..\..\README.md"; DestDir: "{app}"; Flags: isreadme
Source: "..\..\docs\HELP.md"; DestDir: "{app}"

[Icons]
Name: "{group}\Nabra"; Filename: "{app}\Nabra.exe"
Name: "{group}\Nabra help"; Filename: "https://github.com/femifleming/nabra/blob/main/docs/HELP.md"
Name: "{group}\Uninstall Nabra"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Nabra"; Filename: "{app}\Nabra.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Nabra.exe"; Description: "Launch Nabra"; Flags: nowait postinstall skipifsilent
