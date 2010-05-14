# Microsoft Developer Studio Project File - Name="licecap" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=licecap - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "licecap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "licecap.mak" CFG="licecap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "licecap - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "licecap - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "licecap - Win32 Release Profile" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "licecap - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "PNG_WRITE_SUPPORTED" /D "USE_ICC" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "licecap - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "PNG_WRITE_SUPPORTED" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "licecap - Win32 Release Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_Profile"
# PROP BASE Intermediate_Dir "Release_Profile"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Profile"
# PROP Intermediate_Dir "Release_Profile"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "PNG_WRITE_SUPPORTED" /D "USE_ICC" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "PNG_WRITE_SUPPORTED" /D "USE_ICC" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /fixed:no
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "licecap - Win32 Release"
# Name "licecap - Win32 Debug"
# Name "licecap - Win32 Release Profile"
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
# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"
# End Source File
# Begin Source File

SOURCE=..\WDL\giflib\egif_lib.c
# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"
# End Source File
# Begin Source File

SOURCE=..\WDL\giflib\gif_hash.c
# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"
# End Source File
# Begin Source File

SOURCE=..\WDL\giflib\gifalloc.c
# ADD CPP /I "../WDL/giflib" /D "HAVE_CONFIG_H"
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_gif.cpp
# End Source File
# End Group
# Begin Group "png"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\WDL\libpng\png.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngerror.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pnggccrd.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngget.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngmem.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngpread.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngread.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngrio.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngrtran.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngrutil.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngset.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngtrans.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngvcrd.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngwio.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngwrite.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngwtran.c
# End Source File
# Begin Source File

SOURCE=..\WDL\libpng\pngwutil.c
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

SOURCE=..\WDL\lice\lice_gif_write.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_lcf.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_lcf.h
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_palette.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_png.cpp
# End Source File
# Begin Source File

SOURCE=..\WDL\lice\lice_png_write.cpp
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

SOURCE=.\licecap.cpp
# End Source File
# Begin Source File

SOURCE=.\licecap_ui.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\licecap.rc
# End Source File
# End Group
# End Target
# End Project
