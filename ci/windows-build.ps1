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

Write-Output "Installing OpenSSL"
Invoke-WebRequest "${env:OPENSSL_URL}" -OutFile "OpenSSL.msi"
Start-Process msiexec.exe -Wait -ArgumentList '/i OpenSSL.msi /l OpenSSL-install.log /qn'
Remove-Item OpenSSL.msi
Remove-Item OpenSSL-install.log
Write-Output "Installed OpenSSL"

Write-Output "Installing Doxygen"
New-Item -ItemType directory -Path doxygen | Out-Null
Set-Location doxygen
Invoke-WebRequest "${env:DOXYGEN_URL}" -OutFile "doxygen.zip"
7z x doxygen.zip
Remove-Item doxygen.zip
Set-Location "${ROOT_DIR}"
Write-Output "Installed Doxygen"

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
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" -DDOXYGEN_EXECUTABLE="${ROOT_DIR}/doxygen/doxygen.exe" -DGENERATE_DOCUMENTATION=ON -DWINDOWS_SERVICE=ON -DCMAKE_CUSTOM_CONFIGURATION_TYPES="${env:CONFIGURATION}" -DOPENSSL_ROOT_DIR="${env:OPENSSL_ROOT_DIR}" -DUSE_SYSTEM_OPENSSL=ON -G"${env:GENERATOR}" ..

Write-Output "Running build"
cmake --build . --config "${env:CONFIGURATION}"
cmake --build . --config "${env:CONFIGURATION}" --target package

# Write-Output "Cleaning build"
# cmake --build . --config "${env:CONFIGURATION}" --target clean
