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

if [ -z "$USE_STATIC_ANALYZER" ]
then
    USE_STATIC_ANALYZER=0
fi

if [ -n "$NO_CLANG_FORMAT" ]
then
    NO_CLANG_FORMAT=-DNO_CLANG_FORMAT=1
fi

if [ -z "$OUT" ]
then
    OUT=../out/x64
fi
echo OUT=$OUT

mkdir -p "$OUT"
cd "$OUT"
cmake ../../build -DCMAKE_BUILD_TYPE="${CONFIG}" \
    -G "${TARGET}" \
    -DUSE_STATIC_ANALYZER=${USE_STATIC_ANALYZER} \
    ${NO_CLANG_FORMAT}
