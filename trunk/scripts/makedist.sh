#!/bin/sh

if (( $# < 3 )); then
	echo 
	echo "   "`basename $0`" output suffix inmanual outmanual(.html) 0|1(uses MIDI) input1 [input2 [...]]"
	echo 
	exit 1
fi

ENTRYDIR=`pwd`
OUTPUTFILE="$1"-$2
TEMPDIR=`dirname "$1"`/`basename "$1"`_temp_$$_`jot -r -n -p 20 1`

echo
echo "   creating temporary directory  "$TEMPDIR
mkdir "${TEMPDIR}"
echo "   creating  "$4" manual.html  in  "`basename "${TEMPDIR}"`
makedocs "$3" "${TEMPDIR}"/"$4 manual.html"
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

cd "${TEMPDIR}"
echo
echo "   deleting the following files:"
find -f . \( -name "pbdevelopment.plist" \) -print
find -f . \( -name "pbdevelopment.plist" \) -delete
find -f . \( -name ".DS_Store" \) -print
find -f . \( -name ".DS_Store" \) -delete

echo
echo "   creating output file  "$OUTPUTFILE".tar.gz"
gnutar -cv -f "$OUTPUTFILE".tar *
gzip --best "$OUTPUTFILE".tar

cd "${ENTRYDIR}"
echo
echo "   removing temp directory  "$TEMPDIR
rm -R "${TEMPDIR}"

echo
echo "   done with "`basename "${OUTPUTFILE}"`".tar.gz!"
echo
exit 0

# $*, $@
# All the arguments as a blank separated string.  Watch out for "$*" vs. "$@"
