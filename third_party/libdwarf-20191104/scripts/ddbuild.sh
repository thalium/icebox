# ddbuild.sh
# A primitive build.
# Intended for simple non-elf builds on systems
# with no libelf, no elf.h, no libz.
# This script is by David Anderson and hereby 
# put into the public domain
# for anyone to use in any way.

# Requires a basic config.h at top level

d=`pwd`
db=`basename $d`

if [ x$db != "xdwarfdump" ]
then
   echo FAIL Run this in the dwarfdump directory.
   exit 1
fi


set -x
top_builddir=..
top_srcdir=..
CC="gcc -g  -I.. -I../libdwarf"
EXEXT=.exe

cp $top_builddir/libdwarf/dwarf_names.c .
cp $top_builddir/libdwarf/dwarf_names.h .
$CC -DTRIVIAL_NAMING dwarf_names.c common.c \
dwarf_tsearchbal.c \
dwgetopt.c \
esb.c \
makename.c \
naming.c \
sanitized.c \
tag_attr.c \
tag_common.c -o tag_attr_build$EXEXT
if [ $? -ne 0 ]
then
   echo tag_attr_build compile fail
   exit 1
fi

$CC  -DTRIVIAL_NAMING dwarf_names.c common.c \
dwarf_tsearchbal.c \
dwgetopt.c \
esb.c \
makename.c \
naming.c \
sanitized.c \
tag_common.c \
tag_tree.c -o tag_tree_build$EXEXT
if [ $? -ne 0 ]
then
   echo tag_tree_build compile fail
   exit 1
fi
rm -f tmp-t1.c
cp $top_srcdir/dwarfdump/tag_tree.list tmp-t1.c
$CC -E tmp-t1.c >tmp-tag-tree-build1.tmp
./tag_tree_build$EXEXT -s -i tmp-tag-tree-build1.tmp -o dwarfdump-tt-table.h
if [ $? -ne 0 ]
then
   echo tag_tree_build 1  FAIL
   exit 1
fi
rm -f tmp-tag-tree-build1.tmp 
rm -f tmp-t1.c

rm -f tmp-t2.c
cp $top_srcdir/dwarfdump/tag_attr.list tmp-t2.c
$CC -DTRIVIAL_NAMING  -I$top_srcdir/libdwarf -E tmp-t2.c >tmp-tag-attr-build2.tmp
./tag_attr_build$EXEXT -s -i tmp-tag-attr-build2.tmp -o dwarfdump-ta-table.h
if [ $? -ne 0 ]
then
   echo tag_attr_build 2 FAIL
   exit 1
fi
rm -f tmp-tag-attr-build2.tmp 
rm -f tmp-t2.c

rm -f tmp-t3.c
cp $top_srcdir/dwarfdump/tag_attr_ext.list tmp-t3.c
$CC  -I$top_srcdir/libdwarf -DTRIVIAL_NAMING -E tmp-t3.c > tmp-tag-attr-build3.tmp
./tag_attr_build$EXEXT -e -i tmp-tag-attr-build3.tmp -o dwarfdump-ta-ext-table.h
if [ $? -ne 0 ]
then
   echo tag_attr_build 3 FAIL
   exit 1
fi
rm -f tmp-tag-attr-build3.tmp 
rm -f tmp-t3.c

rm -f tmp-t4.c
cp $top_srcdir/dwarfdump/tag_tree_ext.list tmp-t4.c
$CC  -I$top_srcdir/libdwarf  -DTRIVIAL_NAMING -E tmp-t4.c > tmp-tag-tree-build4.tmp
./tag_tree_build$EXEXT -e -i tmp-tag-tree-build4.tmp -o dwarfdump-tt-ext-table.h
if [ $? -ne 0 ]
then
   echo tag_tree_build 4 compile fail
   exit 1
fi
rm -f tmp-tag-tree-build4.tmp 
rm -f tmp-t4.c

rm -f tag_attr_build$EXEXT
rm -f tag_tree_build$EXEXT

exit 0
