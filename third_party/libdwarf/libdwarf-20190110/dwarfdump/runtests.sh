#!/bin/sh
#
# Intended to be run only on local machine.
# Run in the dwarfdump directory
# Run only after config.h created in a configure
# in the source directory

top_blddir=`pwd`/..
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/dwarfdump

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
CFLAGS="-g -O2 -I$top_blddir -I$top_srcdir/libdwarf  -I$top_blddir/libdwarf"

echo "dwgetopt test"
$CC $CFLAGS -o getopttest $srcdir/getopttest.c $srcdir/dwgetopt.c
chkres $? "compiling getopttest test"
./getopttest
chkres $? "running getopttest"
rm ./getopttest
echo "Now use system getopt to validate our tests"
$CC $CFLAGS -DGETOPT_FROM_SYSTEM -o getopttestnat $srcdir/getopttest.c $srcdir/dwgetopt.c
chkres $? "compiling getopttestnat "
./getopttestnat -c 1
chkres $? "running getopttestnat -c 1 "
./getopttestnat -c 2
chkres $? "running getopttestnat -c 2 "
./getopttestnat -c 3
chkres $? "running getopttestnat -c 3 "
./getopttestnat -c 5
chkres $? "running getopttestnat -c 5 "
./getopttestnat -c 6
chkres $? "running getopttestnat -c 6 "
./getopttestnat -c 7
chkres $? "running getopttestnat -c 7 "
./getopttestnat -c 8
chkres $? "running getopttestnat -c 8 "
./getopttestnat -c 9
chkres $? "running getopttestnat -c 9 "
./getopttestnat -c 10
chkres $? "running getopttestnat -c 10 "
rm  ./getopttestnat

echo "start selfmakename"
$CC $CFLAGS -DSELFTEST  -c $srcdir/esb.c
chkres $? "compiling esb.c test"
$CC -c $CFLAGS $srcdir/dwarf_tsearchbal.c
chkres $? "compiling dwarf_tsearchbal.c test"
$CC -g -DSELFTEST  $CFLAGS $srcdir/makename.c dwarf_tsearchbal.o -o selfmakename
chkres $? "compiling selfmakename test"
./selfmakename
chkres $? "running selfmakename "
rm  ./selfmakename

echo "start selfhelpertree"
$CC -DSELFTEST $CFLAGS -g $srcdir/helpertree.c esb.o dwarf_tsearchbal.o -o selfhelpertree
chkres $? "compiling helpertree.c selfhelpertree"
./selfhelpertree
chkres $? "running selfhelpertree "
rm  ./selfhelpertree

echo "start selfmc"
$CC -DSELFTEST $CFLAGS -g $srcdir/macrocheck.c esb.o dwarf_tsearchbal.o -o selfmc
chkres $? "compiling macrocheck.c selfmc"
./selfmc
chkres $? "running selfmc "
rm -f ./selfmc

echo "start selfesb"
$CC -DSELFTEST $CFLAGS $srcdir/testesb.c esb.o -o selfesb
chkres $? "compiling selfesb.c selfesb"
./selfesb
chkres $? "running selfesb "
rm  ./selfesb

echo "start selfsetion_bitmaps"
$CC -DSELFTEST $CFLAGS -g $srcdir/section_bitmaps.c -o selfsection_bitmaps
chkres $? "compiling bitmaps.c section_bitmaps"
./selfsection_bitmaps
chkres $? "running selfsection_bitmaps "
rm  ./selfsection_bitmaps

echo "start selfprint_reloc"
$CC -DSELFTEST $CFLAGS $srcdir/print_reloc.c esb.o -o selfprint_reloc
chkres $? "compiling print_reloc.c selfprint_reloc"
./selfprint_reloc
chkres $? "running selfprint_reloc "
rm ./selfprint_reloc

