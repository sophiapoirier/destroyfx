
rc scrubby.rc

cl  /FD  /MD  /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "USE_SCRUBBY_TEMPO_RATES" /I..\scrubby\gui\ /I..\scrubby\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "SCRUBBY_EXPORTS" ..\scrubby\scrubbyformalities.cpp ..\scrubby\scrubbyprocess.cpp ..\scrubby\GUI\scrubbyedit.cpp ..\scrubby\GUI\scrubbyeditor.cpp ..\scrubby\GUI\scrubbykeyboard.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\multikick.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\vstmidi.cpp ..\dfx-library\temporatetable.cpp ..\dfx-library\dfxguimulticontrols.cpp ..\dfx-library\indexbitmap.cpp ..\vstsdk\audioeffect.cpp ..\vstsdk\audioeffectx.cpp /c

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"scrubby.pdb" /machine:I386 /def:".\scrubby.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Scrubby.dll" /implib:"scrubby.lib" scrubbyformalities.obj scrubbyprocess.obj scrubbyedit.obj scrubbyeditor.obj scrubbykeyboard.obj dfxgui.obj dfxmisc.obj multikick.obj vstchunk.obj vstmidi.obj temporatetable.obj dfxguimulticontrols.obj indexbitmap.obj audioeffect.obj audioeffectx.obj scrubby.res ..\vstsdk_win32\vstgui.lib

del *.obj
del *.pdb
del *.idb
del *.res
del *.lib
