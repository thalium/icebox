include(AutoconfHelper)

ac_init()
ac_check_headers(elf.h libelf.h libelf/libelf.h sgidefs.h sys/types.h stdafx.h Windows.h)
ac_check_lib(${LIBELF_LIBRARIES} elf elf32_getehdr)
ac_check_lib(${LIBELF_LIBRARIES} elf elf64_getehdr)

# Find out where the elf header is.
if(HAVE_ELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<elf.h>")
elseif(HAVE_LIBELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<libelf.h>")
elseif(HAVE_LIBELF_LIBELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<libelf/libelf.h>")
endif()

ac_try_compile("
#include ${HAVE_LOCATION_OF_LIBELFHEADER}
int main()
{
    Elf64_Rel *p; int i; i = p->r_info;
    return 0;
}"
HAVE_ELF64_R_INFO)

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

ac_try_compile([=[
#include <sys/types.h>
#include <regex.h>
int main()
{
    int i; 
    regex_t r;
    int cflags = REG_EXTENDED;
    const char *s = "abc";
    i = regcomp(&r,s,cflags);
    regfree(&r);
    return 0;
}]=]
HAVE_REGEX)

ac_try_compile("
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
message("Checking if __attribute__ unused compiles ok... ${HAVE_UNUSED_ATTRIBUTE}")

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
if(HAVE_ZLIB)
    set(dwfzlib "z")
endif()

ac_try_compile("
#include <libelf.h>
int main()
{
    int p; p = 0;
    return 0;
}" 
HAVE_RAW_LIBELF_OK)

ac_try_compile("
#define _GNU_SOURCE
#include <libelf.h>
int main()
{
    off64_t  p; p = 0;
    return 0;
}" 
HAVE_LIBELF_OFF64_OK)
message(STATUS "Checking is off64_t type supported... ${HAVE_LIBELF_OFF64_OK}")

configure_file(config.h.in.cmake config.h)
