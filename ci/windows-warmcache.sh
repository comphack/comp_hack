#!/bin/bash
set -e

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

# Just for debug to make sure the cache is setup right
echo "State of cache:"
ls -lh

mkdir deploy
cp "${CACHE_DIR}/Qt-${WINDOWS_QT_VERSION}-${PLATFORM}.tar" deploy/
echo "Running deployment..."
dropbox_setup
echo "TRAVIS_REPO_SLUG=$TRAVIS_REPO_SLUG"
echo "DROPBOX_ENV=$DROPBOX_ENV"
dropbox_upload comp_hack "deploy/Qt-${WINDOWS_QT_VERSION}-${PLATFORM}.tar"
dropbox_download "comp_hack/Qt-${WINDOWS_QT_VERSION}-${PLATFORM}.tar" bob.tar
echo "Finished running deployment."
ls -lh
