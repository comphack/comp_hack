@echo off
mkdir build32 > nul 2> nul
cd build32
set SCRIPT_DIR=%~dp0
set CMAKE_PREFIX_PATH=C:\Qt\5.12.3\msvc2017
cmake -G"Visual Studio 15 2017" -DWINDOWS_SERVICE=OFF -DCMAKE_INSTALL_PREFIX="%SCRIPT_DIR%\build32\install" -DCPACK_WIX_ROOT="C:/Program Files (x86)/WiX Toolset v3.11" -DUSE_SYSTEM_OPENSSL=ON -DOPENSSL_ROOT_DIR="C:\Program Files (x86)\OpenSSL-Win32" -DBUILD_BINARIES32=ON ..
pause
