# Microsoft Developer Studio Project File - Name="lice" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lice - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lice.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lice.mak" CFG="lice - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lice - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lice - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lice - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../zlib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "PNG_USE_PNGVCRD" /D "PNG_LIBPNG_SPECIALBUILD" /D "__MMX__" /D "PNG_HAVE_MMX_COMBINE_ROW" /D "PNG_HAVE_MMX_READ_INTERLACE" /D "PNG_HAVE_MMX_READ_FILTER_ROW" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "lice - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../zlib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "PNG_USE_PNGVCRD" /D "PNG_DEBUG" /D "PNG_LIBPNG_SPECIALBUILD" /D "__MMX__" /D "PNG_HAVE_MMX_COMBINE_ROW" /D "PNG_HAVE_MMX_READ_INTERLACE" /D "PNG_HAVE_MMX_READ_FILTER_ROW" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "lice - Win32 Release"
# Name "lice - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "libpng"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\libpng\png.c
# End Source File
# Begin Source File

SOURCE=..\libpng\png.h
# End Source File
# Begin Source File

SOURCE=..\libpng\pngconf.h
# End Source File
# Begin Source File

SOURCE=..\libpng\pngerror.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pnggccrd.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngget.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngmem.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngpread.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngread.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngrio.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngrtran.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngrutil.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngset.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngtest.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngtrans.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngvcrd.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngwio.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngwrite.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngwtran.c
# End Source File
# Begin Source File

SOURCE=..\libpng\pngwutil.c
# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=..\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=..\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=..\zlib\zutil.c
# End Source File
# Begin Source File

SOURCE=..\zlib\zutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\lice.cpp
# End Source File
# Begin Source File

SOURCE=.\lice_bmp.cpp
# End Source File
# Begin Source File

SOURCE=.\lice_ico.cpp
# End Source File
# Begin Source File

SOURCE=.\lice_line.cpp
# End Source File
# Begin Source File

SOURCE=.\lice_png.cpp
# End Source File
# Begin Source File

SOURCE=.\lice_texgen.cpp
# End Source File
# Begin Source File

SOURCE=.\lice_text.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\lice.h
# End Source File
# Begin Source File

SOURCE=.\lice_combine.h
# End Source File
# End Group
# End Target
# End Project
