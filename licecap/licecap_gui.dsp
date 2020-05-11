# Microsoft Developer Studio Project File - Name="licecap_gui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=licecap_gui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "licecap_gui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "licecap_gui.mak" CFG="licecap_gui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "licecap_gui - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "licecap_gui - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "licecap_gui - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "licecap_gui___Win32_Release"
# PROP BASE Intermediate_Dir "licecap_gui___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "licecap_gui___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PNG_WRITE_SUPPORTED" /D "USE_ICC" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/licecap.exe"

!ELSEIF  "$(CFG)" == "licecap_gui - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "licecap_gui___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/licecap.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "licecap_gui - Win32 Release"
# Name "licecap_gui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "WDL"

# PROP Default_Filter ""
# Begin Group "lice"

# PROP Default_Filter ""
# Begin Group "giflib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\WDL\giflib\dgif_lib.c

!IF  "$(CFG)" == "licecap_gui - Win32 Release"

# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "licecap_gui - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\WDL\giflib\egif_lib.c

!IF  "$(CFG)" == "licecap_gui - Win32 Release"

# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "licecap_gui - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\WDL\giflib\gif_hash.c

!IF  "$(CFG)" == "licecap_gui - Win32 Release"

# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "licecap_gui - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\WDL\giflib\gifalloc.c

!IF  "$(CFG)" == "licecap_gui - Win32 Release"

# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "licecap_gui - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_gif.cpp

!IF  "$(CFG)" == "licecap_gui - Win32 Release"

# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "licecap_gui - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\WDL\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\WDL\zlib\zutil.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\WDL\lice\lice.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_arc.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_gif_write.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_lcf.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_lcf.h
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_line.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_palette.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_text.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\WDL\filebrowse.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\win32_utf8.c
# End Source File
# Begin Source File

SOURCE=..\WDL\wingui\wndsize.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\licecap_ui.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\licecap_version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\licecap.rc
# End Source File
# End Group
# End Target
# End Project
