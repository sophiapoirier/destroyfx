# Destroy FX Library

This is the Destroy FX Library, a collection of useful source code
material for writing audio processing plugins.  Our base library code
is composed of the `dfxplugin` and `dfxparameter` files, plus
`dfxmidi`, `dfxenvelope`, and `dfxsettings` for plugins that use MIDI.
And there is oodles of other stuff too.

`dfxplugin` has the base class from which we derive when making
plugins.  The main functions that it serves are (1) making it really
easy to make a new plugin from scratch and (2) making it really easy
to support multiple plugin APIs (Audio Unit and VST at the moment).

`dfxparameter` has the parameter class with all kinds of fanciness 
that we use in dfxplugins.

`dfxmidi` has MIDI handling and related routines.

`dfxsettings` has routines for storing special plugin settings in 
custom data blocks and also for adding MIDI learn functionality 
for fully MIDI control of all plugin parameters.

`dfx-au-utilities` has some stuff that's useful for pretty much any AU
plugin or host, so it's in separate files because we possibly also use
that stuff in non-DFX projects.  Also, for that same reason, these
files are an exception in that they are offered under the terms of a
modified BSD License (see bsd-license.txt) rather than the GNU 
General Public License (GPL).

`dfx-base.h` a bunch of core definitions for Destroy FX plugins,
and some base stuff that's just useful in all of our the source tree.

`dfxmath.h` has various handy math-related constants and functions.

`dfxmutex` has an implementation for mutexes that we use in Geometer.
