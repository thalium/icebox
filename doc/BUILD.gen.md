**This file was generated from the .gitlab-ci.yml file that is used
                for continuous integration.**<br>**It contains useful commands to
                build the project.**<br>
# Environment variables
```bash
APT_UPDATE = apt-get update -qq -y
APT_INSTALL = apt-get install -qq -y
DISABLE_CLANG_FORMAT = true
USER = user
```
# Stage: build

## Job: gcc
Tags:
* ubuntu
* docker


Commands:
```bash
$APT_UPDATE && $APT_INSTALL cmake python3 gcc-8 g++-8 ninja-build > /dev/null
cd build
CC=/usr/bin/gcc-8 CXX=/usr/bin/g++-8 TARGET=Ninja ./configure.sh
cd ../out/x64
ninja
```

## Job: clang
Tags:
* ubuntu
* docker


Commands:
```bash
$APT_UPDATE && $APT_INSTALL cmake python3 clang-7 clang-tidy-7 ninja-build > /dev/null
cd build
CC=/usr/bin/clang-7 CXX=/usr/bin/clang++-7 TARGET=Ninja USE_STATIC_ANALYZER=1 ./configure.sh
cd ../out/x64
ninja
```

## Job: msvc
Tags:
* msvc2017


Commands:
```bash
mkdir out\x64
cd out/x64
cmake ../../build -G "Visual Studio 15 2017 Win64" -DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo
cmake --build . --config RelWithDebInfo -- -verbosity:minimal
```

## Job: vbox_vmm gcc
Tags:
* ubuntu
* docker


Commands:
```bash
$APT_UPDATE && $APT_INSTALL acpica-tools build-essential g++-multilib gcc-multilib libcap-dev libcurl4-openssl-dev libdevmapper-dev libidl-dev libelf-dev libopus-dev libpam0g-dev libqt5x11extras5-dev libsdl1.2-dev libsdl2-dev libssl-dev libvpx-dev libxml2-dev libxmu-dev linux-headers-$(uname -r) linux-libc-dev makeself p7zip-full python-dev qt5-default qttools5-dev-tools xsltproc > /dev/null
cd third_party/virtualbox/include
ln -s ../../../src/FDP
cd ..
./configure --disable-hardening --disable-docs --disable-java
source env.sh
kmk VBoxVMM VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE= VBOX_DO_STRIP=1
```

## Job: vbox_vmm msvc
Tags:
* msvc2017


Varbiables:
```bash
QT_DIR = c:/qt-5.6.3
OPENSSL_DIR = c:/OpenSSL-Win64
CURL_DIR = c:/curl-7.64.0
```
Commands:
```bash
set PATH=%PATH%;c:/Windows/System32
cmd /C ""c:/Program Files/Microsoft SDKs/Windows/v7.1/Bin/SetEnv.Cmd" /x64 /Release"
cd third_party/virtualbox/include
mklink /J FDP ..\..\..\src\FDP
cd ..
cscript configure.vbs --with-MinGW-w64=c:/mingw64 --with-libSDL=c:/SDL-1.2.15 --with-openssl=%OPENSSL_DIR% --with-openssl32=c:/OpenSSL-Win32 --with-qt5=%QT_DIR%/qtbase --with-python=c:/Python37-32 --with-libcurl=%CURL_DIR%/builds/libcurl-vc10-x64-release-dll-ipv6-sspi-winssl --with-libcurl32=%CURL_DIR%/builds/libcurl-vc10-x86-release-dll-ipv6-sspi-winssl
call env.bat
kmk VBoxVMM VBOX_WITHOUT_HARDENING=1 VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE=
```


# Stage: virtualbox

## Job: gcc
Tags:
* ubuntu
* docker


