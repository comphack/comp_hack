#!/bin/bash
set -e

export ROOT_DIR=`pwd`
export CACHE_DIR=`pwd`/cache

# Install packages
sudo apt-get update -q
sudo apt-get install libssl-dev docbook-xsl doxygen texlive-font-utils xmlto \
    libsqlite3-dev sqlite3 libuv-dev libqt5webkit5-dev unzip \
    ruby ruby-rspec-core ruby-mechanize ruby-json -y

# Install Ninja
sudo mkdir -p /opt/ninja/bin
cd /opt/ninja/bin
sudo unzip $CACHE_DIR/ninja-linux.zip
sudo chmod 755 ninja

# Install CMake
cd /opt
sudo tar xf $CACHE_DIR/cmake-3.6.1-Linux-x86_64.tar.gz

# Install external dependencies
cd ${ROOT_DIR}

if [ "$CXX" == "clang++" ]; then
    tar xf $CACHE_DIR/external-0.1.1-clang.tar.bz2;
fi

if [ "$CXX" != "clang++" ]; then
    tar xf $CACHE_DIR/external-0.1.1-gcc5.tar.bz2
fi

mv external-* binaries
chmod +x binaries/ttvfs/bin/ttvfs_gen

# Install libcomp build
cd ${ROOT_DIR}
if [ "$CXX" == "clang++" ]; then
    tar xf $CACHE_DIR/libcomp-*-clang.tar.bz2
fi

if [ "$CXX" != "clang++" ]; then
    tar xf $CACHE_DIR/libcomp-*-gcc5.tar.bz2
fi

mv libcomp-* deps/libcomp

# Install Clang
cd /opt

if [ "$CXX" == "clang++" ]; then
    sudo tar xf $CACHE_DIR/clang+llvm-3.8.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz
fi
