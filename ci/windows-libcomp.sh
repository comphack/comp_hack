#!/bin/bash
set -e

export ROOT_DIR=`pwd`
export CACHE_DIR=`pwd`/cache

export CONFIGURATION="RelWithDebInfo"

if [ "$PLATFORM" != "win32" ]; then
    export CMAKE_PREFIX_PATH="C:/Qt/5.12.3/msvc2017_64"
    export GENERATOR="Visual Studio 15 2017 Win64"
    export MSPLATFORM="x64"
else
    export CMAKE_PREFIX_PATH="C:/Qt/5.12.3/msvc2017"
    export GENERATOR="Visual Studio 15 2017"
    export MSPLATFORM="Win32"
fi

echo "Platform      = $PLATFORM"
echo "MS Platform   = $MSPLATFORM"
echo "Configuration = $CONFIGURATION"
echo "Generator     = $GENERATOR"

#
# Dependencies
#

cd "${ROOT_DIR}/libcomp"
mkdir build
cd build

echo Installing external dependencies
echo "${ROOT_DIR}/cache/external-0.1.1-${PLATFORM}.zip"
ls -lh "${ROOT_DIR}/cache/"
unzip "${ROOT_DIR}/cache/external-0.1.1-${PLATFORM}.zip"
# unzip "${ROOT_DIR}/cache/external-0.1.1-${PLATFORM}.zip" | ../ci/report-progress.sh
mv external* ../binaries
echo Installed external dependencies

echo Installing Doxygen
mkdir doxygen
cd doxygen
unzip "${ROOT_DIR}/cache/doxygen.zip" | ../../ci/report-progress.sh
cd ..
export PATH="${ROOT_DIR}/libcomp/build/doxygen;${PATH}"
echo Installed Doxygen

#
# Build
#

cd "${ROOT_DIR}/libcomp/build"

echo Running cmake
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DGENERATE_DOCUMENTATION=ON -DWINDOWS_SERVICE=ON \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..

echo Running build
cmake --build . --config $CONFIGURATION --target package

echo Copying package to cache for next stage
cp libcomp-*.zip "${ROOT_DIR}/cache/libcomp-${PLATFORM}.zip"
