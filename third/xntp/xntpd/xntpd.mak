# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=xntpd - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to xntpd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "xntpd - Win32 Release" && "$(CFG)" != "xntpd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "xntpd.mak" CFG="xntpd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xntpd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "xntpd - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "xntpd - Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xntpd - Win32 Release"

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

ALL : "$(OUTDIR)\xntpd.exe" "$(OUTDIR)\xntpd.bsc"

CLEAN : 
	-@erase "$(INTDIR)\messages.res"
	-@erase "$(INTDIR)\ntp_config.obj"
	-@erase "$(INTDIR)\ntp_config.sbr"
	-@erase "$(INTDIR)\ntp_control.obj"
	-@erase "$(INTDIR)\ntp_control.sbr"
	-@erase "$(INTDIR)\ntp_filegen.obj"
	-@erase "$(INTDIR)\ntp_filegen.sbr"
	-@erase "$(INTDIR)\ntp_intres.obj"
	-@erase "$(INTDIR)\ntp_intres.sbr"
	-@erase "$(INTDIR)\ntp_io.obj"
	-@erase "$(INTDIR)\ntp_io.sbr"
	-@erase "$(INTDIR)\ntp_leap.obj"
	-@erase "$(INTDIR)\ntp_leap.sbr"
	-@erase "$(INTDIR)\ntp_loopfilter.obj"
	-@erase "$(INTDIR)\ntp_loopfilter.sbr"
	-@erase "$(INTDIR)\ntp_monitor.obj"
	-@erase "$(INTDIR)\ntp_monitor.sbr"
	-@erase "$(INTDIR)\ntp_peer.obj"
	-@erase "$(INTDIR)\ntp_peer.sbr"
	-@erase "$(INTDIR)\ntp_proto.obj"
	-@erase "$(INTDIR)\ntp_proto.sbr"
	-@erase "$(INTDIR)\ntp_refclock.obj"
	-@erase "$(INTDIR)\ntp_refclock.sbr"
	-@erase "$(INTDIR)\ntp_request.obj"
	-@erase "$(INTDIR)\ntp_request.sbr"
	-@erase "$(INTDIR)\ntp_restrict.obj"
	-@erase "$(INTDIR)\ntp_restrict.sbr"
	-@erase "$(INTDIR)\ntp_timer.obj"
	-@erase "$(INTDIR)\ntp_timer.sbr"
	-@erase "$(INTDIR)\ntp_unixclock.obj"
	-@erase "$(INTDIR)\ntp_unixclock.sbr"
	-@erase "$(INTDIR)\ntp_util.obj"
	-@erase "$(INTDIR)\ntp_util.sbr"
	-@erase "$(INTDIR)\ntpd.obj"
	-@erase "$(INTDIR)\ntpd.sbr"
	-@erase "$(INTDIR)\refclock_acts.obj"
	-@erase "$(INTDIR)\refclock_acts.sbr"
	-@erase "$(INTDIR)\refclock_as2201.obj"
	-@erase "$(INTDIR)\refclock_as2201.sbr"
	-@erase "$(INTDIR)\refclock_atom.obj"
	-@erase "$(INTDIR)\refclock_atom.sbr"
	-@erase "$(INTDIR)\refclock_chu.obj"
	-@erase "$(INTDIR)\refclock_chu.sbr"
	-@erase "$(INTDIR)\refclock_conf.obj"
	-@erase "$(INTDIR)\refclock_conf.sbr"
	-@erase "$(INTDIR)\refclock_datum.obj"
	-@erase "$(INTDIR)\refclock_datum.sbr"
	-@erase "$(INTDIR)\refclock_heath.obj"
	-@erase "$(INTDIR)\refclock_heath.sbr"
	-@erase "$(INTDIR)\refclock_irig.obj"
	-@erase "$(INTDIR)\refclock_irig.sbr"
	-@erase "$(INTDIR)\refclock_leitch.obj"
	-@erase "$(INTDIR)\refclock_leitch.sbr"
	-@erase "$(INTDIR)\refclock_local.obj"
	-@erase "$(INTDIR)\refclock_local.sbr"
	-@erase "$(INTDIR)\refclock_msfees.obj"
	-@erase "$(INTDIR)\refclock_msfees.sbr"
	-@erase "$(INTDIR)\refclock_mx4200.obj"
	-@erase "$(INTDIR)\refclock_mx4200.sbr"
	-@erase "$(INTDIR)\refclock_nmea.obj"
	-@erase "$(INTDIR)\refclock_nmea.sbr"
	-@erase "$(INTDIR)\refclock_parse.obj"
	-@erase "$(INTDIR)\refclock_parse.sbr"
	-@erase "$(INTDIR)\refclock_pst.obj"
	-@erase "$(INTDIR)\refclock_pst.sbr"
	-@erase "$(INTDIR)\refclock_ptbacts.obj"
	-@erase "$(INTDIR)\refclock_ptbacts.sbr"
	-@erase "$(INTDIR)\refclock_tpro.obj"
	-@erase "$(INTDIR)\refclock_tpro.sbr"
	-@erase "$(INTDIR)\refclock_trak.obj"
	-@erase "$(INTDIR)\refclock_trak.sbr"
	-@erase "$(INTDIR)\refclock_usno.obj"
	-@erase "$(INTDIR)\refclock_usno.sbr"
	-@erase "$(INTDIR)\refclock_wwvb.obj"
	-@erase "$(INTDIR)\refclock_wwvb.sbr"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\xntpd.bsc"
	-@erase "$(OUTDIR)\xntpd.exe"
	-@erase "$(OUTDIR)\xntpd.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\include\winnt" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\include\winnt" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/xntpd.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/messages.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xntpd.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntp_config.sbr" \
	"$(INTDIR)\ntp_control.sbr" \
	"$(INTDIR)\ntp_filegen.sbr" \
	"$(INTDIR)\ntp_intres.sbr" \
	"$(INTDIR)\ntp_io.sbr" \
	"$(INTDIR)\ntp_leap.sbr" \
	"$(INTDIR)\ntp_loopfilter.sbr" \
	"$(INTDIR)\ntp_monitor.sbr" \
	"$(INTDIR)\ntp_peer.sbr" \
	"$(INTDIR)\ntp_proto.sbr" \
	"$(INTDIR)\ntp_refclock.sbr" \
	"$(INTDIR)\ntp_request.sbr" \
	"$(INTDIR)\ntp_restrict.sbr" \
	"$(INTDIR)\ntp_timer.sbr" \
	"$(INTDIR)\ntp_unixclock.sbr" \
	"$(INTDIR)\ntp_util.sbr" \
	"$(INTDIR)\ntpd.sbr" \
	"$(INTDIR)\refclock_acts.sbr" \
	"$(INTDIR)\refclock_as2201.sbr" \
	"$(INTDIR)\refclock_atom.sbr" \
	"$(INTDIR)\refclock_chu.sbr" \
	"$(INTDIR)\refclock_conf.sbr" \
	"$(INTDIR)\refclock_datum.sbr" \
	"$(INTDIR)\refclock_heath.sbr" \
	"$(INTDIR)\refclock_irig.sbr" \
	"$(INTDIR)\refclock_leitch.sbr" \
	"$(INTDIR)\refclock_local.sbr" \
	"$(INTDIR)\refclock_msfees.sbr" \
	"$(INTDIR)\refclock_mx4200.sbr" \
	"$(INTDIR)\refclock_nmea.sbr" \
	"$(INTDIR)\refclock_parse.sbr" \
	"$(INTDIR)\refclock_pst.sbr" \
	"$(INTDIR)\refclock_ptbacts.sbr" \
	"$(INTDIR)\refclock_tpro.sbr" \
	"$(INTDIR)\refclock_trak.sbr" \
	"$(INTDIR)\refclock_usno.sbr" \
	"$(INTDIR)\refclock_wwvb.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\xntpd.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib winmm.lib ..\libntp\WinRel\libntp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /machine:I386
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\WinRel\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/xntpd.pdb" /machine:I386 /out:"$(OUTDIR)/xntpd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\messages.res" \
	"$(INTDIR)\ntp_config.obj" \
	"$(INTDIR)\ntp_control.obj" \
	"$(INTDIR)\ntp_filegen.obj" \
	"$(INTDIR)\ntp_intres.obj" \
	"$(INTDIR)\ntp_io.obj" \
	"$(INTDIR)\ntp_leap.obj" \
	"$(INTDIR)\ntp_loopfilter.obj" \
	"$(INTDIR)\ntp_monitor.obj" \
	"$(INTDIR)\ntp_peer.obj" \
	"$(INTDIR)\ntp_proto.obj" \
	"$(INTDIR)\ntp_refclock.obj" \
	"$(INTDIR)\ntp_request.obj" \
	"$(INTDIR)\ntp_restrict.obj" \
	"$(INTDIR)\ntp_timer.obj" \
	"$(INTDIR)\ntp_unixclock.obj" \
	"$(INTDIR)\ntp_util.obj" \
	"$(INTDIR)\ntpd.obj" \
	"$(INTDIR)\refclock_acts.obj" \
	"$(INTDIR)\refclock_as2201.obj" \
	"$(INTDIR)\refclock_atom.obj" \
	"$(INTDIR)\refclock_chu.obj" \
	"$(INTDIR)\refclock_conf.obj" \
	"$(INTDIR)\refclock_datum.obj" \
	"$(INTDIR)\refclock_heath.obj" \
	"$(INTDIR)\refclock_irig.obj" \
	"$(INTDIR)\refclock_leitch.obj" \
	"$(INTDIR)\refclock_local.obj" \
	"$(INTDIR)\refclock_msfees.obj" \
	"$(INTDIR)\refclock_mx4200.obj" \
	"$(INTDIR)\refclock_nmea.obj" \
	"$(INTDIR)\refclock_parse.obj" \
	"$(INTDIR)\refclock_pst.obj" \
	"$(INTDIR)\refclock_ptbacts.obj" \
	"$(INTDIR)\refclock_tpro.obj" \
	"$(INTDIR)\refclock_trak.obj" \
	"$(INTDIR)\refclock_usno.obj" \
	"$(INTDIR)\refclock_wwvb.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\xntpd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xntpd - Win32 Debug"

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

