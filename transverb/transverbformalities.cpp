
/* DFX Transverb plugin by Tom 7 and Marc 3 */

#include "transverb.hpp"

#include "transverbeditor.hpp"

#include <string.h>
#include <time.h>


PLUGIN::PLUGIN(audioMasterCallback audioMaster)
  : AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMS) {

  FPARAM(fBsize, kBsize, "buffer size", (2700.0f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN), "ms");
  FPARAM(drymixParam, kDrymix, "dry mix", 1.0f, "dB");
  FPARAM(mix1param, kMix1, "1:mix", 1.0f, "dB");
  FPARAM(dist1, kDist1, "1:dist", 0.90009f, "units");
  FPARAM(speed1param, kSpeed1, "1:speed", (0.0f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN), "units");
  FPARAM(feed1, kFeed1, "1:feedback", 0.0f, "units");
  FPARAM(mix2param, kMix2, "2:mix", 0.0f, "dB");
  FPARAM(dist2, kDist2, "2:dist", 0.1f, "units");
  FPARAM(speed2param, kSpeed2, "2:speed", (1.0f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN), "units");
  FPARAM(feed2, kFeed2, "2:feedback", 0.0f, "units");
  FPARAM(fQuality, kQuality, "quality", 1.0f, " ");
  FPARAM(fTomsound, kTomsound, "TOMSOUND", 0.0f, " ");
  FPARAM(fSpeed1mode, kSpeed1mode, "1:speed mode", 0.0f, " ");
  FPARAM(fSpeed2mode, kSpeed2mode, "2:speed mode", 0.0f, " ");

  // default this to something, at least for the sake of getGetTailSize()
  MAXBUF = (int) (BUFFER_MAX * 44.1f);
  buf1[0] = NULL;
  buf2[0] = NULL;
#ifdef TRANSVERB_STEREO
  buf1[1] = NULL;
  buf2[1] = NULL;
#endif
  filter1 = new IIRfilter[NUM_CHANNELS];
  filter2 = new IIRfilter[NUM_CHANNELS];
  firCoefficients1 = new float[numFIRtaps];
  firCoefficients2 = new float[numFIRtaps];
  setup();

  chunk = new VstChunk(NUM_PARAMS, NUM_PROGRAMS, PLUGINID, this);
  /* since we don't use notes for any specialized control of Geometer, 
     allow them to be assigned to control parameters via MIDI learn */
  chunk->setAllowPitchbendEvents(true);
  chunk->setAllowNoteEvents(true);

  programs = new TransverbProgram[NUM_PROGRAMS];
  strcpy(programs[0].name, PLUGINNAME);	// default program name
  initPresets();

  suspend();
  setProgram(0);

  srand((unsigned int)time(NULL));	// sets a seed value for rand() from the system clock

  editor = new TransverbEditor(this);
}

PLUGIN::~PLUGIN() {

  if (buf1[0])
    free(buf1[0]);
  if (buf2[0])
    free(buf2[0]);
#ifdef TRANSVERB_STEREO
  if (buf1[1])
    free(buf1[1]);
  if (buf2[1])
    free(buf2[1]);
#endif
  if (filter1)
    delete[] filter1;
  if (filter2)
    delete[] filter2;
  if (firCoefficients1)
    delete[] firCoefficients1;
  if (firCoefficients2)
    delete[] firCoefficients2;

  if (programs)
    delete[] programs;
  if (chunk)
    delete chunk;
}

void PLUGIN::suspend () {

  clearBuffers();
  writer = 0;
  read1 = read2 = 0.0;
  smoothcount1[0] = smoothcount2[0] = 0;
  lastr1val[0] = lastr2val[0] = 0.0f;
  filter1[0].reset();
  filter2[0].reset();
#ifdef TRANSVERB_STEREO
  smoothcount1[1] = smoothcount2[1] = 0;
  lastr1val[1] = lastr2val[1] = 0.0f;
  filter1[1].reset();
  filter2[1].reset();
#endif
  SAMPLERATE = getSampleRate();
  if (SAMPLERATE <= 0.0f)   SAMPLERATE = 44100.0f;
  bsize = bufferScaled(fBsize);
  speed1hasChanged = speed2hasChanged = true;
}

