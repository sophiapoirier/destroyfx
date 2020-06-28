
# (Experimental) makefile for compiling with free mingw-x86_64 toolchain (GCC 9.2.0), 2020.

default: gendef.exe

# for 64 bits on windows
CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-g++

CFLAGS=-Wall -O2

OBJECTS=gendef_def.o gendef.o fsredir.o compat_string.o

gendef.exe : $(OBJECTS)
	$(CC) -o $@ $^ -static

clean :
	rm -f $(OBJECTS)
