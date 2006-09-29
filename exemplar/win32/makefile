
INCLUDES = /I..\\ann\\include\\ -I..\\fftw\\rfftw\\ -I..\\fftw\\fftw\\ /I../exemplar/ /I..\\vstsdk\\ /I..\\dfx-library\\ /FI "..\\exemplar\\exemplardefs.h"

FLAGS = /FD  /MD /nologo /O2 /Ot /Og /Oi /Oy /GX /Gs /GD /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /D "ANN_PERF" /D "ANN_NO_RANDOM" /LD /D "TARGET_API_VST" /D "VST_NUM_CHANNELS=2" $(INCLUDES)

# ..\exemplar   cpp
SOURCES_EXEMPLAR = exemplar

# ..\vstsdk   cpp
SOURCES_VST = AudioEffect audioeffectx

# ..\dfx-library
SOURCES_DFX = dfxplugin dfxparameter dfxsettings dfxmidi dfxplugin-vst 

# ..\ann\src
SOURCES_ANN = ANN bd_fix_rad_search bd_pr_search bd_search bd_tree brute kd_dump kd_fix_rad_search kd_pr_search kd_search kd_split kd_tree kd_util perf

# ..\fftw\rfftw
SOURCES_FFT = fcr_1 fcr_10 fcr_11 fcr_12 fcr_128 fcr_13 fcr_14 fcr_15 fcr_16 fcr_2 fcr_3 fcr_32 fcr_4 fcr_5 fcr_6 fcr_64 fcr_7 fcr_8 fcr_9 fhb_10 fhb_16 fhb_2 fhb_3 fhb_32 fhb_4 fhb_5 fhb_6 fhb_7 fhb_8 fhb_9 fhf_10 fhf_16 fhf_2 fhf_3 fhf_32 fhf_4 fhf_5 fhf_6 fhf_7 fhf_8 fhf_9 frc_1 frc_10 frc_11 frc_12 frc_128 frc_13 frc_14 frc_15 frc_16 frc_2 frc_3 frc_32 frc_4 frc_5 frc_6 frc_64 frc_7 frc_8 frc_9 rconfig rexec rexec2 rfftwf77 rfftwnd rgeneric rplanner

# ..\fftw\fftw
SOURCES_FFT2 = config executor fftwf77 fftwnd fn_1 fn_10 fn_11 fn_12 fn_13 fn_14 fn_15 fn_16 fn_2 fn_3 fn_32 fn_4 fn_5 fn_6 fn_64 fn_7 fn_8 fn_9 fni_1 fni_10 fni_11 fni_12 fni_13 fni_14 fni_15 fni_16 fni_2 fni_3 fni_32 fni_4 fni_5 fni_6 fni_64 fni_7 fni_8 fni_9 ftw_10 ftw_16 ftw_2 ftw_3 ftw_32 ftw_4 ftw_5 ftw_6 ftw_64 ftw_7 ftw_8 ftw_9 ftwi_10 ftwi_16 ftwi_2 ftwi_3 ftwi_32 ftwi_4 ftwi_5 ftwi_6 ftwi_64 ftwi_7 ftwi_8 ftwi_9 generic malloc planner putils rader timer twiddle wisdom wisdomio

%.obj : ../exemplar/%.cpp
	cl $(FLAGS) /c $^

%.obj : ../vstsdk/%.cpp
	cl $(FLAGS) /c $^

%.obj : ../dfx-library/%.cpp
	cl $(FLAGS) /c $^

%.obj : ../fftw/rfftw/%.c
	cl $(FLAGS) /c $^

%.obj : ../fftw/fftw/%.c
	cl $(FLAGS) /c $^



ALLOBJECTS = \
   $(addsuffix .obj, $(SOURCES_EXEMPLAR)) \
   $(addsuffix .obj, $(SOURCES_VST)) \
   $(addsuffix .obj, $(SOURCES_DFX)) \
   $(addsuffix .obj, $(SOURCES_ANN)) \
   $(addsuffix .obj, $(SOURCES_FFT)) \
   $(addsuffix .obj, $(SOURCES_FFT2)) \


# actually puts it in vstplugins dir
dfx_exemplar.dll : $(ALLOBJECTS)
#	rem $(ALLSOURCES)
#	echo $(SOURCES_FFT2)
#	echo $(FLAGS)
	link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"exemplar.pdb" /machine:I386 /def:".\exemplar.def" /out:"C:\Progra~1\Steinberg\VstPlugIns\dfx Exemplar.dll" /implib:"exemplar.lib" ..\\vstsdk_win32\\vstgui.lib $(ALLOBJECTS)

clean :
	rm -f *.obj core
