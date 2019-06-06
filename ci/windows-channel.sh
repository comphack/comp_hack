#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

# Run the cache again just in case it was not warmed.
source "ci/windows-warmcache.sh"

#
# Dependencies
#

# Check for existing build.
if [ $USE_DROPBOX ]; then
    dropbox_setup

    if dropbox-deployment -d -e "$DROPBOX_ENV" -u build -a . -s "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"; then
        exit 0
    fi
fi

cd "${ROOT_DIR}"
mkdir build
cd build

echo "Installing external dependencies"
unzip "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.zip" | ../ci/report-progress.sh
mv external* ../binaries
echo "Installed external dependencies"

echo "Installing OpenSSL"
cp "${CACHE_DIR}/OpenSSL-${OPENSSL_VERSION}-${PLATFORM}.msi" OpenSSL.msi
powershell -Command "Start-Process msiexec.exe -Wait -ArgumentList '/i OpenSSL.msi /l OpenSSL-install.log /qn'"
rm -f OpenSSL.msi OpenSSL-install.log
echo "Installed OpenSSL"

echo "Installing libcomp"

if [ $USE_DROPBOX ]; then
    dropbox_download "build/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
else
    cp "${CACHE_DIR}/libcomp-${PLATFORM}.zip" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
fi

unzip "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip" | ../ci/report-progress.sh
rm "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
ls -lh
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
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" \
    -DUSE_SYSTEM_OPENSSL=ON -G"$GENERATOR" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

if [ $USE_DROPBOX ]; then
    echo "Copying build result to the cache"
    mv comp_hack-*.zip "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
    dropbox_upload build "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
fi
