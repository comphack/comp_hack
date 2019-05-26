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

    if dropbox-deployment -d -e "$DROPBOX_ENV" -u build -a . -s "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"; then
        exit 0
    fi
fi

cd "${ROOT_DIR}/libcomp"
mkdir build
cd build

echo "Installing external dependencies"
tar xf "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2" | "${ROOT_DIR}/ci/report-progress.sh"
mv external* ../binaries
chmod +x ../binaries/ttvfs/bin/ttvfs_gen
echo "Installed external dependencies"

#
# Build
#

cd "${ROOT_DIR}/libcomp/build"

echo "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCOVERALLS="$COVERALLS_ENABLE" -DBUILD_OPTIMIZED=OFF \
    -DSINGLE_SOURCE_PACKETS=ON \ -DSINGLE_OBJGEN="${SINGLE_OBJGEN}" \
    -DUSE_COTIRE=ON -DGENERATE_DOCUMENTATION=ON -G "${GENERATOR}" ..

echo "Running build"
cmake --build . --target package

echo "Copying package to cache for next stage"

mv libcomp-*.tar.bz2 "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_upload build "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
else
    cp "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "${CACHE_DIR}/"
fi
