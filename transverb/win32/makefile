
# (Experimental) makefile for compiling with free mingw-x86_64 toolchain (GCC 9.2.0), 2020.

default: dfx-transverb-64.dll

# for 64 bits on windows
CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-gcc
WINDRES=x86_64-w64-mingw32-windres

DFXLIB=../../dfx-library
VSTSDK=../../vstsdk
DFXGUI=../../dfxgui
# git repository is called "vstgui" and contains a toplevel directory "vstgui"
VSTGUI=../../../vstgui/vstgui

# WIN32_WINNT >= 0x0601 is critical (Windows 7) for compiling vstgui
DEFINES=-DWIN32 -D_WIN32_WINNT=0x0601 -DTARGET_API_VST -DENABLE_TRACE=1
INCLUDES=-I .. -I $(DFXLIB) -I $(VSTSDK) -I $(DFXGUI) -I $(VSTGUI) -I $(VSTGUI)/plugin-bindings -include "../transverbdef.h"
CXXFLAGS=$(DEFINES) $(INCLUDES) -m64 -Wall -Wno-unknown-pragmas --std=c++17 -O2
# mwindows to select the GUI subsystem (not console)
# -s to strip symbols (that aren't exported). This causes 'file' to say something
# different than my working reference dll, so left off for now.
# -static-libgcc and -static-libstdc++ avoid having the output depend on these mingw DLLs.
LFLAGS=-m64 -shared -mwindows -static-libgcc -static-libstdc++

# $(DFXLIB)/multikick.o  $(DFXLIB)/vstchunk.o $(DFXLIB)/dfxguimulticontrols.o
DFXLIB_OBJECTS=$(DFXLIB)/dfxmisc.o  $(DFXLIB)/dfxmidi.o $(DFXLIB)/firfilter.o $(DFXLIB)/iirfilter.o $(DFXLIB)/dfxenvelope.o $(DFXLIB)/dfxplugin.o $(DFXLIB)/dfxparameter.o $(DFXLIB)/dfxplugin-vst.o $(DFXLIB)/dfxsettings.o

VSTSDK_OBJECTS=$(VSTSDK)/pluginterfaces/vst2.x/audioeffect.o $(VSTSDK)/pluginterfaces/vst2.x/audioeffectx.o $(VSTSDK)/pluginterfaces/vst2.x/vstplugmain.o

# The vstgui_win32 object #includes all the .cpp files in the library (!)
# It would be better for compile times to compile them individually, but including them all together usually
# permits more optimization, and it's certainly simpler...
VSTGUI_OBJECTS=$(VSTGUI)/vstgui_win32.o $(VSTGUI)/plugin-bindings/aeffguieditor.o

DFXGUI_OBJECTS=$(DFXGUI)/dfxguitextdisplay.o $(DFXGUI)/dfxguieditor.o $(DFXGUI)/dfxguibutton.o $(DFXGUI)/dfxguislider.o $(DFXGUI)/dfxguicontrol.o $(DFXGUI)/dfxguidialog.o $(DFXGUI)/dfxguimisc.o

OBJECTS=$(DFXLIB_OBJECTS) $(VSTSDK_OBJECTS) $(VSTGUI_OBJECTS) $(DFXGUI_OBJECTS) resources.o ../transverbprocess.o ../transverbformalities.o ../gui/transverbeditor.o

MINGWLIB=/usr/x86_64-w64-mingw32/sys-root/mingw/lib

PTHREAD_OBJECTS=$(MINGWLIB)/libwinpthread.a
# These are needed by vstgui. We link against the .a files so that the
# resulting dll doesn't try (and fail) to find these as DLLs. (TODO: These standard
# windows ones might work with the import libs?)
VSTGUI_DEP_OBJECTS=$(MINGWLIB)/libopengl32.a $(MINGWLIB)/libole32.a $(MINGWLIB)/libd2d1.a $(MINGWLIB)/libdwrite.a $(MINGWLIB)/libshlwapi.a $(MINGWLIB)/libwindowscodecs.a $(MINGWLIB)/libuuid.a $(MINGWLIB)/libdwmapi.a


# embed png files. XXX this is an untested port from the old msvc code, and they used to be bmps.
# might need to do something custom?
resources.o : resources.rc ../gui/graphics/*
	$(WINDRES) -i resources.rc -o $@

# XXX -Wl stuff may be unnecessary. I think the .lib is only
# needed if we want to build something (e.g. host) that is linking against
# this DLL. And anyway mingw g++ can allegedly just get that same info
# from the DLL itself.
dfx-transverb-64.dll : $(OBJECTS) $(PTHREAD_OBJECTS) $(VSTGUI_DEP_OBJECTS)
	$(CXX) $(LFLAGS) -o $@ $^ -Wl,--out-implib,libdfx-transverb-64.a

$(VSTGUI)/vstgui_win32.o : $(VSTGUI)/*.cpp $(VSTGUI)/lib/*.cpp $(VSTGUI)/lib/controls/*.cpp $(VSTGUI)/lib/animation/*.cpp $(VSTGUI)/lib/platform/common/*.cpp
	$(CXX) $(CXXFLAGS) $(VSTGUI)/vstgui_win32.cpp -c -o $@

# Testing crap!
# don't use lflags here; we want a console executable...
load.exe : load.o $(PTHREAD_OBJECTS)
	$(CXX) -static -static-libgcc -static-libstdc++ -o $@ $^

# XXX just for Tom's testing
install : dfx-transverb-64.dll
	rm -f /d/cakewalk_bandlab/vst/dfx-transverb-64.dll
	cp dfx-transverb-64.dll /d/cakewalk_bandlab/vst/

# 
# rc transverb.rc
# 
# cl  /FD  /MD  /O2 /Ot /Og /Oi /Oy /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "USING_HERMITE" /I..\transverb\gui\ /I..\transverb\ /I..\vstsdk\ /I..\dfx-library\ /LD /D "TRANSVERB_EXPORTS" ..\transverb\transverbformalities.cpp ..\transverb\transverbprocess.cpp ..\transverb\GUI\transverbedit.cpp ..\transverb\GUI\transverbeditor.cpp ..\transverb\GUI\transverbeditmain.cpp   /c
# 
# link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"transverb.pdb" /machine:I386 /def:".\transverb.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\DFX Transverb.dll" /implib:"transverb.lib" transverbformalities.obj transverbprocess.obj transverbedit.obj transverbeditmain.obj transverbeditor.obj dfxgui.obj dfxmisc.obj multikick.obj vstchunk.obj vstmidi.obj dfxguimulticontrols.obj audioeffect.obj audioeffectx.obj iirfilter.obj firfilter.obj transverb.res ..\vstsdk_win32\vstgui.lib
# 
# del *.obj
# 



clean :
	rm -f $(DFXLIB_OBJECTS) $(VSTSDK_OBJECTS) $(OBJECTS)
