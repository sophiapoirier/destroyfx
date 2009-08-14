/*---------------------------------------------------------------

   (c) 2001, Sophberg Soft und Hard GmbH, All Rights Perversed
   (c) 2001, TOMBORG!!!@!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

---------------------------------------------------------------*/

#include "intercom.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


//----------------------------------------------------------------------- 
// constants & macros
const long kRMSWindowSize = 630;

// this is for shaping the parameter values into more dB-like values
#define noiseGainScaled(val) (val*val*4.0f)


//------------------------------------------------------------------
// initializations & such

Intercom::Intercom(audioMasterCallback audioMaster)
        : AudioEffectX(audioMaster, 1, kNumParameters) {
  fNoiseGain = 0.5f;

  setNumInputs(2);      // stereo in
  setNumOutputs(2);     // stereo out
  setUniqueID('DXic');  // identify
  canMono();    // it's okay to feed both inputs with the same signal
  canProcessReplacing();

  strcpy(programName, "Smoking is not permitted.");

  specialk = 0.0f;
  specialw = 0.05f;
  specialm = 1.0f;

  srand((unsigned int)time(NULL));	// sets a seed value for rand() from the system clock

  suspend();
}


void Intercom::suspend() {
  rms1 = 0.0;
  rms2 = 0.0;
  rmscount = 0;
}


// Destroy FX infos

bool Intercom::getEffectName(char *name) {
  strcpy (name, "Intercom (stereo)");   // name max 32 char
  return true;
}

long Intercom::getVendorVersion() {
  return 1;
}

bool Intercom::getErrorText(char *text) {
  strcpy (text, "I don't know what happened."); // max 256 char
  return true;
}

bool Intercom::getVendorString(char *text) {
  strcpy (text, "Destroy FX");
  return true;
}

bool Intercom::getProductString(char *text) {
  // a string identifying the product name (max 64 char)
  strcpy (text, "Super Destroy FX bipolar VST plugin pack");
  return true;
}


Intercom::~Intercom() {
        // nothing to do here
}


void Intercom::setProgramName(char *name) {
  strcpy(programName, name);
}


void Intercom::getProgramName(char *name) {
  strcpy(name, programName);
}


void Intercom::setParameter(long index, float value) {
  switch(index) {
  case kParam_NoiseGain:
    fNoiseGain = value;
    break;
  case kParam_SpecialK:
    specialk = value;
    break;
  case kParam_SpecialW:
    specialw = value;
    break;
  case kParam_SpecialM:
  default:
    specialm = value;
    break;
  }
}


float Intercom::getParameter(long index) {

  switch(index) {
  case kParam_NoiseGain:
    return fNoiseGain;
  case kParam_SpecialK:
    return specialk;
  case kParam_SpecialW:
    return specialw;
  case kParam_SpecialM:
  default:
    return specialm;
  }
}


// titles of each parameter

void Intercom::getParameterName(long index, char *label) {
  switch(index) {
  case kParam_NoiseGain:
    strcpy(label, "Brand Loyalty");
    break;
  case kParam_SpecialK:
    strcpy(label, "special k");
    break;
  case kParam_SpecialW:
    strcpy(label, "special w");
    break;
  default:
  case kParam_SpecialM:
    strcpy(label, "special m");
    break;
  }
}


// numerical display of each parameter's gradiations

void Intercom::getParameterDisplay(long index, char *text) {
#if TARGET_OS_MAC
  #define REGISTERED_SYMBOL_CHARACTER	"\xA8"
#else
  #define REGISTERED_SYMBOL_CHARACTER	"\xAE"
#endif

  switch(index) {
  case kParam_NoiseGain:
        if (fNoiseGain <= 0.25f)
                strcpy(text, "Sears"REGISTERED_SYMBOL_CHARACTER);
        if ( (fNoiseGain > 0.25f) && (fNoiseGain <= 0.5f) )
                strcpy(text, "Fisher Price"REGISTERED_SYMBOL_CHARACTER);
        if ( (fNoiseGain > 0.5f) && (fNoiseGain <= 0.75f) )
                strcpy(text, "Tonka"REGISTERED_SYMBOL_CHARACTER);
        if (fNoiseGain > 0.75f)
                strcpy(text, "Radio Shack"REGISTERED_SYMBOL_CHARACTER);
        break;
  default:
    {float val;
    switch(index) {
    case kParam_SpecialK:
      val = specialk; break;
    case kParam_SpecialW:
      val = specialw; break;
    case kParam_SpecialM:
    default:
      val = specialm;
    }
    int vv = val * 0xFFFFFFFE;
    text[0] = " .,:;1I#"[((vv >> 24)&255) >> 5];
    text[1] = " .,:;1I#"[((vv >> 16)&255) >> 5];
    text[2] = " .,:;1I#"[((vv >>  8)&255) >> 5];
    text[3] = " .,:;1I#"[((vv      )&255) >> 5];
    text[4] = 0;
    break;
    }
  }
}


