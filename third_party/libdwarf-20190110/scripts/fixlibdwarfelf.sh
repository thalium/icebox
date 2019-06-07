#!/bin/sh
if [ $# -ne 3 ]
then
  echo "ERROR fixlibdwarfelf.sh  wrong argument count: $#"
  exit 1
fi
# Yes means use struct _Elf, no means use struct Elf
yorn=$1
srcdir=$2/libdwarf
builddir=$3/libdwarf

#echo "dadebug $yourn $srcdir $builddir"
if [ x$yorn = "xno" ]
then
  cp $srcdir/libdwarf.h.in $builddir/libdwarf.h
else
  sed 's/struct Elf/struct _Elf/g' <$srcdir/libdwarf.h.in >$builddir/libdwarf.h
fi
exit 0


