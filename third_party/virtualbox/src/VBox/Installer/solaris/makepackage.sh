#!/bin/sh
# $Id: makepackage.sh $
## @file
# VirtualBox package creation script, Solaris hosts.
#

#
# Copyright (C) 2007-2015 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

#
# Usage:
#       makepackage.sh [--hardened] $(PATH_TARGET)/install packagename {$(KBUILD_TARGET_ARCH)|neutral} $(VBOX_SVN_REV) [VBIPackageName]


# Parse options.
HARDENED=""
while test $# -ge 1;
do
    case "$1" in
        --hardened)
            HARDENED=1
            ;;
    *)
        break
        ;;
    esac
    shift
done

if [ -z "$4" ]; then
    echo "Usage: $0 installdir packagename x86|amd64 svnrev [VBIPackage]"
    echo "-- packagename must not have any extension (e.g. VirtualBox-SunOS-amd64-r28899)"
    exit 1
fi

PKG_BASE_DIR=$1
VBOX_INSTALLED_DIR=$1/opt/VirtualBox
VBOX_PKGFILE=$2.pkg
VBOX_ARCHIVE=$2.tar.gz
# VBOX_PKG_ARCH is currently unused.
VBOX_PKG_ARCH=$3
VBOX_SVN_REV=$4

VBOX_PKGNAME=SUNWvbox
VBOX_GGREP=/usr/sfw/bin/ggrep
VBOX_AWK=/usr/bin/awk
#VBOX_GTAR=/usr/sfw/bin/gtar

# check for GNU grep we use which might not ship with all Solaris
if test ! -f "$VBOX_GGREP" && test ! -h "$VBOX_GGREP"; then
    echo "## GNU grep not found in $VBOX_GGREP."
    exit 1
fi

# check for GNU tar we use which might not ship with all Solaris
#if test ! -f "$VBOX_GTAR" && test ! -h "$VBOX_GTAR"; then
#    echo "## GNU tar not found in $VBOX_GTAR."
#    exit 1
#fi

# bail out on non-zero exit status
set -e

# Fixup filelist using awk, the parameters must be in awk syntax
# params: filename condition action
filelist_fixup()
{
  "$VBOX_AWK" 'NF == 6 && '"$2"' { '"$3"' } { print }' "$1" > "tmp-$1"
  mv -f "tmp-$1" "$1"
}

dirlist_fixup()
{
  "$VBOX_AWK" 'NF == 6 && $1 == "d" && '"$2"' { '"$3"' } { print }' "$1" > "tmp-$1"
  mv -f "tmp-$1" "$1"
}

hardlink_fixup()
{
  "$VBOX_AWK" 'NF == 3 && $1=="l" && '"$2"' { '"$3"' } { print }' "$1" > "tmp-$1"
  mv -f "tmp-$1" "$1"
}

symlink_fixup()
{
  "$VBOX_AWK" 'NF == 3 && $1=="s" && '"$2"' { '"$3"' } { print }' "$1" > "tmp-$1"
  mv -f "tmp-$1" "$1"
}

create_hardlink()
{
    if test -f "$VBOX_INSTALLED_DIR/amd64/$1" || test -f "$VBOX_INSTALLED_DIR/i386/$1"; then
        ln -f ./VBoxISAExec "$VBOX_INSTALLED_DIR/$1"
    fi
}

# Prepare file list
cd "$PKG_BASE_DIR"
echo 'i pkginfo=./vbox.pkginfo' > prototype
echo 'i checkinstall=./checkinstall.sh' >> prototype
echo 'i postinstall=./postinstall.sh' >> prototype
echo 'i preremove=./preremove.sh' >> prototype
echo 'i space=./vbox.space' >> prototype
if test -f "./vbox.copyright"; then
    echo 'i copyright=./vbox.copyright' >> prototype
fi

# Create relative hardlinks for executables to either the 32-bit or 64-bit subfolders
cd "$VBOX_INSTALLED_DIR"
create_hardlink VBoxManage
create_hardlink VBoxSDL
create_hardlink VBoxAutostart
create_hardlink vboxwebsrv
create_hardlink VBoxZoneAccess
create_hardlink VBoxSVC
create_hardlink VBoxBugReport
create_hardlink VBoxBalloonCtrl
create_hardlink VBoxTestOGL
create_hardlink VirtualBox
create_hardlink vbox-img
create_hardlink VBoxHeadless

# Exclude directories to not cause install-time conflicts with existing system directories
cd "$PKG_BASE_DIR"
find . ! -type d | $VBOX_GGREP -v -E 'prototype|makepackage.sh|vbox.pkginfo|postinstall.sh|checkinstall.sh|preremove.sh|ReadMe.txt|vbox.space|vbox.depend|vbox.copyright|VirtualBoxKern' | pkgproto >> prototype

# Include opt/VirtualBox and subdirectories as we want uninstall to clean up directory structure.
# Include var/svc for manifest class action script does not create them.
find . -type d | $VBOX_GGREP -E 'opt/VirtualBox|var/svc/manifest/application/virtualbox' | pkgproto >> prototype

# fix up file permissions (owner/group)
# don't grok for class-specific files (like sed, if any)
filelist_fixup prototype '$2 == "none"'                                                                '$5 = "root"; $6 = "bin"'

# don't include autoresponse and LICENSE from the base folder into / of
# the package, it goes into .tar.gz and another one already exists
# in /opt/VirtualBox
sed '/f none autoresponse/d' prototype > prototype2
mv -f prototype2 prototype
sed '/f none LICENSE/d' prototype > prototype2
mv -f prototype2 prototype

