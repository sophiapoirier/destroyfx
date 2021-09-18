#!/bin/bash

# Build all of the maintained plugin DLLs for win32.
# Note that because of our use of defines and the fact that we
# leave .o files alongside their source files, we need to
# 'make clean' when switching plugins; so we should not use
# something like 'make -C' here.

# 30 is probably overkill unless you have a lot of cores!
THREADS=30

function fail() {
    echo "$1"
    exit -1
}

function buildplugin() {
    local plugin="$1"
    local plugin_dll="dfx-${plugin}-64.dll"
    pushd "${plugin}/win32"
    [[ $PWD = */win32 ]] || fail "not in the expected directory?"
    make -s clean
    make -s -j "${THREADS}" "${plugin_dll}" install 2>/dev/null >/dev/null
    test -f "${plugin_dll}" || fail "failed to build ${plugin_dll}"
    popd >/dev/null
    [[ $PWD = */destroyfx ]] || fail "not in the expected directory?"
    echo "Built ${plugin_dll}."
}

[[ $PWD = */destroyfx ]] || fail "please run this from destroyfx/ (e.g. ./tools/win32-make-all.sh)"


buildplugin geometer
buildplugin scrubby
buildplugin bufferoverride
buildplugin transverb
buildplugin rezsynth
buildplugin skidder

buildplugin monomaker
buildplugin midigater
buildplugin eqsync
buildplugin polarizer

# need to be ported to vstplugin
# buildplugin rmsbuddy

