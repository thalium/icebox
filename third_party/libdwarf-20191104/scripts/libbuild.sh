# libbuild.sh
# Build the .c .h needed for libdwarf.
# Intended for simple non-elf builds on systems
# This script is by David Anderson and hereby put into the public domain
# for anyone to use in any way.

# Requires a basic config.h at top level
# Possibly cp scripts/baseconfig.h config.h
# Run this from the libdwarf directory.

d=`pwd`
db=`basename $d`

if [ x$db != "xlibdwarf" ]
then
   echo FAIL Run this in the libdwarf directory.
   exit 1
fi

set -x
CC="gcc -g  -I.."
$CC gennames.c dwgetopt.c -o gennames
rm -f dwarf_names.h dwarf_names.c  dwarf_names_enum.h dwarf_names_new.h
./gennames -i . -o .
if [ $? -ne 0 ]
then
   echo gennames fail
   exit 1
fi

$CC dwarf_test_errmsg_list.c  -o errmsg_check
if [ $? -ne 0 ]
then
   echo build errmsgcheck fail
   exit 1
fi

# This produces no output. 
# If it fails it indicates a problem with the DW_DLE names or strings.
# If it passes there is no problem.
grep DW_DLE libdwarf.h.in >errmsg_check_list
./errmsg_check -f errmsg_check_list
if [ $? -ne 0 ]
then
   echo errmsg check fail
   exit 1
fi

rm -f gennames 
rm -f errmsg_check_list
rm -f errmsg_check
exit 0
