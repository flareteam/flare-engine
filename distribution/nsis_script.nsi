; Flare NSIS script
;
; IMPORTANT! This script is not to be run in this distribution directory
; It is to be copied to an empty directory along with the following files:
;
; flare.exe
; *.dll (These are the SDL libs, and are not included here)
; COPYING
; CREDITS.txt (part of flare-game)
; CREDITS.engine.txt
; LICENSE.txt (part of flare-game)
; README.md (part of flare-game)
; README.engine.md
; RELEASE_NOTES.txt
; mods/mods.txt
; mods/default/
; mods/fantasycore/ (part of flare-game)
; mods/empyrean_campaign/ (part of flare-game)
; mods/centered_statbars/ (part of flare-game)
; distribution/Flare.ico
; distribution/Flare.bmp
;
;--------------------------------

;--------------------------------
;Include Modern UI

!include "MUI2.nsh"
!include "x64.nsh"

!define MUI_ICON "Flare.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "Flare.bmp"
!define MUI_HEADERIMAGE_RIGHT

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Launch Flare"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"

!define MUI_FINISHPAGE_NOAUTOCLOSE

Function LaunchLink
	SetOutPath $INSTDIR
	ExecShell "" "$INSTDIR\flare.exe"
FunctionEnd

; The name of the installer
Name "Flare"

; The file to write
OutFile "flare_install.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Flare

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Flare" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

Function .onInit
	${If} ${RunningX64}
	StrCpy $INSTDIR "$PROGRAMFILES64\Flare"
	${Else}
	StrCpy $INSTDIR "$PROGRAMFILES\Flare"
	${EndIf}
FunctionEnd

;--------------------------------
;Pages

 ; !insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\Modern UI\License.txt"
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES
	!insertmacro MUI_PAGE_FINISH

	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

	!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; The "" makes the section hidden.
Section "" SecUninstallPrevious

	Call UninstallPrevious

SectionEnd

Function UninstallPrevious
	; We need to use INSTDIR for the uninstaller, so save the current value here
	StrCpy $R0 $INSTDIR

	; try old 32-bit location
	${If} ${RunningX64}
		SetRegView 32
		ReadRegStr $R1 HKLM "SOFTWARE\Flare" "Install_Dir"
		${If} $R1 != ""
			DetailPrint "Removing previous installation: $R1"
			StrCpy $INSTDIR "$R1"
			Call UninstallAll
		${EndIf}
		SetRegView 64
	${Endif}

	ReadRegStr $R1 HKLM "SOFTWARE\Flare" "Install_Dir"

	${If} $R1 != ""
		DetailPrint "Removing previous installation: $R1"
		StrCpy $INSTDIR "$R1"
		Call UninstallAll
	${EndIf}

	StrCpy $INSTDIR $R0
FunctionEnd

; The stuff to install
SectionGroup "!Flare (64-bit)"

Section "Flare engine" SecEngine

	SectionIn RO

	; Set output path to the installation directory.
	SetOutPath $INSTDIR

	; Put file there
	File "flare.exe"
	File "*.dll"
	File "COPYING"
	File "CREDITS.txt"
	File "CREDITS.engine.txt"
	File "LICENSE.txt"
	File "README.md"
	File "README.engine.md"
	File "RELEASE_NOTES.txt"

	CreateDirectory "$INSTDIR\mods\default"
	SetOutPath "$INSTDIR\mods"
	File "mods\mods.txt"
	File /r "mods\default"

	; Write the installation path into the registry
	WriteRegStr HKLM SOFTWARE\Flare "Install_Dir" "$INSTDIR"

	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Flare" "DisplayName" "Flare"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Flare" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Flare" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Flare" "NoRepair" 1
	WriteUninstaller "uninstall.exe"

	; Create the user mods directory so we can link to it in the start menu
	CreateDirectory "$APPDATA\flare\userdata\mods"

SectionEnd

Section "Flare: Empyrean Campaign" SecGame
	SetOutPath $INSTDIR

	CreateDirectory "$INSTDIR\mods\fantasycore"
	CreateDirectory "$INSTDIR\mods\empyrean_campaign"
	SetOutPath "$INSTDIR\mods"
	File /r "mods\fantasycore"
	File /r "mods\empyrean_campaign"
	File /r "mods\centered_statbars"