ALL : "$(OUTDIR)\xntpd.exe" "$(OUTDIR)\xntpd.bsc"

CLEAN : 
	-@erase "$(INTDIR)\messages.res"
	-@erase "$(INTDIR)\ntp_config.obj"
	-@erase "$(INTDIR)\ntp_config.sbr"
	-@erase "$(INTDIR)\ntp_control.obj"
	-@erase "$(INTDIR)\ntp_control.sbr"
	-@erase "$(INTDIR)\ntp_filegen.obj"
	-@erase "$(INTDIR)\ntp_filegen.sbr"
	-@erase "$(INTDIR)\ntp_intres.obj"
	-@erase "$(INTDIR)\ntp_intres.sbr"
	-@erase "$(INTDIR)\ntp_io.obj"
	-@erase "$(INTDIR)\ntp_io.sbr"
	-@erase "$(INTDIR)\ntp_leap.obj"
	-@erase "$(INTDIR)\ntp_leap.sbr"
	-@erase "$(INTDIR)\ntp_loopfilter.obj"
	-@erase "$(INTDIR)\ntp_loopfilter.sbr"
	-@erase "$(INTDIR)\ntp_monitor.obj"
	-@erase "$(INTDIR)\ntp_monitor.sbr"
	-@erase "$(INTDIR)\ntp_peer.obj"
	-@erase "$(INTDIR)\ntp_peer.sbr"
	-@erase "$(INTDIR)\ntp_proto.obj"
	-@erase "$(INTDIR)\ntp_proto.sbr"
	-@erase "$(INTDIR)\ntp_refclock.obj"
	-@erase "$(INTDIR)\ntp_refclock.sbr"
	-@erase "$(INTDIR)\ntp_request.obj"
	-@erase "$(INTDIR)\ntp_request.sbr"
	-@erase "$(INTDIR)\ntp_restrict.obj"
	-@erase "$(INTDIR)\ntp_restrict.sbr"
	-@erase "$(INTDIR)\ntp_timer.obj"
	-@erase "$(INTDIR)\ntp_timer.sbr"
	-@erase "$(INTDIR)\ntp_unixclock.obj"
	-@erase "$(INTDIR)\ntp_unixclock.sbr"
	-@erase "$(INTDIR)\ntp_util.obj"
	-@erase "$(INTDIR)\ntp_util.sbr"
	-@erase "$(INTDIR)\ntpd.obj"
	-@erase "$(INTDIR)\ntpd.sbr"
	-@erase "$(INTDIR)\refclock_acts.obj"
	-@erase "$(INTDIR)\refclock_acts.sbr"
	-@erase "$(INTDIR)\refclock_as2201.obj"
	-@erase "$(INTDIR)\refclock_as2201.sbr"
	-@erase "$(INTDIR)\refclock_atom.obj"
	-@erase "$(INTDIR)\refclock_atom.sbr"
	-@erase "$(INTDIR)\refclock_chu.obj"
	-@erase "$(INTDIR)\refclock_chu.sbr"
	-@erase "$(INTDIR)\refclock_conf.obj"
	-@erase "$(INTDIR)\refclock_conf.sbr"
	-@erase "$(INTDIR)\refclock_datum.obj"
	-@erase "$(INTDIR)\refclock_datum.sbr"
	-@erase "$(INTDIR)\refclock_heath.obj"
	-@erase "$(INTDIR)\refclock_heath.sbr"
	-@erase "$(INTDIR)\refclock_irig.obj"
	-@erase "$(INTDIR)\refclock_irig.sbr"
	-@erase "$(INTDIR)\refclock_leitch.obj"
	-@erase "$(INTDIR)\refclock_leitch.sbr"
	-@erase "$(INTDIR)\refclock_local.obj"
	-@erase "$(INTDIR)\refclock_local.sbr"
	-@erase "$(INTDIR)\refclock_msfees.obj"
	-@erase "$(INTDIR)\refclock_msfees.sbr"
	-@erase "$(INTDIR)\refclock_mx4200.obj"
	-@erase "$(INTDIR)\refclock_mx4200.sbr"
	-@erase "$(INTDIR)\refclock_nmea.obj"
	-@erase "$(INTDIR)\refclock_nmea.sbr"
	-@erase "$(INTDIR)\refclock_parse.obj"
	-@erase "$(INTDIR)\refclock_parse.sbr"
	-@erase "$(INTDIR)\refclock_pst.obj"
	-@erase "$(INTDIR)\refclock_pst.sbr"
	-@erase "$(INTDIR)\refclock_ptbacts.obj"
	-@erase "$(INTDIR)\refclock_ptbacts.sbr"
	-@erase "$(INTDIR)\refclock_tpro.obj"
	-@erase "$(INTDIR)\refclock_tpro.sbr"
	-@erase "$(INTDIR)\refclock_trak.obj"
	-@erase "$(INTDIR)\refclock_trak.sbr"
	-@erase "$(INTDIR)\refclock_usno.obj"
	-@erase "$(INTDIR)\refclock_usno.sbr"
	-@erase "$(INTDIR)\refclock_wwvb.obj"
	-@erase "$(INTDIR)\refclock_wwvb.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\xntpd.bsc"
	-@erase "$(OUTDIR)\xntpd.exe"
	-@erase "$(OUTDIR)\xntpd.ilk"
	-@erase "$(OUTDIR)\xntpd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /ML /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\include\winnt" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\include\winnt"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/xntpd.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/messages.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/xntpd.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntp_config.sbr" \
	"$(INTDIR)\ntp_control.sbr" \
	"$(INTDIR)\ntp_filegen.sbr" \
	"$(INTDIR)\ntp_intres.sbr" \
	"$(INTDIR)\ntp_io.sbr" \
	"$(INTDIR)\ntp_leap.sbr" \
	"$(INTDIR)\ntp_loopfilter.sbr" \
	"$(INTDIR)\ntp_monitor.sbr" \
	"$(INTDIR)\ntp_peer.sbr" \
	"$(INTDIR)\ntp_proto.sbr" \
	"$(INTDIR)\ntp_refclock.sbr" \
	"$(INTDIR)\ntp_request.sbr" \
	"$(INTDIR)\ntp_restrict.sbr" \
	"$(INTDIR)\ntp_timer.sbr" \
	"$(INTDIR)\ntp_unixclock.sbr" \
	"$(INTDIR)\ntp_util.sbr" \
	"$(INTDIR)\ntpd.sbr" \
	"$(INTDIR)\refclock_acts.sbr" \
	"$(INTDIR)\refclock_as2201.sbr" \
	"$(INTDIR)\refclock_atom.sbr" \
	"$(INTDIR)\refclock_chu.sbr" \
	"$(INTDIR)\refclock_conf.sbr" \
	"$(INTDIR)\refclock_datum.sbr" \
	"$(INTDIR)\refclock_heath.sbr" \
	"$(INTDIR)\refclock_irig.sbr" \
	"$(INTDIR)\refclock_leitch.sbr" \
	"$(INTDIR)\refclock_local.sbr" \
	"$(INTDIR)\refclock_msfees.sbr" \
	"$(INTDIR)\refclock_mx4200.sbr" \
	"$(INTDIR)\refclock_nmea.sbr" \
	"$(INTDIR)\refclock_parse.sbr" \
	"$(INTDIR)\refclock_pst.sbr" \
	"$(INTDIR)\refclock_ptbacts.sbr" \
	"$(INTDIR)\refclock_tpro.sbr" \
	"$(INTDIR)\refclock_trak.sbr" \
	"$(INTDIR)\refclock_usno.sbr" \
	"$(INTDIR)\refclock_wwvb.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\xntpd.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wsock32.lib winmm.lib ..\libntp\WinDebug\libntp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\WinDebug\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/xntpd.pdb" /debug /machine:I386 /out:"$(OUTDIR)/xntpd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\messages.res" \
	"$(INTDIR)\ntp_config.obj" \
	"$(INTDIR)\ntp_control.obj" \
	"$(INTDIR)\ntp_filegen.obj" \
	"$(INTDIR)\ntp_intres.obj" \
	"$(INTDIR)\ntp_io.obj" \
	"$(INTDIR)\ntp_leap.obj" \
	"$(INTDIR)\ntp_loopfilter.obj" \
	"$(INTDIR)\ntp_monitor.obj" \
	"$(INTDIR)\ntp_peer.obj" \
	"$(INTDIR)\ntp_proto.obj" \
	"$(INTDIR)\ntp_refclock.obj" \
	"$(INTDIR)\ntp_request.obj" \
	"$(INTDIR)\ntp_restrict.obj" \
	"$(INTDIR)\ntp_timer.obj" \
	"$(INTDIR)\ntp_unixclock.obj" \
	"$(INTDIR)\ntp_util.obj" \
	"$(INTDIR)\ntpd.obj" \
	"$(INTDIR)\refclock_acts.obj" \
	"$(INTDIR)\refclock_as2201.obj" \
	"$(INTDIR)\refclock_atom.obj" \
	"$(INTDIR)\refclock_chu.obj" \
	"$(INTDIR)\refclock_conf.obj" \
	"$(INTDIR)\refclock_datum.obj" \
	"$(INTDIR)\refclock_heath.obj" \
	"$(INTDIR)\refclock_irig.obj" \
	"$(INTDIR)\refclock_leitch.obj" \
	"$(INTDIR)\refclock_local.obj" \
	"$(INTDIR)\refclock_msfees.obj" \
	"$(INTDIR)\refclock_mx4200.obj" \
	"$(INTDIR)\refclock_nmea.obj" \
	"$(INTDIR)\refclock_parse.obj" \
	"$(INTDIR)\refclock_pst.obj" \
	"$(INTDIR)\refclock_ptbacts.obj" \
	"$(INTDIR)\refclock_tpro.obj" \
	"$(INTDIR)\refclock_trak.obj" \
	"$(INTDIR)\refclock_usno.obj" \
	"$(INTDIR)\refclock_wwvb.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\xntpd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "xntpd - Win32 Release"
