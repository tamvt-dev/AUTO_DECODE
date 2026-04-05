; HyperDecode (Qt) - Inno Setup Script

[Setup]
AppName=HyperDecode
AppVersion=2.0.0
AppPublisher=HyperDecode Team
DefaultDirName={autopf}\HyperDecode
DefaultGroupName=HyperDecode
OutputDir=..\output
OutputBaseFilename=HyperDecode_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
SetupIconFile=..\Qt\icons\app.ico
VersionInfoVersion=2.0.0.0
VersionInfoCompany=HyperDecode Team
VersionInfoDescription=HyperDecode Beta 2.0 Qt Desktop Installer

; Optional signing hook.
; Replace the SignTool command below with your real signtool.exe path, certificate,
; timestamp server, and password or certificate store settings when ready.
; Example usage in [Files] or output signing:
;   SignTool=mysigntool $f
;
; SignTool=mysigntool $f

[SignTools]
; Name: "mysigntool"; Command: """C:\Path\To\signtool.exe"" sign /f ""C:\Path\To\certificate.pfx"" /p ""YOUR_PASSWORD"" /tr ""http://timestamp.digicert.com"" /td sha256 /fd sha256 ""$f"""

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\Qt\release\HyperDecode.exe"; DestDir: "{app}"; Flags: ignoreversion
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
Name: "{group}\HyperDecode"; Filename: "{app}\HyperDecode.exe"; IconFilename: "{app}\icons\app.ico"
Name: "{group}\Uninstall HyperDecode"; Filename: "{uninstallexe}"
Name: "{autodesktop}\HyperDecode"; Filename: "{app}\HyperDecode.exe"; IconFilename: "{app}\icons\app.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\HyperDecode.exe"; Description: "{cm:LaunchProgram,HyperDecode}"; Flags: postinstall nowait skipifsilent
