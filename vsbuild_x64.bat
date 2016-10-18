@echo off
mkdir build64 > nul 2> nul
cd build64
set CMAKE_PREFIX_PATH=C:\Qt\5.7\msvc2015_64
cmake -DCMAKE_CONFIGURATION_TYPES="Debug;Release" -G"Visual Studio 14 2015 Win64" ..
pause