void PLUGIN::resume() {

  createAudioBuffers();

  bsize = bufferScaled(fBsize);
  wantEvents();	// for program changes && CC parameter automation
}

void PLUGIN::createAudioBuffers() {

  SAMPLERATE = getSampleRate();
  if (SAMPLERATE <= 0.0f)   SAMPLERATE = 44100.0f;

  long oldmax = MAXBUF;
  MAXBUF = (int) (BUFFER_MAX * 0.001f * SAMPLERATE);

  // if the sampling rate (& therefore the max buffer size) has changed, 
  // then delete & reallocate the buffers according to the sampling rate
  if (MAXBUF != oldmax)
  {
    if (buf1[0] != NULL)
      free(buf1[0]);
    buf1[0] = NULL;
    if (buf2[0] != NULL)
      free(buf2[0]);
    buf2[0] = NULL;
#ifdef TRANSVERB_STEREO
    if (buf1[1] != NULL)
      free(buf1[1]);
    buf1[1] = NULL;
    if (buf2[1] != NULL)
      free(buf2[1]);
    buf2[1] = NULL;
#endif
  }
  if (buf1[0] == NULL)
    buf1[0] = (float*)malloc(MAXBUF * sizeof (float));
  if (buf2[0] == NULL)
    buf2[0] = (float*)malloc(MAXBUF * sizeof (float));
#ifdef TRANSVERB_STEREO
  if (buf1[1] == NULL)
    buf1[1] = (float*)malloc(MAXBUF * sizeof (float));
  if (buf2[1] == NULL)
    buf2[1] = (float*)malloc(MAXBUF * sizeof (float));
#endif
}

void PLUGIN::clearBuffers() {
  if ( (buf1[0] != NULL) && (buf2[0] != NULL) ) {
    for (int j=0; j < MAXBUF; j++) buf1[0][j] = buf2[0][j] = 0.0f;
  }
#ifdef TRANSVERB_STEREO
  if ( (buf1[1] != NULL) && (buf2[1] != NULL) ) {
    for (int k=0; k < MAXBUF; k++) buf1[1][k] = buf2[1][k] = 0.0f;
  }
#endif
}

//--------- programs --------
TransverbProgram::TransverbProgram() {
	name = new char[32];
	parameter = new float[NUM_PARAMS];

	parameter[kBsize] =(2700.0f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN);
	parameter[kDrymix] = 1.0f;
	parameter[kMix1] = 1.0f;
	parameter[kDist1] = 0.90009f;
	parameter[kSpeed1] = (0.0f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN);
	parameter[kFeed1] = 0.0f;
	parameter[kMix2] = 0.0f;
	parameter[kDist2] = 0.1f;
	parameter[kSpeed2] = (1.0f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN);
	parameter[kFeed2] = 0.0f;
	parameter[kQuality] = 1.0f;
	parameter[kTomsound] = 0.0f;
	parameter[kSpeed1mode] = 0.0f;
	parameter[kSpeed2mode] = 0.0f;

	strcpy(name, "default");
}

TransverbProgram::~TransverbProgram() {
	if (name)
		delete[] name;
	if (parameter)
		delete[] parameter;
}

void PLUGIN::setProgram(long programNum) {
  if ( (programNum < NUM_PROGRAMS) && (programNum >= 0) )
  {
    AudioEffectX::setProgram(programNum);
    if ( strcmp(programs[programNum].name, "random") == 0 )
      randomizeParameters();
    else
    {
      for (int i=0; i < NUM_PARAMS; i++)
        setParameter(i, programs[programNum].parameter[i]);
    }
  }
  // tell the host to update the editor display with the new settings
  AudioEffectX::updateDisplay();
}

void PLUGIN::setProgramName(char *name) {
  strcpy(programs[curProgram].name, name);
}

void PLUGIN::getProgramName(char *name) {
  if ( !strcmp(programs[curProgram].name, "default") )
      sprintf(name, "default %ld", curProgram+1);
  else
      strcpy(name, programs[curProgram].name);
}

bool PLUGIN::getProgramNameIndexed(long category, long index, char *text) {
  if ( (index < NUM_PROGRAMS) && (index >= 0) )
  {
    strcpy(text, programs[index].name);
    return true;
  }
  return false;
}

