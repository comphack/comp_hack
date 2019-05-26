#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

cd "${CACHE_DIR}"

# Ninja
if [ ! -f "ninja-linux.zip" ]; then
    wget -q "https://github.com/ninja-build/ninja/releases/download/v${LINUX_NINJA_VERSION}/ninja-linux.zip"
fi

# CMake
if [ ! -f "cmake-${LINUX_CMAKE_FULL_VERSION}-Linux-x86_64.tar.gz" ]; then
    wget -q "https://cmake.org/files/v${LINUX_CMAKE_VERSION}/cmake-${LINUX_CMAKE_FULL_VERSION}-Linux-x86_64.tar.gz"
fi

# Clang
if [ "$PLATFORM" == "clang" ]; then
    if [ ! -f "clang+llvm-${LINUX_CLANG_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz" ]; then
        wget -q "http://llvm.org/releases/${LINUX_CLANG_VERSION}/clang+llvm-${LINUX_CLANG_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz"
    fi
fi

# External dependencies for Clang
if [ "$PLATFORM" == "clang" ]; then
    if [ ! -f "external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2" ]; then
        wget -q "https://github.com/comphack/external/releases/download/${EXTERNAL_RELEASE}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2"
    fi
fi

# External dependencies for GCC
if [ "$PLATFORM" == "gcc5" ]; then
    if [ ! -f "external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2" ]; then
        wget -q "https://github.com/comphack/external/releases/download/${EXTERNAL_RELEASE}/external-${EXTERNAL_VERSION}-${PLATFORM}.tar.bz2"
    fi
fi

# Just for debug to make sure the cache is setup right
echo "State of cache:"
ls -lh
