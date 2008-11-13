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
BuildOutputDirectory="bin-dbg\wince-arm\ie"
^,^1^,^1^,^m4_dnl
BuildOutputDirectory="bin-opt\wince-arm\ie"
^)
SetupDll="setup.dll"
GearsDll="PRODUCT_SHORT_NAME_UQ.dll"

[CEStrings]
; Should be 'Gears for Internet Explorer', but is left as-is for
; backwards-compatibility
AppName="Gears"  ;[naming]
InstallDir="%CE1%\PRODUCT_FRIENDLY_NAME_UQ"

; The source directories for the files that will be in in the CAB
[SourceDisksNames]
1=,"DllSourceDirectory",,%BuildOutputDirectory%

; The files that will be in the CAB
[SourceDisksFiles]
%GearsDll%=1  ; In DllSourceDirectory
%SetupDll%=1  ; In DllSourceDirectory

; The destination directories for the files that will be in the CAB
[DestinationDirs]
DllDestinationDirectory=0,%InstallDir%

; The files to go in the DLL destination directory
[DllDestinationDirectory]
%GearsDll%,%GearsDll%,,0

; The action to take during installation
[DefaultInstall]
CopyFiles=DllDestinationDirectory
CESelfRegister=%GearsDll%
CESetupDLL=%SetupDll%  ; The DLL that gets run during installation
