default: rezsynth.png skidder.png bogrid.png dither.exe

# for 64 bits on windows
CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-gcc

# This is tom's cc-lib from tom7misf.sf.net (image reading/writing).
CCLIB=../../sf_svn/cc-lib

DEFINES=
INCLUDES=-I $(CCLIB)
CXXFLAGS=$(DEFINES) $(INCLUDES) -m64 -march=native -Wall --std=c++17 -O2
LFLAGS=-m64 -march=native -s -static

CCLIB_OBJECTS=$(CCLIB)/base/stringprintf.o $(CCLIB)/base/logging.o $(CCLIB)/util.o $(CCLIB)/image.o $(CCLIB)/stb_image.o $(CCLIB)/stb_image_write.o $(CCLIB)/stb_truetype.o $(CCLIB)/color-util.o $(CCLIB)/arcfour.o

OBJECTS=$(CCLIB_OBJECTS)

MINGWLIB=/usr/x86_64-w64-mingw32/sys-root/mingw/lib

rezsynth.exe : $(OBJECTS) rezsynth.o
	$(CXX) $(LFLAGS) -o $@ $^

skidder.exe : $(OBJECTS) skidder.o
	$(CXX) $(LFLAGS) -o $@ $^

bogrid.exe : $(OBJECTS) bogrid.o
	$(CXX) $(LFLAGS) -o $@ $^

rezsynth.png : rezsynth.exe
	./rezsynth.exe

skidder.png : skidder.exe skidder-*.png
	./skidder.exe

bogrid.png : bogrid.exe
	./bogrid.exe

dither.exe : $(OBJECTS) dither.o
	$(CXX) $(LFLAGS) -o $@ $^

clean :
	rm -f $(OBJECTS) rezsynth.exe skidder.exe rezsynth.png skidder.png *.o
