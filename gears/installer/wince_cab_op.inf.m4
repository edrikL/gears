m4_changequote(`^',`^')m4_dnl

[Version]
Signature="$Windows NT$"
Provider="Google"  ;[naming]
CESignature="$Windows CE$"

[CEDevice]
VersionMin=4.0
VersionMax=6.99
BuildMax=0xE0000000

; String aliases for convenience
[Strings]
m4_ifelse(DEBUG,^1^,^m4_dnl
BuildOutputDirectory="bin-dbg\wince-arm\opera"
^,^1^,^1^,^m4_dnl
BuildOutputDirectory="bin-opt\wince-arm\opera"
^)
SetupDll="setup.dll"
GearsDll="PRODUCT_SHORT_NAME_UQ.dll"
PermissionsDialog="permissions_dialog.html"
SettingsDialog="settings_dialog.html"
ShortcutsDialog="shortcuts_dialog.html"
InstallDir="\Application Data\Opera 9.5"

[CEStrings]
; Must be different from that for Gears for IE to avoid conflicts.
AppName="Gears for Opera Mobile"  ;[naming]

; The source directories for the files that will be in in the CAB
[SourceDisksNames]
1=,"DllSourceDirectory",,%BuildOutputDirectory%
2=,"HtmlSourceDirectory",,%BuildOutputDirectory%\genfiles

; The files that will be in the CAB
[SourceDisksFiles]
%GearsDll%=1           ; In DllSourceDirectory
%SetupDll%=1           ; In DllSourceDirectory
%PermissionsDialog%=2  ; In HtmlSourceDirectory
%SettingsDialog%=2     ; In HtmlSourceDirectory
%ShortcutsDialog%=2    ; In HtmlSourceDirectory

; The destination directories for the files that will be in the CAB
[DestinationDirs]
DllDestinationDirectory=0,%InstallDir%\plugins
HtmlDestinationDirectory=0,%InstallDir%

; The files to go in the DLL destination directory
[DllDestinationDirectory]
%GearsDll%,%GearsDll%,,0

; The files to go in the HTML destination directory
[HtmlDestinationDirectory]
%PermissionsDialog%,%PermissionsDialog%,,0
%SettingsDialog%,%SettingsDialog%,,0
%ShortcutsDialog%,%ShortcutsDialog%,,0

; The action to take during installation
[DefaultInstall]
CopyFiles=DllDestinationDirectory,HtmlDestinationDirectory
CESetupDLL=%SetupDll%  ; The DLL that gets run during installation
