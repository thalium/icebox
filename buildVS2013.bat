
rem out_86
mkdir out_x86
pushd out_x86 
cmake -G "Visual Studio 12 2013" ..\build\
popd
cmake --build out_x86 --config Release

rem out_x64
mkdir out_x64
pushd out_x64
cmake -G "Visual Studio 12 2013 Win64" ..\build\
popd
cmake --build out_x64 --config Release