#!/bin/sh

#VERSION=`/usr/libexec/PlistBuddy -c "Print :CFBundleVersion" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"`
VERSION="${DFX_PLUGIN_VERSION_STRING}"
COPYRIGHT="Copyright (C) "`date "+%Y"`" ${DFX_PLUGIN_CREATOR_NAME}"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion ${VERSION}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString ${VERSION}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
/usr/libexec/PlistBuddy -c "Set :NSHumanReadableCopyright ${COPYRIGHT}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"

# convert the M.m.b string value to 0xMMMMmmbb hex value
VERNUMS_ARRAY=( ${VERSION//./ } )
(( VERNUM = (${VERNUMS_ARRAY[0]} * 256 * 256) + (${VERNUMS_ARRAY[1]} * 256) + ${VERNUMS_ARRAY[2]} ))
INDEX=0
PLIST_ERROR=0
# there is no way to query the array count, so just iterate until operating on an array index fails
while [[ $PLIST_ERROR == 0 ]]
do
	/usr/libexec/PlistBuddy -c "Set :AudioComponents:${INDEX}:version ${VERNUM}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
	PLIST_ERROR=$?
	INDEX=$((INDEX+1))
done