bool PLUGIN::copyProgram(long destination) {
  if ( (destination < NUM_PROGRAMS) && (destination >= 0) )
  {
    for (long i = 0; i < NUM_PARAMS; i++)
      programs[destination].parameter[i] = programs[curProgram].parameter[i];
    strcpy(programs[destination].name, programs[curProgram].name);
    return true;
  }
  return false;
}

void PLUGIN::initPresets() {
  int i = 1;

	programs[i].parameter[kBsize] =(48.687074827f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN);
	programs[i].parameter[kDrymix] = 0.45f;
	programs[i].parameter[kMix1] = 0.5f;
	programs[i].parameter[kDist1] = 0.9f;
	programs[i].parameter[kSpeed1] = newSpeed(sqrtf((1.0028f-OLD_SPEED_MIN)/(OLD_SPEED_MAX-OLD_SPEED_MIN)));
	programs[i].parameter[kFeed1] = 0.67f;
	programs[i].parameter[kMix2] = 0.0f;
	programs[i].parameter[kDist2] = 0.0f;
	programs[i].parameter[kSpeed2] = 0.0f;
	programs[i].parameter[kFeed2] = 0.0f;
	programs[i].parameter[kQuality] = 1.0f;
	programs[i].parameter[kTomsound] = 0.0f;
	strcpy(programs[i].name, "phaser up");
	i++;

	programs[i].parameter[kBsize] =(27.0f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN);
	programs[i].parameter[kDrymix] = 0.45f;
	programs[i].parameter[kMix1] = 0.5f;
	programs[i].parameter[kDist1] = 0.0f;
	programs[i].parameter[kSpeed1] = newSpeed(sqrtf((0.9972f-OLD_SPEED_MIN)/(OLD_SPEED_MAX-OLD_SPEED_MIN)));
	programs[i].parameter[kFeed1] = 0.76f;
	programs[i].parameter[kMix2] = 0.0f;
	programs[i].parameter[kDist2] = 0.0f;
	programs[i].parameter[kSpeed2] = 0.0f;
	programs[i].parameter[kFeed2] = 0.0f;
	programs[i].parameter[kQuality] = 1.0f;
	programs[i].parameter[kTomsound] = 0.0f;
	strcpy(programs[i].name, "phaser down");
	i++;

	programs[i].parameter[kBsize] =(2605.1f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN);
	programs[i].parameter[kDrymix] = 61.0f/77.0f;
	programs[i].parameter[kMix1] = 1.0f;
	programs[i].parameter[kDist1] = 0.993f;
	programs[i].parameter[kSpeed1] = newSpeed(sqrtf((0.0394f-OLD_SPEED_MIN)/(OLD_SPEED_MAX-OLD_SPEED_MIN)));
	programs[i].parameter[kFeed1] = 0.54f;
	programs[i].parameter[kMix2] = 67.0f/77.0f;
	programs[i].parameter[kDist2] = 0.443f;
	programs[i].parameter[kSpeed2] = newSpeed(sqrtf((5.4435f-OLD_SPEED_MIN)/(OLD_SPEED_MAX-OLD_SPEED_MIN)));
	programs[i].parameter[kFeed2] = 0.46f;
	programs[i].parameter[kQuality] = 1.0f;
	programs[i].parameter[kTomsound] = 0.0f;
	strcpy(programs[i].name, "aquinas");
	i++;

	programs[i].parameter[kBsize] =(184.27356f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN);
	programs[i].parameter[kDrymix] = sqrtf(0.157f);
	programs[i].parameter[kMix1] = 1.0f;
	programs[i].parameter[kDist1] = 0.0945f;
	programs[i].parameter[kSpeed1] = newSpeed(sqrtf((0.63f-OLD_SPEED_MIN)/(OLD_SPEED_MAX-OLD_SPEED_MIN)));
	programs[i].parameter[kFeed1] = 0.976f;
	programs[i].parameter[kMix2] = 0.0f;
	programs[i].parameter[kDist2] = 0.197f;
	programs[i].parameter[kSpeed2] = newSpeed(sqrtf((1.97f-OLD_SPEED_MIN)/(OLD_SPEED_MAX-OLD_SPEED_MIN)));
	programs[i].parameter[kFeed2] = 0.0f;
	programs[i].parameter[kQuality] = 1.0f;
	programs[i].parameter[kTomsound] = 0.0f;
	strcpy(programs[i].name, "glup drums");
	i++;

	programs[i].parameter[kSpeed1] = ((-0.23f/12.0f)-SPEED_MIN)/(SPEED_MAX-SPEED_MIN);
	programs[i].parameter[kFeed1] = 0.73f;
	programs[i].parameter[kDist1] = 0.1857f;
	programs[i].parameter[kSpeed2] = ((4.3f/12.0f)-SPEED_MIN)/(SPEED_MAX-SPEED_MIN);
	programs[i].parameter[kFeed2] = 0.41f;
	programs[i].parameter[kDist2] = 0.5994f;
	programs[i].parameter[kBsize] =(16.8f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN);
	programs[i].parameter[kDrymix] = 0.024f;
	programs[i].parameter[kMix1] = 1.0f;
	programs[i].parameter[kMix2] = 0.35f;
	programs[i].parameter[kQuality] = 1.0f;
	programs[i].parameter[kTomsound] = 0.0f;
	strcpy(programs[i].name, "space invaders");
	i++;

/*
	programs[i].parameter[kSpeed1] = (f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN);
	programs[i].parameter[kFeed1] = 0.f;
	programs[i].parameter[kDist1] = 0.f;
	programs[i].parameter[kSpeed2] = (f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN);
	programs[i].parameter[kFeed2] = 0.f;
	programs[i].parameter[kDist2] = 0.f;
	programs[i].parameter[kBsize] =(f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN);
	programs[i].parameter[kDrymix] = 0.f;
	programs[i].parameter[kMix1] = 0.f;
	programs[i].parameter[kMix2] = 0.f;
	programs[i].parameter[kQuality] = 1.0f;
	programs[i].parameter[kTomsound] = 0.0f;
	strcpy(programs[i].name, "");
	i++;
*/

	strcpy(programs[NUM_PROGRAMS-1].name, "random");	// randomize settings
};