SectionEnd

; Optional section (can be disabled by the user)
Section "Desktop Shortcut"
	SetOutPath $INSTDIR
	CreateShortcut "$DESKTOP\Flare.lnk" "$INSTDIR\flare.exe" "" "$INSTDIR\flare.exe" 0
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"
	SetOutPath $INSTDIR
	CreateDirectory "$SMPROGRAMS\Flare"
	CreateShortcut "$SMPROGRAMS\Flare\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	CreateShortcut "$SMPROGRAMS\Flare\Flare.lnk" "$INSTDIR\flare.exe" "" "$INSTDIR\flare.exe" 0
	CreateShortcut "$SMPROGRAMS\Flare\Flare Mods.lnk" "$APPDATA\flare\userdata\mods" "" "$APPDATA\flare\userdata\mods" 0
SectionEnd

SectionGroupEnd

SectionGroup "Flare (32-bit)"

; Optional section (can be disabled by the user)
Section /o "Flare engine" SecEngine32
	SetOutPath $INSTDIR

	CreateDirectory "$INSTDIR\x86"
	File /r "x86"
SectionEnd

; Optional section (can be disabled by the user)
Section /o "Desktop Shortcut"
	SetOutPath $INSTDIR
	CreateShortcut "$DESKTOP\Flare (32-bit).lnk" "$INSTDIR\x86\flare.exe" "--data-path=$\"$INSTDIR$\"" "$INSTDIR\x86\flare.exe" 0
SectionEnd

; Optional section (can be disabled by the user)
Section /o "Start Menu Shortcuts"
	SetOutPath $INSTDIR
	CreateDirectory "$SMPROGRAMS\Flare"
	CreateShortcut "$SMPROGRAMS\Flare\Flare (32-bit).lnk" "$INSTDIR\x86\flare.exe" "--data-path=$\"$INSTDIR$\"" "$INSTDIR\x86\flare.exe" 0
SectionEnd

SectionGroupEnd

;--------------------------------
;Descriptions

	;Language strings
	LangString DESC_SecEngine ${LANG_ENGLISH} "The Flare engine without any game or mods."
	LangString DESC_SecEngine32 ${LANG_ENGLISH} "The Flare engine (32-bit version)."
	LangString DESC_SecGame ${LANG_ENGLISH} "The Empyrean Campaign game developed by the Flare team."

	;Assign language strings to sections
	!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecEngine} $(DESC_SecEngine)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecEngine32} $(DESC_SecEngine32)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecGame} $(DESC_SecGame)
	!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------

; Uninstaller

!macro UninstallAll un
Function ${un}UninstallAll
	; Remove registry keys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Flare"
	DeleteRegKey HKLM SOFTWARE\Flare

	; Remove files and uninstaller
	Delete "$INSTDIR\flare.exe"
	Delete "$INSTDIR\COPYING"
	Delete "$INSTDIR\CREDITS.engine.txt"
	Delete "$INSTDIR\CREDITS.txt"
	Delete "$INSTDIR\README.engine.md"
	Delete "$INSTDIR\README.md"
	Delete "$INSTDIR\RELEASE_NOTES.txt"
	Delete "$INSTDIR\LICENSE.txt"
	Delete "$INSTDIR\*.dll"
	Delete "$INSTDIR\uninstall.exe"
	RMDir /r "$INSTDIR\mods\"
	RMDir /r "$INSTDIR\x86\"

	; Remove shortcuts, if any
	Delete "$SMPROGRAMS\Flare\*.*"
	Delete "$DESKTOP\Flare.lnk"
	Delete "$DESKTOP\Flare (32-bit).lnk"

	; Remove directories used
	RMDir "$SMPROGRAMS\Flare"
	RMDir "$INSTDIR"
FunctionEnd
!macroend

!insertmacro UninstallAll ""
!insertmacro UninstallAll "un."

Section "Uninstall"
	Call un.UninstallAll
SectionEnd
