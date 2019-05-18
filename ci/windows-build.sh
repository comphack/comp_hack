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
# echo Installing WiX
# cinst wixtoolset
# echo Installed WiX

cd $ROOT_DIR
mkdir build
cd build

echo Downloading external dependencies
if [ "$PLATFORM" != "win32" ]; the
    curl -vLo external.zip https://github.com/comphack/external/releases/download/external-25/external-0.1.1-win64.zip
else
    curl -vLo external.zip https://github.com/comphack/external/releases/download/external-25/external-0.1.1-win32.zip
fi

unzip external.zip
rm external.zip
mv external* ../binaries
echo Installed external dependencies

echo Downloading libcomp
curl -vLo libcomp.zip https://github.com/comphack/libcomp/releases/download/v4.1.2/libcomp-4.1.2-win64.zip
zunip libcomp.zip
rm libcomp.zip
mv libcomp* ../deps/libcomp
ls ../deps/libcomp
echo Installed libcomp

# Restore the cache
echo Restoring cache
ci/travis-cache-windows.sh restore
echo Restored cache

#
# Build
#

echo Running cmake
cmake -DUSE_PREBUILT_LIBCOMP=ON -DGENERATE_DOCUMENTATION=ON \
    -DWINDOWS_SERVICE=ON \
    -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" -G"$GENERATOR" ..
echo Running build
cmake --build . --config $CONFIGURATION --target package
