#!/bin/sh

"${DFX_ROOT_SOURCE_PATH}"/dfx-library/xcode/dfxplugin-postbuild.sh

DFX_METADATA_TOOL="${BUILT_PRODUCTS_DIR}"/dfxplugin-metadata
TYPE=`"${DFX_METADATA_TOOL}" type`
SUBTYPE=`"${DFX_METADATA_TOOL}" id`
# convert the M.m.b string value to 0xMMMMmmbb hex value
VERSION=`"${DFX_METADATA_TOOL}" version`
VERSION_NUMS_ARRAY=( ${VERSION//./ } )
(( VERSION_NUM = (${VERSION_NUMS_ARRAY[0]} * 256 * 256) + (${VERSION_NUMS_ARRAY[1]} * 256) + ${VERSION_NUMS_ARRAY[2]} ))

INDEX=0
PLIST_ERROR=0
# there is no way to query the array count, so just iterate until operating on an array index fails
while [[ $PLIST_ERROR == 0 ]]
do
	/usr/libexec/PlistBuddy -c "Set :AudioComponents:${INDEX}:type ${TYPE}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
	/usr/libexec/PlistBuddy -c "Set :AudioComponents:${INDEX}:subtype ${SUBTYPE}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
	/usr/libexec/PlistBuddy -c "Set :AudioComponents:${INDEX}:version ${VERSION_NUM}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
	PLIST_ERROR=$?
	INDEX=$((INDEX+1))
done
