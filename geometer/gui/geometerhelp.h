#ifndef __DFX_GEOMETER_HELP_H
#define __DFX_GEOMETER_HELP_H


enum { HELP_EMPTY = 0,
       HELP_WINDOWSHAPE,
       HELP_WINDOWSIZE,
       HELP_LANDMARKS,
       HELP_OPS,
       HELP_RECREATE,
       HELP_MIDILEARN,
       HELP_MIDIRESET,

	   NUM_GEN_HELP_ITEMS
};

enum { HELP_CATEGORY_GENERAL = 0,
       HELP_CATEGORY_WINDOWSHAPE,
       HELP_CATEGORY_LANDMARKS,
       HELP_CATEGORY_OPS,
       HELP_CATEGORY_RECREATE,

       NUM_HELP_CATEGORIES
};

// the number of lines of text in the help box 
// (1 heading line and then up to 6 lines of details)
const long NUM_HELP_TEXT_LINES = 7;


// the general help infos
const char * general_helpstrings[NUM_GEN_HELP_ITEMS][NUM_HELP_TEXT_LINES] = 
{
  {
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "window shape: adjust the fade between windows", 
    "Geometer processes its input in overlapping windows, and ", 
    "then fades between them.  This parameter adjusts the ", 
    "shape of the fade between windows.  Some effects can ", 
    "sound different with different window shapes.", 
    " ", 
    " ", 
  }, 
  {
    "window size: adjust the length of processing window", 
    "Geometer processes its input in overlapping windows, and ", 
    "then fades between them.  This parameter determines the ", 
    "window size (in samples).  When running Geometer on live ", 
    "input, smaller windows reduce latency.  Bigger windows ", 
    "typically result in better quality.  Changes to this parameter ", 
    "will not take effect until the audio is stopped and restarted.", 
  }, 
  {
    "how to generate landmarks", 
    "Geometer works by choosing \"landmarks\" or \"points\" on the ", 
    "input waveform by various methods.  Click the button to the ", 
    "left to choose a method of point generation.  When ", 
    "applicable, the slider under this button adjusts some ", 
    "parameter of landmark generation.", 
    " ", 
  }, 
  {
    "how to mess them up", 
    "After choosing points, Geometer can mess them up by ", 
    "various means.  There are three operation slots, and three ", 
    "parameter sliders corresponding to the slots.  Click a slot to ", 
    "change what operation is performed, or choose empty to ", 
    "perform no operations.", 
    " ", 
  }, 
  {
    "how to recreate the waveform", 
    "Finally Geometer will re-draw a new waveform using the ", 
    "landmarks as a guide.  Chose between the various methods ", 
    "here, and use the slider above the button to control the ", 
    "parameters of the effects.", 
    " ", 
    " ", 
  }, 
  {
    "midi learn: assign midi controller codes to a parameter", 
    "Click the MIDI learn button, and then move one of the ", 
    "sliders to make it glow.  The next MIDI controller you change ", 
    "will be assigned to this parameter, allowing you to control it ", 
    "with your keyboard or MIDI device.  This won't work in all ", 
    "hosts.  Check the included documentation for advanced ", 
    "functionality and troubleshooting.", 
  }, 
  {
    "midi reset: clear a parameter's midi assignment", 
    "If you have assigned a MIDI event to a parameter (see the ", 
    "\"MIDI learn\" help for more info), you can use this button ", 
    "to erase the assignment.  First you need to activate the ", 
    "parameter as a MIDI learner (again, see the \"MIDI learn\" ", 
    "help), then press this button to erase that parameter's ", 
    "MIDI assignment.", 
  }, 
};


