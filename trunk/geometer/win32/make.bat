
rc geometer.rc

cl /nologo /O2 /Ot /Og /Oi /Oy /Gs /GD /I..\vstsdk\ /I..\dfx-library\ /LD /D "GEOMETER_EXPORTS" ..\geometer\geometer.cpp ..\vstsdk\AudioEffect.cpp ..\vstsdk\audioeffectx.cpp geometer.def geometer.res ..\vstsdk_win32\vstgui.lib /Fec:\progra~1\steinberg\vstplugins\dfx-geometer.dll