# Name "xntpd - Win32 Debug"

!IF  "$(CFG)" == "xntpd - Win32 Release"

!ELSEIF  "$(CFG)" == "xntpd - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\refclock_leitch.c
DEP_CPP_REFCL=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCL=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_leitch.obj" : $(SOURCE) $(DEP_CPP_REFCL) "$(INTDIR)"

"$(INTDIR)\refclock_leitch.sbr" : $(SOURCE) $(DEP_CPP_REFCL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_proto.c
DEP_CPP_NTP_P=\
	"..\include\ntp_control.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_P=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_proto.obj" : $(SOURCE) $(DEP_CPP_NTP_P) "$(INTDIR)"

"$(INTDIR)\ntp_proto.sbr" : $(SOURCE) $(DEP_CPP_NTP_P) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_acts.c
DEP_CPP_REFCLO=\
	"..\include\ntp_control.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLO=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_acts.obj" : $(SOURCE) $(DEP_CPP_REFCLO) "$(INTDIR)"

"$(INTDIR)\refclock_acts.sbr" : $(SOURCE) $(DEP_CPP_REFCLO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_leap.c
DEP_CPP_NTP_L=\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_L=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_leap.obj" : $(SOURCE) $(DEP_CPP_NTP_L) "$(INTDIR)"

"$(INTDIR)\ntp_leap.sbr" : $(SOURCE) $(DEP_CPP_NTP_L) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_atom.c
DEP_CPP_REFCLOC=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOC=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_atom.obj" : $(SOURCE) $(DEP_CPP_REFCLOC) "$(INTDIR)"

"$(INTDIR)\refclock_atom.sbr" : $(SOURCE) $(DEP_CPP_REFCLOC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_peer.c
DEP_CPP_NTP_PE=\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_PE=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_peer.obj" : $(SOURCE) $(DEP_CPP_NTP_PE) "$(INTDIR)"

"$(INTDIR)\ntp_peer.sbr" : $(SOURCE) $(DEP_CPP_NTP_PE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_irig.c
DEP_CPP_REFCLOCK=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_irig.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK) "$(INTDIR)"

"$(INTDIR)\refclock_irig.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_intres.c
DEP_CPP_NTP_I=\
	"..\include\ntp_io.h"\
	"..\include\ntp_request.h"\
	"..\include\ntp_select.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netdb.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_I=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_intres.obj" : $(SOURCE) $(DEP_CPP_NTP_I) "$(INTDIR)"

"$(INTDIR)\ntp_intres.sbr" : $(SOURCE) $(DEP_CPP_NTP_I) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_unixclock.c
DEP_CPP_NTP_U=\
	"..\include\ntp_io.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/param.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_U=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_unixclock.obj" : $(SOURCE) $(DEP_CPP_NTP_U) "$(INTDIR)"

"$(INTDIR)\ntp_unixclock.sbr" : $(SOURCE) $(DEP_CPP_NTP_U) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_nmea.c
DEP_CPP_REFCLOCK_=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_nmea.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_) "$(INTDIR)"

"$(INTDIR)\refclock_nmea.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_timer.c
DEP_CPP_NTP_T=\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/signal.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_T=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_timer.obj" : $(SOURCE) $(DEP_CPP_NTP_T) "$(INTDIR)"

"$(INTDIR)\ntp_timer.sbr" : $(SOURCE) $(DEP_CPP_NTP_T) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_mx4200.c
DEP_CPP_REFCLOCK_M=\
	"..\include\mx4200.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_M=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_mx4200.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_M) "$(INTDIR)"

"$(INTDIR)\refclock_mx4200.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_M) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_monitor.c
DEP_CPP_NTP_M=\
	"..\include\ntp_if.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\net/if.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_M=\
	".\..\..\..\sys\sync\queue.h"\
	".\..\..\..\sys\sync\sema.h"\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_monitor.obj" : $(SOURCE) $(DEP_CPP_NTP_M) "$(INTDIR)"

"$(INTDIR)\ntp_monitor.sbr" : $(SOURCE) $(DEP_CPP_NTP_M) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_local.c
DEP_CPP_REFCLOCK_L=\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_L=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_local.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_L) "$(INTDIR)"

"$(INTDIR)\refclock_local.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_L) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_usno.c
DEP_CPP_REFCLOCK_U=\
	"..\include\ntp_control.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_U=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_usno.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_U) "$(INTDIR)"

"$(INTDIR)\refclock_usno.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_U) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_ptbacts.c
DEP_CPP_REFCLOCK_P=\
	"..\include\ntp_control.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	".\refclock_acts.c"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_P=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_ptbacts.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_P) "$(INTDIR)"\
 ".\refclock_acts.c"

"$(INTDIR)\refclock_ptbacts.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_P) "$(INTDIR)"\
 ".\refclock_acts.c"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_io.c
DEP_CPP_NTP_IO=\
	"..\include\ntp_if.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_select.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\net/if.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/param.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_IO=\
	".\..\..\..\sys\sync\queue.h"\
	".\..\..\..\sys\sync\sema.h"\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_io.obj" : $(SOURCE) $(DEP_CPP_NTP_IO) "$(INTDIR)"

"$(INTDIR)\ntp_io.sbr" : $(SOURCE) $(DEP_CPP_NTP_IO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_pst.c
DEP_CPP_REFCLOCK_PS=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_PS=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_pst.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_PS) "$(INTDIR)"

"$(INTDIR)\refclock_pst.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_PS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_control.c
DEP_CPP_NTP_C=\
	"..\include\ntp_control.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_C=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_control.obj" : $(SOURCE) $(DEP_CPP_NTP_C) "$(INTDIR)"

"$(INTDIR)\ntp_control.sbr" : $(SOURCE) $(DEP_CPP_NTP_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_restrict.c
DEP_CPP_NTP_R=\
	"..\include\ntp_if.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\net/if.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_R=\
	".\..\..\..\sys\sync\queue.h"\
	".\..\..\..\sys\sync\sema.h"\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_restrict.obj" : $(SOURCE) $(DEP_CPP_NTP_R) "$(INTDIR)"

"$(INTDIR)\ntp_restrict.sbr" : $(SOURCE) $(DEP_CPP_NTP_R) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_chu.c
DEP_CPP_REFCLOCK_C=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_C=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_chu.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_C) "$(INTDIR)"

"$(INTDIR)\refclock_chu.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_conf.c
DEP_CPP_REFCLOCK_CO=\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_CO=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_conf.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_CO) "$(INTDIR)"

