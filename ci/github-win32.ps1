$ErrorActionPreference = "Stop"

$FILES_DIR = "C:\actions-runner\files"
$CMAKE_PREFIX_PATH = "C:\Qt\Qt5.12.3\5.12.3\msvc2017"

$PLATFORM = "win32"
$CONFIGURATION = "RelWithDebInfo"
$GENERATOR = "Visual Studio 15 2017"
$MSPLATFORM = "Win32"

$ROOT_DIR = $(Get-Location).Path

#
# Dependencies
#

# Depending on the runner we might need to also download/install:
# * CMake
# * Qt
# * Doxygen
# * WiX
# * OpenSSL

echo "Installing external dependencies"
Expand-Archive -Path $FILES_DIR\external-0.1.1-win32.zip -Destination .
Move-Item -Path external-0.1.1-win32 -Destination binaries

#
# Main Build
#

echo "Creating build directory"
New-Item -ItemType directory -Path build | Out-Null
Set-Location -Path build | Out-Null

echo "Running cmake"
cmake -DGENERATE_DOCUMENTATION=OFF -DUPDATER_ONLY=ON `
    -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}" `
    -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}\build\install" `
    -DCMAKE_CUSTOM_CONFIGURATION_TYPES="$CONFIGURATION" `
    -G"$GENERATOR" ..

if ($LastExitCode -ne 0) {
    throw
}

echo "Running build"
cmake --build . --config "$CONFIGURATION" --target package

if ($LastExitCode -ne 0) {
    throw
}

echo "Collecting artifacts"
New-Item -ItemType directory -Path artifacts | Out-Null
Move-Item -Path comp_hack-*.zip -Destination artifacts
