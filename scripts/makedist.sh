#!/bin/sh

if (( $# < 3 )); then
	echo 
	echo "   "`basename $0`" output suffix inmanual pluginname 0|1(uses MIDI) input1 [input2 [...]]"
	echo 
	exit 1
fi

ENTRYDIR=`pwd`
OUTPUTFILE="$1"-"$2"
TEMPDIR=`dirname "$1"`/`basename "$1"`_temp_$$_`jot -r -n -p 20 1`
PLUGINNAME="$4"
PLUGINNAME_FILE=`basename "${6}"`

echo
echo "   creating temporary directory  "$TEMPDIR
mkdir "${TEMPDIR}"
MANUALNAME="${PLUGINNAME} manual.html"
echo "   creating  "$MANUALNAME"  in  "`basename "${TEMPDIR}"`
makedocs "$3" "${TEMPDIR}"/"${MANUALNAME}"
if (( $5 )); then
	echo "   creating  Destroy FX MIDI.html  in  "`basename "${TEMPDIR}"`
	makedocs ~/dfx/vstplugins/docs/destroy-fx-midi.html "${TEMPDIR}"/Destroy\ FX\ MIDI.html
fi
shift 5
while (( $# >= 1 )); do
	echo "   copying  $1  -->  "`basename "${TEMPDIR}"`/
	cp -f -R "$1" "${TEMPDIR}"
	shift
done

#cp -f ~/dfx/vstplugins/scripts/install-au.command "${TEMPDIR}"/"Install ${PLUGINNAME}.command"
cp -f ~/dfx/scripts/install-au.command "${TEMPDIR}"/"Install ${PLUGINNAME}.command"

cd "${TEMPDIR}"
echo
echo "   deleting the following files (possibly none):"
find -f . \( -name "pbdevelopment.plist" \) -print
find -f . \( -name "pbdevelopment.plist" \) -delete
find -f . \( -name ".DS_Store" \) -print
find -f . \( -name ".DS_Store" \) -delete

PLUGIN_BUNDLE_RESOURCES_DIR="${PLUGINNAME_FILE}/Contents/Resources/en.lproj"
# if the English-localized sub-directory doesn't exist, then try the root resources directory
if [ ! -d "${PLUGIN_BUNDLE_RESOURCES_DIR}" ]; then
	PLUGIN_BUNDLE_RESOURCES_DIR=`basename "${PLUGIN_BUNDLE_RESOURCES_DIR}"`
fi
if [ -d "${PLUGIN_BUNDLE_RESOURCES_DIR}" ]; then
	echo
	echo "   copying  "$MANUALNAME"  into  "$PLUGINNAME_FILE
	cp -f "${MANUALNAME}" "${PLUGIN_BUNDLE_RESOURCES_DIR}"/
fi

#OUTPUTFILE_FULLNAME="${OUTPUTFILE}".tar.gz
OUTPUTFILE_FULLNAME="${OUTPUTFILE}".dmg
echo
echo "   creating output file  "$OUTPUTFILE_FULLNAME
#gnutar -cv -f "${OUTPUTFILE}".tar *
#gzip --best "${OUTPUTFILE}".tar
hdiutil create -ov -srcfolder "${TEMPDIR}" -volname "DFX ${PLUGINNAME}" -nouuid -gid 99 -format UDZO -imagekey zlib-level=9 "${OUTPUTFILE}"


cd "${ENTRYDIR}"
echo
echo "   removing temp directory  "$TEMPDIR
rm -R "${TEMPDIR}"

echo
echo "   done with "`basename "${OUTPUTFILE_FULLNAME}"`"!"
echo
exit 0

# $*, $@
# All the arguments as a blank separated string.  Watch out for "$*" vs. "$@"
