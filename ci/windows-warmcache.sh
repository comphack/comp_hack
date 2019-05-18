#!/bin/bash
set -e

export ROOT_DIR=`pwd`
export CACHE_DIR=`pwd`/cache

cd $CACHE_DIR

# External dependencies for Windows
echo Downloading the external dependencies
if [ ! -f external-0.1.1-${PLATFORM}.zip ]; then
    curl -Lo external-0.1.1-${PLATFORM}.zip https://github.com/comphack/external/releases/download/external-25/external-0.1.1-${PLATFORM}.zip
fi

# For a re-download of the libcomp builds
echo Removing cached libcomp
if [ -f libcomp-4.1.2-${PLATFORM}.zip ]; then
    rm libcomp-4.1.2-${PLATFORM}.zip
fi
echo Downloading the external dependencies
curl -Lo libcomp-4.1.2-${PLATFORM}.zip https://github.com/comphack/libcomp/releases/download/v4.1.2/libcomp-4.1.2-${PLATFORM}.zip

# Qt 5.12.3
echo Downloading Qt
if [ ! -f Qt-5.12.3-${PLATFORM}.tar ]; then
    cd $ROOT_DIR

    ci/travis-install-qt-windows.sh
    ci/travis-cache-windows.sh save
fi
