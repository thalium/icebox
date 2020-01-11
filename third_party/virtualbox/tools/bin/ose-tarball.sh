#!/bin/sh
# $Id$
## @file
# Use this script in conjunction with snapshot-ose.sh
#

#
# Copyright (C) 2006-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

vboxdir=`pwd`
if [ ! -r "$vboxdir/Config.kmk" -o ! -r "$vboxdir/Doxyfile.Core" ]; then
  echo "Is $vboxdir really a VBox tree?"
  exit 1
fi
if [ -r "$vboxdir/src/VBox/RDP/server/server.cpp" ]; then
  echo "Found RDP stuff, refused to build OSE tarball!"
  exit 1
fi
vermajor=`grep "^VBOX_VERSION_MAJOR *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verminor=`grep "^VBOX_VERSION_MINOR *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verbuild=`grep "^VBOX_VERSION_BUILD *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verpre=`grep "^VBOX_VERSION_PRERELEASE *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verpub=`grep "^VBOX_BUILD_PUBLISHER *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verstr="$vermajor.$verminor.$verbuild"
[ -n "$verpre" ] && verstr="$verstr"_"$verpre"
[ -n "$verpub" ] && verstr="$verstr$verpub"
rootpath=`cd ..;pwd`
rootname="VirtualBox-$verstr"
if [ $# -eq 1 ]; then
    tarballname="$1"
else
    tarballname="$rootpath/$rootname.tar.bz2"
fi
rm -f "$rootpath/$rootname"
ln -s `basename "$vboxdir"` "$rootpath/$rootname"
if [ $? -ne 0 ]; then
  echo "Cannot create root directory link!"
  exit 1
fi
tar \
  --create \
  --bzip2 \
  --dereference \
  --owner 0 \
  --group 0 \
  --totals \
  --exclude=.svn \
  --exclude="$rootname/out" \
  --exclude="$rootname/env.sh" \
  --exclude="$rootname/configure.log" \
  --exclude="$rootname/build.log" \
  --exclude="$rootname/AutoConfig.kmk" \
  --exclude="$rootname/LocalConfig.kmk" \
  --exclude="$rootname/prebuild" \
  --directory "$rootpath" \
  --file "$tarballname" \
  "$rootname"
echo "Successfully created $tarballname"
rm -f "$rootpath/$rootname"