"$(INTDIR)\refclock_conf.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_CO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_trak.c
DEP_CPP_REFCLOCK_T=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_T=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_trak.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_T) "$(INTDIR)"

"$(INTDIR)\refclock_trak.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_T) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntpd.c
DEP_CPP_NTPD_=\
	"..\include\ntp_io.h"\
	"..\include\ntp_select.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/param.h"\
	"..\include\winnt\sys/resource.h"\
	"..\include\winnt\sys/signal.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	".\..\libntp\log.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTPD_=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntpd.obj" : $(SOURCE) $(DEP_CPP_NTPD_) "$(INTDIR)"

"$(INTDIR)\ntpd.sbr" : $(SOURCE) $(DEP_CPP_NTPD_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_wwvb.c
DEP_CPP_REFCLOCK_W=\
	"..\include\ntp_calendar.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_W=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_wwvb.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_W) "$(INTDIR)"

"$(INTDIR)\refclock_wwvb.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_W) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_config.c
DEP_CPP_NTP_CO=\
	"..\include\ntp_filegen.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\sys/wait.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_CO=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_config.obj" : $(SOURCE) $(DEP_CPP_NTP_CO) "$(INTDIR)"

"$(INTDIR)\ntp_config.sbr" : $(SOURCE) $(DEP_CPP_NTP_CO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_request.c
DEP_CPP_NTP_RE=\
	"..\include\ntp_control.h"\
	"..\include\ntp_if.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_request.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\net/if.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_RE=\
	".\..\..\..\sys\sync\queue.h"\
	".\..\..\..\sys\sync\sema.h"\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_request.obj" : $(SOURCE) $(DEP_CPP_NTP_RE) "$(INTDIR)"

"$(INTDIR)\ntp_request.sbr" : $(SOURCE) $(DEP_CPP_NTP_RE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_tpro.c
DEP_CPP_REFCLOCK_TP=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_TP=\
	".\..\include\gizmo_syslog.h"\
	".\sys\tpro.h"\
	

"$(INTDIR)\refclock_tpro.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_TP) "$(INTDIR)"

"$(INTDIR)\refclock_tpro.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_TP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_heath.c
DEP_CPP_REFCLOCK_H=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_H=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_heath.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_H) "$(INTDIR)"

"$(INTDIR)\refclock_heath.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_H) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_datum.c
DEP_CPP_REFCLOCK_D=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_D=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_datum.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_D) "$(INTDIR)"

"$(INTDIR)\refclock_datum.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_D) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_as2201.c
DEP_CPP_REFCLOCK_A=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_A=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_as2201.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_A) "$(INTDIR)"

