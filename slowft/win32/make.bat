@echo off

rem    To use this, run c:\progra~1\micros~2\vc98\bin\vcvars32.bat 
rem    (or wherever that file is located).

echo -------------- resources.

rem    You might want to compile some resources, if your plugin has a GUI.

rem rc something.rc

echo -------------- compile.

cl  /FD  /MD /nologo /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /I..\slowft\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "TARGET_API_VST" /FI "..\slowft\slowftdefs.h" /D "VST_NUM_CHANNELS=2" ..\slowft\slowft.cpp ..\vstsdk\audioeffect.cpp ..\vstsdk\audioeffectx.cpp ..\dfx-library\dfxplugin.cpp ..\dfx-library\dfxparameter.cpp ..\dfx-library\dfxsettings.cpp ..\dfx-library\dfxmidi.cpp ..\dfx-library\dfxplugin-vst.cpp /c

echo -------------- link.

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"slowft.pdb" /machine:I386 /def:".\slowft.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Slowft.dll" /implib:"slowft.lib" audioeffect.obj audioeffectx.obj slowft.obj  dfxplugin.obj dfxplugin-vst.obj dfxparameter.obj dfxsettings.obj dfxmidi.obj ..\vstsdk_win32\vstgui.lib

