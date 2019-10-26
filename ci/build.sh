#!/bin/bash -xe
set -e
export KERN_VER=4.15.0-66-generic

# set to default gitlab cache folder
export CCACHE_DIR=/cache
ccache -s
CPU_NB="$(grep -c ^processor /proc/cpuinfo)"
# ---- BUILD ----
cd third_party/virtualbox/include
rm -f FDP
ln -s ../../../src/FDP
cd ..
./configure --disable-hardening --disable-docs --disable-java
source env.sh
kmk VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE= VBOX_DO_STRIP=1
cd out/linux.amd64/release/bin/src
make -j $CPU_NB
#make install
cd ../../../../..
mkdir -p out/linux.amd64/release/bin/additions
touch out/linux.amd64/release/bin/additions/VBoxGuestAdditions.iso
kmk packing VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE= VBOX_DO_STRIP=1
mkdir linux.amd64.bin
7za -y e out/linux.amd64/release/bin/*.run -otmp
tar -xjf tmp/VirtualBox.tar.bz2 -C linux.amd64.bin
rm -r tmp
