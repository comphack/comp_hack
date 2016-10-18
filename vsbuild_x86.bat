@echo off
mkdir build32 > nul 2> nul
cd build32
set CMAKE_PREFIX_PATH=C:\Qt\5.7\msvc2015
cmake -DCMAKE_CONFIGURATION_TYPES="Debug;Release" -G"Visual Studio 14 2015" ..
pause
