#!/bin/sh

set -e

"${DFX_ROOT_SOURCE_PATH}"/dfx-library/xcode/dfxplugin-postbuild.sh

/usr/libexec/PlistBuddy -c "Delete :AudioComponents" "${BUILT_PRODUCTS_DIR}"/"${INFOPLIST_PATH}"
