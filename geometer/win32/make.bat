
rc geometer.rc

cl  /FD  /MD  /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /I..\geometer\gui\ /I..\geometer\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "GEOMETER_EXPORTS" ..\geometer\gui\geometerview.cpp ..\geometer\geometer.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\multikick.cpp ..\dfx-library\indexbitmap.cpp ..\geometer\gui\geometereditor.cpp ..\vstsdk\AudioEffect.cpp ..\vstsdk\audioeffectx.cpp /c

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"geometer.pdb" /machine:I386 /def:".\geometer.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Geometer.dll" /implib:"geometer.lib" AudioEffect.obj audioeffectx.obj dfxgui.obj dfxmisc.obj geometer.obj GeometerEditor.obj MultiKick.obj VstChunk.obj geometer.res geometerview.obj indexbitmap.obj ..\vstsdk_win32\vstgui.lib
