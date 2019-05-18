#!/bin/bash
set -e

export ROOT_DIR=`pwd`
export CACHE_DIR=`pwd`/cache

cd $CACHE_DIR

# Qt 5.12.3
if [ ! -f Qt-5.12.3-${PLATFORM}.tar ]; then
    cd $ROOT_DIR

    ci/travis-install-qt-windows.sh
    ci/travis-cache-windows.sh save
fi
