# Super Destroy FX Plugin Sources

These are some of the plugins that are part of the Destroy FX plugin
pack. We support two plugin formats: VST and Audio Unit (AU).
VST versions should be (nearly) source-portable to any platform which
supports that. Audio Unit is a format exclusive to macOS and iOS.
Some advanced features may not work on all platforms or in all hosts.

C++ is currently the only appropriate language for creating audio
plugins, so all of these are written in C++. You will need a C++
compiler for your platform in order to compile them. You'll also need
an appropriate version of the VST SDK (for VST) or CoreAudio SDK (for
Audio Unit). Right now, this means that you'll be able to easily
compile Xcode for Mac, and that it may be more difficult for any other
platform. Warning: Our source code repository often drifts out of date
so that builds do not work; sorry about this. The windows builds are
especially old. Tom doesn't use Visual Studio anymore and is
attempting to make this stuff compile with the free mingw toolchain
(GCC).

These plugins are Copyright (c) Tom Murphy 7 and Sophia Poirier. You
can use them in your music however you like, without royalties. You
can also modify them to your liking. However, if you distribute them
(or derivative/modified versions of them) then you must also
distribute the source package in order to be in compliance with the
license (see the file COPYING for full license terms).

This software comes with no warranty (see the file COPYING). In fact,
it is likely that some of these crazy effects could crash hosts (by
sending out-of-range samples or taking too much time to process), so
you should be careful to save your work when using them in important
songs. Tom uses Cakewalk on the PC, and Sophia uses Logic on the Mac,
so the plugins are likely work properly in those programs, at least.

Pre-packaged versions of (some of) these with fancy GUIs are available
at the Super Destroy FX website:

