
cl /nologo /O2 /Ot /Og /Oi /Oy /Gs /GD /I..\vstsdk\ /LD ..\brokenfft\brokenfft.cpp ..\vstsdk\AudioEffect.cpp ..\vstsdk\audioeffectx.cpp ..\fft-lib\fftdom.cpp ..\fft-lib\fftmisc.c ..\fft-lib\fourierf.c -I..\fft-lib\ brokenfft.def /Fec:\progra~1\steinberg\vstplugins\dfx-brokenfft.dll
