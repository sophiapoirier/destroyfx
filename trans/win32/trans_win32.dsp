# Microsoft Developer Studio Project File - Name="trans_win32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=trans_win32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "trans_win32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "trans_win32.mak" CFG="trans_win32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "trans_win32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "trans_win32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "trans_win32 - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRANS_WIN32_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /Ox /Ot /Og /Oi /Ob2 /I "../fftw/fftw" /I "../fftw/rfftw" /I "../vstsdk/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRANS_WIN32_EXPORTS" /YX /FD /c
# SUBTRACT CPP /Oa /Ow
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"c:\program files\steinberg\vstplugins\DFX trans.dll"

!ELSEIF  "$(CFG)" == "trans_win32 - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRANS_WIN32_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "../vstsdk/" /I "../fftw/fftw" /I "../fftw/rfftw" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRANS_WIN32_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"c:\program files\steinberg\vstplugins\DFX trans.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "trans_win32 - Win32 Release"
# Name "trans_win32 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "vstsdk_code"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\vstsdk\AudioEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\vstsdk\audioeffectx.cpp
# End Source File
# End Group
# Begin Group "fftw"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=..\fftw\fftw\config.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\executor.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_1.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_10.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_11.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_12.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_128.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_13.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_14.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_15.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_16.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_3.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_32.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_4.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_5.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_6.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_64.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_7.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_8.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fcr_9.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fftwf77.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fftwnd.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_10.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_16.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_3.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_32.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_4.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_5.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_6.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_7.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_8.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhb_9.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_10.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_16.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_3.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_32.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_4.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_5.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_6.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_7.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_8.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\fhf_9.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_1.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_10.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_11.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_12.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_13.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_14.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_15.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_16.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_3.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_32.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_4.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_5.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_6.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_64.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_7.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_8.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fn_9.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_1.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_10.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_11.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_12.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_13.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_14.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_15.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_16.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_3.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_32.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_4.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_5.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_6.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_64.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_7.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_8.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fni_9.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_1.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_10.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_11.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_12.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_128.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_13.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_14.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_15.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_16.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_3.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_32.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_4.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_5.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_6.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_64.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_7.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_8.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\frc_9.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_10.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_16.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_3.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_32.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_4.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_5.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_6.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_64.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_7.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_8.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftw_9.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_10.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_16.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_3.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_32.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_4.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_5.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_6.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_64.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_7.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_8.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\ftwi_9.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\generic.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\malloc.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\planner.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\putils.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\rader.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\rconfig.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\rexec.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\rexec2.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\rfftwf77.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\rfftwnd.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\rgeneric.c
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\rplanner.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\timer.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\twiddle.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\wisdom.c
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\wisdomio.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\trans\trans.cpp
# End Source File
# Begin Source File

SOURCE=.\trans.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "vstsdk"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\vstsdk\AEffect.h
# End Source File
# Begin Source File

SOURCE=..\vstsdk\aeffectx.h
# End Source File
# Begin Source File

SOURCE=..\vstsdk\AEffEditor.hpp
# End Source File
# Begin Source File

SOURCE=..\vstsdk\AudioEffect.hpp
# End Source File
# Begin Source File

SOURCE=..\vstsdk\audioeffectx.h
# End Source File
# Begin Source File

SOURCE=..\vstsdk\vstcontrols.h
# End Source File
# Begin Source File

SOURCE=..\vstsdk\vstgui.h
# End Source File
# End Group
# Begin Group "fftw_h"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\fftw\fftw\config.h
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\f77_func.h
# End Source File
# Begin Source File

SOURCE="..\fftw\fftw\fftw-int.h"
# End Source File
# Begin Source File

SOURCE=..\fftw\fftw\fftw.h
# End Source File
# Begin Source File

SOURCE=..\fftw\rfftw\rfftw.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\trans\trans.hpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