Commands:
```bash
$APT_UPDATE && $APT_INSTALL acpica-tools build-essential g++-multilib gcc-multilib libcap-dev libcurl4-openssl-dev libdevmapper-dev libidl-dev libelf-dev libopus-dev libpam0g-dev libqt5x11extras5-dev libsdl1.2-dev libsdl2-dev libssl-dev libvpx-dev libxml2-dev libxmu-dev linux-headers-$(uname -r) linux-libc-dev makeself p7zip-full python-dev qt5-default qttools5-dev-tools xsltproc > /dev/null
cd third_party/virtualbox/include
ln -s ../../../src/FDP
cd ..
./configure --disable-hardening --disable-docs --disable-java
source env.sh
kmk VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE= VBOX_DO_STRIP=1
cd out/linux.amd64/release/bin/src
make
make install
cd ../../../../..
mkdir -p out/linux.amd64/release/bin/additions
touch out/linux.amd64/release/bin/additions/VBoxGuestAdditions.iso
kmk packing VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE= VBOX_DO_STRIP=1
mkdir linux.amd64.bin
7za -y e out/linux.amd64/release/bin/*.run -otmp
tar -xjf tmp/VirtualBox.tar.bz2 -C linux.amd64.bin
rm -r tmp
```

## Job: msvc
Tags:
* msvc2017


Varbiables:
```bash
QT_DIR = c:/qt-5.6.3
OPENSSL_DIR = c:/OpenSSL-Win64
CURL_DIR = c:/curl-7.64.0
```
Commands:
```bash
set PATH=%PATH%;c:/Windows/System32
cmd /C ""c:/Program Files/Microsoft SDKs/Windows/v7.1/Bin/SetEnv.Cmd" /x64 /Release"
cd third_party/virtualbox/include
mklink /J FDP ..\..\..\src\FDP
cd ..
cscript configure.vbs --with-MinGW-w64=c:/mingw64 --with-libSDL=c:/SDL-1.2.15 --with-openssl=%OPENSSL_DIR% --with-openssl32=c:/OpenSSL-Win32 --with-qt5=%QT_DIR%/qtbase --with-python=c:/Python37-32 --with-libcurl=%CURL_DIR%/builds/libcurl-vc10-x64-release-dll-ipv6-sspi-winssl --with-libcurl32=%CURL_DIR%/builds/libcurl-vc10-x86-release-dll-ipv6-sspi-winssl
call env.bat
kmk VBOX_WITHOUT_HARDENING=1 VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE=
cd out/win.amd64/release/bin
rm -r testcase
cp %QT_DIR%/qtbase/bin/Qt5Core.dll .
cp %QT_DIR%/qtbase/bin/Qt5Gui.dll .
cp %QT_DIR%/qtbase/bin/Qt5Widgets.dll .
cp %QT_DIR%/qtbase/bin/Qt5OpenGL.dll .
cp %QT_DIR%/qtbase/bin/Qt5PrintSupport.dll .
cp %OPENSSL_DIR%/bin/libcrypto-1_1-x64.dll .
cp %OPENSSL_DIR%/bin/libssl-1_1-x64.dll .
cp %CURL_DIR%/builds/libcurl-vc10-x64-release-dll-ipv6-sspi-winssl/libcurl.dll .
cp -r %CI_PROJECT_DIR%/third_party/virtualbox/kBuild/bin/win.amd64 kmk
cp -r %CI_PROJECT_DIR%/build/install.cmd .
```


# Stage: deploy

## Job: 
Tags:
* ubuntu
* docker


Commands:
```bash
$APT_UPDATE && $APT_INSTALL git p7zip-full xz-utils > /dev/null
mkdir binaries
cd third_party/virtualbox/out/win.amd64/release/bin
7za a -mx=9 ../../../../../../binaries/vbox-$(git describe --long --tags)-win10.7z *
cd $CI_PROJECT_DIR/third_party/virtualbox/linux.amd64.bin
tar cvJf ../../../binaries/vbox-$(git describe --long --tags)-ubu18.tar.xz *
```

