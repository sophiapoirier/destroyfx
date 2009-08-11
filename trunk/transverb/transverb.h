#ifndef __TOM7_TRANSVERB_H
#define __TOM7_TRANSVERB_H

/* DFX Transverb plugin by Tom 7 and Sophia */

#include "dfxplugin.h"

#include "iirfilter.h"


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

	kNumParameters
};

const long kNumPresets = 16;

const long kAudioSmoothingDur_samples = 42;
const long kNumFIRTaps = 23;


// this stuff is for the speed parameter adjustment mode switch on the GUI
enum { kSpeedMode_Fine, kSpeedMode_Semitone, kSpeedMode_Octave, kSpeedMode_NumModes };

enum { kQualityMode_DirtFi, kQualityMode_HiFi, kQualityMode_UltraHiFi, kQualityMode_NumModes };

enum { kFilterMode_Nothing, kFilterMode_Highpass, kFilterMode_LowpassIIR, kFilterMode_LowpassFIR };



class TransverbDSP : public DfxPluginCore {

public:
  TransverbDSP(DfxPlugin * inDfxPlugin);
  virtual ~TransverbDSP();

  virtual void process(const float * in, float * out, unsigned long inNumFrames, bool replacing=true);
  virtual void reset();
  virtual void processparameters();
  virtual bool createbuffers();
  virtual void clearbuffers();
  virtual void releasebuffers();

private:
  // these get set to the parameter values
  int bsize;
  double speed1, speed2;
  float drymix;
  float mix1, feed1, dist1;
  float mix2, feed2, dist2;
  long quality;
  bool tomsound;

  int writer;
  double read1, read2;

  float * buf1;
  float * buf2;
  int MAXBUF;	// the size of the audio buffer (dependant on sampling rate)

  DfxIIRfilter filter1, filter2;
  bool speed1hasChanged, speed2hasChanged;

  int smoothcount1, smoothcount2;
  int smoothdur1, smoothdur2;
  float smoothstep1, smoothstep2;
  float lastr1val, lastr2val;

  float * firCoefficients1, * firCoefficients2;

  long tomsound_sampoffset;	// essentially the core instance number
};



class Transverb : public DfxPlugin {

public:
  Transverb(TARGET_API_BASE_INSTANCE_TYPE inInstance);
  virtual ~Transverb();

  virtual bool loadpreset(long index);	// overriden to support the random preset
  virtual void randomizeparameters(bool writeAutomation = false);

  long addcore();	// add a DSP core instance to the counter
  void subtractcore();	// subtract one

private:
  void initPresets();

  long speed1mode, speed2mode;	// these are just for the GUI
  long numtransverbcores;	// counter for the number of DSP core instances
};


inline float Transverb_InterpolateHermite (float * data, double address, 
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
inline float Transverb_InterpolateHermitePostLowpass (float * data, float address) {
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

inline float Transverb_InterpolateLinear(float * data, double address, 
				int arraysize, int danger) {
	int posPlus1, pos = (long)address;
	float posFract = (float) (address - (double)pos);

	if (danger == 1) {
		// the upcoming sample is not contiguous because 
		// the write head is about to write to it
		posPlus1 = pos;
	} else {
		// it's alright
		posPlus1 = (pos + 1) % arraysize;
	}
	return (data[pos] * (1.0f-posFract)) + 
			(data[posPlus1] * posFract);
}

#endif
