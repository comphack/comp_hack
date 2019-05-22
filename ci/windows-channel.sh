#!/bin/bash
set -e

# Load the global settings.
source "ci/global.sh"

#
# Dependencies
#

cd "${ROOT_DIR}"
mkdir build
cd build

echo "Installing external dependencies"
unzip "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.zip" | ../ci/report-progress.sh
mv external* ../binaries
echo "Installed external dependencies"

echo "Installing libcomp"
unzip "${CACHE_DIR}/libcomp-${PLATFORM}.zip" | ../ci/report-progress.sh
mv libcomp* ../deps/libcomp
ls ../deps/libcomp
echo "Installed libcomp"

#
# Build
#

cd "${ROOT_DIR}/build"

echo "Running cmake"
cmake -DUSE_PREBUILT_LIBCOMP=ON -DGENERATE_DOCUMENTATION=OFF -DCHANNEL_ONLY=ON \
    -DWINDOWS_SERVICE=ON -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

echo "Copying build result to the cache"
cp comp_hack-*.zip "${CACHE_DIR}/channel-${PLATFORM}.zip"
ls -lh "${CACHE_DIR}"
