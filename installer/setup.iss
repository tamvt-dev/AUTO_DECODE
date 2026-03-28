; Auto Decoder Pro - Inno Setup Script
[Setup]
AppName=Auto Decoder Pro
AppVersion=2.0.0
DefaultDirName={autopf}\Auto Decoder Pro
DefaultGroupName=Auto Decoder Pro
OutputDir=..\output
OutputBaseFilename=AutoDecoderPro_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "vietnamese"; MessagesFile: "compiler:Languages\Vietnamese.isl"
[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\build\bin\auto_decoder_pro.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\auto_decoder.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\ui\resources\*"; DestDir: "{app}\resources"; Flags: ignoreversion recursesubdirs
Source: "license.rtf"; DestDir: "{app}"; Flags: ignoreversion
; 🔥 THÊM DÒNG NÀY (CỰC KỲ QUAN TRỌNG)
Source: "..\build\bin\*.dll"; DestDir: "{app}"; Flags: ignoreversion

Source: "..\ui\resources\*"; DestDir: "{app}\resources"; Flags: ignoreversion recursesubdirs
Source: "license.rtf"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Auto Decoder Pro"; Filename: "{app}\auto_decoder_pro.exe"
Name: "{group}\Auto Decoder CLI"; Filename: "{app}\auto_decoder.exe"
Name: "{group}\Uninstall Auto Decoder Pro"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Auto Decoder Pro"; Filename: "{app}\auto_decoder_pro.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\auto_decoder_pro.exe"; Description: "{cm:LaunchProgram,Auto Decoder Pro}"; Flags: postinstall nowait skipifsilent