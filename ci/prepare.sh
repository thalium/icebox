#!/bin/bash
#linux-headers-$(uname -r)
# /!\ change in build.sh too
KERNEL_VERSION=4.15.0-66-generic
apt update -y && apt install -y ccache build-essential g++-multilib gcc-multilib libcap-dev libcurl4-openssl-dev libdevmapper-dev libidl-dev libelf-dev libopus-dev libpam0g-dev libqt5x11extras5-dev libsdl1.2-dev libsdl2-dev libssl-dev libvpx-dev libxml2-dev libxmu-dev linux-libc-dev makeself p7zip-full python-dev qt5-default qttools5-dev-tools xsltproc
apt install -y acpica-tools wget
apt install -y linux-headers-$KERNEL_VERSION
apt-get clean
/usr/sbin/update-ccache-symlinks
/usr/bin/ccache -M 1G

