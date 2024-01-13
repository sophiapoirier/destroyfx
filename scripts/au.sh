#!/bin/zsh

# input arguments:
#  1 - the proper name (captilized, maybe spaces) name of the plugin
#  2 - whether or not the plugin uses MIDI   (boolean: 0 or 1) (optional: default is 0)
# examples:
#  au.sh Buffer\ Override 1
#  au.sh Monomaker

if (( $# < 1 )); then
	echo
	echo "   "`basename $0`" pluginname [0|1(uses MIDI)]"
	echo
	exit 1
fi

set -e

# the first argument should be the full proper name of the plugin
PLUGINNAME="${1}"
# this is the plugin file name
PLUGINNAME_FILE="dfx ${PLUGINNAME}.component"
# and full path
PLUGINPATH="${HOME}/Library/Audio/Plug-Ins/Components/${PLUGINNAME_FILE}"
# pull the version number
VERSION=`/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "${PLUGINPATH}"/Contents/Info.plist`
# this is the base of the output archive file name 
# (lower case with non-alphanumeric characters removed)
PLUGINNAME_STRIPPED=`echo "${PLUGINNAME}" | tr '[:upper:]' '[:lower:]' | tr -cd '[:alnum:]'`
# this is the base of the docs file name 
DOCSNAME="${PLUGINNAME_STRIPPED}"

MIDIPLUGIN=0
if (( $# >= 2 ))
then
	MIDIPLUGIN=$2
fi

# run the input through makedist.sh
#  * assumes that makedist.sh is in the same directory as this script
#  * uses the current date to generate part of the output file name
#  * ummm, also assumes some other stuff being where it happens 
#    to be on my hard disk, hmmm, maybe I should improve this...
`dirname $0`/makedist.sh ~/dfx/_misc/distributions/"${PLUGINNAME}"/$PLUGINNAME_STRIPPED "${VERSION}" ~/dfx/destroyfx/docs/${DOCSNAME}.html "${PLUGINNAME}" $MIDIPLUGIN "${PLUGINPATH}"
