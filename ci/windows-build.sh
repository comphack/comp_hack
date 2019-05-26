#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

# Run the cache again just in case it was not warmed.
source "ci/windows-warmcache.sh"

#
# Dependencies
#

# Install .NET 3.5 from PowerShell first...
# https://travis-ci.community/t/unable-to-install-wix-toolset/1071/3
echo "Installing .NET 3.5"
powershell Install-WindowsFeature Net-Framework-Core
echo "Installed .NET 3.5"

echo "Installing WiX"
cinst -y wixtoolset | ci/report-progress.sh
set PATH="C:/Program Files (x86)/WiX Toolset v3.11/bin:${PATH}"
echo "Installed WiX"

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

echo "Installing comp_channel"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_download "build/comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip" "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
else
    cp "${CACHE_DIR}/comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip" "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
fi

mkdir bin
cd bin
unzip "../comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
rm "../comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
mv comp_hack-*/comp_channel* ./
rm -rf comp_hack-*/
ls -lh
cd ..

echo "Installed comp_channel"

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
cmake -DUSE_PREBUILT_LIBCOMP=ON -DGENERATE_DOCUMENTATION=OFF -DIMPORT_CHANNEL=ON \
    -DWINDOWS_SERVICE=ON -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCPACK_WIX_ROOT="C:/Program Files (x86)/WiX Toolset v3.11" \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

echo "Copying build result to the cache"
mkdir "${ROOT_DIR}/deploy"
cp comp_hack-*.{zip,msi} "${ROOT_DIR}/deploy/"
mv comp_hack-*.zip "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip"
mv comp_hack-*.msi "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.msi"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_upload comp_hack "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip"
    dropbox_upload comp_hack "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.msi"
fi