// the help infos for window shape
const char * windowshape_helpstrings[NUM_WINDOWSHAPES][NUM_HELP_TEXT_LINES] = 
{
  {
    "triangle: linear fade between windows", 
    "Fades between windows linearly.", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "arrow: sharp fade in, dull fade out", 
    "Favors the tail end of windows.", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "wedge: dull fade in, fast fade out", 
    "Favors the beginning of windows.", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "cos: smooth fades in, out", 
    "A smooth fade-in and fade-out, emphasizing the center of ", 
    "each window.", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
};


// the help infos for "how to generate landmarks"
const char * landmarks_helpstrings[NUM_POINTSTYLES][NUM_HELP_TEXT_LINES] = 
{
  {
    "ext'n cross: place points at extremities and crossings", 
    "Place points at zero-crossings, as well as the peaks and ", 
    "valleys between them.  The slider controls teh tolerance for ", 
    "what is considered 'zero'.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "at freq: places points at specified frequency", 
    "Places points at the frequency specified by the slider.", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "random: places a specified number of points randomly", 
    "Places points randomly on hte waveform.  The number of ", 
    "points is controlled by the slider.", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "span: the sample value determines the next location", 
    "The larger the magnitude of the current sample, the ", 
    "farther away the next sample will be.  The slider controls ", 
    "the spanning factor.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "dy/dx: make point when derivative changes by amount", 
    "Makes a point whenever the slope changes by some ", 
    "threshold.  With the slider > 0.5, this catches sharp rises; ", 
    "at less than 0.5, it makes points on sharp falls.  When the ", 
    "slider is at 0.5, dy/dx makes a point at all peaks and valleys.", 
    " ", 
    " ", 
  }, 
  {
    "level: make point when wave crosses a specific level", 
    "Makes a point when the waveform crosses a specific level ", 
    "(both above and below the origin).  The slider controls the ", 
    "level at which point generation occurs.", 
    " ", 
    " ", 
    " ", 
  }, 
};


// the help infos for the point operations
const char * ops_helpstrings[NUM_OPS][NUM_HELP_TEXT_LINES] = 
{
  {
    "x2: unscientifically double number of points", 
    "Double the number of points.  New points are placed half-", 
    "way between old points, with a magnitude determined by ", 
    "the slider and the magnitudes of its neighbors.  Note that ", 
    "there cannot be more points than samples, so this operation ", 
    "may do nothing.", 
    " ", 
  }, 
  {
    "1/2: thin points by one half", 
    "Erases every other point.", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "1/4: thin points by one quarter", 
    "Erases three out of every four points.", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "longpass: keep only long intervals", 
    "If a point is not at least a certain distance from the ", 
    "previous point, it is removed.  The distance is determined by ", 
    "the slider.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "shortpass: keep only short intervals", 
    "If a point is more than a certain distance from the previous ", 
    "point, its height is set to zero.  The distance is determined ", 
    "by the slider.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "slow: lengthen time scale of points", 
    "Stretches out the points along the x-axis.  Points towards ", 
    "the end of the frame are discarded.  This tends to lower the ", 
    "pitch of the sound without changing its speed.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "fast: shorten time scale of points", 
    "Compresses the points along the x-axis.  Points are repeated ", 
    "to fill the frame.  This operation tends to raise the pitch of ", 
    "the sound without changing its speed.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "empty", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
    " ", 
  }, 
};


// the help infos for "how to recreate the waveform"
const char * recreate_helpstrings[NUM_INTERPSTYLES][NUM_HELP_TEXT_LINES] = 
{
  {
    "polygon: recreate wave with linear interpolation", 
    "Recreate the wave by drawing lines directly between the ", 
    "points.  Like a lowpass filter, but introduces new high ", 
    "frequencies as sharp corners at points.  The slider controls ", 
    "how steep the lines are (if greater than zero, they won't ", 
    "match up!).", 
    " ", 
  }, 
  {
    "wrongygon: linear interpolation in the wrong direction", 
    "Like polygon, but the lines are flipped vertically about their ", 
    "centers.  Very rough sound.  Like polygon, the slider controls ", 
    "how steep the lines are.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "smoothie: smoothly interpolate between points", 
    "Recreates the wave using segments of a smooth cosine ", 
    "curve.  Much less harsh than polygon or wrongygon.  The ", 
    "slider controls the roundness of the curves.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "revisi: reverse segments of the input wave", 
    "Reverses the input wave between points.  Can create ", 
    "bizarre phasing artifacts.  Experiment with low point ", 
    "frequencies and different window sizes for this one.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "pulse: create pulses of specified size at points", 
    "Creates a pulse for each point.  The width of the pulse is ", 
    "specified by the slider.  Wide pulses that overlap do strange ", 
    "stuff; take a look.", 
    " ", 
    " ", 
    " ", 
  }, 
  {
    "friends: neighboring sections make friends", 
    "Stretches the original waveform from a section so that it ", 
    "overlaps the next section.  The slider sets the amount of ", 
    "overlap.  \"Friends\" tends to lower the pitch of the input ", 
    "without altering its speed.", 
    " ", 
    " ", 
  }, 
  {
    "sing: replace the intervals with sine wave", 
    "Replaces each segment with one period of a sine wave.  ", 
    "It's best used with something where landmark generation is ", 
    "related to the sound, like dy/dx (especially on percussive ", 
    "input).  Combine with longpass for a less harsh sound.  The ", 
    "slider makes sing use the sine wave for amplitude modulation ", 
    "instead, resulting in a totally different sound.", 
  }, 
  {
    "shuffle: mix around intervals", 
    "Mixes around the intervals.  Best used with a large window ", 
    "size and long intervals.  When this option is selected, the ", 
    "slider chooses how far the intervals are shuffled.", 
    " ", 
    " ", 
    " ", 
  }, 
};


#endif
