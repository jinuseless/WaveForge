[Setup]
AppName=WaveForge
AppVersion=0.1.0
AppPublisher=WaveForge Audio
DefaultDirName={commonpf64}\WaveForge
DefaultGroupName=WaveForge
DisableDirPage=yes
OutputDir=..\dist
OutputBaseFilename=WaveForge-Windows-Setup
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
Source: "..\build\WaveForge_artefacts\Release\VST3\WaveForge.vst3\*"; DestDir: "{commoncf64}\VST3\WaveForge.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "..\build\WaveForge_artefacts\Release\Standalone\WaveForge.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\WaveForge (Standalone)"; Filename: "{app}\WaveForge.exe"
