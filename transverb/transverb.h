#ifndef __TOM7_TRANSVERB_H
#define __TOM7_TRANSVERB_H

/* DFX Transverb plugin by Tom 7 and Marc 3 */

#include <stdio.h>

#include "dfxmisc.h"
#include "iirfilter.h"
#include "vstchunk.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kBsize,
	kSpeed1,
	kFeed1,
	kDist1,
	kSpeed2,
	kFeed2,
	kDist2,
	kDrymix,
	kMix1,
	kMix2,
	kQuality,
	kTomsound,
	kSpeed1mode,
	kSpeed2mode,

	NUM_PARAMS
};


#define NUM_PROGRAMS 16

#define fsign(f) ((f<0.0f)?-1.0f:1.0f)

#define PLUGIN Transverb
#define PLUGINID 'DFtv'
#define PLUGIN_VERSION 1400
#ifdef TRANSVERB_STEREO
  #define PLUGINNAME "DFX Transverb"
  #define NUM_CHANNELS 2
#else
  #define PLUGINNAME "DFX Transverb (mono)"
  #define NUM_CHANNELS 1
#endif

#define BUFFER_MIN 1.0f
#define BUFFER_MAX 3000.0f
#define bufferMsScaled(A)   ( paramRangeScaled((A), BUFFER_MIN, BUFFER_MAX) )
#define bufferScaled(A)   ( ((int)(bufferMsScaled(A)*SAMPLERATE*0.001f) > MAXBUF) ? MAXBUF : (int)(bufferMsScaled(A)*SAMPLERATE*0.001f) )

#define gainScaled(A)   ((A)*(A))

#define SPEED_MIN (-3.0f)
#define SPEED_MAX 6.0f
#define speedScaled(A)   ( paramRangeScaled((A), SPEED_MIN, SPEED_MAX) )
#define speedUnscaled(A)   ( paramRangeUnscaled((A), SPEED_MIN, SPEED_MAX) )
// for backwards compatibility with versions 1.0 & 1.0.1
#define OLD_SPEED_MIN 0.03f
#define OLD_SPEED_MAX 10.0f
#define oldSpeedScaled(A)   ( paramRangeSquaredScaled((A), OLD_SPEED_MIN, OLD_SPEED_MAX) )

#define qualityScaled(A)   ( paramSteppedScaled((A), numQualities) )
#define qualityUnscaled(A)   ( paramSteppedUnscaled((A), numQualities) )

#define SMOOTH_DUR 42

// this is for converting version 1.0 speed parameter valuess to the current format
//#define newSpeed(A)   ((log2f(oldSpeedScaled((A)))-SPEED_MIN) / (SPEED_MAX-SPEED_MIN))
#define newSpeed(A)   (((logf(oldSpeedScaled((A)))/logf(2.0f))-SPEED_MIN) / (SPEED_MAX-SPEED_MIN))

// this stuff is for the speed parameter adjustment mode switch on the GUI
enum { kFineMode, kSemitoneMode, kOctaveMode, numSpeedModes };
#define speedModeScaled(A)   ( paramSteppedScaled((A), numSpeedModes) )

#define numFIRtaps 23

const float RAND_MAX_FLOAT = (float) RAND_MAX;	// reduces wasteful casting


enum { dirtfi, hifi, ultrahifi, numQualities };

enum { useNothing, useHighpass, useLowpassIIR, useLowpassFIR, numFilterModes };


struct param {
  float * ptr;
  const char * name;
  const char * units;
};

class TransverbProgram
{
friend struct PLUGIN;
public:
	TransverbProgram();
	~TransverbProgram();
private:	
	float *parameter;
	char *name;
};

struct PLUGIN : public AudioEffectX {

  PLUGIN(audioMasterCallback audioMaster);
  ~PLUGIN();

  virtual void processX(float **inputs, float **outputs, long sampleFrames,
			int replacing);
  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void processReplacing(float **inputs, float **outputs, 
				long sampleFrames);
  virtual long processEvents(VstEvents* events);

  virtual void setParameter(long index, float value);
  virtual float getParameter(long index);
  virtual void getParameterLabel(long index, char *label);
  virtual void getParameterDisplay(long index, char *text);
  virtual void getParameterName(long index, char *text);

