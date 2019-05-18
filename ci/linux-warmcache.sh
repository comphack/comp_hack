#!/bin/bash
set -e

export ROOT_DIR=`pwd`
export CACHE_DIR=`pwd`/cache

# Ninja 1.7.1
if [ ! -f ninja-linux.zip ]; then
    wget -q https://github.com/ninja-build/ninja/releases/download/v1.7.1/ninja-linux.zip
fi

# CMake 3.6.1
if [ ! -f cmake-3.6.1-Linux-x86_64.tar.gz ]; then
    wget -q https://cmake.org/files/v3.6/cmake-3.6.1-Linux-x86_64.tar.gz
fi

# Clang 3.8.0
if [ ! -f clang+llvm-3.8.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz ]; then
    wget -q http://llvm.org/releases/3.8.0/clang+llvm-3.8.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz
fi

# External dependencies for Clang
if [ ! -f external-0.1.1-clang.tar.bz2 ]; then
    wget -q https://github.com/comphack/external/releases/download/external-25/external-0.1.1-clang.tar.bz2
fi

# External dependencies for GCC
if [ ! -f external-0.1.1-gcc5.tar.bz2 ]; then
    wget -q https://github.com/comphack/external/releases/download/external-25/external-0.1.1-gcc5.tar.bz2
fi

# For a re-download of the libcomp builds (for Clang and GCC)
rm -f libcomp-*-clang.tar.bz2
wget -q https://github.com/comphack/libcomp/releases/download/v4.1.2/libcomp-4.1.2-clang.tar.bz2
rm -f libcomp-*-gcc5.tar.bz2
wget -q https://github.com/comphack/libcomp/releases/download/v4.1.2/libcomp-4.1.2-gcc5.tar.bz2

# Just for debug to make sure the cache is setup right
ls -lh
