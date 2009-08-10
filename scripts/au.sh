#!/bin/sh

# input arguments:
#  1 - the proper name (captilized, maybe spaces) name of the plugin
#  2 - whether or not the plugin uses MIDI   (boolean:  0 or 1)
# examples:
#  au.sh Buffer\ Override 1
#  au.sh Monomaker 0

if (( $# < 1 )); then
	echo 
	echo "   "`basename $0`" pluginname [0|1(uses MIDI)]"
	echo 
	exit 1
fi

# the first argument should be the full proper name of the plugin
PLUGINNAME="$1"
# this is the plugin file name
PLUGINNAME_FILE="dfx ${PLUGINNAME}.component"
# this is the name mapped to all lowercase letters
PLUGINNAME_LC="`echo "${PLUGINNAME}\c" | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"`"
# this is the base of the docs file name 
# (lowercase with spaces turned to hyphens)
DOCSNAME=`echo "${PLUGINNAME_LC}\c" | sed "y/ /-/"`
# this is the base of the output archive file name 
# (lower case with non-alphanumeric characters removed)
PLUGINNAME_STRIPPED=`echo "${DOCSNAME}\c" | sed "s/\([^0-9A-Za-z]\)//g"`

MIDIPLUGIN=0
if (( $# >= 2 ))
then
	MIDIPLUGIN=$2
fi

# running strip -x reduces the executable file size 
# by removing all local symbols and debugging symbols
strip -x ~/Library/Audio/Plug-Ins/Components/"${PLUGINNAME_FILE}"/Contents/MacOS/*

# run the input through makedist.sh
#  * assumes that makedist.sh is in the same directory as this script
#  * uses the current date to generate part of the output file name
#  * ummm, also assumes some other stuff being where it happens 
#    to be on my hard disk, hmmm, maybe I should improve this...
`dirname $0`/makedist.sh ~/dfx/_misc/distributions/first\ Audio\ Units/$PLUGINNAME_STRIPPED au-`date \`\`+%Y-%m-%d''` ~/dfx/repos/trunk/docs/${DOCSNAME}.html "${PLUGINNAME}" $MIDIPLUGIN ~/Library/Audio/Plug-Ins/Components/"${PLUGINNAME_FILE}" ~/dfx/docs/COPYING.html


exit 0
