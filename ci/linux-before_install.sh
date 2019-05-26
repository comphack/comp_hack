#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

# Run the cache again just in case it was not warmed.
source "ci/linux-warmcache.sh"

# Install packages
sudo apt-get update -q
sudo apt-get install libssl-dev docbook-xsl doxygen texlive-font-utils \
    xmlto libqt5webkit5-dev unzip -y

# Install Ninja
echo "Installing Ninja"
sudo mkdir -p /opt/ninja/bin
cd /opt/ninja/bin
sudo unzip "${CACHE_DIR}/ninja-linux.zip"
sudo chmod 755 ninja
echo "Installed Ninja"

# Install CMake
echo "Installing CMake"
cd /opt
sudo tar xf "${CACHE_DIR}/cmake-${LINUX_CMAKE_FULL_VERSION}-Linux-x86_64.tar.gz"
echo "Installed CMake"

# Install Clang
cd /opt

if [ "$PLATFORM" == "clang" ]; then
    echo "Installing Clang"
    sudo tar xf "${CACHE_DIR}/clang+llvm-${LINUX_CLANG_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz"
    echo "Installed Clang"
fi
