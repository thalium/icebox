#!/bin/sh
#
# Intended to be run only on local machine.
# Run in the libdwarf directory
# Run only after config.h created in a configure
# in the source directory

top_blddir=`pwd`/..
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/libdwarf

echo "TOP topsrc $top_srcdir topbld $top_blddir localsrc $srcdir"
chkres() {
r=$1
m=$2
if [ $r -ne 0 ]
then
   echo "FAIL $m.  Exit status $r"
   exit 1
fi
}

CC=cc
CFLAGS="-g -O2 -I$top_blddir -I$top_srcdir/libdwarf -I$top_blddir/libdwarf"
$CC $CFLAGS -DTESTING  $srcdir/dwarf_leb.c $srcdir/pro_encode_nm.c -o dwarfleb
chkres $? "compiling dwarfleb test"
./dwarfleb
chkres $? "Running dwarfleb test"
rm ./dwarfleb

$CC $CFLAGS -DTESTING $srcdir/dwarf_tied.c $srcdir/dwarf_tsearchhash.c -o dwarftied
chkres $? "compiling dwarftied test"
./dwarftied
chkres $? "Running dwarftiedtest test"
rm ./dwarftied
exit 0
