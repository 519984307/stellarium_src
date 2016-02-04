; Stellarium installer
; Run "make install" first to generate the executable and translation files.
; @ISS_AUTOGENERATED_WARNING@

[Setup]
@ISS_ARCHITECTURE_SPECIFIC@
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
AppVersion=@PACKAGE_VERSION@
AppVerName=Stellarium @PACKAGE_VERSION@
AppCopyright=Copyright (C) @COPYRIGHT_YEARS@ Stellarium team
AppPublisher=Stellarium team
AppPublisherURL=http://www.stellarium.org/
AppSupportURL=http://www.stellarium.org/
AppUpdatesURL=http://www.stellarium.org/
VersionInfoVersion=@PACKAGE_VERSION@
MinVersion=0,@MIN_VERSION@
SetupIconFile=data\stellarium.ico
OutputBaseFilename=stellarium-@PACKAGE_VERSION@-@ISS_PACKAGE_PLATFORM@
OutputDir=installers
; In 64-bit mode, {pf} is equivalent to {pf64},
; see http://www.jrsoftware.org/ishelp/index.php?topic=32vs64bitinstalls
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\data\stellarium.ico
LicenseFile=COPYING
ChangesAssociations=yes
; LZMA2/max required 95 MB RAM for compression and 8 MB RAM for decompression
; Using LZMA2/max algorithm reduces size of package on ~10%
Compression=lzma2/max

[Files]
Source: "@CMAKE_INSTALL_PREFIX@\bin\stellarium.exe"; DestDir: "{app}"
@STELMAINLIB@
@MESALIB@
@REDIST_FILES@
Source: "stellarium.url"; DestDir: "{app}"
Source: "stellarium-guide.url"; DestDir: "{app}"
Source: "stellarium-devdocs.url"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
Source: "ChangeLog"; DestDir: "{app}"; DestName: "ChangeLog.rtf"
Source: "@QtCore_location@"; DestDir: "{app}";
Source: "@QtGui_location@"; DestDir: "{app}";
Source: "@QtOpenGL_location@"; DestDir: "{app}";
Source: "@QtNetwork_location@"; DestDir: "{app}";
Source: "@QtWidgets_location@"; DestDir: "{app}";
Source: "@QtSql_location@"; DestDir: "{app}";
Source: "@QtXmlPatterns_location@"; DestDir: "{app}";
Source: "@QtConcurrent_location@"; DestDir: "{app}";
Source: "@QtPrintSupport_location@"; DestDir: "{app}";
@ISS_QT_SCRIPT@
@ISS_QT_MULTIMEDIA@
@ISS_QT_SERIALPORT@
@ISS_ANGLE_LIBS@
@ISS_ICU_LIBS@
@ISS_QT_PLUGINS@
@ISS_MULTIMEDIA_PLUGINS@
Source: "@CMAKE_INSTALL_PREFIX@\share\stellarium\*"; DestDir: "{app}\"; Flags: recursesubdirs

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: desktopicon\common; Description: "{cm:ForAllUsers}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: exclusive
Name: desktopicon\user; Description: "{cm:ForCurrentUserOnly}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: exclusive unchecked
Name: removecache; Description: "{cm:RemoveCache}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"
Name: removeconfig; Description: "{cm:RemoveMainConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"
Name: removeplugins; Description: "{cm:RemovePluginsConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
Name: removesolar; Description: "{cm:RemoveSolarConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"
Name: removelandscapes; Description: "{cm:RemoveUILandscapes}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
;Name: removeshortcuts; Description: "{cm:RemoveShortcutsConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked

[Run]
;An option to start Stellarium after setup has finished
@REDIST_RUN@
Filename: "{app}\stellarium.exe"; Description: "{cm:LaunchProgram,Stellarium}"; Flags: postinstall nowait skipifsilent unchecked

[InstallDelete]
;The old log file in all cases
Type: files; Name: "{userappdata}\Stellarium\log.txt"
Type: files; Name: "{userappdata}\Stellarium\config.ini"; Tasks: removeconfig
Type: files; Name: "{userappdata}\Stellarium\data\ssystem.ini"; Tasks: removesolar
Type: filesandordirs; Name: "{userappdata}\Stellarium\modules"; Tasks: removeplugins
Type: filesandordirs; Name: "{userappdata}\Stellarium\landscapes"; Tasks: removelandscapes
Type: filesandordirs; Name: "{localappdata}\stellarium\stellarium"; Tasks: removecache
;Type: files; Name: "{userappdata}\Stellarium\data\shortcuts.json"; Tasks: removeshortcuts

