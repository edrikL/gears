[Version]
Signature="$Windows NT$"
Provider="Google"
CESignature="$Windows CE$"

[CEStrings]
AppName="Gears"
InstallDir=%CE1%\%AppName%

[Strings]
Manufacturer="Google"

[CEDevice]
VersionMin=4.0
VersionMax=6.99
BuildMax=0xE0000000

[DefaultInstall]
CopyFiles=Files.Common1
CESelfRegister=gears.dll

[SourceDisksNames]
m4_changequote(`^',`^')m4_dnl
m4_ifelse(DEBUG,^1^,^m4_dnl
1=,"Common1",,"bin-dbg\wince-arm\ie\"
^,^1^,^1^,^m4_dnl
1=,"Common1",,"bin-opt\wince-arm\ie\"
^)

[SourceDisksFiles]
"gears.dll"=1

[DestinationDirs]
Shortcuts=0,%CE2%\Start Menu
Files.Common1=0,"%CE1%\Gears"

[Files.Common1]
"gears.dll","gears.dll",,0

