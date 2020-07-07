#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

# Run the cache again just in case it was not warmed.
source "ci/windows-warmcache.sh"

#
# Dependencies
#

cd "${ROOT_DIR}"
mkdir build
cd build

echo "Installing external dependencies"
unzip "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.zip"
mv external* ../binaries
echo "Installed external dependencies"

echo "Installing Qt"
mkdir -p "${QT_EXTRACT_DIR}"
cd "${QT_EXTRACT_DIR}"
7z x "${CACHE_DIR}/qt-${WINDOWS_QT_VERSION}-${PLATFORM}.7z"
cd ..
echo "Installed Qt"

#
# Build
#

cd "${ROOT_DIR}/build"

echo "Running cmake"
cmake -DGENERATE_DOCUMENTATION=OFF -DUPDATER_ONLY=ON \
    -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

echo "Copying build result to the cache"
mkdir "${ROOT_DIR}/deploy"
cp comp_hack-*.zip "${ROOT_DIR}/deploy/"
mv comp_hack-*.zip "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_setup
    dropbox_upload updater "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip"
fi