//--------- programs (end) --------


void PLUGIN::setParameter(long index, float value) {
  switch (index) {
  case kBsize:
    fBsize = value;
    bsize = bufferScaled(fBsize);
    writer %= bsize;
    read1 = fmod(fabs(read1), (double)bsize);
    read2 = fmod(fabs(read2), (double)bsize);
    break;
  case kDrymix:
    drymixParam = value;
    drymix = gainScaled(drymixParam);
    break;
  case kMix1:
    mix1param = value;
    mix1 = gainScaled(mix1param);
    break;
  case kDist1:
    dist1 = value;
    read1 = fmod(fabs((double)writer + (double)dist1 * (double)MAXBUF), (double)bsize);
    break;
  case kSpeed1:
    speed1hasChanged = true;
    speed1 = powf(2.0f, speedScaled(value));
    speed1param = value;
    speed1hasChanged = true;
    break;
  case kMix2:
    mix2param = value;
    mix2 = gainScaled(mix2param);
    break;
  case kDist2:
    dist2 = value;
    read2 = fmodf(fabsf(writer + dist2 * MAXBUF), (float)bsize);
    break;
  case kSpeed2:
    speed2hasChanged = true;
    speed2 = powf(2.0f, speedScaled(value));
    speed2param = value;
    speed2hasChanged = true;
    break;
  case kQuality:
    fQuality = value;
    quality = qualityScaled(fQuality);
    break;
  case kTomsound:
    fTomsound = value;
    tomsound = (fTomsound > 0.5f);
    break;
  default:
    if (index >= 0 && index < NUM_PARAMS)
      *paramptrs[index].ptr = value;
    /* otherwise, ??? */
    break;
  }

  if ( (index >= 0) && (index < NUM_PARAMS) )
    programs[curProgram].parameter[index] = value;

  if (editor)
    ((AEffGUIEditor*)editor)->setParameter(index, value);
}

float PLUGIN::getParameter(long index) {
  switch (index) {
    /* special cases here */
  default:
    /* otherwise pull it out of array. */
    if (index >= 0 && index < NUM_PARAMS) return *paramptrs[index].ptr;
    else return 0.0f; /* ? */
  }
}

