#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

# Skip if not using Dropbox (we assume the build uses the release libcomp).
if [ ! $USE_DROPBOX ]; then
    echo "Not building libcomp because Dropbox is not setup for this build."
    exit 0
fi

#
# Dependencies
#

# Check for existing build.
if [ $USE_DROPBOX ]; then
    dropbox_setup

    if dropbox-deployment -d -e "$DROPBOX_ENV" -u build -a . -s "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"; then
        exit 0
    fi
fi

cd "${ROOT_DIR}/libcomp"
mkdir build
cd build

echo "Installing external dependencies"
tar xf "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2"
mv external* ../binaries
chmod +x ../binaries/ttvfs/bin/ttvfs_gen
echo "Installed external dependencies"

#
# Build
#

cd "${ROOT_DIR}/libcomp/build"

echo "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCOVERALLS="$COVERALLS_ENABLE" -DBUILD_OPTIMIZED="$BUILD_OPTIMIZED" \
    -DSINGLE_SOURCE_PACKETS=ON \ -DSINGLE_OBJGEN="${SINGLE_OBJGEN}" \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_RULE_MESSAGES:BOOL=OFF \
    -DUSE_COTIRE=ON -DGENERATE_DOCUMENTATION=ON -G "${GENERATOR}" ..

echo "Running build"
cmake --build . --target package

if [ "$CMAKE_GENERATOR" != "Ninja" ]; then
    if [ $USE_DROPBOX ]; then
        echo "Copying package to cache for next stage"
        mv libcomp-*.tar.bz2 "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
        dropbox_upload build "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
    fi
fi
