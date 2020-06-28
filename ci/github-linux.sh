#!/bin/bash
set -ex

export ROOT_DIR=$(pwd)

export PLATFORM="gcc"
export CMAKE_GENERATOR="Ninja"

export BUILD_OPTIMIZED=ON
    export COVERALLS_ENABLE=OFF

if [ "$PLATFORM" == "clang" ]; then
    export CC=/opt/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04/bin/clang
    export CXX=/opt/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04/bin/clang++
fi

if [ "$PLATFORM" == "clang" ]; then
    export SINGLE_OBJGEN=ON
else
    export SINGLE_OBJGEN=OFF
fi

export CC="${COMPILER_CC}"
export CXX="${COMPILER_CXX}"
export GENERATOR="${CMAKE_GENERATOR}"
export CTEST_OUTPUT_ON_FAILURE=1

export FILES_DIR="/home/omega/actions-runner/files"

echo "Installing external dependencies"
tar xf "${FILES_DIR}/external-0.1.1-ubuntu16.04-${PLATFORM}.tar.gz"
mv external* binaries
chmod +x binaries/ttvfs/bin/ttvfs_gen

echo "Creating build directory"
mkdir build
cd build

echo "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCOVERALLS="$COVERALLS_ENABLE" -DBUILD_OPTIMIZED="$BUILD_OPTIMIZED" \
    -DSINGLE_SOURCE_PACKETS=ON -DSINGLE_OBJGEN="${SINGLE_OBJGEN}" \
    -DUSE_COTIRE=ON -DGENERATE_DOCUMENTATION=ON \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_RULE_MESSAGES:BOOL=OFF \
    -G "${GENERATOR}" ..

echo "Running build"
cmake --build . --target git-version
cmake --build .
cmake --build . --target doc
# cmake --build . --target test
# cmake --build . --target coveralls
# cmake --build . --target package

echo "Publish the documentation on the GitHub page"
cp -R ../contrib/pages ../pages
cp -R api ../pages/
