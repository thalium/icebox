# Requirements to compile icebox
## Linux
Install the following packages:
```bash
sudo apt install            \
            cmake           \
            python3         \
            libpython3-dev  \
            gcc-8           \
            g++-8           \
            clang-format    \
            ninja-build
```

*Note: the use of the command **update-alternatives** might be useful to set python3, gcc-8 and g++-8 as default*

## Windows
Install:
```
Visual Studio 2019 Win64
cmake
python3 & debug libraries
clang-format
```

*Note: the easiest way to install clang-format is to install llvm http://releases.llvm.org/download.html*

# Build instructions
Find all the build instructions [here](/doc/BUILD.gen.md).

# Requirements to compile virtualbox
## Linux
Find the requirements [here](/doc/BUILD.gen.md#job-gcc-1).
## Windows
- Install Windows WDK 7.1:
    - SHA1: de6abdb8eb4e08942add4aa270c763ed4e3d8242 *GRMWDK_EN_7600_1.ISO

- Install Visual Studio Express 2010:
    - SHA1: adef5e361a1f64374f520b9a2d03c54ee43721c6 *en_visual_studio_2010_express_x86_dvd_510419.iso

- Install Visual Studio 2010 SP1
    - SHA1: 61c2088850185ede8e18001d1ef3e6d12daa5692 *mu_visual_studio_2010_sp1_x86_dvd_651704.iso

- Install Windows SDK 7.1:
    - SHA1: 9203529f5f70d556a60c37f118a95214e6d10b5a *GRMSDKX_EN_DVD.iso

- Install Visual Studio 2010 SP1 Compiler Update:
    - MD5: 485b2c95b20961b031d2ce0a717461dd *VC-Compiler-KB2519277.exe

- Build Qt 5.6.3 into c:/qt-5.6.3
    - SHA1: f4c69bad0bf9f453500aab72371f835f15f28770 *qt-everywhere-opensource-src-5.6.3.zip
    - SHA1: 30d7e15d76d3ed04206b6e23f9ce08237eedb77e *jom_1_1_3.zip
    - into c:/qt-5.6.3
    - Uncompress jom.exe into c:/qt-5.6.3/qtbase/bin
    - set PATH=c:\qt-5.6.3\qtbase\bin;%PATH%
    - "c:/Program Files/Microsoft SDKs/Windows/v7.1/Bin/SetEnv.Cmd" /x64 /Release
    - configure -platform win32-msvc2010 -opengl desktop -release -opensource -ltcg -no-compile-examples
    - jom

- Install OpenSSL libraries & headers
    - SHA1: 12a4df6aa8f2f89910d095481b86ae1dd8a2f889 *Win32OpenSSL-1_1_1a.exe
    - SHA1: 900007d26f42991aba9d5795e4d8586dbe9c4020 *Win64OpenSSL-1_1_1a.exe
    - into c:\OpenSSL-Win32 & OpenSSL-Win64 respectively

- Install SDL libraries & headers
    - SHA1: 5ed38494ccff04812c88b0b99a4ac6b104d66d8b *SDL-devel-1.2.15-VC.zip
    - into c:/SDL-1.2.15
    - cd c:/SDL-1.2.15/lib
    - mv x64/SDL.dll .
    - mv x64/SDL.lib .
    - mv x64/SDLmain.lib .

- Build Curl
    - SHA1: dc04c71379efe050133a6545ff5d6908c6a6c3a9 *curl-7.64.0.zip
    - into c:/curl-7.64.0
    - Build 64-bit version
    - cd winbuild
    - "c:/Program Files/Microsoft SDKs/Windows/v7.1/Bin/SetEnv.Cmd" /x64 /Release
    - nmake -f Makefile.vc mode=dll VC=10 MACHINE=x64
    - cd c:/curl-7.64.0/builds/libcurl-vc10-x64-release-dll-ipv6-sspi-winssl
    - mv bin/libcurl.dll .
    - mv lib/libcurl.lib .
    - "c:/Program Files/Microsoft SDKs/Windows/v7.1/Bin/SetEnv.Cmd" /x86 /Release
    - nmake -f Makefile.vc mode=dll VC=10 MACHINE=x86
    - cd c:/curl-7.64.0/builds/libcurl-vc10-x86-release-dll-ipv6-sspi-winssl
    - mv bin/libcurl.dll .
    - mv lib/libcurl.lib .

- Install MinGW 4.5.4 64-bit:
    - SHA1: 4c9f4e999fc8dba11bb7c75f17ec0964c0121803 *x86_64-w64-mingw32-gcc-4.5.4-release-win64_rubenvb.7z
    - into c:/mingw64
