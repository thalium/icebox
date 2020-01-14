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

goodcount=0
failcount=0

echo "TOP topsrc $top_srcdir topbld $top_blddir localsrc $srcdir"
chkres() {
r=$1
m=$2
if [ $r -ne 0 ]
then
  echo "FAIL $m.  Exit status for the test $r"
  failcount=`expr $failcount + 1`
else 
  goodcount=`expr $goodcount + 1`
fi
}

which cc
if [ $? -eq 0 ]
then
  CC=cc
else
  which gcc
  if [ $? -eq 0 ]
  then
    CC=gcc
  else
    # we will fail
    CC=cc
  fi
fi
CFLAGS="-g -O2 -I$top_blddir -I$top_srcdir/libdwarf  -I$top_blddir/libdwarf -Wall -Wextra -Wno-unused-private-field -Wpointer-arith -Wmissing-declarations -Wcomment -Wformat -Wpedantic -Wuninitialized -Wshadow -Wno-long-long -Werror"

echo "dwgetopt test"
$CC $CFLAGS  -o getopttest $srcdir/getopttest.c $srcdir/dwgetopt.c
chkres $? "compiling getopttest test"
./getopttest
chkres $? "running getopttest"
# we will want to know if windows
if [ -f getopttest.exe ]
then
  win=y
else
  win=n
fi
rm -f getopttest getopttest.exe

# The following tests are not really relevant: 
# we do not use system getopt in libdwarf etc.
#echo "Now use system getopt to validate our tests"
#$CC $CFLAGS -DGETOPT_FROM_SYSTEM -o getopttestnat $srcdir/getopttest.c $srcdir/dwgetopt.c
#chkres $? "compiling getopttestnat "
#./getopttestnat -c 1
#chkres $? "running getopttestnat -c 1 "
#./getopttestnat -c 2
#chkres $? "running getopttestnat -c 2 "
#./getopttestnat -c 3
#chkres $? "running getopttestnat -c 3 "
#./getopttestnat -c 5
#chkres $? "running getopttestnat -c 5 "
#./getopttestnat -c 6
#chkres $? "running getopttestnat -c 6 "
#./getopttestnat -c 7
#chkres $? "running getopttestnat -c 7 "
#./getopttestnat -c 8
#chkres $? "running getopttestnat -c 8 "
#./getopttestnat -c 9
#chkres $? "running getopttestnat -c 9 "
#./getopttestnat -c 10
#chkres $? "running getopttestnat -c 10 "
#rm  ./getopttestnat

echo "start selfmakename"
$CC $CFLAGS  -c $srcdir/esb.c $srcdir/dwarf_tsearchbal.c 
chkres $? "compiling makename test"
$CC -g $CFLAGS $srcdir/makename.c $srcdir/makename_test.c dwarf_tsearchbal.o esb.o -o selfmakename
chkres $? "compiling selfmakename test"
./selfmakename
chkres $? "running selfmakename "
rm -f selfmakename selfmakename.exe

echo "start selfhelpertree"
$CC $CFLAGS -g $srcdir/helpertree_test.c $srcdir/helpertree.c dwarf_tsearchbal.o -o selfhelpertree
chkres $? "compiling helpertree.c selfhelpertree"
./selfhelpertree
chkres $? "running selfhelpertree "
rm -f selfhelpertree selfhelpertree.exe

echo "start selfmc"
$CC -DSELFTEST $CFLAGS -g $srcdir/macrocheck.c  dwarf_tsearchbal.o -o selfmc
chkres $? "compiling macrocheck.c selfmc"
./selfmc
chkres $? "running selfmc "
rm -f ./selfmc selfmc.exe

#echo "start selfesb"
#$CC  $CFLAGS $srcdir/testesb.c esb.o -o selfesb
#chkres $? "compiling selfesb.c selfesb"
#./selfesb
#chkres $? "running selfesb "
#rm -f ./selfesb  selfesb.exe


echo "start selfsetion_bitmaps"
$CC  $CFLAGS -g $srcdir/section_bitmaps_test.c  $srcdir/section_bitmaps.c -o selfsection_bitmaps
chkres $? "compiling bitmaps.c section_bitmaps"
./selfsection_bitmaps
chkres $? "running selfsection_bitmaps "
rm -f ./selfsection_bitmaps selfsection_bitmaps.exe

echo "start selfprint_reloc"
$CC $CFLAGS -DSELFTEST=1 -DTESTING=1 $srcdir/print_reloc_test.c esb.o -o selfprint_reloc
chkres $? "compiling print_reloc.c selfprint_reloc"
./selfprint_reloc
chkres $? "running selfprint_reloc "
rm -f ./selfprint_reloc selfprint_reloc.exe


droptwoifwin() {
i=$1
l=`wc -l < $i`
if [ $l -gt 2 ]
then
  l=`expr $l - 2`
  tail -$l <$i >junk.tmp
  cp junk.tmp $i
fi
}

# The following stop after 400 lines to limit the size
# of the data here.  
# It is a sanity check, not a full check.
f=$srcdir/testobjLE32PE.exe
b=$srcdir/testobjLE32PE.base
t=junk.testsmallpe
echo "start  dwarfdump sanity check on pe $f"
./dwarfdump $f | head -n 500 > $t
chkres $? "Running dwarfdump $f output to $t base $b"
if [ x$win = "xy" ]
then
  echo "drop two lines"
  droptwoifwin $t
fi
echo "if update required, mv $top_blddir/dwarfdump/$t $b"
which dos2unix
if [ $? -eq 0 ]
then
  dos2unix $t
fi
diff  $b $t > $t.diffjunk.testsmallpe.diff
chkres $? "diff of $b $t"

f=$srcdir/testuriLE64ELf.obj
b=$srcdir/testuriLE64ELf.base
t=junk.smallLE64ELf
echo "start  dwarfdump sanity check on $f"
./dwarfdump $f | head -n 500 > $t
chkres $? "running ./dwarfdump $f otuput to $t base $b "
if [ x$win = "xy" ]
then
  echo "drop two lines"
  droptwoifwin $t
fi
echo "if update required, mv $top_blddir/dwarfdump/$t $b"
which dos2unix
if [ $? -eq 0 ]
then
  dos2unix $t
fi
diff $b $t > $t.diff
chkres $? "diff of $b $t"

f=$srcdir/test-mach-o-32.dSYM
b=$srcdir/test-mach-o-32.base
t=junk.macho-object32
echo "start  dwarfdump sanity check on $f"
./dwarfdump $f | head -n 500 > $t
chkres $? "FAIL dwarfdump/runtests.sh ./dwarfdump $f to $t base $b "
if [ x$win = "xy" ]
then
  echo "drop two lines"
  droptwoifwin $t
fi
chkres $? "Running dwarfdump on $f"
echo "if update required, mv $top_blddir/dwarfdump/$t $b"
which dos2unix
if [ $? -eq 0 ]
then
  dos2unix $t
fi
diff $b $t > $t.diff
chkres $? "dwarfdump/runtests.sh diff of $b $t"
if [ $failcount -ne 0 ]
then
   echo "FAIL $failcount dwarfdump/runtests.sh"
   exit 1
fi
exit 0
