cmake_minimum_required(VERSION 3.2)

project(libdwarf C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include_directories( ${CMAKE_BINARY_DIR} )

# used to compile on MSVC upto 2013 where snprintf is not available
macro(msvc_posix target)
    if(MSVC AND ("${MSVC_VERSION}" LESS 1900))
        # under VS 2015
	target_compile_definitions(${target} PUBLIC 
	    snprintf=_snprintf)
    endif()
endmacro()

# set target folder on IDE
macro(set_folder target folder)
  set_target_properties(${target} PROPERTIES FOLDER ${folder})
endmacro()

# set source groups on IDE
macro(set_source_group list_name group_name)
    set(${list_name} ${ARGN})
    source_group(${group_name} FILES ${ARGN})
endmacro()

# view folders on supported IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# used when finding libelf
# tells cmake to look in 64bit first (cmake
# would probably do this anyway).
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)

enable_testing()

# always include project's folder to includes
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

### begin what was configure.cmake
# cmake macros
include(TestBigEndian)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckSymbolExists)

set(VERSION 20191022)
set(PACKAGE_VERSION "\"${VERSION}\"") 
set(PACKAGE_NAME "libdwarf" )
###set(VERSION ${PACKAGE_VERSION} )
set(PACKAGE_STRING "\"${PACKAGE_NAME} ${VERSION}\"")
string(REGEX REPLACE "[\"]" "" tarname1 "${PACKAGE_STRING}" )
string(REGEX REPLACE "[^a-zA-Z0-9_]" "-" tarname "${tarname1}" )
set(PACKAGE_TARNAME "\"${tarname}\"" )
###set(PACKAGE_VERSION  "\"${PACKAGE_VERSION}\"")

test_big_endian(isBigEndian)
if (${isBigEndian})
  set ( WORDS_BIGENDIAN 1 )
endif()

check_include_file( "sys/types.h"     HAVE_SYS_TYPES_H) 
check_include_file( "sys/stat.h"      HAVE_SYS_STAT_H ) 
check_include_file( "stdlib.h"        HAVE_STDLIB_H   ) 
check_include_file( "string.h"        HAVE_STRING_H   ) 
check_include_file( "memory.h"        HAVE_MEMORY_H   ) 
check_include_file( "strings.h"       HAVE_STRINGS_H  ) 
check_include_file( "stdint.h"        HAVE_STDINT_H   )
check_include_file( "unistd.h"        HAVE_UNISTD_H   )
check_include_file( "sgidefs.h"       HAVE_SGIDEFS_H  )
check_include_file( "stdafx.h"        HAVE_STDAFX_H   )
check_include_file( "Windows.h"       HAVE_WINDOWS_H  )
check_include_file( "elf.h"           HAVE_ELF_H      ) 
check_include_file( "libelf.h"        HAVE_LIBELF_H   ) 
check_include_file( "libelf/libelf.h" HAVE_LIBELF_LIBELF_H) 
check_include_file( "alloca.h"        HAVE_ALLOCA_H   )
check_include_file( "elfaccess.h"     HAVE_ELFACCESS_H)
check_include_file( "sys/elf_386.h"   HAVE_SYS_ELF_386_H  )
check_include_file( "sys/elf_amd64.h" HAVE_SYS_ELF_AMD64_H)
check_include_file( "sys/elf_sparc.h" HAVE_SYS_ELF_SPARC_H)
check_include_file( "sys/ia64/elf.h"  HAVE_SYS_IA64_ELF_H )

