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
  make clean
  cd ../../../../..
  rm -rf out
  rm -rf linux.amd64.bin
  rm -rf tmp
else
  echo "ERROR : need kernel Version 4.x.x to be clean !!"
fi
