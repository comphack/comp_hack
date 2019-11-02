#!/bin/bash
set -ex

# Load the global settings.
source "ci/global.sh"

# Run the cache again just in case it was not warmed.
source "ci/windows-warmcache.sh"

#
# Dependencies
#

cd "${ROOT_DIR}"

# Install .NET 3.5 from PowerShell first...
# https://travis-ci.community/t/unable-to-install-wix-toolset/1071/3
echo "Installing .NET 3.5"
powershell Install-WindowsFeature Net-Framework-Core
echo "Installed .NET 3.5"

echo "Installing WiX"
cinst -y wixtoolset
set PATH="C:/Program Files (x86)/WiX Toolset v3.11/bin:${PATH}"
echo "Installed WiX"

cd "${ROOT_DIR}"
mkdir build
cd build

echo "Installing external dependencies"
unzip "${CACHE_DIR}/external-${EXTERNAL_VERSION}-${PLATFORM}.zip"
mv external* ../binaries
echo "Installed external dependencies"

echo "Installing OpenSSL"
cp "${CACHE_DIR}/OpenSSL-${OPENSSL_VERSION}-${PLATFORM}.msi" OpenSSL.msi
powershell -Command "Start-Process msiexec.exe -Wait -ArgumentList '/i OpenSSL.msi /l OpenSSL-install.log /qn'"
rm -f OpenSSL.msi OpenSSL-install.log
echo "Installed OpenSSL"

echo "Installing libcomp"

if [ $USE_DROPBOX ]; then
    dropbox_setup
    dropbox_download "build/libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
else
    cp "${CACHE_DIR}/libcomp-${PLATFORM}.zip" "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
fi

unzip "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
rm "libcomp-${TRAVIS_COMMIT}-${PLATFORM}.zip"
mv libcomp* ../deps/libcomp
ls ../deps/libcomp

echo "Installed libcomp"

echo "Installing comp_channel"

mkdir bin
cd bin

if [ $USE_DROPBOX ]; then
    dropbox_download "build/comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip" "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
    unzip "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
    rm "comp_channel-${TRAVIS_COMMIT}-${PLATFORM}.zip"
    mv comp_hack-*/comp_channel* ./
    rm -rf comp_hack-*/
else
    # Create a blank file to add to the installer.
    echo "blank" > comp_channel.exe
    echo "blank" > comp_channel.pdb
fi

ls -lh
cd ..

echo "Installed comp_channel"

# Restore the cache (this mostly just handles Qt)
echo "Restoring cache"
cd "${ROOT_DIR}"
ci/travis-cache-windows.sh restore
echo "Restored cache"

#
# Build
#

cd "${ROOT_DIR}/build"

echo "Running cmake"
cmake -DUSE_PREBUILT_LIBCOMP=ON -DGENERATE_DOCUMENTATION=OFF -DIMPORT_CHANNEL=ON \
    -DWINDOWS_SERVICE=ON -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" \
    -DCPACK_WIX_ROOT="C:/Program Files (x86)/WiX Toolset v3.11" \
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" \
    -DUSE_SYSTEM_OPENSSL=ON -G"$GENERATOR" ..

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

echo "Cleaning build"
cmake --build . --config "$CONFIGURATION" --target clean

# Get the original filename for the zip and msi for deploy after upload.
ORIGINAL_ZIP=`ls comp_hack-*.zip`
ORIGINAL_MSI=`ls comp_hack-*.msi`

# Move the files in case we upload to Dropbox.
mv comp_hack-*.zip "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip"
mv comp_hack-*.msi "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.msi"

if [ $USE_DROPBOX ]; then
    echo "Uploading build result to Dropbox"
    dropbox_upload_rel "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip"
    dropbox_upload_rel "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.msi"
fi

# Move the files back and into the deploy folder.
echo "Moving build result for deployment"
mkdir "${ROOT_DIR}/deploy"
mv "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.zip" "${ORIGINAL_ZIP}"
mv "comp_hack-${TRAVIS_COMMIT}-${PLATFORM}.msi" "${ORIGINAL_MSI}"
mv comp_hack-*.{zip,msi} "${ROOT_DIR}/deploy/"

# Change back to the root.
cd "${ROOT_DIR}"
