/*------------------------------------------------------------------------
Copyright (C) 2003-2022  Tom Murphy 7 and Sophia Poirier

This file is part of Geometer.

Geometer is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Geometer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Geometer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/


#pragma once

#include <array>


enum : size_t { HELP_EMPTY = 0,
                HELP_WINDOWSHAPE,
                HELP_WINDOWSIZE,
                HELP_LANDMARKS,
                HELP_OPS,
                HELP_RECREATE,
                HELP_MIDILEARN,
                HELP_MIDIRESET,

                NUM_GEN_HELP_ITEMS
};

enum : size_t { HELP_CATEGORY_GENERAL = 0,
                HELP_CATEGORY_WINDOWSHAPE,
                HELP_CATEGORY_LANDMARKS,
                HELP_CATEGORY_OPS,
                HELP_CATEGORY_RECREATE,

                NUM_HELP_CATEGORIES
};


// the general help infos
constexpr std::array<char const * const, NUM_GEN_HELP_ITEMS> general_helpstrings
{{
  "",
  R"(window shape: adjust the fade between windows
Geometer processes its input in overlapping windows, and
then fades between them.  This parameter adjusts the
shape of the fade between windows.  Some effects can
sound different with different window shapes.)",
  R"DELIM(window size: adjust the length of processing window
Geometer processes its input in overlapping windows, and
then fades between them.  This parameter determines the
window size (in samples).  When running Geometer on live
input, smaller windows reduce latency.  Bigger windows
typically result in better quality.)DELIM",
  R"(how to generate landmarks
Geometer works by choosing "landmarks" or "points" on the
input waveform by various methods.  Click the button to
the left to choose a method of point generation.  When
applicable, the slider below this button adjusts some
parameter of landmark generation.)",
  R"(how to mess them up
After choosing points, Geometer can mess them up by
various means.  There are three operation slots, and three
parameter sliders corresponding to the slots.  Click a slot
to change what operation is performed, or choose empty
to perform no operations.)",
  R"(how to recreate the waveform
Finally, Geometer will re-draw a new waveform using the
landmarks as a guide.  Choose between the various
methods here, and use the slider above the button to
control the parameters of the effects.)",
  R"(midi learn: assign MIDI control changes to a parameter
Enable the MIDI learn button, and then move one of the
sliders to make it glow.  The next MIDI CC you send will
be assigned to this parameter, allowing you to control it
with your keyboard or MIDI device.  This won't work in
all hosts.  Check the included documentation for advanced
functionality and troubleshooting.)",
  R"DELIM(midi reset: clear a parameter's MIDI assignment
If you have assigned a MIDI event to a parameter (see the
"MIDI learn" help for more info), you can use this button
to erase the assignment.  First you need to activate the
parameter as a MIDI learner (again, see the "MIDI learn"
help), then press this button to erase that parameter's
MIDI assignment.)DELIM",
}};


// the help infos for window shape
constexpr std::array<char const * const, NUM_WINDOWSHAPES> windowshape_helpstrings
{{
  R"(triangle: linear fade between windows
Fades between windows linearly.)",
  R"(arrow: sharp fade in, dull fade out
Favors the tail end of windows.)",
  R"(wedge: dull fade in, fast fade out
Favors the beginning of windows.)",
  R"(cos: smooth fades in, out
A smooth fade-in and fade-out, emphasizing the center of
each window.)",
}};


// the help infos for "how to generate landmarks"
constexpr std::array<char const * const, NUM_POINTSTYLES> landmarks_helpstrings
{{
  R"(ext'n cross: place points at extremities and crossings
Place points at zero-crossings, as well as the peaks and
valleys between them.  The slider controls the tolerance
for what is considered 'zero'.)",
  R"(at freq: places points at specified frequency
Places points at the frequency specified by the slider.)",
  R"(random: places a specified number of points randomly
Places points randomly on the waveform.  The number of
points is controlled by the slider.)",
  R"(span: the sample value determines the next location
The larger the magnitude of the current sample, the
farther away the next sample will be.  The slider controls
the spanning factor.)",
  R"(dy/dx: make point when derivative changes by amount
Makes a point whenever the slope changes by some
threshold.  With the slider > 0.5, this catches sharp rises;
at less than 0.5, it makes points on sharp falls.  When
the slider is at 0.5, dy/dx makes a point at all peaks
and valleys.)",
  R"DELIM(level: make point when wave crosses a specific level
Makes a point when the waveform crosses a specific level
(both above and below the origin).  The slider controls the
level at which point generation occurs.)DELIM",
}};


// the help infos for the point operations
constexpr std::array<char const * const, NUM_OPS> ops_helpstrings
{{
  R"(x2: unscientifically double number of points
Double the number of points.  New points are placed half-
way between old points, with a magnitude determined by
the slider and the magnitudes of its neighbors.  Note that
there cannot be more points than samples, so this
operation may do nothing.)",
  R"(1/2: thin points by one half
Erases every other point.)",
  R"(1/4: thin points by one quarter
Erases three out of every four points.)",
  R"(longpass: keep only long intervals
If a point is not at least a certain distance from the
previous point, it is removed.  The distance is determined
by the slider.)",
  R"(shortpass: keep only short intervals
If a point is more than a certain distance from the
previous point, its height is set to zero.  The distance is
determined by the slider.)",
  R"(slow: lengthen time scale of points
Stretches out the points along the x-axis.  Points towards
the end of the frame are discarded.  This tends to lower
the pitch of the sound without changing its speed.)",
  R"(fast: shorten time scale of points
Compresses the points along the x-axis.  Points are
repeated to fill the frame.  This operation tends to raise
the pitch of the sound without changing its speed.)",
  "empty",
}};


// the help infos for "how to recreate the waveform"
constexpr std::array<char const * const, NUM_INTERPSTYLES> recreate_helpstrings
{{
  R"DELIM(polygon: recreate wave with linear interpolation
Recreate the wave by drawing lines directly between the
points.  Like a lowpass filter, but introduces new high
frequencies as sharp corners at points.  The slider controls
how steep the lines are (if greater than zero, they won't
match up!).)DELIM",
  R"(wrongygon: linear interpolation in the wrong direction
Like polygon, but the lines are flipped vertically about
their centers.  Very rough sound.  Like polygon, the slider
controls how steep the lines are.)",
  R"(smoothie: smoothly interpolate between points
Recreates the wave using segments of a smooth cosine
curve.  Much less harsh than polygon or wrongygon.
The slider controls the roundness of the curves.)",
  R"(reversie: reverse segments of the input wave
Reverses the input wave between points.  Can create
bizarre phasing artifacts.  Experiment with low point
frequencies and different window sizes for this one.)",
  R"(pulse: create pulses of specified size at points
Creates a pulse for each point.  The width of the pulse is
specified by the slider.  Wide pulses that overlap do
strange stuff; take a look.)",
  R"(friends: neighboring sections make friends
Stretches the original waveform from a section so that it
overlaps the next section.  The slider sets the amount of
overlap.  "Friends" tends to lower the pitch of the input
without altering its speed.)",
  R"DELIM(sing: replace the intervals with sine wave
Replaces each segment with one period of a sine wave.
It's best used with something where landmark generation
is related to the sound, like dy/dx (especially on percussive
input).  Combine with longpass for a less harsh sound.
The slider makes sing use the sine wave for amplitude
modulation instead, resulting in a totally different sound.)DELIM",
  R"(shuffle: mix around intervals
Mixes around the intervals.  Best used with a large
window size and long intervals.  When this option is
selected, the slider chooses how far the intervals are
shuffled.)",
}};