"$(INTDIR)\refclock_as2201.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_A) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_parse.c
DEP_CPP_REFCLOCK_PA=\
	"..\include\ntp_control.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_select.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\parse.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	".\..\include\parse_conf.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_PA=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_parse.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_PA) "$(INTDIR)"

"$(INTDIR)\refclock_parse.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_PA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_loopfilter.c
DEP_CPP_NTP_LO=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_LO=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_loopfilter.obj" : $(SOURCE) $(DEP_CPP_NTP_LO) "$(INTDIR)"

"$(INTDIR)\ntp_loopfilter.sbr" : $(SOURCE) $(DEP_CPP_NTP_LO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_refclock.c
DEP_CPP_NTP_REF=\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_REF=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_refclock.obj" : $(SOURCE) $(DEP_CPP_NTP_REF) "$(INTDIR)"

"$(INTDIR)\ntp_refclock.sbr" : $(SOURCE) $(DEP_CPP_NTP_REF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_util.c
DEP_CPP_NTP_UT=\
	"..\include\ntp_filegen.h"\
	"..\include\ntp_if.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\net/if.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/ioctl.h"\
	"..\include\winnt\sys/resource.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_UT=\
	".\..\..\..\sys\sync\queue.h"\
	".\..\..\..\sys\sync\sema.h"\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_util.obj" : $(SOURCE) $(DEP_CPP_NTP_UT) "$(INTDIR)"

"$(INTDIR)\ntp_util.sbr" : $(SOURCE) $(DEP_CPP_NTP_UT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\refclock_msfees.c
DEP_CPP_REFCLOCK_MS=\
	"..\include\ntp_calendar.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_refclock.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntp_unixtime.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\sys/time.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_REFCLOCK_MS=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\refclock_msfees.obj" : $(SOURCE) $(DEP_CPP_REFCLOCK_MS) "$(INTDIR)"

"$(INTDIR)\refclock_msfees.sbr" : $(SOURCE) $(DEP_CPP_REFCLOCK_MS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntp_filegen.c
DEP_CPP_NTP_F=\
	"..\include\ntp_calendar.h"\
	"..\include\ntp_filegen.h"\
	"..\include\ntp_io.h"\
	"..\include\ntp_stdlib.h"\
	"..\include\ntp_string.h"\
	"..\include\ntp_syslog.h"\
	"..\include\ntp_types.h"\
	"..\include\ntpd.h"\
	"..\include\winnt\netinet/in.h"\
	"..\include\winnt\sys/socket.h"\
	"..\include\winnt\syslog.h"\
	".\..\include\l_stdlib.h"\
	".\..\include\ntp.h"\
	".\..\include\ntp_fp.h"\
	".\..\include\ntp_machine.h"\
	".\..\include\ntp_malloc.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NTP_F=\
	".\..\include\gizmo_syslog.h"\
	

"$(INTDIR)\ntp_filegen.obj" : $(SOURCE) $(DEP_CPP_NTP_F) "$(INTDIR)"

"$(INTDIR)\ntp_filegen.sbr" : $(SOURCE) $(DEP_CPP_NTP_F) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"

"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=..\libntp\messages.rc
DEP_RSC_MESSA=\
	".\..\libntp\MSG00001.bin"\
	

!IF  "$(CFG)" == "xntpd - Win32 Release"


"$(INTDIR)\messages.res" : $(SOURCE) $(DEP_RSC_MESSA) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/messages.res" /i "\xntp3-5.90.1b\libntp" /d\
 "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "xntpd - Win32 Debug"


"$(INTDIR)\messages.res" : $(SOURCE) $(DEP_RSC_MESSA) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/messages.res" /i "\xntp3-5.90.1b\libntp" /d\
 "_DEBUG" $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
