#!/bin/bash
set -e

export ROOT_DIR=`pwd`
export CACHE_DIR=`pwd`/cache

cd $CACHE_DIR

# External dependencies for Windows
if [ ! -f external-0.1.1-${PLATFORM}.zip ]; then
    curl -L https://github.com/comphack/external/releases/download/external-25/external-0.1.1-${PLATFORM}.zip
fi

# For a re-download of the libcomp builds
rm -f libcomp-*-${PLATFORM}.tar.bz2
curl -L https://github.com/comphack/libcomp/releases/download/v4.1.2/libcomp-4.1.2-${PLATFORM}.zip

# Qt 5.12.3
if [ ! -f Qt-5.12.3-${PLATFORM}.tar ]; then
    cd $ROOT_DIR

    ci/travis-install-qt-windows.sh
    ci/travis-cache-windows.sh save
fi
