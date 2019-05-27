#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

# Skip if not using Dropbox (we assume the build uses the release libcomp).
if [ ! $USE_DROPBOX ]; then
    echo "Not building libcomp because Dropbox is not setup for this build."
    exit 0
fi

# Run the cache again just in case it was not warmed.
source "ci/windows-warmcache.sh"

#
# Dependencies
#

# Check for existing build.
if [ $USE_DROPBOX ]; then
    dropbox_setup

    if dropbox-deployment -d -e "$DROPBOX_ENV" -u build -a . -s "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"; then
        exit 0
    fi
fi

cd "${ROOT_DIR}/libcomp"
mkdir build
cd build

echo "Installing external dependencies"
unzip "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.zip" | "${ROOT_DIR}/ci/report-progress.sh"
mv external* ../binaries
echo "Installed external dependencies"

echo "Installing Doxygen"
mkdir doxygen
cd doxygen
unzip "${CACHE_DIR}/doxygen.zip" | "${ROOT_DIR}/ci/report-progress.sh"
cd ..
export PATH="${ROOT_DIR}/libcomp/build/doxygen;${PATH}"
echo "Installed Doxygen"

#
# Build
#

cd "${ROOT_DIR}/libcomp/build"

echo "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DGENERATE_DOCUMENTATION=ON -DWINDOWS_SERVICE=ON \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

if [ $USE_DROPBOX ]; then
    echo "Copying package to cache for next stage"
    mv libcomp-*.zip "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
    dropbox_upload build "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
fi
