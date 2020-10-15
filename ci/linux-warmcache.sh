#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

cd "${CACHE_DIR}"

# Ninja
check_or_download "https://github.com/ninja-build/ninja/releases/download/v${LINUX_NINJA_VERSION}/ninja-linux.zip" "ninja-linux.zip"

# CMake
check_or_download "https://cmake.org/files/v${LINUX_CMAKE_VERSION}/cmake-${LINUX_CMAKE_FULL_VERSION}-Linux-x86_64.tar.gz" "cmake-${LINUX_CMAKE_FULL_VERSION}-Linux-x86_64.tar.gz"

# Clang
if [ "$PLATFORM" == "clang" ]; then
    check_or_download "http://llvm.org/releases/${LINUX_CLANG_VERSION}/clang+llvm-${LINUX_CLANG_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz" "clang+llvm-${LINUX_CLANG_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz"
fi

# External dependencies for Clang
if [ "$PLATFORM" == "clang" ]; then
    check_or_download "https://github.com/comphack/external/releases/download/${EXTERNAL_RELEASE}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2" "external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2"
fi

# External dependencies for GCC
if [ "$PLATFORM" == "gcc5" ]; then
    check_or_download "https://github.com/comphack/external/releases/download/${EXTERNAL_RELEASE}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2" "external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2"
fi

# Grab the build of libcomp if Dropbox isn't being used.
if [ ! $USE_DROPBOX ]; then
    if [ -f "libcomp-${PLATFORM}.tar.bz2" ]; then
        rm "libcomp-${PLATFORM}.tar.bz2"
    fi

    if [ "$PLATFORM" == "clang" ]; then
        wget -q "https://www.dropbox.com/s/y5edp8x6se75p4b/libcomp-${PLATFORM}.tar.bz2?dl=1" -O "libcomp-${PLATFORM}.tar.bz2"
    else
        wget -q "https://www.dropbox.com/s/inysaqxdzs7o8nx/libcomp-${PLATFORM}.tar.bz2?dl=1" -O "libcomp-${PLATFORM}.tar.bz2"
    fi
fi

# Just for debug to make sure the cache is setup right
echo "State of cache:"
ls -lh

# Change back to the root.
cd "${ROOT_DIR}"
