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
# dependance needs : make gcc zip
#
# --------------------------------------------------------------

tmp=$(mktemp -dt)
trap "rm -rf $tmp ; cd $PWD" 0

version=`dmesg -t | grep "Linux version" | tr -d '\n'`
echo "Version of linux found : '$version'"

hash=`printf '%s' "$version" | sha1sum | cut -c1-40`
mkdir $tmp/$hash

make
mv elf $tmp/$hash

user_script=$USER
sudo cp /boot/System.map-$(uname -r) $tmp/$hash/System.map
sudo chown $user_script:$user_script $tmp/$hash/System.map
chmod 664 $tmp/$hash/System.map

echo "Writing ./profil.zip"
pwd_script=$PWD
cd $tmp
zip -r $pwd_script/profil.zip $hash/

