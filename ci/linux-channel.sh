#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

#
# Dependencies
#

# Check for existing build.
if [ $USE_DROPBOX ]; then
    dropbox_setup

    if dropbox-deployment -d -e "$DROPBOX_ENV" -u build -a . -s "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"; then
        exit 0
    fi
fi

cd "${ROOT_DIR}"
mkdir build
cd build

echo "Installing external dependencies"
tar xf "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2"
mv external* ../binaries
chmod +x ../binaries/ttvfs/bin/ttvfs_gen
echo "Installed external dependencies"

echo "Installing libcomp"

if [ $USE_DROPBOX ]; then
    dropbox_download "build/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
else
    cp "${CACHE_DIR}/libcomp-${PLATFORM}.tar.bz2" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
fi

tar xf "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
rm "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
ls -lh
mv libcomp* ../deps/libcomp
ls ../deps/libcomp

echo "Installed libcomp"

#
# Build
#

cd "${ROOT_DIR}/build"

echo "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCOVERALLS="$COVERALLS_ENABLE" -DBUILD_OPTIMIZED="$BUILD_OPTIMIZED" \
    -DSINGLE_SOURCE_PACKETS=ON \ -DSINGLE_OBJGEN="${SINGLE_OBJGEN}" \
    -DUSE_COTIRE=ON -DGENERATE_DOCUMENTATION=OFF \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_RULE_MESSAGES:BOOL=OFF \
    -DUSE_PREBUILT_LIBCOMP=ON -DCHANNEL_ONLY=ON -G "${GENERATOR}" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

if [ "$CMAKE_GENERATOR" != "Ninja" ]; then
    if [ $USE_DROPBOX ]; then
        echo "Copying build result to the cache"
        mv comp_hack-*.tar.bz2 "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
        dropbox_upload build "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
    fi
fi