# HostDriver vboxdrv
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/vboxdrv"'                                   '$6 = "sys"'
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/amd64/vboxdrv"'                             '$6 = "sys"'

# NetFilter vboxflt
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/vboxflt"'                                   '$6 = "sys"'
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/amd64/vboxflt"'                             '$6 = "sys"'

# NetFilter vboxbow
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/vboxbow"'                                   '$6 = "sys"'
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/amd64/vboxbow"'                             '$6 = "sys"'

# NetAdapter vboxnet
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/vboxnet"'                                   '$6 = "sys"'
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/amd64/vboxnet"'                             '$6 = "sys"'

# USBMonitor vboxusbmon
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/vboxusbmon"'                                '$6 = "sys"'
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/amd64/vboxusbmon"'                          '$6 = "sys"'

# USB Client vboxusb
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/vboxusb"'                                   '$6 = "sys"'
filelist_fixup prototype '$3 == "platform/i86pc/kernel/drv/amd64/vboxusb"'                             '$6 = "sys"'

# Manifest class action scripts
filelist_fixup prototype '$3 == "var/svc/manifest/application/virtualbox/virtualbox-webservice.xml"'    '$2 = "manifest";$6 = "sys"'
filelist_fixup prototype '$3 == "var/svc/manifest/application/virtualbox/virtualbox-balloonctrl.xml"'   '$2 = "manifest";$6 = "sys"'
filelist_fixup prototype '$3 == "var/svc/manifest/application/virtualbox/virtualbox-zoneaccess.xml"'    '$2 = "manifest";$6 = "sys"'

# Use 'root' as group so as to match attributes with the previous installation and prevent a conflict. Otherwise pkgadd bails out thinking
# we're violating directory attributes of another (non existing) package
dirlist_fixup prototype  '$3 == "var/svc/manifest/application/virtualbox"'                              '$6 = "root"'

# Hardening requires some executables to be marked setuid.
if test -n "$HARDENED"; then
    $VBOX_AWK 'NF == 6 \
        && (    $3 == "opt/VirtualBox/amd64/VirtualBox" \
            ||  $3 == "opt/VirtualBox/amd64/VirtualBox3" \
            ||  $3 == "opt/VirtualBox/amd64/VBoxHeadless" \
            ||  $3 == "opt/VirtualBox/amd64/VBoxSDL" \
            ||  $3 == "opt/VirtualBox/i386/VirtualBox" \
            ||  $3 == "opt/VirtualBox/i386/VirtualBox3" \
            ||  $3 == "opt/VirtualBox/i386/VBoxHeadless" \
            ||  $3 == "opt/VirtualBox/i386/VBoxSDL" \
            ) \
       { $4 = "4755" } { print }' prototype > prototype2
    mv -f prototype2 prototype
fi

# Other executables that need setuid root (hardened or otherwise)
$VBOX_AWK 'NF == 6 \
    && (    $3 == "opt/VirtualBox/amd64/VBoxNetAdpCtl" \
        ||  $3 == "opt/VirtualBox/i386/VBoxNetAdpCtl" \
        ||  $3 == "opt/VirtualBox/amd64/VBoxNetDHCP" \
        ||  $3 == "opt/VirtualBox/i386/VBoxNetDHCP" \
        ||  $3 == "opt/VirtualBox/amd64/VBoxNetNAT" \
        ||  $3 == "opt/VirtualBox/i386/VBoxNetNAT" \
        ) \
   { $4 = "4755" } { print }' prototype > prototype2
mv -f prototype2 prototype

# Our package is a non-relocatable package. pkgadd will take care of "relocating" them when they are used for
# remote installations using $PKG_INSTALL_ROOT and not $BASEDIR. Seems this little subtlety led to it's own page:
# http://docs.sun.com/app/docs/doc/820-4042/package-2?a=view
filelist_fixup prototype  '$2 == "none"'                              '$3="/"$3'
filelist_fixup prototype  '$2 == "manifest"'                          '$3="/"$3'
symlink_fixup  prototype  '$2 == "none"'                              '$3="/"$3'
hardlink_fixup prototype  '$2 == "none"'                              '$3="/"$3'

echo " --- start of prototype  ---"
cat prototype
echo " --- end of prototype --- "

# Explicitly set timestamp to shutup warning
VBOXPKG_TIMESTAMP=vbox`date '+%Y%m%d%H%M%S'`_r$VBOX_SVN_REV

# Create the package instance
pkgmk -p $VBOXPKG_TIMESTAMP -o -r .

# Translate into package datastream
pkgtrans -s -o /var/spool/pkg "`pwd`/$VBOX_PKGFILE" "$VBOX_PKGNAME"

# $5 if exists would contain the path to the VBI package to include in the .tar.gz
#if [ -f LICENSE ]; then
#    VBOX_LICENSEFILE=LICENSE
#fi
#if test -f "$5"; then
#    $VBOX_GTAR zcvf "$VBOX_ARCHIVE" $VBOX_LICENSEFILE "$VBOX_PKGFILE" "$5" autoresponse ReadMe.txt
#else
#    $VBOX_GTAR zcvf "$VBOX_ARCHIVE" $VBOX_LICENSEFILE "$VBOX_PKGFILE" autoresponse ReadMe.txt
#fi

echo "## Package file created successfully!"
rm -rf "/var/spool/pkg/$VBOX_PKGNAME"

exit $?

