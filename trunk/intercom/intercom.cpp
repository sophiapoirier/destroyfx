/*---------------------------------------------------------------

   (c) 2001, Marcberg Soft und Hard GmbH, All Rights Perversed
   (c) 2001, TOMBORG!!!@!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

---------------------------------------------------------------*/

#ifndef __intercom
#include "intercom.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

//------------------------------------------------------------------
// initializations & such

Intercom::Intercom(audioMasterCallback audioMaster)
        : AudioEffectX(audioMaster, 1, 4) {
  fNoiseGain = 0.5f;

  setNumInputs(2);      // stereo in
  setNumOutputs(2);     // stereo out
  setUniqueID('DXic');  // identify
  canMono();    // it's okay to feed both inputs with the same signal
  canProcessReplacing();

  strcpy(programName, "Smoking is not permitted.");

  rms1 = 0.0;
  rms2 = 0.0;
  rmscount = 0;

  specialk = 0.0f;
  specialw = 0.05f;
  specialm = 1.0f;

  srand((unsigned int)time(NULL));	// sets a seed value for rand() from the system clock
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
  case 0:
    fNoiseGain = value;
    break;
  case 1:
    specialk = value;
    break;
  case 2:
    specialw = value;
    break;
  case 3:
  default:
    specialm = value;
    break;
  }
}


float Intercom::getParameter(long index) {

  switch(index) {
  case 0:
    return fNoiseGain;
  case 1:
    return specialk;
  case 2:
    return specialw;
  case 3:
  default:
    return specialm;
  }
}


// titles of each parameter

void Intercom::getParameterName(long index, char *label) {
  switch(index) {
  case 0:
    strcpy(label, "Brand Loyalty");
    break;
  case 1:
    strcpy(label, "special k");
    break;
  case 2:
    strcpy(label, "special w");
    break;
  default:
  case 3:
    strcpy(label, "special m");
    break;
  }
}


// numerical display of each parameter's gradiations

void Intercom::getParameterDisplay(long index, char *text) {
  switch(index) {
  case 0:
        if (fNoiseGain <= 0.25f)
                strcpy(text, "Sears\xAE");
        if ( (fNoiseGain > 0.25f) && (fNoiseGain <= 0.5f) )
                strcpy(text, "Fisher Price\xAE");
        if ( (fNoiseGain > 0.5f) && (fNoiseGain <= 0.75f) )
                strcpy(text, "Tonka\xAE");
        if (fNoiseGain > 0.75f)
                strcpy(text, "Radio Shack\xAE");
        break;
  default:
    {float val;
    switch(index) {
    case 1:
      val = specialk; break;
    case 2:
      val = specialw; break;
    case 3:
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
  if (index > 1)
    strcpy(label, "!!!!!");
  else strcpy(label, "");
}


void Intercom::process(float **inputs, float **outputs, long sf) {
  processX(inputs, outputs, sf, 0);
}
                       

void Intercom::processX(float **inputs, float **outputs, long samples,
                        int replacing) {
  float *in1  =  inputs[0];
  float *in2  =  inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float * out1b = out1;
  float * out2b = out2;


  long sampleCount;
  float noise;

  // output a mix of (scaled) noise & the (unscaled) audio input
  for (sampleCount=0; sampleCount < samples; sampleCount++) {
      // collect the RMS datas
#ifdef USE_BACKWARDS_RMS
        rms1 += sqrt( fabs((double)(*in1)) );
        rms2 += sqrt( fabs((double)(*in2)) );
#else
        rms1 += (double) ( (*in1) * (*in1) );
        rms2 += (double) ( (*in2) * (*in2) );
#endif
      rmscount++;

      // load up the latest RMS values if we're ready
      if (rmscount == RMS_WINDOW) {
          // calculate the RMS
#ifdef USE_BACKWARDS_RMS
            lastRMS1 = (float) pow( (rms1/(double)RMS_WINDOW), 2.0 );
            lastRMS2 = (float) pow( (rms2/(double)RMS_WINDOW), 2.0 );
#else
            lastRMS1 = (float) sqrt( rms1 / (double)RMS_WINDOW );
            lastRMS2 = (float) sqrt( rms2 / (double)RMS_WINDOW );
#endif

          // re-initialize these holders
          rms1 = 0.0;
          rms2 = 0.0;
          rmscount = 0;

          // the RMS is subtracted to get the space left over for noise
          //                    noiseAmp1 = noiseGain - lastRMS1;
          //                    noiseAmp2 = noiseGain - lastRMS2;
#ifdef USE_BACKWARDS_RMS
          noiseAmp1 = (0.288f - lastRMS1) * noiseGain;
          noiseAmp2 = (0.288f - lastRMS2) * noiseGain;
#else
          noiseAmp1 = (0.333f - lastRMS1) * noiseGain;
          noiseAmp2 = (0.333f - lastRMS2) * noiseGain;
#endif
          // safety to avoid negative amp scalars
          if (noiseAmp1 < 0.0)
            noiseAmp1 = 0.0f;
          if (noiseAmp2 < 0.0)
            noiseAmp2 = 0.0f;
        }

      noise = (float)rand() / (float)RAND_MAX;
      if (replacing) {
        (*out1++) = (*in1++) + (noise * noiseAmp1);
        (*out2++) = (*in2++) + (noise * noiseAmp2);
      } else {
        (*out1++) += (*in1++) + (noise * noiseAmp1);
        (*out2++) += (*in2++) + (noise * noiseAmp2);
      }
    }

  /* SPECIAL ops 
     whether replacing or not, we move these around destructively.
   */
  for(int i = 0; i < samples; i++) {
    if (((float)rand() / (float)RAND_MAX) < specialk) {
      /* mobile */
      
      int j = (int) ((((float)rand() / (float)RAND_MAX) * samples) * specialw);
      j = (i + j) % samples;

      float t = (float)((out1b[i] * specialm) + (out1b[j] * (1.0 - specialm)));
      out1b[i] =(float)((out1b[j] * specialm) + 
                        (out1b[i] * (1.0 - specialm)));
      out1b[j] = t;

      t = (float)((out2b[i] * specialm) + (out2b[j] * (1.0 - specialm)));
      out2b[i] =(float)((out2b[j] * specialm) + (out2b[i] * (1.0 - specialm)));
      out2b[j] = t;

    }
  }


}


void Intercom::processReplacing(float **inputs, 
                                float **outputs, long samples) {
  processX(inputs, outputs, samples, 1);
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