if(HAVE_STDINT_H)
  check_c_source_compiles("
  #include <stdint.h>
  int main()
  {
      uintptr_t i; i = 12;
      return (int)i;
  }" HAVE_UINTPTR_T)
  check_c_source_compiles("
  #include <stdint.h>
  int main()
  {
      intptr_t i; i = 12;
      return (int)i;
  }" HAVE_INTPTR_T)
endif()
if (HAVE_UINTPTR_T)
  message(STATUS "HAVE_UINTPTR_T 1: uintptr_t defined in stdint.h... YES")
else()
  message(STATUS "uintptr_t defined in stdint.h... NO")
  set(uintptr_t "unsigned long long int" )
  message(STATUS "Setting #define uintptr_t " ${uintptr_t})
endif()
if (uintptr_t) 
  message(STATUS "uintptr_t value considered YES ")
else()
  message(STATUS "uintptr_t value considered NO ")
endif()
if (HAVE_INTPTR_T)
  message(STATUS "HAVE_INTPTR_T 1: intptr_t defined in stdint.h... YES")
else()
  message(STATUS "intptr_t defined in stdint.h... NO")
  set(uintptr_t "long long int" )
  message(STATUS "Setting #define intptr_t " ${intptr_t})
endif()
if (intptr_t) 
  message(STATUS "intptr_t value considered YES ")
else()
  message(STATUS "intptr_t value considered NO ")
endif()


#  It's not really setting the location of libelfheader, 
#  it is really #  either elf.h, or if that is missing 
#  it is assuming  elf.h data is in the supplied libelf.

if(HAVE_ELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<elf.h>")
elseif(HAVE_LIBELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<libelf.h>")
elseif(HAVE_LIBELF_LIBELF_H)
    set(HAVE_LOCATION_OF_LIBELFHEADER "<libelf/libelf.h>")
endif()

if(HAVE_LIBELF_H)
    set(JUST_LIBELF "<libelf.h>")
    set(PLAIN_JUST_LIBELF "libelf.h")
elseif(HAVE_LIBELF_LIBELF_H)
    set(JUST_LIBELF "<libelf/libelf.h>")
    set(PLAIN_JUST_LIBELF "libelf/libelf.h")
endif()

if (HAVE_LIBELF_H OR HAVE_LIBELF_LIBELF_H)
  set (CMAKE_REQUIRED_LIBRARIES elf)
  message(STATUS "libelf header ${PLAIN_JUST_LIBELF} checking for elf64_getehdr")
  check_symbol_exists( elf64_getehdr ${PLAIN_JUST_LIBELF} HAVE_ELF64_GETEHDR)
  message(STATUS "libelf header ${PLAIN_JUST_LIBELF} checking for elf64_getshdr")
  check_symbol_exists( elf64_getshdr ${PLAIN_JUST_LIBELF} HAVE_ELF64_GETSHDR)
  set (CMAKE_REQUIRED_LIBRARIES)
endif()

option(DWARF_WITH_LIBELF "Use libelf (default is YES)" TRUE)
message(STATUS "Building using libelf... ${DWARF_WITH_LIBELF}")
if (DWARF_WITH_LIBELF AND 
    NOT HAVE_LIBELF_H AND 
    NOT HAVE_LIBELF_LIBELF_H)
    set(DWARF_WITH_LIBELF OFF)
endif ()

if (DWARF_WITH_LIBELF)
  message(STATUS "checking using HAVE_ELF_H ... ${HAVE_ELF_H}")
  message(STATUS "checking using elf header ... ${HAVE_LOCATION_OF_LIBELFHEADER}")
  message(STATUS "checking using libelf header ... ${JUST_LIBELF}")
  check_c_source_compiles("
  #include ${HAVE_LOCATION_OF_LIBELFHEADER}
  int main()      
  {
      Elf64_Rel *p; int i; i = p->r_info;
      return 0;
  }" HAVE_ELF64_R_INFO)

  check_c_source_compiles("
  #include  ${HAVE_LOCATION_OF_LIBELFHEADER}
  int main()
  {
      Elf64_Rela p; p.r_offset = 1;
      return 0;
  }" HAVE_ELF64_RELA)
    
  check_c_source_compiles("
  #include ${HAVE_LOCATION_OF_LIBELFHEADER}
  int main()
  {
      Elf64_Sym p; p.st_info = 1;
      return 0;
  }" HAVE_ELF64_SYM)
  check_c_source_compiles("
  #include ${HAVE_LOCATION_OF_LIBELFHEADER}
  int main()
  {
      int p; p = 0;
      return 0;
  }" HAVE_RAW_LIBELF_OK)

  # This is attempting do determine that with GNU_SOURCE
  # we have off64_t. The autoconf version is not attemping
  # to set HAVE_LIBELF_OFF64_OK at present. 
  check_c_source_compiles("
  #define _GNU_SOURCE 1
  #include ${JUST_LIBELF}
  int main()
  { 
      off64_t  p; p = 0;
      return 0;
  }"  HAVE_LIBELF_OFF64_OK)

  check_c_source_compiles("
  #include ${JUST_LIBELF}
  /* This must be at global scope */
  struct _Elf;
  typedef struct _Elf Elf;
  struct _Elf *a = 0;
  int main()
  {
   int i = 12;
   return 0;
  }" HAVE_STRUCT_UNDERSCORE_ELF)
endif()
message(STATUS "Assuming struct Elf for the default libdwarf.h")
# Because cmake treats ; in an interesting way attempting
# to read/update/write Elf to _Elf fails badly: semicolons vanish.
# So for _Elf use a pre-prepared version.
if(HAVE_STRUCT_UNDERSCORE_ELF AND DWARF_WITH_LIBELF)
   message(STATUS "Found struct _Elf in ${JUST_LIBELF}")
   message(STATUS "Using struct _Elf in libdwarf.h")
   configure_file(libdwarf/generated_libdwarf.h.in 
       libdwarf/libdwarf.h COPYONLY)
else()
   configure_file(libdwarf/libdwarf.h.in 
       libdwarf/libdwarf.h COPYONLY)
   message(STATUS "${JUST_LIBELF} does not have struct _Elf")
   message(STATUS "Using struct Elf in libdwarf.h")
endif()

check_c_source_runs("
  static unsigned foo( unsigned x, 
      __attribute__ ((unused)) int y)
  {  
      unsigned x2 = x + 1;
      return x2;
  } 
  
  int main(void) {
      unsigned y = 0;
      y = foo(12,y);
      return 0;
  }"    HAVE_UNUSED_ATTRIBUTE)
message(STATUS "Checking compiler supports __attribute__ unused... ${HAVE_UNUSED_ATTRIBUTE}")

#  checking for ia 64 types, which might be enums, 
#  using HAVE_R_IA_64_DIR32LSB
#  to stand in for a small set.
check_c_source_compiles("
  #include ${HAVE_LOCATION_OF_LIBELFHEADER}
  int main()
  {  
      int p; p = R_IA_64_DIR32LSB;
      return 0;
  }" HAVE_R_IA_64_DIR32LSB)

check_c_source_compiles([=[
  #include "stdafx.h"
  int main() 
  { 
      int p; p = 27;
      return 0;
  }]=] HAVE_STDAFX_H)
#message(STATUS "Checking have windows stdafx.h... ${HAVE_STDAFX_H}")

check_c_source_compiles([=[
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
  } ]=]  HAVE_REGEX)

set(CMAKE_REQUIRED_LIBRARIES z)
check_c_source_compiles( [=[
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
  } ]=]  HAVE_ZLIB )
set(CMAKE_REQUIRED_LIBRARIES)
if (HAVE_ZLIB) 
  # For linking in libz
  set(DW_FZLIB "z")
endif()

check_c_source_compiles([=[
#include <stdint.h>
int main()
{
    intptr_t p; 
    p = 27;
    return 0;
}]=] HAVE_INTPTR_T)

message(STATUS "CMAKE_SIZEOF_VOID_P ... " ${CMAKE_SIZEOF_VOID_P} )

# libdwarf default-disabled shared
option(BUILD_SHARED "build shared library libdwarf.so and use it if present" FALSE)
option(BUILD_NON_SHARED "build archive library libdwarf.a" TRUE)

#  This adds compiler option -Wall (gcc compiler warnings)
option(WALL "Add -Wall" FALSE)

#  DW_FWALLXX are gnu C++ options.
if (WALL)
  set(DW_FWALLXX -Wall -Wextra -Wno-unused-private-field -Wpointer-arith -Wmissing-declarations -Wcomment -Wformat -Wpedantic -Wuninitialized -Wshadow -Wno-long-long -Werror)
  set(DW_FWALL ${DW_FWALLXX} -Wpointer-arith -Wmissing-declarations -Wmissing-prototypes -Wdeclaration-after-statement -Wextra -Wcomment -Wformat -Wpedantic -Wuninitialized -Wno-long-long -Wshadow -Werror )
  message(STATUS "Compiler warning options... YES ${DW_FWALL}")
else()
  unset(DW_FWALL )
  message(STATUS "Compiler warning options... NO")
endif()

option(HAVE_CUSTOM_LIBELF "Have a custom libelf library (default is NO)" FALSE)
message(STATUS "Including a custom libelf library... ${HAVE_CUSTOM_LIBELF}")

option(HAVE_NONSTANDARD_PRINTF_64_FORMAT "Use a special printf format for 64bit (default is NO)" FALSE)
message(STATUS "Checking enable non standard printf... ${HAVE_NONSTANDARD_PRINTF_64_FORMAT}")

option(BUILD_DWARFGEN "Build dwarfgen (default is NO)" FALSE)
message(STATUS "Building dwarfgen    ... ${BUILD_DWARFGEN}")

option(BUILD_DWARFEXAMPLE "Build dwarfexample (default is NO)" FALSE)
message(STATUS "Building dwarfexample... ${BUILD_DWARFEXAMPLE}")

option(DO_TESTING "Do certain api tests (default is NO)" FALSE)
message(STATUS "Building api tests   ... ${DOTESTS}")
### end what was configure.cmake

#  This changes the gennames option from -s  to -t
#  though in  2019 builds we no longer run gennames.
option(NAMES_TABLE "string functions implemented as binary search (default is with C switch)" TRUE)
if(NAMES_TABLE)
    set(dwarf_namestable "-s")
else()
    set(dwarf_namestable "-t")
endif()
    
option(HAVE_WINDOWS_PATH "Detect certain Windows paths as full paths (default is NO)" FALSE)
message(STATUS "Checking enable windows path... ${HAVE_WINDOWS_PATH}")
    
option(HAVE_OLD_FRAME_CFA_COL "Use HAVE_OLD_FRAME_CFA_COL (default is to use new DW_FRAME_CFA_COL3)" FALSE)
message(STATUS "Checking enable old frame columns... ${HAVE_OLD_FRAME_CFA_COL}")

#  See pro_init(), HAVE_DWARF2_99_EXTENSION also generates
#  32bit offset dwarf unless DW_DLC_OFFSET_SIZE_64 flag passed to
#  pro_init.
option(HAVE_SGI_IRIX_OFFSETS "Force producer to SGI IRIX offset dwarf" FALSE)
message(STATUS "Checking  producer generates SGI IRIX output... ${HAVE_SGI_IRIX_OFFSETS}")

option(HAVE_STRICT_DWARF2_32BIT_OFFSET "Force producer to generate only DWARF format 32bit" FALSE)
set(HAVE_DWARF2_99_EXTENSION NOT ${HAVE_STRICT_DWARF2_32BIT_OFFSET})
message(STATUS "Checking producer generates only 32bit... ${HAVE_STRICT_DWARF2_32BIT_OFFSET}")


# This references cmake/FindLibElf.cmake. See cmake documentation.
find_package(LibElf REQUIRED)
list(APPEND CMAKE_REQUIRED_INCLUDES ${LIBELF_INCLUDE_DIRS})

configure_file(config.h.in.cmake config.h)

if(BUILD_NON_SHARED)
	set(DWARF_TARGETS dwarf-static)
	set(DWARF_TYPES STATIC)
	set(dwarf-target dwarf-static)
endif()
if(BUILD_SHARED)
	list(APPEND DWARF_TARGETS dwarf-shared)
	list(APPEND DWARF_TYPES SHARED)
	set(dwarf-target dwarf-shared)
endif()

add_subdirectory(libdwarf)
add_subdirectory(dwarfdump)
if ( BUILD_DWARFGEN )
	if ( DWARF_WITH_LIBELF )
	    add_subdirectory(dwarfgen)
        else ()
            message(STATUS "Not building dwarfgen because libelf is not available" )
        endif ()
endif()
if ( BUILD_DWARFEXAMPLE )
	add_subdirectory(dwarfexample)
endif()

message(STATUS "Install prefix ... ${CMAKE_INSTALL_PREFIX}" )
add_custom_target(dd DEPENDS ${DWARF_TARGETS} dwarfdump)
