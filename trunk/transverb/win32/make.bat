
rc transverb.rc

cl  /FD  /MD  /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "USING_HERMITE" /I..\transverb\gui\ /I..\transverb\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "TRANSVERB_EXPORTS" ..\transverb\transverbformalities.cpp ..\transverb\transverbprocess.cpp ..\transverb\GUI\transverbedit.cpp ..\transverb\GUI\transverbeditor.cpp ..\transverb\GUI\transverbeditmain.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\multikick.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\vstmidi.cpp ..\dfx-library\firfilter.cpp ..\dfx-library\dfxguimulticontrols.cpp ..\dfx-library\iirfilter.cpp ..\vstsdk\audioeffect.cpp ..\vstsdk\audioeffectx.cpp /c

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"transverb.pdb" /machine:I386 /def:".\transverb.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Transverb.dll" /implib:"transverb.lib" transverbformalities.obj transverbprocess.obj transverbedit.obj transverbeditmain.obj transverbeditor.obj dfxgui.obj dfxmisc.obj multikick.obj vstchunk.obj vstmidi.obj dfxguimulticontrols.obj audioeffect.obj audioeffectx.obj iirfilter.obj firfilter.obj transverb.res ..\vstsdk_win32\vstgui.lib

del *.obj
