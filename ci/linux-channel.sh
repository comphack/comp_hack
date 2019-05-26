#!/bin/bash
set -e

# Load the global settings.
source "ci/global.sh"

#
# Dependencies
#

# Check for existing build.
if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_setup

    if dropbox-deployment -d -e "$DROPBOX_ENV" -u build -a . -s "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"; then
        exit 0
    fi
fi

cd "${ROOT_DIR}"
mkdir build
cd build

echo "Installing external dependencies"
tar xf "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2" | ../ci/report-progress.sh
mv external* ../binaries
chmod +x ../binaries/ttvfs/bin/ttvfs_gen
echo "Installed external dependencies"

echo "Installing libcomp"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_download "build/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
else
    cp "${CACHE_DIR}/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
fi

tar xf "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" | ../ci/report-progress.sh
rm "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
mv libcomp* ../deps/libcomp
ls ../deps/libcomp

echo "Installed libcomp"

#
# Build
#

cd "${ROOT_DIR}/build"

echo "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCOVERALLS="$COVERALLS_ENABLE" -DBUILD_OPTIMIZED=OFF \
    -DSINGLE_SOURCE_PACKETS=ON \ -DSINGLE_OBJGEN="${SINGLE_OBJGEN}" \
    -DUSE_COTIRE=ON -DGENERATE_DOCUMENTATION=OFF \
    -DUSE_PREBUILT_LIBCOMP=ON -DCHANNEL_ONLY=ON -G "${GENERATOR}" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

echo "Copying build result to the cache"

mv comp_hack-*.tar.bz2 "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"

if [ "$CMAKE_GENERATOR" != "Ninja" ]; then
    if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
        dropbox_upload build "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
    else
        cp "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "${CACHE_DIR}/"
    fi
fi
