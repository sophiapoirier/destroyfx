#ifndef __TOM7_TRANSVERB_H
#define __TOM7_TRANSVERB_H

/* DFX Transverb plugin by Tom 7 and Marc 3 */

#include <audioeffectx.h>

#ifdef WIN32
/* turn off warnings about default but no cases in switch, etc. */
   #pragma warning( disable : 4065 57 4200 4244 )
   #include <windows.h>
#endif

#define fsign(f) ((f<0.0)?-1.0:1.0)
#define pi (3.1415926535)

#define PLUGIN Transverb
#define PLUGINID 'DFtv'
#define PLUGINNAME "DFX TRANSVERB"

#define NUM_PARAMS 12

#define MAXBUF 16384

#define SMOOTH_DUR 42

enum { dirtfi, lofi, hifi } fidelity;

#define qualityScaled(A) ( (long)((A)*2.7f) )


struct param {
  float * ptr;
  const char * name;
  const char * units;
};

struct PLUGIN : public AudioEffectX {

  PLUGIN(audioMasterCallback audioMaster);
  ~PLUGIN();

  virtual void processX(float **inputs, float **outputs, long sampleFrames,
			int replacing);
  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void processReplacing(float **inputs, float **outputs, 
				long sampleFrames);
  virtual void setParameter(long index, float value);
  virtual float getParameter(long index);
  virtual void getParameterLabel(long index, char *label);
  virtual void getParameterDisplay(long index, char *text);
  virtual void getParameterName(long index, char *text);

  virtual void suspend();

  bool getVendorString(char *text) {
    strcpy (text, "Destroy FX");
    return true; 
  }

  bool getProductString(char *text) {
    strcpy (text, "Super Destroy FX bipolar VST plugin pack");
    return true; 
  }

  bool getEffectName(char *name) {
    strcpy (name, PLUGINNAME);
    return true; 
  }

  void setup() {
    setNumInputs(2);		/* stereo */
    setNumOutputs(2);
    canMono();
    setUniqueID(PLUGINID);

    canProcessReplacing();
    strcpy(programName, PLUGINNAME);
  }

  void setProgramName(char *name) {
    strcpy(programName, name);
  }

  void getProgramName(char *name) {
    strcpy(name, programName);
  }

protected:
  char programName[32];

  param paramptrs[NUM_PARAMS];

  float bsize, drymix;
  float mix1, dist1, speed1, feed1;
  float mix2, dist2, speed2, feed2;
  float fQuality, fTomsound;
  long quality;
  bool tomsound;

  int writer;
  float read1, read2;

  float * buf[2];

  int smoothcount1[2], smoothcount2[2], smoothdur1[2], smoothdur2[2];
  float smoothstep1[2], smoothstep2[2], lastr1val[2], lastr2val[2];
};

#define FPARAM(pname, idx, nm, init, un) do { pname = (init); paramptrs[idx].ptr = &pname; paramptrs[idx].name = (nm); paramptrs[idx].units = (un); } while (0)


inline float interpolateHermite (float *data, float address, 
				 int arraysize, int danger) {
  int pos, posMinus1, posPlus1, posPlus2;
  float posFract, a, b, c;

  pos = (long)address;
  posFract = address - (float)pos;

  // because the readers & writer are not necessarilly aligned, 
  // upcoming or previous samples could be discontiguous, in which case 
  // just "interpolate" with repeated samples
  switch (danger) {
    case 0:		// the previous sample is bogus
      posMinus1 = pos;
      posPlus1 = (pos+1) % arraysize;
      posPlus2 = (pos+2) % arraysize;
      break;
    case 1:		// the next 2 samples are bogus
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = posPlus2 = pos;
      break;
    case 2:		// the sample 2 steps ahead is bogus
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = posPlus2 = (pos+1) % arraysize;
      break;
    default:	// everything's cool
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = (pos+1) % arraysize;
      posPlus2 = (pos+2) % arraysize;
      break;
    }

  a = ( (3.0f*(data[pos]-data[posPlus1])) - 
	 data[posMinus1] + data[posPlus2] ) * 0.5f;
  b = (2.0f*data[posPlus1]) + data[posMinus1] - 
         (2.5f*data[pos]) - (data[posPlus2]*0.5f);
  c = (data[posPlus1] - data[posMinus1]) * 0.5f;

  return ( ((a*posFract)+b) * posFract + c ) * posFract+data[pos];
}

inline float interpolateLinear (float *data, float address, 
				int arraysize, int danger) {
	int posPlus1, pos = (long)address;
	float posFract = address - (float)pos;

	if (danger == 1) {
	  /* the upcoming sample is not contiguous because 
	     the write head is about to write to it */
	  posPlus1 = pos;
	} else {
	  // it's all right
	  posPlus1 = pos + 1;
	}
	return (data[pos] * (1.0f-posFract)) + 
	       (data[(posPlus1)%arraysize] * posFract);
}


#endif
