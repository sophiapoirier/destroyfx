
rc bufferoverride.rc

cl  /FD  /MD  /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /I..\bufferoverride\gui\ /I..\bufferoverride\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "USE_BUFFER_OVERRIDE_TEMPO_RATES" /D "BUFFEROVERRIDE_STEREO" /D "BUFFEROVERRIDE_EXPORTS" ..\bufferoverride\bufferoverrideformalities.cpp ..\bufferoverride\bufferoverrideprocess.cpp ..\bufferoverride\bufferoverridemidi.cpp ..\bufferoverride\GUI\bufferoverrideedit.cpp ..\bufferoverride\GUI\bufferoverrideeditor.cpp ..\bufferoverride\GUI\bufferoverrideeditmain.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\multikick.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\vstmidi.cpp  ..\dfx-library\dfxguimulticontrols.cpp ..\dfx-library\temporatetable.cpp ..\dfx-library\lfo.cpp ..\vstsdk\audioeffect.cpp ..\vstsdk\audioeffectx.cpp /c

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"bufferoverride.pdb" /machine:I386 /def:".\bufferoverridestereo.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Buffer Override stereo.dll" /implib:"bufferoverride.lib" bufferoverrideformalities.obj bufferoverridemidi.obj bufferoverrideprocess.obj bufferoverrideedit.obj bufferoverrideeditmain.obj bufferoverrideeditor.obj dfxgui.obj dfxmisc.obj multikick.obj vstchunk.obj vstmidi.obj dfxguimulticontrols.obj audioeffect.obj audioeffectx.obj lfo.obj temporatetable.obj bufferoverride.res ..\vstsdk_win32\vstgui.lib

del *.obj
del *.pdb
del *.idb
del *.res
del *.lib
