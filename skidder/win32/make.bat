
rc skidder.rc

cl  /FD  /MD  /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /I..\skidder\gui\ /I..\skidder\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "MSKIDDER" /D "SKIDDER_EXPORTS" ..\skidder\skidderformalities.cpp ..\skidder\skidderprocess.cpp ..\skidder\skiddermidi.cpp ..\skidder\GUI\skidderedit.cpp ..\skidder\GUI\skiddereditor.cpp ..\skidder\GUI\skiddereditmain.cpp ..\dfx-library\dfxgui.cpp ..\dfx-library\dfxmisc.cpp ..\dfx-library\multikick.cpp ..\dfx-library\vstchunk.cpp ..\dfx-library\vstmidi.cpp  ..\dfx-library\dfxguimulticontrols.cpp ..\dfx-library\temporatetable.cpp ..\dfx-library\lfo.cpp ..\vstsdk\audioeffect.cpp ..\vstsdk\audioeffectx.cpp /c

link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"skidder.pdb" /machine:I386 /def:".\skidder.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Skidder.dll" /implib:"skidder.lib" skidderformalities.obj skiddermidi.obj skidderprocess.obj skidderedit.obj skiddereditmain.obj skiddereditor.obj dfxgui.obj dfxmisc.obj multikick.obj vstchunk.obj vstmidi.obj dfxguimulticontrols.obj audioeffect.obj audioeffectx.obj lfo.obj temporatetable.obj skidder.res ..\vstsdk_win32\vstgui.lib

del *.obj
del *.pdb
del *.idb
del *.res
del *.lib
