#define MyAppName "wordSnap"
#define MyAppPublisher "wordSnap"
#define MyAppExeName "wordSnapV1.exe"

#ifndef AppVersion
  #define AppVersion "0.0.0-local"
#endif

#ifndef SourceDir
  #define SourceDir "..\\..\\dist\\wordSnap-local\\staging"
#endif

#ifndef OutputDir
  #define OutputDir "..\\..\\dist\\wordSnap-local"
#endif

#ifndef OutputBaseFilename
  #define OutputBaseFilename "wordSnap-local-setup"
#endif

#ifndef AppIconIco
  #define AppIconIco AddBackslash(SourceDir) + "wordSnapLogo.ico"
#endif

[Setup]
AppId={{D5F8585C-6D2D-4CEA-BC77-6C0DAA4EFCA6}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
SetupIconFile={#AppIconIco}
UninstallDisplayIcon={app}\wordSnapLogo.ico
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseFilename}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\wordSnapLogo.ico"; WorkingDir: "{app}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\wordSnapLogo.ico"; Tasks: desktopicon; WorkingDir: "{app}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
