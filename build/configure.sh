#!/bin/bash

if [ -z "$TARGET" ]
then
    TARGET="Unix Makefiles"
fi
echo TARGET=$TARGET [Unix Makefiles, Ninja]

if [ -z "$CONFIG" ]
then
    CONFIG=RelWithDebInfo
fi
echo CONFIG=$CONFIG [Debug, RelWithDebInfo]

if [ -z "$OUT" ]
then
    OUT=../out/x64
fi
echo OUT=$OUT

mkdir -p "$OUT"
cd "$OUT" 
cmake ../../build -DCMAKE_BUILD_TYPE="${CONFIG}" 
