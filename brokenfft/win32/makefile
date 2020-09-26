# makefile for compiling with free mingw-x86_64 toolchain (GCC 9.2.0), 2020.

default: dfx-brokenfft-64.dll

# for 64 bits on windows
CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-gcc
WINDRES=x86_64-w64-mingw32-windres

DFXLIB=../../dfx-library
VSTSDK=../../vstsdk
DFXGUI=../../dfxgui
# git repository is called "vstgui" and contains a toplevel directory "vstgui"
VSTGUI=../../vstgui/vstgui
FFTLIB=../../fft-lib
# (I forgot what rfftw is, but we use it. real fft?)
FFTW=../../fftw/fftw
RFFTW=../../fftw/rfftw

# WIN32_WINNT >= 0x0601 is critical (Windows 7) for compiling vstgui
#  -DENABLE_TRACE=1
DEFINES=-DWIN32=1 -D_WIN32_WINNT=0x0601 -DTARGET_OS_WIN32=1 -DTARGET_API_VST=1 -DVSTGUI_ENABLE_DEPRECATED_METHODS=0 -DNDEBUG=1
INCLUDES=-I .. -I $(DFXLIB) -I $(VSTSDK) -I $(DFXGUI) -I $(VSTGUI) -I $(VSTGUI)/plugin-bindings -I $(FFTLIB) -I $(FFTW) -I $(RFFTW) -include "../brokenfftdef.h"
CXXFLAGS=$(DEFINES) $(INCLUDES) -m64 -Wall -Wno-unknown-pragmas --std=c++17 -O2
CFLAGS=$(DEFINES) $(INCLUDES) -m64 -Wall -Wno-unknown-pragmas -O2
# -mwindows to select the GUI subsystem (not console)
# -s (strip unexported symbols) for release! 18mb DLL unstripped, 2.5mb stripped.
# -static-libgcc and -static-libstdc++ avoid having the output depend on these mingw DLLs.
LFLAGS=-m64 -shared -mwindows -static-libgcc -static-libstdc++ -s

%.o : %.c
	@$(CXX) $(CFLAGS) -c $< -o $@
	@bash -c "echo -n '.'"

FFTLIB_OBJECTS=$(FFTLIB)/fftdom.o $(FFTLIB)/fftmisc.o $(FFTLIB)/fourierf.o

RFFTW_OBJECTS=$(RFFTW)/fcr_1.o $(RFFTW)/fcr_10.o $(RFFTW)/fcr_11.o $(RFFTW)/fcr_12.o $(RFFTW)/fcr_128.o $(RFFTW)/fcr_13.o $(RFFTW)/fcr_14.o $(RFFTW)/fcr_15.o $(RFFTW)/fcr_16.o $(RFFTW)/fcr_2.o $(RFFTW)/fcr_3.o $(RFFTW)/fcr_32.o $(RFFTW)/fcr_4.o $(RFFTW)/fcr_5.o $(RFFTW)/fcr_6.o $(RFFTW)/fcr_64.o $(RFFTW)/fcr_7.o $(RFFTW)/fcr_8.o $(RFFTW)/fcr_9.o $(RFFTW)/fhb_10.o $(RFFTW)/fhb_16.o $(RFFTW)/fhb_2.o $(RFFTW)/fhb_3.o $(RFFTW)/fhb_32.o $(RFFTW)/fhb_4.o $(RFFTW)/fhb_5.o $(RFFTW)/fhb_6.o $(RFFTW)/fhb_7.o $(RFFTW)/fhb_8.o $(RFFTW)/fhb_9.o $(RFFTW)/fhf_10.o $(RFFTW)/fhf_16.o $(RFFTW)/fhf_2.o $(RFFTW)/fhf_3.o $(RFFTW)/fhf_32.o $(RFFTW)/fhf_4.o $(RFFTW)/fhf_5.o $(RFFTW)/fhf_6.o $(RFFTW)/fhf_7.o $(RFFTW)/fhf_8.o $(RFFTW)/fhf_9.o $(RFFTW)/frc_1.o $(RFFTW)/frc_10.o $(RFFTW)/frc_11.o $(RFFTW)/frc_12.o $(RFFTW)/frc_128.o $(RFFTW)/frc_13.o $(RFFTW)/frc_14.o $(RFFTW)/frc_15.o $(RFFTW)/frc_16.o $(RFFTW)/frc_2.o $(RFFTW)/frc_3.o $(RFFTW)/frc_32.o $(RFFTW)/frc_4.o $(RFFTW)/frc_5.o $(RFFTW)/frc_6.o $(RFFTW)/frc_64.o $(RFFTW)/frc_7.o $(RFFTW)/frc_8.o $(RFFTW)/frc_9.o $(RFFTW)/rconfig.o $(RFFTW)/rexec.o $(RFFTW)/rexec2.o $(RFFTW)/rfftwf77.o $(RFFTW)/rfftwnd.o $(RFFTW)/rgeneric.o $(RFFTW)/rplanner.o

