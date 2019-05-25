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

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_setup
    dropbox_download "build/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
else
    cp "${CACHE_DIR}/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
fi

unzip "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip" | ../ci/report-progress.sh
rm "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
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

mv comp_hack-*.zip "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_upload build "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
else
    cp "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip" "${CACHE_DIR}/"
fi
