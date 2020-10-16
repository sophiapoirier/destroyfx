#!/bin/sh

#VERSION=`/usr/libexec/PlistBuddy -c "Print :CFBundleVersion" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"`
VERSION="${DFX_PLUGIN_VERSION_STRING}"
BUILD_NUM=`date "+%Y.%m.%d"`
COPYRIGHT="©${DFX_PLUGIN_COPYRIGHT_PREFIX}"`date "+%Y"`" ${DFX_PLUGIN_COPYRIGHT_NAME}"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion ${BUILD_NUM}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString ${VERSION}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
/usr/libexec/PlistBuddy -c "Set :NSHumanReadableCopyright ${COPYRIGHT}" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"

"${DFX_ROOT_SOURCE_PATH}"/dfx-library/xcode/dfxplugin-copy-docs.sh

# appease codesign by removing all Finder info extended attributes that seem to attach themselves to any files in use
if [ -d "${CODESIGNING_FOLDER_PATH}" ]
then
	xattr -cr "${CODESIGNING_FOLDER_PATH}"
fi