[UninstallDelete]

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,Stellarium}"; Filename: "{app}\stellarium.url"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\{cm:UserGuideOnTheWeb}"; Filename: "{app}\stellarium-guide.url"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\{cm:DevelopersDocsOnTheWeb}"; Filename: "{app}\stellarium-devdocs.url"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
; Name: "{group}\Stellarium {cm:FallbackMode}"; Filename: "{app}\stellarium.exe"; Parameters: "--safe-mode"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Stellarium {cm:DebugMode}"; Filename: "{app}\stellarium.exe"; Parameters: "--dump-opengl-details"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
; Name: "{group}\Stellarium {cm:AngleMode}"; Filename: "{app}\stellarium.exe"; Parameters: "--angle-mode"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
; Name: "{group}\Stellarium {cm:AngleD3D9Mode}"; Filename: "{app}\stellarium.exe"; Parameters: "--angle-d3d9"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
; Name: "{group}\Stellarium {cm:AngleD3D11Mode}"; Filename: "{app}\stellarium.exe"; Parameters: "--angle-d3d11"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
; Name: "{group}\Stellarium {cm:AngleWarpMode}"; Filename: "{app}\stellarium.exe"; Parameters: "--angle-warp"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Stellarium {cm:AngleMode}"; Filename: "{app}\stellarium.exe"; Parameters: "--angle-d3d9"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Stellarium {cm:MesaMode}"; Filename: "{app}\stellarium.exe"; Parameters: "--mesa-mode"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\{cm:UninstallProgram,Stellarium}"; Filename: "{uninstallexe}"
Name: "{group}\config.ini"; Filename: "{userappdata}\Stellarium\config.ini"
Name: "{group}\{cm:LastRunLog}"; Filename: "{userappdata}\Stellarium\log.txt"
Name: "{group}\{cm:ChangeLog}"; Filename: "{app}\ChangeLog.rtf"
Name: "{group}\{cm:Scenery3dDocs}"; Filename: "{app}\data\Scenery3d.pdf"
Name: "{commondesktop}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"; Tasks: desktopicon\common
Name: "{userdesktop}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"; Tasks: desktopicon\user

[Registry]
; Set file associations for Stellarium scripts
Root: HKCR; Subkey: ".ssc"; ValueType: string; ValueName: ""; ValueData: "Stellarium.Script"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "Stellarium.Script"; ValueType: string; ValueName: ""; ValueData: "Stellarium Script"; Flags: uninsdeletekey
Root: HKCR; Subkey: "Stellarium.Script\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\stellarium.exe,0"
Root: HKCR; Subkey: "Stellarium.Script\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\stellarium.exe"" --startup-script ""%1"""

; Recommended use Inno Setup 5.5.3+
[Languages]
; Official translations of GUI of Inno Setup + translation Stellarium specific lines
Name: "en"; MessagesFile: "compiler:Default.isl,util\ISL\EnglishCM.isl"
Name: "ca"; MessagesFile: "compiler:Languages\Catalan.isl,util\ISL\CatalanCM.isl"
Name: "co"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "cs"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "da"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "fi"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl,util\ISL\FrenchCM.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "el"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "he"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hu"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "no"; MessagesFile: "compiler:Languages\Norwegian.isl,util\ISL\NorwegianCM.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl,util\ISL\BrazilianPortugueseCM.isl"
Name: "pt"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl,util\ISL\RussianCM.isl"
Name: "sr"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "uk"; MessagesFile: "compiler:Languages\Ukrainian.isl,util\ISL\UkrainianCM.isl"
; Unofficial translations of GUI of Inno Setup
Name: "bg"; MessagesFile: "util\ISL\Bulgarian.isl,util\ISL\BulgarianCM.isl"
Name: "bs"; MessagesFile: "util\ISL\Bosnian.isl,util\ISL\BosnianCM.isl"
Name: "gla"; MessagesFile: "util\ISL\ScotsGaelic.isl"

[Code]
#IFDEF UNICODE
  #DEFINE AW "W"
#ELSE
  #DEFINE AW "A"
#ENDIF
type
  INSTALLSTATE = Longint;
const
  INSTALLSTATE_INVALIDARG = -2;  // An invalid parameter was passed to the function.
  INSTALLSTATE_UNKNOWN = -1;     // The product is neither advertised or installed.
  INSTALLSTATE_ADVERTISED = 1;   // The product is advertised but not installed.
  INSTALLSTATE_ABSENT = 2;       // The product is installed for a different user.
  INSTALLSTATE_DEFAULT = 5;      // The product is installed for the current user.

  VC_REDIST_X86 = '{13A4EE12-23EA-3371-91EE-EFB36DDFFF3E}'; //Microsoft.VS.VC_RuntimeMinimumVSU_x86,v12
  VC_REDIST_X64 = '{A749D8E6-B613-3BE3-8F5F-045C84EBA29B}'; //Microsoft.VS.VC_RuntimeMinimumVSU_amd64,v12

function MsiQueryProductState(szProduct: string): INSTALLSTATE; 
  external 'MsiQueryProductState{#AW}@msi.dll stdcall';

function VCVersionInstalled(const ProductID: string): Boolean;
begin
  Result := MsiQueryProductState(ProductID) = INSTALLSTATE_DEFAULT;
end;

function VCRedistNeedsInstall: Boolean;
begin
  // here the Result must be True when you need to install your VCRedist
  // or False when you don't need to, so now it's upon you how you build
  // this statement, the following won't install your VC redist only when
  // the Visual C++ 2013 Redist are installed for the current user
  Result := not (VCVersionInstalled(@REDIST_VERSION@));
end;
