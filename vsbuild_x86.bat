@echo off
mkdir build32 > nul 2> nul
cd build32
cmake -G"Visual Studio 14 2015" -DLIBUV_INCLUDE_DIR="%~dp0\deps\win32\include" -DLIBUV_LIBRARY="%~dp0\deps\win32\lib\libuv.lib" -DOPENSSL_ROOT_DIR="%~dp0\deps\win32" ..
pause
