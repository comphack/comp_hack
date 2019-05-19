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

# Install .NET 3.5 from PowerShell first...
# https://travis-ci.community/t/unable-to-install-wix-toolset/1071/3
echo Installing .NET 3.5
powershell Install-WindowsFeature Net-Framework-Core
echo Installed .NET 3.5

echo Installing WiX
cinst -y wixtoolset | ci/report-progress.sh
echo Installed WiX

cd $ROOT_DIR
mkdir build
cd build

echo Installing external dependencies
unzip ../cache/external-0.1.1-${PLATFORM}.zip | ../ci/report-progress.sh
mv external* ../binaries
echo Installed external dependencies

echo Installing libcomp
unzip ../cache/libcomp-${PLATFORM}.zip | ../ci/report-progress.sh
mv libcomp* ../deps/libcomp
ls ../deps/libcomp
echo Installed libcomp

# Restore the cache (this mostly just handles Qt)
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
