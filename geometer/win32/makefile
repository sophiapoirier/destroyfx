# makefile for compiling with free mingw-x86_64 toolchain (GCC 9.2.0), 2020.

default: dfx-geometer-64.dll

# for 64 bits on windows
CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-gcc
WINDRES=x86_64-w64-mingw32-windres

DFXLIB=../../dfx-library
VSTSDK=../../vstsdk
DFXGUI=../../dfxgui
# git repository is called "vstgui" and contains a toplevel directory "vstgui"
VSTGUI=../../vstgui/vstgui

# WIN32_WINNT >= 0x0601 is critical (Windows 7) for compiling vstgui
#  -DENABLE_TRACE=1
DEFINES=-DWIN32=1 -D_WIN32_WINNT=0x0601 -DTARGET_OS_WIN32=1 -DTARGET_API_VST=1 -DVSTGUI_ENABLE_DEPRECATED_METHODS=0 -DNDEBUG=1
INCLUDES=-I .. -I $(DFXLIB) -I $(VSTSDK) -I $(DFXGUI) -I $(VSTGUI) -I $(VSTGUI)/plugin-bindings -include "../geometerdef.h"
CXXFLAGS=$(DEFINES) $(INCLUDES) -m64 -Wall -Wno-unknown-pragmas --std=c++17 -O2
# -mwindows to select the GUI subsystem (not console)
# -s (strip unexported symbols) for release! 18mb DLL unstripped, 2.5mb stripped.
# -static-libgcc and -static-libstdc++ avoid having the output depend on these mingw DLLs.
LFLAGS=-m64 -shared -mwindows -static-libgcc -static-libstdc++ -s

DFXLIB_OBJECTS=$(DFXLIB)/dfxplugin.o $(DFXLIB)/dfxplugin-vst.o $(DFXLIB)/dfxmisc.o $(DFXLIB)/dfxsettings.o $(DFXLIB)/dfxparameter.o $(DFXLIB)/dfxmidi.o $(DFXLIB)/dfxenvelope.o $(DFXLIB)/iirfilter.o $(DFXLIB)/dfxmutex.o

VSTSDK_OBJECTS=$(VSTSDK)/public.sdk/source/vst2.x/audioeffect.o $(VSTSDK)/public.sdk/source/vst2.x/audioeffectx.o $(VSTSDK)/public.sdk/source/vst2.x/vstplugmain.o

# The vstgui_win32 object #includes all the .cpp files in the library (!)
# It would be better for compile times to compile them individually, but including them all together usually
# permits more optimization, and it's certainly simpler...
VSTGUI_OBJECTS=$(VSTGUI)/vstgui_win32.o $(VSTGUI)/plugin-bindings/aeffguieditor.o

DFXGUI_OBJECTS=$(DFXGUI)/dfxguieditor.o $(DFXGUI)/dfxgui-fontfactory.o $(DFXGUI)/dfxguitextdisplay.o $(DFXGUI)/dfxguibutton.o $(DFXGUI)/dfxguislider.o $(DFXGUI)/dfxguidialog.o $(DFXGUI)/dfxguimisc.o $(DFXGUI)/dfxguicontrol.o  

OBJECTS=$(DFXLIB_OBJECTS) $(VSTSDK_OBJECTS) $(VSTGUI_OBJECTS) $(DFXGUI_OBJECTS) resources.o ../geometer.o ../gui/geometereditor.o ../gui/geometerview.o

MINGWLIB=/usr/x86_64-w64-mingw32/sys-root/mingw/lib

PTHREAD_OBJECTS=$(MINGWLIB)/libwinpthread.a
# These are needed by vstgui. We link against the .a files so that the
# resulting dll doesn't try (and fail) to find these as DLLs. (TODO: These standard
# windows ones might work with the import libs?)
VSTGUI_DEP_OBJECTS=$(MINGWLIB)/libopengl32.a $(MINGWLIB)/libole32.a $(MINGWLIB)/libd2d1.a $(MINGWLIB)/libdwrite.a $(MINGWLIB)/libshlwapi.a $(MINGWLIB)/libwindowscodecs.a $(MINGWLIB)/libuuid.a $(MINGWLIB)/libdwmapi.a


# embed png files.
resources.o : resources.rc ../gui/graphics/*
	$(WINDRES) -i resources.rc -o $@

dfx-geometer-64.dll : $(OBJECTS) $(PTHREAD_OBJECTS) $(VSTGUI_DEP_OBJECTS)
	$(CXX) $(LFLAGS) -o $@ $^

$(VSTGUI)/vstgui_win32.o : $(VSTGUI)/*.cpp $(VSTGUI)/lib/*.cpp $(VSTGUI)/lib/controls/*.cpp $(VSTGUI)/lib/animation/*.cpp $(VSTGUI)/lib/platform/common/*.cpp
	$(CXX) $(CXXFLAGS) $(VSTGUI)/vstgui_win32.cpp -c -o $@

# XXX just for Tom's testing
install : dfx-geometer-64.dll
	rm -f /d/cakewalk_bandlab/vst/dfx-geometer-64.dll
	cp dfx-geometer-64.dll /d/cakewalk_bandlab/vst/

clean :
	rm -f $(DFXLIB_OBJECTS) $(VSTSDK_OBJECTS) $(OBJECTS)
