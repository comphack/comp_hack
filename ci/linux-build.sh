#!/bin/bash
set -ex

# Load the global settings.
export ROOT_DIR=$(pwd)
export CHECKSUM_DIR="${ROOT_DIR}/ci/checksums"

export CTEST_OUTPUT_ON_FAILURE=1
export BUILD_OPTIMIZED=ON
export COVERALLS_ENABLE=OFF

if [ "${COMPILER}" == "clang" ]; then
    export SINGLE_OBJGEN=ON
    export USE_COTIRE=OFF
else
    export SINGLE_OBJGEN=OFF
    export USE_COTIRE=ON
fi

function check_or_download {
    if ! sha1sum -c "${CHECKSUM_DIR}/$2.sha1" &> /dev/null; then
        rm -f "$2"
        echo "Downloading $2 from $1"
        curl -L -o "$2" "$1"
        if ! sha1sum -c "${CHECKSUM_DIR}/$2.sha1" &> /dev/null; then
            echo "Failed to download $2"
            exit 1
        fi
    fi
}

# Install packages
if [ "${INSTALL_TOOLS}" == "YES" ]; then
    sudo apt-get update -q
    sudo apt-get install libssl-dev docbook-xsl doxygen texlive-font-utils \
        xmlto libqt5webkit5-dev unzip -y

    if [ "${GENERATOR}" == "Ninja" ]; then
        sudo apt-get install ninja-build -y
    fi
fi

#
# Dependencies
#

# External dependencies
echo "Installing external dependencies"
check_or_download "https://github.com/comphack/external/releases/download/${EXTERNAL_RELEASE}/external-${PLATFORM}-${COMPILER}.tar.bz2" "external-${PLATFORM}-${COMPILER}.tar.bz2"
tar xf "external-${PLATFORM}-${COMPILER}.tar.bz2"
rm "external-${PLATFORM}-${COMPILER}.tar.bz2"
mv external* binaries
chmod +x binaries/ttvfs/bin/ttvfs_gen
echo "Installed external dependencies"

#
# Build
#

mkdir -p "${ROOT_DIR}/build"
cd "${ROOT_DIR}/build"

echo "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCOVERALLS="${COVERALLS_ENABLE}" -DBUILD_OPTIMIZED="${BUILD_OPTIMIZED}" \
    -DSINGLE_SOURCE_PACKETS=ON -DSINGLE_OBJGEN="${SINGLE_OBJGEN}" \
    -DUSE_COTIRE="${USE_COTIRE}" -DGENERATE_DOCUMENTATION=ON \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_RULE_MESSAGES:BOOL=OFF \
    -DUSE_PREBUILT_LIBCOMP=OFF -DIMPORT_CHANNEL=OFF -G "${GENERATOR}" ..

echo "Running build"
cmake --build . --target git-version
cmake --build .
cmake --build . --target install
# cmake --build . --target doc
# cmake --build . --target test
# cmake --build . --target coveralls
# cmake --build . --target package

# echo "Publish the documentation on the GitHub page"
# cp -R ../contrib/pages ../pages
# cp -R api ../pages/
# find "${ROOT_DIR}/pages" -type d

if [ "${COMPILER}" == "gcc" ]; then
    mkdir comp_translator
    cp bin/comp_translator comp_translator/
    cp -R install/schema comp_translator/
    cd comp_translator/
    tar cjf ../comp_translator.tar.bz2 .
    cd ../
fi
