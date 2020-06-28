
# (Experimental) makefile for compiling with free mingw-x86_64 toolchain (GCC 9.2.0), 2020.

default: dfx-transverb-64.dll

# for 64 bits on windows
CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-gcc
WINDRES=x86_64-w64-mingw32-windres

DFXLIB=../../dfx-library
VSTSDK=../../vstsdk
DFXGUI=../../dfxgui

DEFINES=-DWIN32 -DTARGET_API_VST
INCLUDES=-I $(DFXLIB) -I $(VSTSDK) -I $(DFXGUI) -include "../transverbdef.h"
CXXFLAGS=$(DEFINES) $(INCLUDES) -m64 -Wall -Wno-unknown-pragmas --std=c++17 -O2
# mwindows to select the GUI subsystem (not console)
# -s to strip symbols (that aren't exported). This causes 'file' to say something
# different than my working reference dll, so left off for now.
# -static-libgcc and -static-libstdc++ avoid having the output depend on these mingw DLLs.
LFLAGS=-m64 -shared -mwindows -static-libgcc -static-libstdc++

# $(DFXLIB)/multikick.o  $(DFXLIB)/vstchunk.o $(DFXLIB)/dfxguimulticontrols.o
DFXLIB_OBJECTS=$(DFXLIB)/dfxmisc.o  $(DFXLIB)/dfxmidi.o $(DFXLIB)/firfilter.o $(DFXLIB)/iirfilter.o $(DFXLIB)/dfxenvelope.o $(DFXLIB)/dfxplugin.o $(DFXLIB)/dfxparameter.o $(DFXLIB)/dfxplugin-vst.o $(DFXLIB)/dfxsettings.o

VSTSDK_OBJECTS=$(VSTSDK)/pluginterfaces/vst2.x/audioeffect.o $(VSTSDK)/pluginterfaces/vst2.x/audioeffectx.o $(VSTSDK)/pluginterfaces/vst2.x/vstplugmain.o

OBJECTS=$(DFXLIB_OBJECTS) $(VSTSDK_OBJECTS) resources.o ../transverbprocess.o ../transverbformalities.o # ../gui/transverbeditor.o

PTHREAD_OBJECTS=/usr/x86_64-w64-mingw32/sys-root/mingw/lib/libwinpthread.a

# embed png files. XXX this is an untested port from the old msvc code, and they used to be bmps.
# might need to do something custom?
resources.o : resources.rc ../gui/graphics/*
	$(WINDRES) -i resources.rc -o $@

# XXX -Wl stuff may be unnecessary. I think perhaps the .lib is only
# needed if we want to build something (e.g. host) that is linking against
# this DLL. And anyway mingw g++ can allegedly just get that same info
# from the DLL itself.
dfx-transverb-64.dll : $(OBJECTS) $(PTHREAD_OBJECTS)
	$(CXX) $(LFLAGS) -o $@ $^ -Wl,--out-implib,libdfx-transverb-64.a


# Testing crap!
# don't use lflags here; we want a console executable...
load.exe : load.o
	$(CXX) -o $@ $^

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
