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
tar xf "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2" | ../ci/report-progress.sh
mv external* ../binaries
chmod +x ../binaries/ttvfs/bin/ttvfs_gen
echo "Installed external dependencies"

echo "Installing libcomp"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_setup
    dropbox_download "build/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
else
    cp "${CACHE_DIR}/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
fi

tar xf "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" | ../ci/report-progress.sh
rm "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
mv libcomp* ../deps/libcomp
ls ../deps/libcomp

echo "Installed libcomp"

echo "Installing comp_channel"

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    dropbox_download "build/comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
else
    cp "${CACHE_DIR}/comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2" "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
fi

mkdir bin
cd bin
tar xf "../comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
rm "../comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.tar.bz2"
mv comp_hack-*/comp_channel* ./
rm -rf comp_hack-*/
ls -lh
cd ..

echo "Installed comp_channel"

#
# Build
#

cd "${ROOT_DIR}/build"

echo "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCOVERALLS="$COVERALLS_ENABLE" -DBUILD_OPTIMIZED=OFF \
    -DSINGLE_SOURCE_PACKETS=ON \ -DSINGLE_OBJGEN="${SINGLE_OBJGEN}" \
    -DUSE_COTIRE=ON -DGENERATE_DOCUMENTATION=ON \
    -DUSE_PREBUILT_LIBCOMP=ON -DIMPORT_CHANNEL=ON -G "${GENERATOR}" ..

echo "Running build"
cmake --build . --target git-version
cmake --build .
cmake --build . --target doc
cmake --build . --target guide
# cmake --build . --target test
# cmake --build . --target coveralls
# cmake --build . --target package

# echo "Copying build result to the cache"
# mv comp_hack-*.tar.bz2 "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.tar.gz"

# if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
#     dropbox_upload comp_hack "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.tar.gz"
# fi

echo "Publish the documentation on the GitHub page"
cp -R ../contrib/pages ../pages
cp -R api guide ../pages/
find -type d "${ROOT_DIR}/pages"
