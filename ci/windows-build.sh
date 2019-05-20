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
cmake -DUSE_PREBUILT_LIBCOMP=ON -DGENERATE_DOCUMENTATION=ON \
    -DWINDOWS_SERVICE=ON -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCPACK_WIX_ROOT="C:/Program Files (x86)/WiX Toolset v3.11" \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..

echo "Running build"
echo "BUILDING TARGET comp_bdpatch"
time cmake --build . --config "$CONFIGURATION" --target comp_bdpatch
echo "BUILT TARGET comp_bdpatch"
echo "BUILDING TARGET comp_bgmtool"
time cmake --build . --config "$CONFIGURATION" --target comp_bgmtool
echo "BUILT TARGET comp_bgmtool"
echo "BUILDING TARGET comp_capgrep"
time cmake --build . --config "$CONFIGURATION" --target comp_capgrep
echo "BUILT TARGET comp_capgrep"
echo "BUILDING TARGET comp_cathedral"
time cmake --build . --config "$CONFIGURATION" --target comp_cathedral
echo "BUILT TARGET comp_cathedral"
echo "BUILDING TARGET comp_decrypt"
time cmake --build . --config "$CONFIGURATION" --target comp_decrypt
echo "BUILT TARGET comp_decrypt"
echo "BUILDING TARGET comp_encrypt"
time cmake --build . --config "$CONFIGURATION" --target comp_encrypt
echo "BUILT TARGET comp_encrypt"
echo "BUILDING TARGET comp_logger"
time cmake --build . --config "$CONFIGURATION" --target comp_logger
echo "BUILT TARGET comp_logger"
echo "BUILDING TARGET comp_logger_headless"
time cmake --build . --config "$CONFIGURATION" --target comp_logger_headless
echo "BUILT TARGET comp_logger_headless"
echo "BUILDING TARGET comp_nifcrypt"
time cmake --build . --config "$CONFIGURATION" --target comp_nifcrypt
echo "BUILT TARGET comp_nifcrypt"
echo "BUILDING TARGET comp_patcher"
time cmake --build . --config "$CONFIGURATION" --target comp_patcher
echo "BUILT TARGET comp_patcher"
echo "BUILDING TARGET comp_rehash"
time cmake --build . --config "$CONFIGURATION" --target comp_rehash
echo "BUILT TARGET comp_rehash"
echo "BUILDING TARGET comp_updater"
time cmake --build . --config "$CONFIGURATION" --target comp_updater
echo "BUILT TARGET comp_updater"
echo "BUILDING TARGET comp_updater_headless"
time cmake --build . --config "$CONFIGURATION" --target comp_updater_headless
echo "BUILT TARGET comp_updater_headless"
echo "BUILDING TARGET comp_lobby"
time cmake --build . --config "$CONFIGURATION" --target comp_lobby
echo "BUILT TARGET comp_lobby"
echo "BUILDING TARGET comp_world"
time cmake --build . --config "$CONFIGURATION" --target comp_world
echo "BUILT TARGET comp_world"
echo "BUILDING TARGET comp_channel"
time cmake --build . --config "$CONFIGURATION" --target comp_channel
echo "BUILT TARGET comp_channel"
echo "BUILDING TARGET all"
time cmake --build . --config "$CONFIGURATION" --target all
echo "BUILT TARGET all"
echo "BUILDING TARGET package"
time cmake --build . --config "$CONFIGURATION" --target package
echo "BUILT TARGET package"

echo "Copying build result to the cache"
rm -rf "${CACHE_DIR}/build-${PLATFORM}/"
mkdir "${CACHE_DIR}/build-${PLATFORM}"
cp comp_hack-*.{zip,msi} "${CACHE_DIR}/build-${PLATFORM}/"
ls -lh "${CACHE_DIR}/build-${PLATFORM}/"
