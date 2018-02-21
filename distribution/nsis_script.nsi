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

!define MUI_ICON "Flare.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "Flare.bmp"
!define MUI_HEADERIMAGE_RIGHT

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Launch Flare"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"

Function LaunchLink
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

; The stuff to install
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
  File /oname=README.md "README"
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
SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecEngine ${LANG_ENGLISH} "The Flare engine without any game or mods."
  LangString DESC_SecGame ${LANG_ENGLISH} "The Empyrean Campaign game developed by the Flare team."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecEngine} $(DESC_SecEngine)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecGame} $(DESC_SecGame)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------

; Uninstaller

Section "Uninstall"

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
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR\mods\"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Flare\*.*"
  Delete "$DESKTOP\Flare.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Flare"
  RMDir "$INSTDIR"

SectionEnd
