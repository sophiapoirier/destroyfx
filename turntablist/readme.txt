JUNE 14, 2004

I am releasing the source code to these plugins since I do not plan on working on the anymore and would rather have others continue their development or use them as learning tools.  I have only included the source code and art files.  There are no compiler project files or any other files that are needed to build the actual plugins.  You should have experience with developing VST plugins if you really want to make any use of this code.  This is not for newbies and there is no support for anything.

Development platforms used:
Windows 98SE, XP: Visual Studio 6, Visual Studio .NET 2002 and 2003
Mac OS9: Codewarrior 7
Mac OSX: Codewarrior 9

Libaries needed by the plugins:
For sample loading in Turntablist Pro and Drumbox, I used version 1.05 of the libsnfile library.  Get it from here: http://www.mega-nerd.com/libsndfile/
For TTP, Creakbox and Drumbox I used version 2.3 of the VST/VSTGUI SDKs from http://www.steinberg.net/.  I probably used a beta version of 2.2 for LiveGate but it shouldn't be much trouble to get it working with 2.3.


About the plugins:
Turntablist Pro - This is my first plugin so it is a big mess.  The core scratching/playback engine has been changed many times and there are huge chunks of commented out code I kept around for reference.  Scratcha was the development name of it.  The version of the source is 1.05 which fixes the bug in OS9 that prevent it from loading samples from any other drive than the main drive.  The last release version was 1.04.

Creakbox Bassline - This is the VSTi version of the TB-303 clone originally programmed by Frank Reitz.  Silverbox and CB303 were the development names of it.  This version is way beyond the free Creakbox Demo that was released in 2003.  It supports 100 patterns that can be from 0 to 64 steps long.  Oh, and it has some nice bugs that I gave up on.  Good luck to anyone who wants to get this fixed.

Drumbox - This was a mutant child of TTP and Creakbox that was the last plugin I started.  Sample based step sequencer with some interesting features.  The SampleChannel class was originally a structure that is why it is in the header file.  I just kept on adding more and more to it and never bothered to move it out to a .cpp file.  This has a lot of potential for anyone who wants to finish and expand it.

LiveGate - This plugin was specifically coded for use with Ableton Live.  I used it during my live sets and it is really fun.  This was never released to the public.

Martin Robaszewski

If you really want to contact me, just PM bioroid on http://www.kvr-vst.com but I will not be offering any support.

