@echo off

rem    To use this, run c:\progra~1\micros~2\vc98\bin\vcvars32.bat 
rem    (or wherever that file is located).

echo -------------- resources.

rc geometer.rc

echo -------------- compile.

rem   XXX remove TARGET_PLUGIN_USES_VSTGUI define once we have our GUI lib
cl  /FD  /MD /nologo /O2 /Ot /Og /Oi /Oy /Gs /GD /D "TARGET_PLUGIN_USES_VSTGUI" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /I..\geometer\gui\ /I..\geometer\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "GEOMETER_EXPORTS" /D "TARGET_API_VST" /FI "..\geometer\geometerdef.h" /D "VST_NUM_CHANNELS=2" ..\geometer\geometer.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\multikick.cpp ..\dfx-library\indexbitmap.cpp ..\geometer\gui\geometereditor.cpp ..\geometer\gui\geometerview.cpp ..\vstsdk\AudioEffect.cpp ..\vstsdk\audioeffectx.cpp ..\dfx-library\dfxplugin.cpp ..\dfx-library\dfxparameter.cpp ..\dfx-library\dfxsettings.cpp ..\dfx-library\dfxmidi.cpp ..\dfx-library\dfxplugin-vst.cpp /c

echo -------------- link.

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"geometer.pdb" /machine:I386 /def:".\geometer.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Geometer.dll" /implib:"geometer.lib" AudioEffect.obj audioeffectx.obj dfxgui.obj geometer.obj geometereditor.obj multikick.obj geometer.res geometerview.obj indexbitmap.obj dfxplugin.obj dfxplugin-vst.obj dfxparameter.obj dfxsettings.obj dfxmidi.obj ..\vstsdk_win32\vstgui.lib