// unit of measure for each parameter

void Intercom::getParameterLabel(long index, char *label) {
  if (index >= kParam_SpecialK)
    strcpy(label, "!!!!!");
  else strcpy(label, "");
}


void Intercom::process(float **inputs, float **outputs, long sf) {
  processX(inputs, outputs, sf, 0);
}

void Intercom::processReplacing(float **inputs, float **outputs, long samples) {
  processX(inputs, outputs, samples, 1);
}


void Intercom::processX(float **inputs, float **outputs, long samples, int replacing) {
  const float * in1 = inputs[0];
  const float * in2 = inputs[1];
  float * out1 = outputs[0];
  float * out2 = outputs[1];

  const float noiseGain = noiseGainScaled(fNoiseGain);
  const float randMaxInv = 1.0f / (float)RAND_MAX;

  // output a mix of (scaled) noise & the (unscaled) audio input
  for (long i=0; i < samples; i++) {
      // collect the RMS datas
#ifdef USE_BACKWARDS_RMS
        rms1 += sqrt( fabs((double)in1[i]) );
        rms2 += sqrt( fabs((double)in2[i]) );
#else
        rms1 += (double) (in1[i] * in1[i]);
        rms2 += (double) (in2[i] * in2[i]);
#endif
      rmscount++;

      // load up the latest RMS values if we're ready
      if (rmscount == kRMSWindowSize) {
          // calculate the RMS
#ifdef USE_BACKWARDS_RMS
            lastRMS1 = (float) pow( (rms1/(double)kRMSWindowSize), 2.0 );
            lastRMS2 = (float) pow( (rms2/(double)kRMSWindowSize), 2.0 );
#else
            lastRMS1 = (float) sqrt( rms1 / (double)kRMSWindowSize );
            lastRMS2 = (float) sqrt( rms2 / (double)kRMSWindowSize );
#endif

          // re-initialize these holders
          rms1 = 0.0;
          rms2 = 0.0;
          rmscount = 0;

          // the RMS is subtracted to get the space left over for noise
//        noiseAmp1 = noiseGain - lastRMS1;
//        noiseAmp2 = noiseGain - lastRMS2;
#ifdef USE_BACKWARDS_RMS
          noiseAmp1 = (0.288f - lastRMS1) * noiseGain;
          noiseAmp2 = (0.288f - lastRMS2) * noiseGain;
#else
          noiseAmp1 = (0.333f - lastRMS1) * noiseGain;
          noiseAmp2 = (0.333f - lastRMS2) * noiseGain;
#endif
          // safety to avoid negative amp scalars
          if (noiseAmp1 < 0.0f)
            noiseAmp1 = 0.0f;
          if (noiseAmp2 < 0.0f)
            noiseAmp2 = 0.0f;
        }

      float noise = (float)rand() * randMaxInv;
      if (replacing) {
        out1[i] = in1[i] + (noise * noiseAmp1);
        out2[i] = in2[i] + (noise * noiseAmp2);
      } else {
        out1[i] += in1[i] + (noise * noiseAmp1);
        out2[i] += in2[i] + (noise * noiseAmp2);
      }
    }

  /* SPECIAL ops 
     whether replacing or not, we move these around destructively.
   */
  for (long i = 0; i < samples; i++) {
    if (((float)rand() * randMaxInv) < specialk) {
      /* mobile */
      
      int j = (int) ((((float)rand() * randMaxInv) * samples) * specialw);
      j = (i + j) % samples;

      float t = (out1[i] * specialm) + (out1[j] * (1.0f - specialm));
      out1[i] = (out1[j] * specialm) + (out1[i] * (1.0f - specialm));
      out1[j] = t;

      t       = (out2[i] * specialm) + (out2[j] * (1.0f - specialm));
      out2[i] = (out2[j] * specialm) + (out2[i] * (1.0f - specialm));
      out2[j] = t;

    }
  }


}



/* ------------------------------ boring -------------------- */

// prototype of the export function main
#if BEOS
#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin(audioMasterCallback audioMaster);

#else
AEffect *main(audioMasterCallback audioMaster);
#endif

AEffect *main(audioMasterCallback audioMaster) {
  // get vst version
  if ( !audioMaster(0, audioMasterVersion, 0, 0, 0, 0) )
    return 0;  // old version

  AudioEffect* effect = new Intercom(audioMaster);
  if (!effect)
    return 0;
  return effect->getAeffect();
}

#if WIN32
#include <windows.h>
void* hInstance;
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
  hInstance = hInst;
  return 1;
}
#endif
