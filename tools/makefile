
default : randbench.exe

CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-gcc
WINDRES=x86_64-w64-mingw32-windres

DEFINES=-DWIN32=1 -D_WIN32_WINNT=0x0601 -DTARGET_OS_WIN32=1 -DTARGET_API_VST=1 -DVSTGUI_ENABLE_DEPRECATED_METHODS=0 -DNDEBUG=1 -DGetMatchingFonts=GetMatchingFonts_

INCLUDES=-I../dfx-library

CXXFLAGS=$(DEFINES) $(INCLUDES) -m64 -Wall -Wno-unknown-pragmas --std=c++17 -O2

LFLAGS=-m64 -static -mwindows
# -static-libgcc -static-libstdc++ -s

randbench.exe : randbench.o ../dfx-library/dfxmath.h
	$(CXX) -o $@ $< $(LFLAGS)


clean :
	rm -f *.exe *.o