  virtual bool getInputProperties(long index, VstPinProperties* properties);
  virtual bool getOutputProperties(long index, VstPinProperties* properties);
  virtual long canDo(char* text);

  virtual void suspend();
  virtual void resume();

  bool getVendorString(char *text) {
    strcpy (text, "Destroy FX");
    return true;
  }

  long getVendorVersion() {
    return PLUGIN_VERSION;
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
    setNumInputs(NUM_CHANNELS);
    setNumOutputs(NUM_CHANNELS);
  #ifdef TRANSVERB_STEREO
    canMono();
    setUniqueID(PLUGINID);	// 'DFtv'
  #else
    setUniqueID('DtvM');
  #endif

    canProcessReplacing();
  }

  virtual void setProgram(long programNum);
  virtual void setProgramName(char *name);
  virtual void getProgramName(char *name);
  virtual bool getProgramNameIndexed(long category, long index, char *text);
  virtual bool copyProgram(long destination);

  virtual long getChunk(void **data, bool isPreset) {
    return chunk->getChunk(data, isPreset);
  }
  virtual long setChunk(void *data, long byteSize, bool isPreset) {
    return chunk->setChunk(data, byteSize, isPreset);
  }

// the GUI needs to be able to look at these, so they are public
  void randomizeParameters(bool writeAutomation = false);
  float fBsize;	// parameter
  VstChunk *chunk;

protected:
  void initPresets();
  void createAudioBuffers();
  void clearBuffers();
  TransverbProgram *programs;

  param paramptrs[NUM_PARAMS];

  float drymix;
  int bsize;
  float mix1, speed1, feed1, dist1;
  float mix2, speed2, feed2, dist2;
  float drymixParam, mix1param, mix2param, speed1param, speed2param;
  float fQuality, fTomsound;
  float fSpeed1mode, fSpeed2mode;	// these are just for the GUI
  long quality;
  bool tomsound;

  int writer;
  double read1, read2;

  float * buf1[2];
  float * buf2[2];
  int MAXBUF;	// the size of the audio buffer (dependant on sampling rate)

  IIRfilter *filter1, *filter2;
  bool speed1hasChanged, speed2hasChanged;

  int smoothcount1[2], smoothcount2[2], smoothdur1[2], smoothdur2[2];
  float smoothstep1[2], smoothstep2[2], lastr1val[2], lastr2val[2];

  float SAMPLERATE;

  float *firCoefficients1, *firCoefficients2;
};

#define FPARAM(pname, idx, nm, init, un) do { pname = (init); paramptrs[idx].ptr = &pname; paramptrs[idx].name = (nm); paramptrs[idx].units = (un); } while (0)


inline float interpolateHermite (float *data, double address, 
				 int arraysize, int danger) {
  int pos, posMinus1, posPlus1, posPlus2;
  float posFract, a, b, c;

  pos = (long)address;
  posFract = (float) (address - (double)pos);

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

  return ( ((a*posFract)+b) * posFract + c ) * posFract + data[pos];
}
/*
inline float interpolateHermitePostLowpass (float *data, float address) {
  long pos;
  float posFract, a, b, c;

  pos = (long)address;
  posFract = address - (float)pos;

  a = ( (3.0f*(data[1]-data[2])) - 
	 data[0] + data[3] ) * 0.5f;
  b = (2.0f*data[2]) + data[0] - 
         (2.5f*data[1]) - (data[3]*0.5f);
  c = (data[2] - data[0]) * 0.5f;

  return ( ((a*posFract)+b) * posFract + c ) * posFract + data[1];
}
*/

inline float interpolateLinear(float *data, double address, 
				int arraysize, int danger) {
	int posPlus1, pos = (long)address;
	float posFract = (float) (address - (double)pos);

	if (danger == 1) {
	  /* the upcoming sample is not contiguous because 
	     the write head is about to write to it */
	  posPlus1 = pos;
	} else {
	  // it's all right
	  posPlus1 = (pos + 1) % arraysize;
	}
	return (data[pos] * (1.0f-posFract)) + 
	       (data[posPlus1] * posFract);
}

#endif
