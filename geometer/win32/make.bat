
rc geometer.rc

# /nologo /mt /w3 /GX /YX /FD /c /mktyplib203 /win32 /l 0x409

# cl        /O2 /Ot /Og /Oi /Oy /Gs /GD /I..\geometer\gui\ /I..\geometer\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "GEOMETER_EXPORTS" ..\geometer\geometer.cpp ..\vstsdk\AudioEffect.cpp ..\vstsdk\audioeffectx.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\multikick.cpp ..\geometer\gui\geometeredit.cpp ..\geometer\gui\geometereditmain.cpp ..\geometer\gui\geometereditor.cpp geometer.def geometer.res ..\vstsdk_win32\vstgui.lib /Fec:\progra~1\steinberg\vstplugins\dfx-geometer.dll /link /nodefaultlib /libpath:..\vstsdk_win32\ vstgui.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSVCRT.lib mmc.lib mfc42.lib /verbose /dll /win32

# cl    /O2 /Ot /Og /Oi /Oy /Gs /GD /I..\geometer\gui\ /I..\geometer\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "GEOMETER_EXPORTS" ..\geometer\geometer.cpp ..\vstsdk\AudioEffect.cpp ..\vstsdk\audioeffectx.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\multikick.cpp ..\geometer\gui\geometeredit.cpp ..\geometer\gui\geometereditmain.cpp ..\geometer\gui\geometereditor.cpp geometer.def geometer.res ..\vstsdk_win32\vstgui.lib /Fec:\progra~1\steinberg\vstplugins\dfx-geometer.dll /link /nodefaultlib /libpath:..\vstsdk_win32\ vstgui.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSVCRT.lib mmc.lib mfc42.lib /verbose /dll /win32 /incremental:yes /pdb:"boring.pdb" /pdbtype:sept

cl  /FD  /MD  /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /I..\geometer\gui\ /I..\geometer\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "GEOMETER_EXPORTS" ..\geometer\geometer.cpp ..\vstsdk\AudioEffect.cpp ..\vstsdk\audioeffectx.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\multikick.cpp ..\geometer\gui\geometeredit.cpp ..\geometer\gui\geometereditmain.cpp ..\geometer\gui\geometereditor.cpp /c

# /Fec:\progra~1\steinberg\vstplugins\dfx-geometer.dll

# link /link /nodefaultlib /libpath:..\vstsdk_win32\ vstgui.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSVCRT.lib mmc.lib mfc42.lib /verbose /dll /win32 /incremental:yes /pdb:"boring.pdb" /pdbtype:sept

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"geometer.pdb" /machine:I386 /def:".\geometer.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Geometer.dll" /implib:"geometer.lib" AudioEffect.obj audioeffectx.obj dfxgui.obj dfxmisc.obj geometer.obj GeometerEdit.obj GeometerEditMain.obj GeometerEditor.obj MultiKick.obj VstChunk.obj geometer.res ..\vstsdk_win32\vstgui.lib
