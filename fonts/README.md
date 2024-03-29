# a word about our fonts

Some of our plugins' GUIs use special fonts for text display:

* Buffer Override uses "DFX Wetar 16px" and "DFX Pasement 9px"
* Geometer uses "DFX Snooty 10px"
* Polarizer uses "Boring Boron"
* Rez Synth uses "DFX Snooty 10px"
* Scrubby uses "DFX Snooty 10px"
* Skidder uses "DFX Snooty 10px"
* Transverb uses "DFX Snooty 10px"

`bboron.ttf` ("Boring Boron") is included in this directory. The font,
unlike other things in this repository module, is not covered by the
GPL. It was made by Tom, though. This font, along with boatloads of
other fonts by Tom, are available at
[http://fonts.tom7.com/](http://fonts.tom7.com/)


As of 2021, there are also some DFX-specific fonts, which are part of
this project and available under the GPL:

* DFX Pasement 9px
* DFX Wetar 16px
* DFX Snooty 10px

These are "fake bitmap" TTFs, which are actually vector data (straight
lines on a grid) generated from bitmap files. The workflow for
generating a TTF is like so:

* The inputs are the png file (contains the character bitmaps) and
  a matching .cfg file (describes the dimensions, font's name, etc.)
  In the PNG file, pure white `#fff` is an "on" pixel in the font.
  A column of pure black `#000` sets the width for each character.
  Any other color is ignored (usually some checkerboard pattern to
  make the grid easy to see).

* The `makesfd.exe` program from Tom's subversion projects must
  be built separately:

  https://sourceforge.net/p/tom7misc/svn/HEAD/tree/trunk/

  It's pure C++17 without external dependencies, so it shouldn't be
  too bad--but you're on your own!

* `makesfd.exe` generates a `.sfd` file from the inputs above (see
  the makefile for an invocation)

* The free font editor FontForge loads `.sfd` files (its native format).
  Just open up the .sfd file and use `File` > `Generate fonts...` to
  generate a TTF. You can ignore the validation error about
  self-intersecting curves (this is because on the pixel grid, some
  corners will be coincident; it doesn't seem to cause any rendering
  problems).
