#!/bin/bash
set -e

export ROOT_DIR=`pwd`
export CACHE_DIR=`pwd`/cache

export COVERALLS_ENABLE=OFF
export COVERALLS_SERVICE_NAME=travis-ci

export PATH="/opt/ninja/bin:${PATH}"
export PATH="/opt/cmake-3.6.1-Linux-x86_64/bin:${PATH}"
export LD_LIBRARY_PATH="/opt/cmake-3.6.1-Linux-x86_64/lib"

if [ "$CXX" == "clang++" ]; then
    export COVERALLS_ENABLE=OFF
else
    export COVERALLS_ENABLE=ON
fi

if [ "$CXX" == "clang++" ]; then
    export PATH="/opt/clang+llvm-3.8.0-x86_64-linux-gnu-ubuntu-14.04/bin:${PATH}"
    export LD_LIBRARY_PATH="/opt/clang+llvm-3.8.0-x86_64-linux-gnu-ubuntu-14.04/lib:${LD_LIBRARY_PATH}"
fi

if [ "$CXX" == "clang++" ]; then
    export SINGLE_OBJGEN=ON
else
    export SINGLE_OBJGEN=OFF
fi

export CC="${COMPILER_CC}"
export CXX="${COMPILER_CXX}"
export GENERATOR="${CMAKE_GENERATOR}"
export CTEST_OUTPUT_ON_FAILURE=1

# Stop coverity here.
if [ "${COVERITY_SCAN_BRANCH}" == 1 ]; then
    exit 0
fi

#
# Build
#
cd ${ROOT_DIR}

mkdir -p build
cd build
cmake -DCOVERALLS=$COVERALLS_ENABLE -DUSE_PREBUILT_LIBCOMP=ON \
    -DBUILD_OPTIMIZED=OFF -DSINGLE_SOURCE_PACKETS=ON \
    -DSINGLE_OBJGEN=${SINGLE_OBJGEN} -DUSE_COTIRE=ON -G "${GENERATOR}" ..
cmake --build . --target git-version
cmake --build .
cmake --build . --target doc
cmake --build . --target guide
# cmake --build . --target test

# if [ "$CXX" != "clang++" ]; the
#     cmake --build . --target coveralls
# fi
