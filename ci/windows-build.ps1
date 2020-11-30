$ErrorActionPreference = "Stop"

$ROOT_DIR = $(Get-Location).Path

# Report disk space
Get-CimInstance -Class CIM_LogicalDisk | Select-Object @{Name = "Size(GB)"; Expression = { $_.size / 1gb } }, @{Name = "Free Space(GB)"; Expression = { $_.freespace / 1gb } }, @{Name = "Free (%)"; Expression = { "{0,6:P0}" -f (($_.freespace / 1gb) / ($_.size / 1gb)) } }, DeviceID, DriveType | Where-Object DriveType -EQ '3'

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

if ("${env:BUILD_OUTSIDE}" -eq "YES") {
    New-Item -ItemType directory -Path C:\build-outside | Out-Null
    Set-Location C:\build-outside
}

Write-Output "Running cmake"
cmake -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/build/install" -DDOXYGEN_EXECUTABLE="${ROOT_DIR}/doxygen/doxygen.exe" -DGENERATE_DOCUMENTATION=ON -DWINDOWS_SERVICE=ON -DCMAKE_CUSTOM_CONFIGURATION_TYPES="${env:CONFIGURATION}" -DCOMPRESS_PDB="${env:COMPRESS_PDB}" -DOPENSSL_ROOT_DIR="${env:OPENSSL_ROOT_DIR}" -DUSE_SYSTEM_OPENSSL=ON -G"${env:GENERATOR}" "${ROOT_DIR}"

Write-Output "Running build"
cmake --build . --config "${env:CONFIGURATION}"

# This actually causes the build to start over...
# Write-Output "Clean build"
# cmake --build . --config "${env:CONFIGURATION}" --target clean

# Report disk space
Get-CimInstance -Class CIM_LogicalDisk | Select-Object @{Name = "Size(GB)"; Expression = { $_.size / 1gb } }, @{Name = "Free Space(GB)"; Expression = { $_.freespace / 1gb } }, @{Name = "Free (%)"; Expression = { "{0,6:P0}" -f (($_.freespace / 1gb) / ($_.size / 1gb)) } }, DeviceID, DriveType | Where-Object DriveType -EQ '3'

Write-Output "Package build"
cmake --build . --config "${env:CONFIGURATION}" --target package

# Report disk space
Get-CimInstance -Class CIM_LogicalDisk | Select-Object @{Name = "Size(GB)"; Expression = { $_.size / 1gb } }, @{Name = "Free Space(GB)"; Expression = { $_.freespace / 1gb } }, @{Name = "Free (%)"; Expression = { "{0,6:P0}" -f (($_.freespace / 1gb) / ($_.size / 1gb)) } }, DeviceID, DriveType | Where-Object DriveType -EQ '3'

# Move the packages to be found by the atrifact process
if ("${env:BUILD_OUTSIDE}" -eq "YES") {
    Move-Item -Path *.zip -Destination "${ROOT_DIR}\build"
    Move-Item -Path *.msi -Destination "${ROOT_DIR}\build"
}
