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

export DROPBOX_ENV=DROPBOX_OAUTH_BEARER_$(echo $TRAVIS_REPO_SLUG | LC_ALL=C sed -e "s/\//_/" | awk '{ print toupper($0) }')
export USE_DROPBOX=

set +x

if [[ ! -z "${!DROPBOX_ENV}" ]];then
    export USE_DROPBOX=1
fi

set -x

if [ $USE_DROPBOX ]; then
    echo "Dropbox will be used for a full branch build."
else
    echo "Dropbox will NOT be used for this build."
    echo "Nothing will be saved and the release libcomp will be used."
fi

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

if [ "$TRAVIS_OS_NAME" == "linux" ]; then
    # If we turn it off the build may go faster but it will be a huge file.
    export BUILD_OPTIMIZED=ON
    export COVERALLS_ENABLE=OFF
    export COVERALLS_SERVICE_NAME=travis-ci

    export PATH="/opt/ninja/bin:${PATH}"
    export PATH="/opt/cmake-${LINUX_CMAKE_FULL_VERSION}-Linux-x86_64/bin:${PATH}"
    export LD_LIBRARY_PATH="/opt/cmake-${LINUX_CMAKE_FULL_VERSION}-Linux-x86_64/lib"

    if [ "$PLATFORM" == "clang" ]; then
        export COVERALLS_ENABLE=OFF
    else
        export COVERALLS_ENABLE=ON
    fi

    if [ "$PLATFORM" == "clang" ]; then
        export PATH="/opt/clang+llvm-${LINUX_CLANG_VERSION}-x86_64-linux-gnu-ubuntu-14.04/bin:${PATH}"
        export LD_LIBRARY_PATH="/opt/clang+llvm-${LINUX_CLANG_VERSION}-x86_64-linux-gnu-ubuntu-14.04/lib:${LD_LIBRARY_PATH}"
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
fi

function dropbox_setup {
    gem install "${ROOT_DIR}/ci/dropbox-deployment-1.0.0.gem"
}

function dropbox_download {
    dropbox-deployment -d -e "$DROPBOX_ENV" -f "$1" -a "$2"
}

function dropbox_upload {
    dropbox-deployment -d -e "$DROPBOX_ENV" -u "$1" -a "$2" --max-files 6 --max-days 5
}

function dropbox_upload_rel {
    dropbox-deployment -d -e "$DROPBOX_ENV" -u comp_hack -a "$1" --max-files 2 --max-days 5
}
