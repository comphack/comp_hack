#!/bin/bash

export ROOT_DIR="${TRAVIS_BUILD_DIR}"
export CACHE_DIR="${TRAVIS_BUILD_DIR}/cache"

export DOXYGEN_VERSION=1.8.14
export DOXYGEN_EXTERNAL_RELEASE=external-25

export EXTERNAL_RELEASE=external-25
export EXTERNAL_VERSION=0.1.1

export LINUX_NINJA_VERSION=1.7.1
export LINUX_CMAKE_VERSION=3.6
export LINUX_CMAKE_FULL_VERSION=3.6.1
export LINUX_CLANG_VERSION=3.8.0

export WINDOWS_QT_VERSION=5.12.3

if [ "$TRAVIS_OS_NAME" == "windows" ]; then
    export CONFIGURATION="RelWithDebInfo"

    if [ "$PLATFORM" != "win32" ]; then
        export CMAKE_PREFIX_PATH="C:/Qt/5.12.3/msvc2017_64"
        export GENERATOR="Visual Studio 15 2017 Win64"
        export MSPLATFORM="x64"
    else
        export CMAKE_PREFIX_PATH="C:/Qt/5.12.3/msvc2017"
        export GENERATOR="Visual Studio 15 2017"
        export MSPLATFORM="Win32"
    fi

    echo "Platform      = $PLATFORM"
    echo "MS Platform   = $MSPLATFORM"
    echo "Configuration = $CONFIGURATION"
    echo "Generator     = $GENERATOR"
fi
