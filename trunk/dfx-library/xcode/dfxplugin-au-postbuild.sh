#!/bin/sh

"${DFX_ROOT_SOURCE_PATH}"/dfx-library/xcode/dfxplugin-postbuild.sh

# convert the M.m.b string value to 0xMMMMmmbb hex value
VERSION_NUMS_ARRAY=( ${DFX_PLUGIN_VERSION_STRING//./ } )
(( VERSION_NUM = (${VERSION_NUMS_ARRAY[0]} * 256 * 256) + (${VERSION_NUMS_ARRAY[1]} * 256) + ${VERSION_NUMS_ARRAY[2]} ))

INDEX=0
PLIST_ERROR=0
# there is no way to query the array count, so just iterate until operating on an array index fails
while [[ $PLIST_ERROR == 0 ]]
do
	/usr/libexec/PlistBuddy -c "Set :AudioComponents:${INDEX}:version ${VERSION_NUM}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
	PLIST_ERROR=$?
	INDEX=$((INDEX+1))
done
