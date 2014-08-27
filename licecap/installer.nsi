;Include Modern UI


!define MUI_COMPONENTSPAGE_NODESC

  !include "MUI.nsh"

;--------------------------------
;General

!searchparse /file licecap_version.h '#define LICECAP_VERSION "v' VER_MAJOR '.' VER_MINOR '"'

SetCompressor lzma



  ;Name and file
  Name "LICEcap v${VER_MAJOR}.${VER_MINOR}"
  OutFile "licecap${VER_MAJOR}${VER_MINOR}-install.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\LICEcap"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\LICEcap" ""

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "license.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"




;--------------------------------
;Installer Sections


Section "Required files"

  SectionIn RO
  SetOutPath "$INSTDIR"


  File release\LICEcap.exe
;  File release\LICEcap_cli.exe
    
  ;Store installation folder
  WriteRegStr HKLM "Software\LICEcap" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  File license.txt
  File whatsnew.txt

SectionEnd

Section "Desktop Icon"
  CreateShortcut "$DESKTOP\LICEcap.lnk" "$INSTDIR\LICEcap.exe"
SectionEnd

Section "Start Menu Shortcuts"

  SetOutPath $SMPROGRAMS\LICEcap
  CreateShortcut "$OUTDIR\LICEcap.lnk" "$INSTDIR\LICEcap.exe"
  CreateShortcut "$OUTDIR\LICEcap License.lnk" "$INSTDIR\license.txt"
  CreateShortcut "$OUTDIR\Whatsnew.txt.lnk" "$INSTDIR\whatsnew.txt"
  CreateShortcut "$OUTDIR\Uninstall LICEcap.lnk" "$INSTDIR\uninstall.exe"

  SetOutPath $INSTDIR

SectionEnd

;Section "LICEcap Source Code"
;  SetOutPath $INSTDIR\Source
;  File requires_wdl.txt
;
;  SetOutPath $INSTDIR\Source\licecap
;  File installer.nsi
;  File license.txt
;  File licecap.dsw
;  File licecap_cli.dsp
;  File licecap_gui.dsp
;  File icon1.ico
;  File licecap.rc
;  File resource.h
;  File licecap_version.h
;  File licecap_cli.cpp
;  File licecap_ui.cpp
;  File requires_wdl.txt
;  File whatsnew.txt
;  
;  File licecap.icns
;  File capturewindow.mm
;
;  File licecap-Info.plist
;  File main.m
;  File licecap_Prefix.pch
;  File makedmg.sh
;  File pkg-dmg
;  File stage_DS_Store
;  File background.png
;
;  File /r English.lproj
;
;  SetOutPath $INSTDIR\Source\licecap\licecap.xcodeproj
;  File licecap.xcodeproj\project.pbxproj
;
;SectionEnd
;


;--------------------------------
;Uninstaller Section

Section "Uninstall"

  DeleteRegKey HKLM "Software\LICEcap"

  Delete "$INSTDIR\LICEcap_cli.exe"
  Delete "$INSTDIR\LICEcap.exe"

  Delete "$INSTDIR\license.txt"
  Delete "$INSTDIR\whatsnew.txt"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$SMPROGRAMS\LICEcap\*.lnk"
  RMDir $SMPROGRAMS\LICEcap
  Delete "$DESKTOP\LICEcap.lnk"

  Delete $INSTDIR\Source\LICEcap\installer.nsi
  Delete $INSTDIR\Source\LICEcap\license.txt
  Delete $INSTDIR\Source\LICEcap\whatsnew.txt
  Delete $INSTDIR\Source\LICEcap\licecap.dsw
  Delete $INSTDIR\Source\LICEcap\licecap_cli.dsp
  Delete $INSTDIR\Source\LICEcap\licecap_gui.dsp
  Delete $INSTDIR\Source\LICEcap\icon1.ico
  Delete $INSTDIR\Source\LICEcap\licecap.rc
  Delete $INSTDIR\Source\LICEcap\resource.h
  Delete $INSTDIR\Source\LICEcap\licecap_version.h
  Delete $INSTDIR\Source\LICEcap\licecap_cli.cpp
  Delete $INSTDIR\Source\LICEcap\background.png
  Delete $INSTDIR\Source\LICEcap\licecap_ui.cpp
  Delete $INSTDIR\Source\LICEcap\requires_wdl.txt
  Delete $INSTDIR\Source\requires_wdl.txt

  Delete $INSTDIR\Source\licecap.icns
  Delete $INSTDIR\Source\capturewindow.mm

  Delete $INSTDIR\Source\makedmg.sh
  Delete $INSTDIR\Source\pkg-dmg
  Delete $INSTDIR\Source\stage_DS_Store

  Delete $INSTDIR\Source\licecap-Info.plist
  Delete $INSTDIR\Source\main.m
  Delete $INSTDIR\Source\licecap_Prefix.pch

  Delete "$INSTDIR\English.lproj\*.*"
  Delete "$INSTDIR\Source\licecap\licecap.xcodeproj\*.*"

  RMDir $INSTDIR\English.lproj
  RMDir  "$INSTDIR\Source\licecap\licecap.xcodeproj"

  RMDir $INSTDIR\Source\LICEcap
  RMDir $INSTDIR\Source
  
  RMDir "$INSTDIR"

SectionEnd
