#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

cd "${CACHE_DIR}"

# Doxygen for Windows (only warm from one job)
if [ "$PLATFORM" != "win32" ]; then
    echo "Downloading Doxygen"
    if [ ! -f doxygen.zip ]; then
        curl -Lo doxygen.zip "https://github.com/comphack/external/releases/download/${DOXYGEN_EXTERNAL_RELEASE}/doxygen-${DOXYGEN_VERSION}.windows.x64.bin.zip"
    fi
fi

# OpenSSL for Windows (only needed for the full build)
if [ "$PLATFORM" != "win32" ]; then
    echo "Downloading OpenSSL"
    if [ ! -f "OpenSSL-${OPENSSL_VERSION}-${PLATFORM}.msi" ]; then
        curl -Lo "OpenSSL-${OPENSSL_VERSION}-${PLATFORM}.msi" "https://github.com/comphack/external/releases/download/${DOXYGEN_EXTERNAL_RELEASE}/Win64OpenSSL-${OPENSSL_VERSION}.msi"
    fi
fi

# External dependencies for Windows
echo "Downloading the external dependencies"
if [ ! -f "external-0.1.1-${PLATFORM}.zip" ]; then
    curl -Lo "external-0.1.1-${PLATFORM}.zip" "https://github.com/comphack/external/releases/download/${EXTERNAL_RELEASE}/external-${EXTERNAL_VERSION}-${PLATFORM}.zip"
fi

# Grab Qt
echo "Downloading Qt"
if [ ! -f "Qt-${WINDOWS_QT_VERSION}-${PLATFORM}.tar" ]; then
    cd "${ROOT_DIR}"

    ci/travis-install-qt-windows.sh
    ci/travis-cache-windows.sh save
fi

# Grab the build of libcomp if Dropbox isn't being used.
if [ ! $USE_DROPBOX ]; then
    if [ ! -f "libcomp-${PLATFORM}.zip" ]; then
        curl -Lo "libcomp-${PLATFORM}.zip" "https://www.dropbox.com/s/z8t1lkumu9zz9fn/libcomp-${PLATFORM}.zip?dl=1"
    fi
fi

# Just for debug to make sure the cache is setup right
echo "State of cache:"
ls -lh

# Change back to the root.
cd "${ROOT_DIR}"