void PLUGIN::getParameterName(long index, char *label) {
  switch(index) {
    /* special cases here */
  default:
    if (index >= 0 && index < NUM_PARAMS && paramptrs[index].name) 
      strcpy(label, paramptrs[index].name);
    else strcpy(label, "?");
    break;
  }
}

void PLUGIN::getParameterDisplay(long index, char *text) {
  switch(index) {
    /* special cases here */
  case kBsize:
    sprintf(text, "%.3f", bufferMsScaled(fBsize));
    break;

  case kDrymix:
  case kMix1:
  case kMix2:
    sprintf(text, "%.2f", dBconvert(gainScaled(getParameter(index))));
    break;

  case kSpeed1:
  case kSpeed2:
    sprintf(text, "%.3f", speedScaled(getParameter(index)));
    break;

  case kQuality:
    switch(quality) {
    case dirtfi:    strcpy(text, "dirt-fi");        break;
    case hifi:      strcpy(text, "hi-fi");          break;
    case ultrahifi: strcpy(text, "ultra hi-fi");    break;
    default:        strcpy(text, "?");              break;
    }
    break;

  case kTomsound:
    if (tomsound) strcpy(text, "in da house");
    else strcpy(text, "off");
    break;

  case kSpeed1mode:
  case kSpeed2mode:
    sprintf(text, "for the GUI");
    break;

  default:
    float2string(getParameter(index), text);
    break;
  }
}

void PLUGIN::getParameterLabel(long index, char *label) {
  switch(index) {
    /* special cases here */
  default:
    if (index >= 0 && index < NUM_PARAMS && paramptrs[index].units) 
      strcpy(label, paramptrs[index].units);
    else strcpy(label, "?");
    break;
  }
}

// we use this only for program changes in Transverb
long PLUGIN::processEvents(VstEvents* events) {
  processProgramChangeEvents(events, this);
  chunk->processParameterEvents(events);
  // says that we want more events; 0 means stop sending them
  return 1;
}

bool PLUGIN::getInputProperties(long index, VstPinProperties* properties) {
	if ( (index >= 0) && (index < NUM_CHANNELS) )
	{
		sprintf(properties->label, "Transverb input %ld", index+1);
		sprintf(properties->shortLabel, "in %ld", index+1);
	#ifdef TRANSVERB_STEREO
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
	#else
		properties->flags = kVstPinIsActive;
	#endif
		return true;
	}
	return false;
}

bool PLUGIN::getOutputProperties(long index, VstPinProperties* properties) {
	if ( (index >= 0) && (index < NUM_CHANNELS) )
	{
		sprintf (properties->label, "Transverb output %ld", index+1);
		sprintf (properties->shortLabel, "out %ld", index+1);
	#ifdef TRANSVERB_STEREO
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
	#else
		properties->flags = kVstPinIsActive;
	#endif
		return true;
	}
	return false;
}

// this tells the host the plugin can do, which is nothing special in this case
long PLUGIN::canDo(char* text) {
	if (strcmp(text, "plugAsChannelInsert") == 0)
		return 1;
	if (strcmp(text, "plugAsSend") == 0)
		return 1;
	if (strcmp(text, "mixDryWet") == 0)
		return 1;
	if (strcmp(text, "1in1out") == 0)
		return 1;
#ifdef TRANSVERB_STEREO
	if (strcmp(text, "1in2out") == 0)
		return 1;
	if (strcmp(text, "2in2out") == 0)
		return 1;
#endif
	if (strcmp(text, "TOMSOUND") == 0)
		return 1;

	return -1;	// explicitly can't do; 0 => don't know
}

