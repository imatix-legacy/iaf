# Microsoft Developer Studio Project File - Name="libscl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libscl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libscl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libscl.mak" CFG="libscl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libscl - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libscl - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libscl - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\libscl.lib"

!ELSEIF  "$(CFG)" == "libscl - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\libscld.lib"

!ENDIF 

# Begin Target

# Name "libscl - Win32 Release"
# Name "libscl - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\sources\ggcode.c
# End Source File
# Begin Source File

SOURCE=..\sources\ggcomm.c
# End Source File
# Begin Source File

SOURCE=..\sources\ggeval.c
# End Source File
# Begin Source File

SOURCE=..\sources\ggfile.c
# End Source File
# Begin Source File

SOURCE=..\sources\ggfunc.c
# End Source File
# Begin Source File

SOURCE=..\sources\ggobjt.c
# End Source File
# Begin Source File

SOURCE=..\sources\ggpars.c
# End Source File
# Begin Source File

SOURCE=..\sources\ggstrn.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflbits.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflcomp.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflcons.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflconv.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflcvdp.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflcvsb.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflcvtp.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfldate.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfldes.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfldir.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflenv.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflfile.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflfind.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflhttp.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflidea.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfllist.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflmail.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflmem.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflmime.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflnode.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflproc.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflrc4.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflsha.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflsha1.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflsock.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflstr.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflsymb.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfltok.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfluhttp.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfluid.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflunic.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflusymb.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfluxml.c
# End Source File
# Begin Source File

SOURCE=..\sources\sfluxmll.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflxml.c
# End Source File
# Begin Source File

SOURCE=..\sources\sflxmll.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
