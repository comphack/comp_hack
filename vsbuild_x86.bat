@echo off
mkdir build32 > nul 2> nul
cd build32
cmake -G"Visual Studio 14 2015" -DOPENSSL_ROOT_DIR="%~dp0\deps\win32" ..
pause
