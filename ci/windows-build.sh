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

# WiX is broke because it tried to install .NET 3.5 and failed
# After that it tries anyway and hangs...
echo Installing WiX
# cinst wixtoolset --ignore-dependencies | ci/report-progress.sh
curl -Lo wix.exe https://github.com/wixtoolset/wix3/releases/download/wix3111rtm/wix311.exe
cmd wix.exe /install /quiet /norestart
rm wix.exe
echo Installed WiX

cd $ROOT_DIR
mkdir build
cd build

echo Installing external dependencies
unzip ../cache/external-0.1.1-${PLATFORM}.zip | ../ci/report-progress.sh
mv external* ../binaries
echo Installed external dependencies

echo Installing libcomp
unzip ../cache/libcomp-4.1.2-${PLATFORM}.zip | ../ci/report-progress.sh
mv libcomp* ../deps/libcomp
ls ../deps/libcomp
echo Installed libcomp

# Restore the cache
echo Restoring cache
cd "${ROOT_DIR}"
ci/travis-cache-windows.sh restore
echo Restored cache

#
# Build
#

cd "${ROOT_DIR}/build"

echo Running cmake
cmake -DUSE_PREBUILT_LIBCOMP=ON -DGENERATE_DOCUMENTATION=ON \
    -DWINDOWS_SERVICE=ON \
    -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..
echo Running build
cmake --build . --config $CONFIGURATION --target package
