
rc firefly.rc

cl  /FD  /MD  /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GUI" /I..\firefly\gui\ /I..\firefly\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "FIREFLY_EXPORTS" ..\firefly\firefly.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\multikick.cpp ..\dfx-library\indexbitmap.cpp ..\firefly\gui\fireflyeditor.cpp ..\vstsdk\AudioEffect.cpp ..\vstsdk\audioeffectx.cpp /c

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"firefly.pdb" /machine:I386 /def:".\firefly.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Firefly.dll" /implib:"firefly.lib" AudioEffect.obj audioeffectx.obj dfxgui.obj dfxmisc.obj firefly.obj FireflyEditor.obj MultiKick.obj VstChunk.obj firefly.res indexbitmap.obj ..\vstsdk_win32\vstgui.lib
