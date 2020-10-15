#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

cd "${CACHE_DIR}"

# Doxygen for Windows (only warm from one job)
if [ "$PLATFORM" != "win32" ]; then
    check_or_download "https://github.com/comphack/external/releases/download/${DOXYGEN_EXTERNAL_RELEASE}/doxygen-${DOXYGEN_VERSION}.windows.x64.bin.zip" "doxygen.zip"
fi

# OpenSSL for Windows (only needed for the full build)
if [ "$PLATFORM" != "win32" ]; then
    check_or_download "https://github.com/comphack/external/releases/download/${DOXYGEN_EXTERNAL_RELEASE}/Win64OpenSSL-${OPENSSL_VERSION}.msi" "OpenSSL-${OPENSSL_VERSION}-${PLATFORM}.msi"
fi

# WiX Toolset
if [ "$PLATFORM" != "win32" ]; then
    check_or_download "https://github.com/wixtoolset/wix3/releases/download/wix3112rtm/wix311.exe" "wix311.exe"
fi

# External dependencies for Windows
check_or_download "https://github.com/comphack/external/releases/download/${EXTERNAL_RELEASE}/external-${EXTERNAL_VERSION}-${PLATFORM}.zip" "external-0.1.1-${PLATFORM}.zip"

# Grab Qt
check_or_download "https://github.com/comphack/external/releases/download/${QT_EXTERNAL_RELEASE}/qt-${WINDOWS_QT_VERSION}-${PLATFORM}.7z" "qt-${WINDOWS_QT_VERSION}-${PLATFORM}.7z"

# Grab the build of libcomp if Dropbox isn't being used.
if [ ! $USE_DROPBOX ]; then
    if [-f "libcomp-${PLATFORM}.zip" ]; then
        rm "libcomp-${PLATFORM}.zip"
    fi

    curl -Lo "libcomp-${PLATFORM}.zip" "https://www.dropbox.com/s/z8t1lkumu9zz9fn/libcomp-${PLATFORM}.zip?dl=1"
fi

# Just for debug to make sure the cache is setup right
echo "State of cache:"
ls -lh

# Change back to the root.
cd "${ROOT_DIR}"