[http://destroyfx.org](http://destroyfx.org)

If you are simply interested in making music, you should check there
first. If instead you want to get your feet wet making your own effect
plugins, this source code might be a good place to start. (You can
also see and play with our effects before they're finished!)

---

Here are descriptions of each of the plugins included. First, we have
our "finished" plugins; these have been released, have fancy GUIs, and
are pretty thoroughly tested:

### transverb/

Sort of like a tape-loop with independently-moving read/write heads.
Creates reverb-like effects with more variation over time.
Enable Tomsound for extra glitches!

### scrubby/

Zips around the audio buffer like a DJ (or robot DJ) scratching a
record. Has the ability to constrain the scratch speeds using a
MIDI keyboard, among other advanced features.

### bufferoverride/

Reads data into a buffer that might be smaller or larger than the
host buffer size, and then keeps repeating that buffer over and over.
Gives a sort of robotic or stuttering effect to your sound, though
many twisted uses are possible.

### skidder/

Turns your sound on and off at regular or random intervals.
Practically every facet is controllable by a parameter. Skidder also
features the possibility for extreme settings and MIDI control.

### geometer/

A laboratory for waveform geometry. All sorts of crazy effects are
possible with this thing, and sports a neat visual display of what's
happening to your sound.

### rezsynth/

Allows you to "play" resonant bandpass filters with MIDI notes.

### rmsbuddy/

RMS and peak audio monitoring utility.

### monomaker/

Monomaker is just a simple stereo utility.  It can do panning and
merge a stereo signal to mono.

### midigater/

A MIDI note controlled audio gate.

### eqsync/

Total novelty effect that randomly retunes biquad filter coefficents
at regular intervals synchronized to tempo.

### polarizer/

Inverts the polarity of every Nth audio sample, resulting in a
crispy digital noisy effect.

### freeverb/

Freeverb is a public domain reverb algorithm by Jezar at Dreampoint.
There are several VST implementations of it, but no AU, so we ported
it, and have made some improvements to it too.

### fake-app/

A fake macOS application that associates icons with Audio Units and
their preset files.  Launching the app does nothing.

### max-patches/

Very very ancient [Max](https://cycling74.com) patches that we once
upon a time exported and released as utility applications.

### turntablist/

Turntablist is a record scratching emulation instrument by
[bioroid media development](http://www.bioroid.com/vst/),
but has since been abondaned. The author released the source code
after abandoning it, and we ported it to Audio Unit, and made
several improvements along the way. We no longer maintain it.

---

These plugins are somewhat mature, but are lacking GUIs and thorough
testing:

### brokenfft/

One of Tom's favorite plugins; this converts to the frequency domain
using the FFT, and then does a number of crazy effects. It has an
almost limitless variety of sounds that will come out of it...


These plugins are experiments and may or may not work:

### exemplar/

Tries to recreate your sound from a training set using
nearest-neighbor techniques. I thought this was going to be awesome
but initial experiments were disappointing. Should be revisited now
that I know more about machine learning.

### vardelay/

Allows for short delays of each sample, but the amount of delay is
dependent on the amplitude of the sample. (Several bands are
individually adjustable).

### trans/

Converts to and from other domains (sin, tan, derivative, e^x, FFT);
the idea is that you run this and its inverse with some other plugins
in-between.

### slowft/

The "slow fourier transform." Not sure what this is about yet. ;)

### firefly/

The idea was to provide a general-purpose finite impulse response
filter with a drawable model. Didn't get very far, except drawing a
placeholder GUI.

These plugins are not in development, perhaps because their
functionality has been subsumed by another plugin:

### decimate/

Reduces bit depth and sample rate in order to produce artifacting.
Extreme settings. Also includes a bonus "DESTROY" effect. Geometer
is much more flexible and beautiful than this oldie.

### intercom/

Adds noise to your sound based on its RMS volume; can also swap
samples around. Again, Geometer probably has more flexible effects
that are similar in sound.

---

In the source package, each plugin has a corresponding directory, as
well as a win32 subdirectory containing Windows build files. In 
these directories, a file called "makefile" can be used from the 
command line to build the plugin.

Most of these plugins have mac subdirectories with stuff for 
building Audio Unit versions of our plugins.  The plugin.xcodeproj 
"files" (well, they are bundles, thus technically directories 
appearing as files) are Xcode projects.  The plugin.exp files are 
the entry point symbol files.  The InfoPlist.strings files are 
localized values for some Info.plist values and are located in 
<language-code>.lproj subdirectories accordingly.

In order to build Audio Units, you need a Mac with Xcode installed. 
Xcode is available in the Mac App Store.

for building 64-bit Windows DLLs:  
Tom is using mingw-x86\_64 now, which is GCC targeting windows 64.
It's free software. I use it installed through cygwin (64), which is a
compatibility shim between unix and windows APIs. I recommend against
the cygwin compilers (e.g. "g++") unless you're trying to compile
software written for unix (Destroy FX plugins are not). mingw-x86\_64
instead uses the native windows api (`#include "windows.h"` etc.) and
generates native windows binaries that don't depend on cygwin. Cygwin
here just helps us install and update mingw-x86\_64, and also provides
useful unix tools like bash, make, etc.

From bash, you should be able to just run `make` from e.g.
transverb/win32 (or I run `make -j 30` to run 30 parallel processes)
and produce dfx-transverb-64.dll. Because the build process has a
bunch of `#defines`, you generally want to `make clean` before
building a plugin; otherwise you might end up combining something
like "dfx-plugin.o built with Transverb flags like `HAS_GUI`" with
"some-plugin.o that doesn't define `HAS_GUI`" and get bad results.

It may also be possible to cross-compile windows DLLs from other
platforms. I think it is possible to install mingw on say, linux, and
the build process shouldn't need to run anything other than the
compiler, resource compiler, and linker. I haven't tried it, though.

We are using C++17 features in DFX now, so you might need a pretty
new compiler. This is known to work:

	$ x86_64-w64-mingw32-g++ --version
	sx86_64-w64-mingw32-g++ (GCC) 9.2.0


You will also need the VSTGUI library to compile the UIs for these
plugins. We put this in a directory called "vstgui" next to the
destroyfx repository (it then contains another directory called
"vstgui"). VSTGUI is free and open source; you can just check it
out from GitHub: 
[https://github.com/steinbergmedia/vstgui](https://github.com/steinbergmedia/vstgui)

---

Here is a description of each of the remaining directories in the
source distribution:

### dfx-library/

This is the library of common code that we use as a shortcut for
creating new plugins and to generalize away all of the differences
between the various plugin formats that we support.

### dfxgui/

This is the library of common code that we use as a shortcut for
creating the GUIs for our plugins, with a lot of common features
included and hooks into the special DFX sauces. The library leverages
VSTGUI as a generalized foundation for platform-specific GUI APIs.

### docs/

The documentation for users.  We include these in the binary
distributions of our plugins.

### fonts/

Some of our GUIs render text with custom fonts. Tom made these!  
[Divide By Zero Fonts](http://fonts.tom7.com)

### scripts/

Some scripts that we utilize during development.

### site/

The files that comprise our website.

### stub-plugin/

Sources for starting a new plugin. Much nicer than the Steinberg
examples (in my opinion).

### windowingstub/

Sources for starting a new plugin with buffering and windowing. This
is important when discontinuities at the beginning and end of
processing buffers translate into audible artifacts in the output.
(For example, when doing FFT.) This adds certain overhead and extra
complexity, and requires running twice as much effect processing.
In addition, some effects do not fit the model well (a memory-driven
delay plugin, for instance). If your plugin does not need this, use
stub-plugin below instead.

### CoreAudioUtilityClasses/

Apple's CoreAudio SDK that includes the Audio Unit SDK, required for
building the AU versions of our plugins.

### vstgui/

Steinberg's cross-platform and cross-plugin-format (despite the VST
in its name) library for GUI creation.  DFX GUI is based atop this.

### vstsdk/

SDK from Steinberg for creating VST plugins. Required to build VST
versions of the DFX plugins. There is no code in this directory;
you should get the headers and class stubs from
[Steinberg's website](https://www.steinberg.net).

### fftw/

"Fastest Fourier Transform in the West", which actually comes from
the East at MIT. A very fast FFT routine (GPL).

### fft-lib/

FFT Library from Don Cross. Public Domain.

### ann/

The "Approximate Nearest Neighbors" library, which can be used to do
pretty efficient nearest neighbor calculations as long as the set of
points is preprocessed (GPL).

### notes/

secret diaries
