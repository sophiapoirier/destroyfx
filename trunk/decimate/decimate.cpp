
/*
  Sonic Decimatatronifier

  Effect known as "Decimate" or "Bitcrush" -- 2 DA EXTREME!!
                                   (ARE U D00D ENOUGH 2 HANDLE IT???)
 */

#include "decimate.hpp"


Decimate::Decimate(audioMasterCallback audioMaster)
  : AudioEffectX(audioMaster, 1, 3) {
  bitres = 1.;
  samplehold = 1.0;
  samplesleft = 0;
  destruct = 1.0;
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID('T7dc');
  canMono();
  canProcessReplacing();
  strcpy(programName, "Decimatatronifier");
}

Decimate::~Decimate() {
}

void Decimate::setProgramName(char *name) {
  strcpy(programName, name);
}

void Decimate::getProgramName(char *name) {
  strcpy(name, programName);
}

void Decimate::setParameter(long index, float value) {
  switch (index) {
  case 0:
    bitres = value;
    break;
  case 1:
    samplehold = value;
    break;
  case 2: 
    destruct = value;
    break;
  }
}

float Decimate::getParameter(long index) {
  switch (index) {
  default:
  case 0: return bitres;
  case 1: return samplehold;
  case 2: return destruct;
  }
}

void Decimate::getParameterName(long index, char *label) {
  switch(index) {
  default:
  case 0:
    strcpy(label, "bits");
    break;
  case 1:
    strcpy(label, "samples");
    break;
  case 2:
    strcpy(label, "DESTROY");
  }
}

void Decimate::getParameterDisplay(long index, char *text) {
  switch(index) {
  case 0:
    float2string(bitres, text);
    break;
  case 1:
    float2string(samplehold, text);
    break;
  case 2:
    float2string(destruct, text);
  }
}

void Decimate::getParameterLabel(long index, char *label) {
  strcpy(label, "numbers");
}

inline float quantize(float old, float q) {
  if (q >= 1.0) return old;

  /* favor low end, more exciting there */
  old = old * old * old;
#if 1
  long scale = ( long)(512 * q);
  if (scale == 0) scale = 2;
  long X = ( long)(scale * old);
        
  return (float)(X / (float)scale);
#else

  float scale = ((float)512. * q);
  if (scale == 0.0) scale = 1.1;
  float X = (scale * old);
        
  return (float)(X / (float)scale);

#if 0
  return ( ((float)0xFFFFFFFF * q) * old) / ((float)0xFFFFFFFF * q);
#endif
#endif
}

inline float destroy(float q, float old) {
  if (old >= 0.999999) return q;
  if (q <= 0.000001) return 1.0;
  
  unsigned long scale = (unsigned long)(65536 * q);
        
  unsigned long X = (unsigned long)(scale * old);
        
  return (float)(X / (float)scale);
}


void Decimate::process(float **inputs, float **outputs, long sampleFrames) {
  float *in1  =  inputs[0];
  float *in2  =  inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];

  while(--sampleFrames >= 0) {
    if (samplesleft--) {
      in1++;
      in2++;
    } else {
      bit1 = *in1++;
      bit2 = *in2++;
      samplesleft = (int)(512 * (1.0 - samplehold));
    }
    (*out1++) += destroy(quantize(bit1,bitres), destruct);    // accumulating
    (*out2++) += destroy(quantize(bit2,bitres), destruct);
                        
  }
}

//-----------------------------------------------------------------------------------------
void Decimate::processReplacing(float **inputs, float **outputs, 
                                long sampleFrames) {
  float *in1  =  inputs[0];
  float *in2  =  inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];

  while(--sampleFrames >= 0) {
    if (samplesleft--) {
      in1++;
      in2++;
    } else {
      bit1 = *in1++;
      bit2 = *in2++;
      samplesleft = (int)(256 * (1.0 - samplehold));
    }
    (*out1++) = destroy(quantize(bit1,bitres), destruct);    // overwrite
    (*out2++) = destroy(quantize(bit2,bitres), destruct);

  }
}


/* ------------------------------ boring -------------------- */

static AudioEffect *effect = 0;
bool oome = false;

#if MAC
#pragma export on
#endif

// prototype of the export function main
#if BEOS
#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin (audioMasterCallback audioMaster);

#else
AEffect *main (audioMasterCallback audioMaster);
#endif

AEffect *main (audioMasterCallback audioMaster) {
  // get vst version
  if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
    return 0;  // old version

  effect = new Decimate (audioMaster);
  if (!effect)
    return 0;
  if (oome) {
      delete effect;
      return 0;
    }
  return effect->getAeffect ();
}

#if MAC
#pragma export off
#endif


#if WIN32
#include <windows.h>
void* hInstance;
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
  hInstance = hInst;
  return 1;
}
#endif
