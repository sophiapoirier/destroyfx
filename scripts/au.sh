#!/bin/sh

# input arguments:
#  1 - the all-lowercase, all-one-word name of the plugin
#  2 - the proper name (captilized, maybe spaces) name of the plugin
#  3 - the name of the plugin's documentation file
#  4 - whether or not the plugin uses MIDI   (boolean:  0 or 1)
# examples:
#  au.sh bufferoverride Buffer\ Override buffer-override.html 1
#  au.sh monomaker Monomaker monomaker.html 0

# the COPY_PHASE_STRIP option in Project Builder runs strip -x on bundles, 
# which doesn't seem to get all of the debugging symbols out
# running strip -S reduces the executable file size a bit more
strip -S ~/Library/Audio/Plug-Ins/Components/"$2.component"/Contents/MacOS/*

# run the input through makedist.sh
#  * assumes that makedist.sh is in the same directory as this script
#  * uses the date to generate part of the output file name
#  * ummm, also assumes some other stuff being where it happens 
#    to be on Marc's hard disk, hmmm, maybe I should improve this...
`dirname $0`/makedist.sh ~/dfx/_misc/distributions/first\ Audio\ Units/$1 au-`date \`\`+%m-%d-%Y''` ~/dfx/vstplugins/docs/$3 "$2" $4 ~/Library/Audio/Plug-Ins/Components/"$2.component" ~/dfx/docs/COPYING.rtf

exit 0
