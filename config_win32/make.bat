
echo ----- resource

rc /l 0x409  /d "NDEBUG" config.rc

echo ----- compile

cl /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c  config.c

echo ----- link

link /nologo config.obj /out:config.exe config.res kernel32.lib user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386