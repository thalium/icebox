#!/bin/bash -xe
set -e
KERNEL_HEADER_V=$(uname -r)
#compile work only on kernel version 4.X.X
if [[ ${KERNEL_HEADER_V:0:1} == "4" ]] ; then
  # set to default gitlab cache folder
  export CCACHE_DIR=/cache
  ccache -s
  CPU_NB="$(grep -c ^processor /proc/cpuinfo)"
  echo "---- BUILD ----"
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
  rm -rf linux.amd64.bin  
  mkdir linux.amd64.bin
  7za -y e out/linux.amd64/release/bin/*.run -otmp
  tar -xjf tmp/VirtualBox.tar.bz2 -C linux.amd64.bin
  rm -r tmp
else
  echo "ERROR : need kernel Version 4.x.x to be build !!"
fi
