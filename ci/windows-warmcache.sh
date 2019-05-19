#!/bin/bash
set -e

export ROOT_DIR="$(pwd)"
export CACHE_DIR="$(pwd)/cache"

# Load the global settings.
source "${ROOT_DIR}/ci/global.sh"

cd "${CACHE_DIR}"

# Doxygen for Windows (only warm from one job)
if [ "$PLATFORM" != "win32" ]; then
    echo "Downloading Doxygen"
    if [ ! -f doxygen.zip ]; then
        curl -Lo doxygen.zip "https://github.com/comphack/external/releases/download/${DOXYGEN_EXTERNAL_RELEASE}/doxygen-${DOXYGEN_VERSION}.windows.x64.bin.zip"
    fi
fi

# External dependencies for Windows
echo "Downloading the external dependencies"
if [ ! -f "external-0.1.1-${PLATFORM}.zip" ]; then
    curl -Lo "external-0.1.1-${PLATFORM}.zip" "https://github.com/comphack/external/releases/download/${EXTERNAL_RELEASE}/external-${EXTERNAL_VERSION}-${PLATFORM}.zip"
fi

# For a re-download of the libcomp builds
# echo "Removing cached libcomp"
# if [ -f libcomp-4.1.2-${PLATFORM}.zip ]; then
#     rm libcomp-4.1.2-${PLATFORM}.zip
# fi
# echo Downloading the external dependencies
# curl -Lo libcomp-4.1.2-${PLATFORM}.zip https://github.com/comphack/libcomp/releases/download/v4.1.2/libcomp-4.1.2-${PLATFORM}.zip

# Grab Qt
echo "Downloading Qt"
if [ ! -f "Qt-${WINDOWS_QT_VERSION}-${PLATFORM}.tar" ]; then
    cd "${ROOT_DIR}"

    ci/travis-install-qt-windows.sh
    ci/travis-cache-windows.sh save
fi

# Just for debug to make sure the cache is setup right
echo "State of cache:"
ls -lh