/* this randomizes the values of all of Transverb's parameters, sometimes in smart ways */
void PLUGIN::randomizeParameters(bool writeAutomation)
{
  float mixSum, newDrymix, newMix1, newMix2, mixScalar, tempRand;


	// randomize the first 7 parameters
	for (long i=0; (i < kDrymix); i++)
	{
		// make slow speeds more probable (for fairer distribution)
		if ( (i == kSpeed1) || (i == kSpeed1) )
		{
			tempRand = (float)rand() / RAND_MAX_FLOAT;
			if ( tempRand < 0.5f )
				tempRand *= fabsf(SPEED_MIN/(SPEED_MAX-SPEED_MIN)) * 2.0f;
			else
				tempRand = fabsf(SPEED_MIN/(SPEED_MAX-SPEED_MIN)) + 
							( tempRand * fabsf(SPEED_MAX/(SPEED_MAX-SPEED_MIN)) );
			if (writeAutomation)
				setParameterAutomated(i, (float)rand()/RAND_MAX_FLOAT);
			else
				setParameter(i, (float)rand()/RAND_MAX_FLOAT);
		}
		// make smaller buffer sizes more probable (because they sound better)
		else if (i == kBsize)
		{
			if (writeAutomation)
				setParameterAutomated( i, powf(((float)rand()/RAND_MAX_FLOAT*0.93f)+0.07f, 1.38f) );
			else
				setParameter( i, powf(((float)rand()/RAND_MAX_FLOAT*0.93f)+0.07f, 1.38f) );
		}
		else
		{
			if (writeAutomation)
				setParameterAutomated(i, (float)rand()/RAND_MAX_FLOAT);
			else
				setParameter(i, (float)rand()/RAND_MAX_FLOAT);
		}
	}

	// store the current total gain sum
	mixSum = gainScaled(drymixParam) + 
				gainScaled(mix1param) + 
				gainScaled(mix2param);

	// randomize the mix parameters
	newDrymix = (float)rand() / RAND_MAX_FLOAT;
	newMix1 = (float)rand() / RAND_MAX_FLOAT;
	newMix2 = (float)rand() / RAND_MAX_FLOAT;
	// calculate a scalar to make up for total gain changes
	mixScalar = mixSum / (gainScaled(newDrymix)+gainScaled(newMix1)+gainScaled(newMix2));

	// apply the scalar to the new mix parameter values
	newDrymix = sqrtf( mixScalar * gainScaled(newDrymix) );
	newMix1 = sqrtf( mixScalar * gainScaled(newMix1) );
	newMix2 = sqrtf( mixScalar * gainScaled(newMix2) );

	// clip the the mix values at 1.0 so that we don't get mega-feedback blasts
	newDrymix = ( newDrymix > 1.0f ? 1.0f : newDrymix );
	newMix1 = ( newMix1 > 1.0f ? 1.0f : newMix1 );
	newMix2 = ( newMix2 > 1.0f ? 1.0f : newMix2 );

	if (writeAutomation)
	{
		// set the new randomized mix parameter values as the new values
		setParameterAutomated(kDrymix, newDrymix);
		setParameterAutomated(kMix1, newMix1);
		setParameterAutomated(kMix2, newMix2);

		// make higher qualities more probable (happen 4/5 of the time)
		setParameterAutomated( kQuality, (float)(((rand()%5)+1)%3) * 0.5f );
		// make TOMSOUND less probable (only 1/3 of the time)
		setParameterAutomated( kTomsound, (float)((rand()%3)%2) );
	}
	else
	{
		setParameter(kDrymix, newDrymix);
		setParameter(kMix1, newMix1);
		setParameter(kMix2, newMix2);
		setParameter( kQuality, (float)(((rand()%5)+1)%3) * 0.5f );
		setParameter( kTomsound, (float)((rand()%3)%2) );
	}

/*	// write the mix parameter changes to a log file
	mixSum = gainScaled(drymixParam) + gainScaled(mix1param) + gainScaled(mix2param);
	FILE *mixData = fopen("mix sums", "a");
	if (mixData != NULL)
	{
		fprintf(mixData, "drymix = %.3f,   mix1 = %.3f,   mix2 = %.3f,   sum = %.6f\n", newDrymix, newMix1, newMix2, mixSum);
		fclose(mixData);
	}
*/
}


void PLUGIN::processReplacing(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 1);
}

void PLUGIN::process(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 0);
}



/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */ 
/* ---------- boring stuff below this line ----------- */
/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */ 

#if BEOS
#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin (audioMasterCallback audioMaster);

#else
AEffect *main (audioMasterCallback audioMaster);
#endif

AEffect *main (audioMasterCallback audioMaster) {
  // get vst version
  if ( !audioMaster(0, audioMasterVersion, 0, 0, 0, 0) )
    return 0;  // old version

  AudioEffect* effect = new PLUGIN(audioMaster);
  if (!effect)
    return 0;
  return effect->getAeffect();
}

#if WIN32
void* hInstance;
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
  hInstance = hInst;
  return 1;
}
#endif
