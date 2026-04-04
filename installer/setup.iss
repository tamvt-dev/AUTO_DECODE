; Auto Decoder Pro (Qt) - Inno Setup Script

[Setup]
AppName=Auto Decoder Pro
AppVersion=2.0.0
DefaultDirName={autopf}\Auto Decoder Pro
DefaultGroupName=Auto Decoder Pro
OutputDir=..\output
OutputBaseFilename=AutoDecoderPro_Qt_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
SetupIconFile=..\Qt\icons\app.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\Qt\release\auto_decoder_qt.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Qt\release\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Qt\release\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Qt\release\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Qt\release\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Qt\release\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Qt\release\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Qt\release\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Qt\release\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Qt\release\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Qt\release\share\*"; DestDir: "{app}\share"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "..\Qt\icons\app.ico"; DestDir: "{app}\icons"; Flags: ignoreversion
Source: "..\Qt\icons\app.png"; DestDir: "{app}\icons"; Flags: ignoreversion
Source: "license.rtf"; DestDir: "{app}"; Flags: ignoreversion
Source: "readme.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Auto Decoder Pro (Qt)"; Filename: "{app}\auto_decoder_qt.exe"; IconFilename: "{app}\icons\app.ico"
Name: "{group}\Uninstall Auto Decoder Pro"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Auto Decoder Pro"; Filename: "{app}\auto_decoder_qt.exe"; IconFilename: "{app}\icons\app.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\auto_decoder_qt.exe"; Description: "{cm:LaunchProgram,Auto Decoder Pro}"; Flags: postinstall nowait skipifsilent