FFTW_OBJECTS=$(FFTW)/config.o $(FFTW)/executor.o $(FFTW)/fftwf77.o $(FFTW)/fftwnd.o $(FFTW)/fn_1.o $(FFTW)/fn_10.o $(FFTW)/fn_11.o $(FFTW)/fn_12.o $(FFTW)/fn_13.o $(FFTW)/fn_14.o $(FFTW)/fn_15.o $(FFTW)/fn_16.o $(FFTW)/fn_2.o $(FFTW)/fn_3.o $(FFTW)/fn_32.o $(FFTW)/fn_4.o $(FFTW)/fn_5.o $(FFTW)/fn_6.o $(FFTW)/fn_64.o $(FFTW)/fn_7.o $(FFTW)/fn_8.o $(FFTW)/fn_9.o $(FFTW)/fni_1.o $(FFTW)/fni_10.o $(FFTW)/fni_11.o $(FFTW)/fni_12.o $(FFTW)/fni_13.o $(FFTW)/fni_14.o $(FFTW)/fni_15.o $(FFTW)/fni_16.o $(FFTW)/fni_2.o $(FFTW)/fni_3.o $(FFTW)/fni_32.o $(FFTW)/fni_4.o $(FFTW)/fni_5.o $(FFTW)/fni_6.o $(FFTW)/fni_64.o $(FFTW)/fni_7.o $(FFTW)/fni_8.o $(FFTW)/fni_9.o $(FFTW)/ftw_10.o $(FFTW)/ftw_16.o $(FFTW)/ftw_2.o $(FFTW)/ftw_3.o $(FFTW)/ftw_32.o $(FFTW)/ftw_4.o $(FFTW)/ftw_5.o $(FFTW)/ftw_6.o $(FFTW)/ftw_64.o $(FFTW)/ftw_7.o $(FFTW)/ftw_8.o $(FFTW)/ftw_9.o $(FFTW)/ftwi_10.o $(FFTW)/ftwi_16.o $(FFTW)/ftwi_2.o $(FFTW)/ftwi_3.o $(FFTW)/ftwi_32.o $(FFTW)/ftwi_4.o $(FFTW)/ftwi_5.o $(FFTW)/ftwi_6.o $(FFTW)/ftwi_64.o $(FFTW)/ftwi_7.o $(FFTW)/ftwi_8.o $(FFTW)/ftwi_9.o $(FFTW)/generic.o $(FFTW)/malloc.o $(FFTW)/planner.o $(FFTW)/putils.o $(FFTW)/rader.o $(FFTW)/timer.o $(FFTW)/twiddle.o $(FFTW)/wisdom.o $(FFTW)/wisdomio.o         

DFXLIB_OBJECTS=$(DFXLIB)/dfxplugin.o $(DFXLIB)/dfxplugin-vst.o $(DFXLIB)/dfxmisc.o $(DFXLIB)/dfxsettings.o $(DFXLIB)/dfxparameter.o $(DFXLIB)/dfxmidi.o $(DFXLIB)/dfxenvelope.o $(DFXLIB)/dfxmutex.o

VSTSDK_OBJECTS=$(VSTSDK)/public.sdk/source/vst2.x/audioeffect.o $(VSTSDK)/public.sdk/source/vst2.x/audioeffectx.o $(VSTSDK)/public.sdk/source/vst2.x/vstplugmain.o

# The vstgui_win32 object #includes all the .cpp files in the library (!)
# It would be better for compile times to compile them individually, but including them all together usually
# permits more optimization, and it's certainly simpler...
VSTGUI_OBJECTS=$(VSTGUI)/vstgui_win32.o $(VSTGUI)/plugin-bindings/aeffguieditor.o

DFXGUI_OBJECTS=$(DFXGUI)/dfxguieditor.o $(DFXGUI)/dfxgui-fontfactory.o $(DFXGUI)/dfxguitextdisplay.o $(DFXGUI)/dfxguibutton.o $(DFXGUI)/dfxguislider.o $(DFXGUI)/dfxguidialog.o $(DFXGUI)/dfxguimisc.o $(DFXGUI)/dfxguicontrol.o  

# XXX add back gui: 
# ../gui/brokenffteditor.o 
OBJECTS=$(DFXLIB_OBJECTS) $(VSTSDK_OBJECTS) $(VSTGUI_OBJECTS) $(DFXGUI_OBJECTS) $(FFTLIB_OBJECTS) $(RFFTW_OBJECTS) $(FFTW_OBJECTS) resources.o ../brokenfft.o ../gui/brokenfftview.o

MINGWLIB=/usr/x86_64-w64-mingw32/sys-root/mingw/lib

PTHREAD_OBJECTS=$(MINGWLIB)/libwinpthread.a
# These are needed by vstgui. We link against the .a files so that the
# resulting dll doesn't try (and fail) to find these as DLLs. (TODO: These standard
# windows ones might work with the import libs?)
VSTGUI_DEP_OBJECTS=$(MINGWLIB)/libopengl32.a $(MINGWLIB)/libole32.a $(MINGWLIB)/libd2d1.a $(MINGWLIB)/libdwrite.a $(MINGWLIB)/libshlwapi.a $(MINGWLIB)/libwindowscodecs.a $(MINGWLIB)/libuuid.a $(MINGWLIB)/libdwmapi.a


# embed png files.
resources.o : resources.rc ../gui/graphics/*
	$(WINDRES) -i resources.rc -o $@

dfx-brokenfft-64.dll : $(OBJECTS) $(PTHREAD_OBJECTS) $(VSTGUI_DEP_OBJECTS)
	$(CXX) $(LFLAGS) -o $@ $^

$(VSTGUI)/vstgui_win32.o : $(VSTGUI)/*.cpp $(VSTGUI)/lib/*.cpp $(VSTGUI)/lib/controls/*.cpp $(VSTGUI)/lib/animation/*.cpp $(VSTGUI)/lib/platform/common/*.cpp
	$(CXX) $(CXXFLAGS) $(VSTGUI)/vstgui_win32.cpp -c -o $@

# XXX just for Tom's testing
install : dfx-brokenfft-64.dll
	rm -f /d/cakewalk_bandlab/vst/dfx-brokenfft-64.dll
	cp dfx-brokenfft-64.dll /d/cakewalk_bandlab/vst/

clean :
	rm -f $(DFXLIB_OBJECTS) $(VSTSDK_OBJECTS) $(OBJECTS)
