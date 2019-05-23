#!/bin/bash
set -e

# Load the global settings.
source "ci/global.sh"

#
# Prepare build results to BinTray
#

echo "Patching bintray.json"
cat "${ROOT_DIR}/ci/bintray.json.in" | sed \
    -e "s/@TRAVIS_BUILD_NUMBER@/$TRAVIS_BUILD_NUMBER/g" \
    -e "s/@TRAVIS_BRANCH@/$TRAVIS_BRANCH/g" \
    -e "s/@TRAVIS_COMMIT@/$TRAVIS_COMMIT/g" > "${ROOT_DIR}/ci/bintray.json"

echo "Contents of the cache directory:"
ls -lh ${CACHE_DIR}

echo "Copying files to deploy"
mkdir deploy

# Copy the Windows 64-bit build into the deploy folder.
cp ${CACHE_DIR}/comp_hack-*.{zip,msi} deploy/
