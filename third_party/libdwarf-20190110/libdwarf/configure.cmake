include(AutoconfHelper)

ac_init()
ac_check_headers(alloca.h elf.h elfaccess.h libelf.h libelf/libelf.h  sys/types.h sys/elf_386.h sys/elf_amd64.h sys/elf_sparc.h sys/ia64/elf.h)

#  The default libdwarf is the one with struct Elf
message(STATUS "Assuming struct Elf for the default libdwarf.h")
configure_file(libdwarf.h.in libdwarf.h COPYONLY)

ac_check_lib(${LIBELF_LIBRARIES} elf elf64_getehdr)
ac_check_lib(${LIBELF_LIBRARIES} elf elf64_getshdr)

# Find out where the elf header is.
if(HAVE_ELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<elf.h>")
elseif(HAVE_LIBELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<libelf.h>")
elseif(HAVE_LIBELF_LIBELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<libelf/libelf.h>")
endif()

ac_try_compile("
int main()
{
    __uint32_t p; 
    p = 3;
    return 0;
}" 
HAVE___UINT32_T)

ac_try_compile("
int main()
{
    __uint64_t p; 
    p = 3;
    return 0;
}" 
HAVE___UINT64_T)

ac_try_compile("
#include <sys/types.h>
int main()
{
    __uint32_t p; 
    p = 3;
    return 0;
}" 
HAVE___UINT32_T_IN_SYS_TYPES_H)
ac_try_compile("
#include <sys/types.h>
int main()
{
    __uint64_t p; 
    p = 3;
    return 0;
}" 
HAVE___UINT64_T_IN_SYS_TYPES_H)

check_c_source_runs("
static unsigned foo( unsigned x, __attribute__ ((unused)) int y)
{  
    unsigned x2 = x + 1;
    return x2;
}

int main(void) {
    unsigned y = 0;
    y = foo(12,y);
    return 0;
}"
HAVE_UNUSED_ATTRIBUTE)
message(STATUS "Checking compiler supports __attribute__ unused... ${HAVE_UNUSED_ATTRIBUTE}")

ac_try_compile([=[
#include "zlib.h"
int main()
{
    Bytef dest[100]; 
    uLongf destlen = 100; 
    Bytef *src = 0;
    uLong srclen = 3;
    int res = uncompress(dest,&destlen,src,srclen);
    if (res == Z_OK) {
         /* ALL IS WELL */
    }
    return 0;
}]=]
HAVE_ZLIB)
message(STATUS "Checking zlib.h usability... ${HAVE_ZLIB}")
set(dwfzlib $<$<BOOL:${HAVE_ZIB}>:"z")

#  The following are for FreeBSD and others which
#  use struct _Elf as the actual struct type.
if(HAVE_LIBELF_H)
    set(_Elf_HEADER "<libelf.h>")
else()
    set(_Elf_HEADER "<libelf/libelf.h>")
endif()

ac_try_compile("
#include ${_Elf_HEADER}
struct _Elf; 
typedef struct _Elf Elf;
int main()
{
    struct _Elf *a = 0;
    return 0;
}"
_Elf_found)

if(_Elf_found)
   message(STATUS "Found struct _Elf in ${_ELF_HEADER}, using it in libdwarf.h")
   file(READ libdwarf.h.in CONTENT)
   string(REPLACE "struct Elf" "struct _Elf" CONTENT ${CONTENT})
   file(WRITE libdwarf.h ${CONTENT})
else()
   message(STATUS "${_ELF_HEADER} does not have struct _Elf")
endif()
#  checking for ia 64 types, which might be enums, using HAVE_R_IA_64_DIR32LSB
#  to stand in for a small set.
ac_try_compile("
#include ${HAVE_LOCATION_OF_LIBELFHEADER}
int main()
{
    int p; p = R_IA_64_DIR32LSB;
    return 0;
}" 
HAVE_R_IA_64_DIR32LSB)

ac_try_compile("
#include ${LOCATION_OF_LIBELFHEADER}
int main()
{
    struct _Elf a; int i; i = 0;
    return 0;
}" 
HAVE_STRUCT_UNDERSCORE_ELF)
message(STATUS "Checking  libelf defines struct _Elf... ${HAVE_STRUCT_UNDERSCORE_ELF}")

ac_try_compile("
#include ${LOCATION_OF_LIBELFHEADER}
int main()
{
    int p; p = 0;
    return 0;
}" 
HAVE_RAW_LIBELF_OK)
message(STATUS "Checking  some libelf found ... ${HAVE_RAW_LIBELF_OK}")

ac_try_compile("
#define _GNU_SOURCE
#include ${LOCATION_OF_LIBELFHEADER}
int main()
{
    off64_t  p; p = 0;
    return 0;
}" 
HAVE_LIBELF_OFF64_OK)
message(STATUS "Checking  off64_t ok ... ${HAVE_LIBELF_OFF64_OK}")


#  the existence of sgidefs.h does not prove it's truly SGI, nor
#  prove that __uint32_t or __uint64_t is defined therein.

ac_try_compile("
#include <sgidefs.h>
int main()
{
    __uint32_t p; p = 27;
    return 0;
}" 
HAVE___UINT32_T_IN_SGIDEFS_H)


ac_try_compile("
#include <sgidefs.h>
int main()
{
    __uint64_t p; p = 27;
    return 0;
}" 
HAVE___UINT64_T_IN_SGIDEFS_H)

ac_try_compile("
#include  ${HAVE_LOCATION_OF_LIBELF_HEADER}
int main()
{
    Elf64_Rela p; p.r_offset = 1;
    return 0;
}" 
HAVE_ELF64_RELA)

ac_try_compile("
#include ${HAVE_LOCATION_OF_LIBELF_HEADER}
int main()
{
    Elf64_Sym p; p.st_info = 1;
    return 0;
}" 
HAVE_ELF64_SYM)

#   This changes the gennames option from -s  to -t
option(namestable "string functions implemented as binary search (default is with C switch)" TRUE)
if(namestable)
    set(dwarf_namestable "-s")
else()
    set(dwarf_namestable "-t")
endif()

option(windowspath "Detect certain Windows paths as full paths (default is NO)" FALSE)
set(HAVE_WINDOWS_PATH ${windowspath})
message(STATUS "Checking enable  windowspath... ${HAVE_WINDOWS_PATH}")

option(oldframecol "Use HAVE_OLD_FRAME_CFA_COL (default is to use new DW_FRAME_CFA_COL3)" FALSE)
set(HAVE_OLD_FRAME_CFA_COL ${oldframecol})
message(STATUS "Checking enable old frame columns... ${HAVE_OLD_FRAME_CFA_COL}")

ac_try_compile([=[
#include "stdafx.h"
int main()
{
    int p; p = 27;
    return 0;
}]=]
HAVE_STDAFX_H)
message(STATUS "Checking have windows stdafx.h... ${HAVE_STDAFX_H}")

#  See pro_init(), HAVE_DWARF2_99_EXTENSION also generates
#  32bit offset dwarf unless DW_DLC_OFFSET_SIZE_64 flag passed to
#  pro_init.
option(dwarf_format_sgi_irix "Force producer to SGI IRIX offset dwarf" FALSE)
set(HAVE_SGI_IRIX_OFFSETS ${dwarf_format_sgi_irix})
message(STATUS "Checking  producer generates SGI IRIX output... ${HAVE_SGI_IRIX_OFFSETS}")

option(dwarf_format_strict_32bit "Force producer to generate only DWARF format 32bit" FALSE)
set(HAVE_STRICT_DWARF2_32BIT_OFFSET ${dwarf_format_strict_32bit})
set(HAVE_DWARF2_99_EXTENSION NOT ${dwarf_format_strict_32bit})
message(STATUS "Checking producer generates only 32bit... ${HAVE_STRICT_DWARF2_32BIT_OFFSET}")

configure_file(config.h.in.cmake config.h)
