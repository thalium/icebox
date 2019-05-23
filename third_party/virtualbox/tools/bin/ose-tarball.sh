#!/bin/sh
# Use this script in conjunction with snapshot-ose.sh

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
rootpath=`cd ..;pwd`
verstr="$vermajor.$verminor.$verbuild"
rootname="VirtualBox-$verstr"
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
  --file "$rootpath/$rootname.tar.bz2" \
  "$rootname"
echo "Successfully created $rootpath/$rootname.tar.bz2"
rm -f "$rootpath/$rootname"
