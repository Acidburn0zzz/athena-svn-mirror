# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=xntpdc - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to xntpdc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "xntpdc - Win32 Release" && "$(CFG)" != "xntpdc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "xntpdc.mak" CFG="xntpdc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xntpdc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "xntpdc - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "xntpdc - Win32 Release"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xntpdc - Win32 Release"

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

ALL : "$(OUTDIR)\xntpdc.exe" "$(OUTDIR)\xntpdc.bsc"

CLEAN : 
	-@erase "$(INTDIR)\ntpdc.obj"
	-@erase "$(INTDIR)\ntpdc.sbr"
	-@erase "$(INTDIR)\ntpdc_ops.obj"
	-@erase "$(INTDIR)\ntpdc_ops.sbr"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\xntpdc.bsc"
	-@erase "$(OUTDIR)\xntpdc.exe"
	-@erase "$(OUTDIR)\xntpdc.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\include\winnt" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\include\winnt" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/xntpdc.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xntpdc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpdc.sbr" \
	"$(INTDIR)\ntpdc_ops.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\xntpdc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib winmm.lib ..\libntp\WinRel\libntp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /machine:I386
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\WinRel\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/xntpdc.pdb" /machine:I386 /out:"$(OUTDIR)/xntpdc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpdc.obj" \
	"$(INTDIR)\ntpdc_ops.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\xntpdc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xntpdc - Win32 Debug"

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

ALL : "$(OUTDIR)\xntpdc.exe" "$(OUTDIR)\xntpdc.bsc"

CLEAN : 
	-@erase "$(INTDIR)\ntpdc.obj"
	-@erase "$(INTDIR)\ntpdc.sbr"
	-@erase "$(INTDIR)\ntpdc_ops.obj"
	-@erase "$(INTDIR)\ntpdc_ops.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\xntpdc.bsc"
	-@erase "$(OUTDIR)\xntpdc.exe"
	-@erase "$(OUTDIR)\xntpdc.ilk"
	-@erase "$(OUTDIR)\xntpdc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /ML /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\include\winnt" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\include\winnt"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/xntpdc.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xntpdc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpdc.sbr" \
	"$(INTDIR)\ntpdc_ops.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\xntpdc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wsock32.lib winmm.lib ..\libntp\WinDebug\libntp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\WinDebug\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/xntpdc.pdb" /debug /machine:I386 /out:"$(OUTDIR)/xntpdc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpdc.obj" \
	"$(INTDIR)\ntpdc_ops.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\xntpdc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "xntpdc - Win32 Release"
# Name "xntpdc - Win32 Debug"

!IF  "$(CFG)" == "xntpdc - Win32 Release"

!ELSEIF  "$(CFG)" == "xntpdc - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ntpdc_ops.c
DEP_CPP_NTPDC=\
	"..\include\ntp_control.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\winnt\arpa/inet.h"\
	"..\include\winnt\netdb.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/time.h"\
	".\ntpdc.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\ntpdc_ops.obj" : $(SOURCE) $(DEP_CPP_NTPDC) "$(INTDIR)"

"$(INTDIR)\ntpdc_ops.sbr" : $(SOURCE) $(DEP_CPP_NTPDC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntpdc.c
DEP_CPP_NTPDC_=\
	"..\include\ntp_io.h"\
	"..\include\ntp_machine.h"\
	"..\include\ntp_select.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_types.h"\
	"..\include\winnt\netdb.h"\
	"..\include\winnt\sys/time.h"\
	".\ntpdc.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\ntpdc.obj" : $(SOURCE) $(DEP_CPP_NTPDC_) "$(INTDIR)"

"$(INTDIR)\ntpdc.sbr" : $(SOURCE) $(DEP_CPP_NTPDC_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"

"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
