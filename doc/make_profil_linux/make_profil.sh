#!/bin/sh
# --------------------------------------------------------------
# make_profil - create a profil of a linux kernel for Icebox
#
# profil files will be created in a folder with a hash of
# linux_banner as name
# they are :
#   - the "System.map" file
#   - the "elf" file which is a binary of an empty module
#     including required structures
#
# dependance needed for this script : make gcc zip
#
# --------------------------------------------------------------

tmp=$(mktemp -dt)
trap "rm -rf $tmp ; cd $PWD" 0

version=`sudo dmesg -t | grep "Linux version" | tr -d '\n'`
echo "Version of linux found : '$version'"

hash=`printf '%s' "$version" | sha1sum | cut -c1-40`
wdir=$tmp/kernel/$hash
mkdir $tmp/kernel
mkdir $wdir

make
mv elf $wdir

user_script=$USER
sudo cp /boot/System.map-$(uname -r) $wdir/System.map || sudo cp /proc/kallsyms $wdir/System.map
sudo chown $user_script:$user_script $wdir/System.map
chmod 664 $wdir/System.map

echo "Writing ./profil.zip"
pwd_script=$PWD
cd $tmp
zip -r $pwd_script/profil.zip kernel/

