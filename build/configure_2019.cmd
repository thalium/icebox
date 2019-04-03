@echo off
set OUT=..\out\x64
mkdir %OUT% 2>NUL
set TARGET="Visual Studio 16 2019"
set CFGS=Debug;RelWithDebInfo
cmd /C "pushd %OUT% & cmake ../../build -G %TARGET% -A x64 -DCMAKE_CONFIGURATION_TYPES=%CFGS%"
