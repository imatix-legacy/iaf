# Microsoft Developer Studio Project File - Name="usemail" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=usemail - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "usemail.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "usemail.mak" CFG="usemail - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "usemail - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "usemail - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "usemail - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "DBIO_ODBC" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"usemail.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "usemail - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "DBIO_ODBC" /YX /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"usemail.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "usemail - Win32 Release"
# Name "usemail - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\emaildb.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflcons.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflconv.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflcvdp.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflcvsb.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflcvtp.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sfldate.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sfldir.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflenv.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflfile.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflfind.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflhttp.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sfllist.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflmail.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflmem.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflmime.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflnode.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflsock.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflstr.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflsymb.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflsyst.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sfltok.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sfltron.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sfluid.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflxml.c
# End Source File
# Begin Source File

SOURCE=C:\Imatix\Develop\Sfl\sflxmll.c
# End Source File
# Begin Source File

SOURCE=.\usemail.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
