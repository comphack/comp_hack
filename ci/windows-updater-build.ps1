$ErrorActionPreference = "Stop"

$ROOT_DIR = $(Get-Location).Path

Write-Output "Platform      = ${env:PLATFORM}"
Write-Output "MS Platform   = ${env:MSPLATFORM}"
Write-Output "Configuration = ${env:CONFIGURATION}"
Write-Output "Generator     = ${env:GENERATOR}"

Write-Output "Installing external dependencies"
Invoke-WebRequest "https://github.com/comphack/external/releases/download/${env:EXTERNAL_RELEASE}/external-${env:PLATFORM}-${env:COMPILER}.zip" -OutFile "external-${env:PLATFORM}-${env:COMPILER}.zip"
7z x "external-${env:PLATFORM}-${env:COMPILER}.zip"
Remove-Item "external-${env:PLATFORM}-${env:COMPILER}.zip"
Move-Item external* binaries
Write-Output "Installed external dependencies"

Write-Output "Installing Qt"
New-Item -ItemType directory -Path "${env:QT_EXTRACT_DIR}" | Out-Null
Set-Location "${env:QT_EXTRACT_DIR}"
Invoke-WebRequest "${env:QT_URL}" -OutFile "Qt.7z"
7z x "Qt.7z"
Remove-Item Qt.7z
Set-Location "${ROOT_DIR}"
Write-Output "Installed Qt"

New-Item -ItemType directory -Path build | Out-Null
Set-Location build

Write-Output "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" -DGENERATE_DOCUMENTATION=OFF -DUPDATER_ONLY=ON -DCMAKE_CUSTOM_CONFIGURATION_TYPES="${env:CONFIGURATION}" -G"${env:GENERATOR}" ..

Write-Output "Running build"
cmake --build . --config "${env:CONFIGURATION}"
cmake --build . --config "${env:CONFIGURATION}" --target package

# Write-Output "Cleaning build"
# cmake --build . --config "${env:CONFIGURATION}" --target clean
