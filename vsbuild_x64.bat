@echo off
mkdir build64 > nul 2> nul
cd build64
cmake -G"Visual Studio 14 2015 Win64" -DLIBUV_INCLUDE_DIR="%~dp0\deps\win64\include" -DLIBUV_LIBRARY="%~dp0\deps\win64\lib\libuv.lib" -DOPENSSL_ROOT_DIR="%~dp0\deps\win64" ..
pause
