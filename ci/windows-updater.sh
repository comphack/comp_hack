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

# Restore the cache (this mostly just handles Qt)
echo "Restoring cache"
cd "${ROOT_DIR}"
ci/travis-cache-windows.sh restore
echo "Restored cache"

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
mv comp_hack-*.zip "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_upload comp_hack "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip"
fi
