# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=ntpq - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to ntpq - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ntpq - Win32 Release" && "$(CFG)" != "ntpq - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntpq.mak" CFG="ntpq - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntpq - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ntpq - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "ntpq - Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ntpq - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : "$(OUTDIR)\ntpq.exe" "$(OUTDIR)\ntpq.bsc"

CLEAN : 
	-@erase "$(INTDIR)\ntpq.obj"
	-@erase "$(INTDIR)\ntpq.sbr"
	-@erase "$(INTDIR)\ntpq_ops.obj"
	-@erase "$(INTDIR)\ntpq_ops.sbr"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpq.bsc"
	-@erase "$(OUTDIR)\ntpq.exe"
	-@erase "$(OUTDIR)\ntpq.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\include\winnt" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\include\winnt" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/ntpq.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ntpq.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpq.sbr" \
	"$(INTDIR)\ntpq_ops.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpq.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib winmm.lib ..\libntp\WinRel\libntp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /machine:I386
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\WinRel\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/ntpq.pdb" /machine:I386 /out:"$(OUTDIR)/ntpq.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpq.obj" \
	"$(INTDIR)\ntpq_ops.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpq.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ntpq - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : "$(OUTDIR)\ntpq.exe" "$(OUTDIR)\ntpq.bsc"

CLEAN : 
	-@erase "$(INTDIR)\ntpq.obj"
	-@erase "$(INTDIR)\ntpq.sbr"
	-@erase "$(INTDIR)\ntpq_ops.obj"
	-@erase "$(INTDIR)\ntpq_ops.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpq.bsc"
	-@erase "$(OUTDIR)\ntpq.exe"
	-@erase "$(OUTDIR)\ntpq.ilk"
	-@erase "$(OUTDIR)\ntpq.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /ML /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\include\winnt" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\include\winnt"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/ntpq.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ntpq.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpq.sbr" \
	"$(INTDIR)\ntpq_ops.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpq.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wsock32.lib winmm.lib ..\libntp\WinDebug\libntp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\WinDebug\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/ntpq.pdb" /debug /machine:I386 /out:"$(OUTDIR)/ntpq.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpq.obj" \
	"$(INTDIR)\ntpq_ops.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpq.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "ntpq - Win32 Release"
# Name "ntpq - Win32 Debug"

!IF  "$(CFG)" == "ntpq - Win32 Release"

!ELSEIF  "$(CFG)" == "ntpq - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ntpq.c
DEP_CPP_NTPQ_=\
	"..\include\ntp_calendar.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_machine.h"\
	"..\include\ntp_select.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\winnt\netdb.h"\
	"..\include\winnt\sys/time.h"\
	".\ntpq.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\ntpq.obj" : $(SOURCE) $(DEP_CPP_NTPQ_) "$(INTDIR)"

"$(INTDIR)\ntpq.sbr" : $(SOURCE) $(DEP_CPP_NTPQ_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"

"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntpq_ops.c
DEP_CPP_NTPQ_O=\
	"..\include\ntp_machine.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_types.h"\
	"..\include\winnt\netdb.h"\
	"..\include\winnt\sys/time.h"\
	".\ntpq.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\ntpq_ops.obj" : $(SOURCE) $(DEP_CPP_NTPQ_O) "$(INTDIR)"

"$(INTDIR)\ntpq_ops.sbr" : $(SOURCE) $(DEP_CPP_NTPQ_O) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
