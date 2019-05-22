#!/bin/bash
set -e

# Load the global settings.
source "ci/global.sh"

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
unzip "${CACHE_DIR}/libcomp-${PLATFORM}.zip" | ../ci/report-progress.sh
mv libcomp* ../deps/libcomp
ls ../deps/libcomp
echo "Installed libcomp"

echo "Installing comp_channel"
echo mkdir
mkdir bin
echo cd
cd bin
ls -lh "${CACHE_DIR}"
echo unzip
unzip "${CACHE_DIR}/channel-${PLATFORM}.zip" | ../ci/report-progress.sh
echo mv
mv comp_hack-*/comp_channel* ./
echo rm
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
cmake -DUSE_PREBUILT_LIBCOMP=ON -DGENERATE_DOCUMENTATION=OFF \
    -DWINDOWS_SERVICE=ON -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCPACK_WIX_ROOT="C:/Program Files (x86)/WiX Toolset v3.11" \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

echo "Copying build result to the cache"
rm -rf "${CACHE_DIR}/build-${PLATFORM}/"
mkdir "${CACHE_DIR}/build-${PLATFORM}"
cp comp_hack-*.{zip,msi} "${CACHE_DIR}/build-${PLATFORM}/"
ls -lh "${CACHE_DIR}/build-${PLATFORM}/"
