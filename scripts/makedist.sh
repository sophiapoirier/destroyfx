#!/bin/zsh

if (( $# < 3 )); then
	echo 
	echo "   "`basename $0`" output version manual pluginname 0|1(uses MIDI) input1 [input2 [...]]"
	echo 
	exit 1
fi

set -e

VERSION="${2}"
OUTPUTFILE="${1}-${VERSION}-mac"
TEMPDIR=`dirname "${1}"`/`basename "${1}"`_temp_$$_`jot -r -n -p 20 1`
PLUGINNAME="${4}"
BUNDLEID=org.destroyfx.`echo "${PLUGINNAME}" | tr -cd '[:alnum:]'`
LICENSENAME="COPYING.html"
ROOTDIR="${TEMPDIR}/root"
AUDIR="${ROOTDIR}/Library/Audio/Plug-Ins/Components"
DOCSDIR="${ROOTDIR}/Library/Documentation/Destroy FX"
INSTALLERDIR="${TEMPDIR}/installer"
IMAGEDIR="${TEMPDIR}/image"
DFXDIR="${HOME}/dfx/destroyfx"

echo
echo "   creating temporary directory  "$TEMPDIR
mkdir "${TEMPDIR}"
mkdir -p "${AUDIR}"
mkdir -p "${DOCSDIR}"
mkdir -p "${INSTALLERDIR}"
mkdir -p "${IMAGEDIR}"

MANUALNAME="${PLUGINNAME} manual.html"
echo "   creating  "$MANUALNAME"  in  "$DOCSDIR
makedocs "${3}" "${DOCSDIR}"/"${MANUALNAME}"
if (( $5 )); then
	echo "   creating  Destroy FX MIDI.html  in  "$DOCSDIR
	makedocs "${DFXDIR}"/docs/destroy-fx-midi.html "${DOCSDIR}"/Destroy\ FX\ MIDI.html
fi
# copy documentation to image staging as well
cp "${DOCSDIR}"/*.* "${IMAGEDIR}"
echo "   copying  "$LICENSENAME"  into  "$DOCSDIR
cp "${DFXDIR}"/docs/"${LICENSENAME}" "${DOCSDIR}"/

shift 5
while (( $# >= 1 )); do
	echo "   copying  ${1}  -->  "$AUDIR
	cp -f -R "${1}" "${AUDIR}"
	shift
done

echo
echo "   deleting the following files (possibly none):"
find -f "${ROOTDIR}" \( -name "pbdevelopment.plist" \) -print
find -f "${ROOTDIR}" \( -name "pbdevelopment.plist" \) -delete
find -f "${ROOTDIR}" \( -name ".DS_Store" \) -print
find -f "${ROOTDIR}" \( -name ".DS_Store" \) -delete

INSTALLERFILE="${IMAGEDIR}/Install ${PLUGINNAME}.pkg"
echo
echo "   creating installer package "$INSTALLERFILE
COMPONENT_PLIST="${INSTALLERDIR}"/component.plist
pkgbuild --analyze --root "${ROOTDIR}" "${COMPONENT_PLIST}"
COMPONENT_PACKAGE="${INSTALLERDIR}/${PLUGINNAME} ${VERSION}.pkg"
pkgbuild --root "${ROOTDIR}" --identifier "${BUNDLEID}" --version "${VERSION}" --install-location / --component-plist "${COMPONENT_PLIST}" "${COMPONENT_PACKAGE}"
DISTRIBUTIONFILE="${INSTALLERDIR}/distribution.xml"
DISTRIBUTIONFILE_AMENDED="${INSTALLERDIR}/distribution+.xml"
productbuild --synthesize --product "${DFXDIR}"/dfx-library/xcode/installer-requirements.plist --package "${COMPONENT_PACKAGE}" "${DISTRIBUTIONFILE}"
echo "   amending distribution file "$DISTRIBUTIONFILE
while read -r LINE || [ -n "${LINE}" ]
do
	if (( `echo "${LINE}" | grep -c "</installer-gui-script"` )); then
		echo "<title>Destroy FX : ${PLUGINNAME}</title>" >> "${DISTRIBUTIONFILE_AMENDED}"
		echo "<license file=\"${LICENSENAME}\" uti=\"public.html\"/>" >> "${DISTRIBUTIONFILE_AMENDED}"
	fi
	echo "${LINE}" >> "${DISTRIBUTIONFILE_AMENDED}"
done < "${DISTRIBUTIONFILE}"
diff -wq "${DISTRIBUTIONFILE}" "${DISTRIBUTIONFILE_AMENDED}" >> /dev/null
if (( $? == 0 )); then
	echo
	echo "ERROR: failed to amend distribution file!"
	exit 1
fi
productbuild --distribution "${DISTRIBUTIONFILE_AMENDED}" --resources "${DFXDIR}"/docs --package-path "${INSTALLERDIR}" --component-compression auto --sign "Developer ID Installer: Sophia Poirier (N8VK88P4LV)" --timestamp "${INSTALLERFILE}"

OUTPUTFILE_FULLNAME="${OUTPUTFILE}".dmg
echo
echo "   creating output file  "$OUTPUTFILE_FULLNAME
hdiutil create -ov -srcfolder "${IMAGEDIR}" -volname "dfx ${PLUGINNAME}" -format UDBZ -imagekey zlib-level=9 "${OUTPUTFILE}"

echo
echo "   removing temp directory  "$TEMPDIR
rm -R "${TEMPDIR}"

NOTARIZE="~/dfx/scripts/notarize.sh \"${OUTPUTFILE_FULLNAME}\" ${BUNDLEID}"
echo -n $NOTARIZE | pbcopy
echo
echo "   done with "`basename "${OUTPUTFILE_FULLNAME}"`"!"
echo "   but don't forget to:"
echo
echo
echo
echo $NOTARIZE
echo
echo "   (also copied to clipboard)"
echo
exit 0
