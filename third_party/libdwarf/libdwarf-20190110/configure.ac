###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com>
###This code is public domain and can be freely used or copied.

dnl defines the version name of  the libdwarf.so
m4_define([v_maj], [1])
m4_define([v_min], [0])
m4_define([v_mic], [0])
###m4_define([v_ver], [v_maj.v_min.v_mic])
###Returning to older version, .so.1
m4_define([v_ver], [v_maj])

m4_define([v_rel], [])
m4_define([lt_cur], [m4_eval(v_maj + v_min)])
m4_define([lt_rev], [v_mic])
m4_define([lt_age], [v_min])

### Sets the release name. 
###m4_define([v_date], [m4_esyscmd_s([date "+%Y%m%d"])])
m4_define([v_date], [20190110])])

AC_PREREQ([2.52])
### 2nd arg to AC_INIT is the version 'number'.
AC_INIT([libdwarf], [v_date], [libdwarf-list -at- linuxmail -dot- org])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])


### Additional options to configure

AC_ARG_ENABLE([dwarfgen],
   [AS_HELP_STRING([--enable-dwarfgen],
                   [enable dwarfgen compilation @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_dwarfgen="yes"],
        [enable_dwarfgen="no"])
   ],
   [enable_dwarfgen="no"])

AM_CONDITIONAL([HAVE_DWARFGEN], [test "x${enable_dwarfgen}" = "xyes"])

AC_ARG_ENABLE([dwarfexample],
   [AS_HELP_STRING([--enable-dwarfexample],
                   [enable dwarfexample compilation @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_dwarfexample="yes"],
        [enable_dwarfexample="no"])
   ],
   [enable_dwarfexample="no"])

AM_CONDITIONAL([HAVE_DWARFEXAMPLE], [test "x${enable_dwarfexample}" = "xyes"])

AC_ARG_ENABLE([sanitize],
   [AS_HELP_STRING([--enable-sanitize],
                   [enable sanitize compiler option @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_sanitize="yes"],
        [enable_sanitize="no"])
   ],
   [enable_sanitize="no"])

AC_ARG_ENABLE([oldframecol],
   [AS_HELP_STRING([--enable-oldframecol],
                   [enable old frame columns @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_oldframecol="yes"],
        [enable_oldframecol="no"])
   ],
   [enable_oldframecol="no"])

AS_IF(
    [test "x${enable_oldframecol}" = "xyes"],
    [AC_DEFINE(
         [HAVE_OLD_FRAME_CFA_COL], [1],
         [Set to 1 if old frame columns are enabled.])])

AC_ARG_ENABLE([namestable],
   [AS_HELP_STRING([--enable-namestable],
                   [enable name string functions implemented as binary search (default is with C switch) @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_namestable="yes"],
        [enable_namestable="no"])
   ],
   [enable_namestable="no"])

AS_IF(
    [test "x${enable_namestable}" = "xyes"],
    [AC_SUBST([dwarf_namestable], [-s])],
    [AC_SUBST([dwarf_namestable], [-t])])

AC_ARG_ENABLE([windowspath],
   [AS_HELP_STRING([--enable-windowspath],
                  [Detect certain Windows paths as full paths (default is NO)])],
   [ 
     AC_DEFINE([HAVE_WINDOWS_PATH],[1],
       [Define 1 if want to allow Windows full path detection] )
       [enable_windowspath="yes"]
   ],
   [ 
     enable_windowspath="no"
   ])

AC_ARG_ENABLE([wall],
   [AS_HELP_STRING([--enable-wall],
                   [enable -Wall and other options @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_wall="yes"],
        [enable_wall="no"])
   ],
   [enable_wall="no"])

AC_ARG_ENABLE(nonstandardprintf,AS_HELP_STRING([--enable-nonstandardprintf],
        [Use a special printf format for 64bit (default is NO)]),
        [ AC_DEFINE([HAVE_NONSTANDARD_PRINTF_64_FORMAT],[1],
           [Define 1 if need nonstandard printf format for  64bit] )
           [enable_nonstandardprintf="yes"]
        ],
        [enable_nonstandardprintf="no"])




### Default options with respect to host

AC_CANONICAL_HOST

have_win32="no"
case "$host_os" in
   mingw*)
      have_win32="yes"
   ;;
esac

AM_CONDITIONAL([HAVE_WIN32], [test "x${have_win32}" = "xyes"])


### Checks for programs

AM_PROG_AR

AM_INIT_AUTOMAKE([1.6 dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

LT_INIT([win32-dll disable-shared static])

version_info="lt_cur:lt_rev:lt_age"
release_info="v_rel"
AC_SUBST([version_info])
AC_SUBST([release_info])

AC_PROG_CC
AX_PROG_CC_FOR_BUILD
AC_PROG_CXX
AC_PROG_CC_C_O

###PKG_PROG_PKG_CONFIG Intentionally not using pkg-config

### Checks for libraries

AC_SUBST([DWARF_LIBS])
AC_ARG_VAR([DWARF_LIBS], [linker flags when linking libdwarf])


### Checks for header files

### MacOS does not have malloc.h
AC_CHECK_HEADERS([unistd.h sys/types.h regex.h malloc.h])
AC_CHECK_HEADERS([sgidefs.h])
AC_CHECK_HEADERS([windows.h])

# test Elf headers in the preprocessor path search

CPPFLAGS_save=${CPPFLAGS}

# test libelf.h header. exits if not found
AC_CHECK_HEADERS([libelf.h libelf/libelf.h])
AS_IF(
    [test "x${ac_cv_header_libelf_h}" != "xyes" -a "x${ac_cv_header_libelf_libelf_h}" != "xyes"],
    [AC_MSG_ERROR([libelf.h not found, exiting...])])

AC_CHECK_HEADERS([zlib.h])
AS_IF(
    [test "x${ac_cv_header_zlib_h}" = "xyes"],
    [ have_pc_zlib="yes" ; echo "have zlib" ],
    [ have_pc_zlib="no" ; echo "no zlib" ])

AC_CHECK_HEADERS([elf.h elfaccess.h sys/elf_386.h sys/elf_amd64.h sys/elf_SPARC.h sys/ia64/elf.h])

CPPFLAGS=${CPPFLAGS_save}

## test if zlib.h is in the preprocessor path search
#AS_IF(
#    [test "x${have_pc_zlib}" = "xno"],
#    [AC_CHECK_HEADERS([zlib.h], [have_zlib="yes"], [have_zlib="no"])])

### Using header setings, add to DWARF_LIBS

AS_IF(
    [test "x${have_pc_zlib}" = "xyes"],
    [DWARF_LIBS="${DWARF_LIBS} -lelf -lz"],
    [DWARF_LIBS="${DWARF_LIBS} -lelf"])
    


### Checks for types

# __uint32_t

AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
         [[
         ]],
         [[
__uint32_t p = 2;
         ]])
    ],
    [
     have___uint32_t="yes"
     AC_DEFINE(
         [HAVE___UINT32_T], [1],
         [Set to 1 if the OS defines __uint32_t.])
    ],
    [have___uint32_t="no"])

AC_MSG_CHECKING([for __uint32_t is defined by the OS])
AC_MSG_RESULT([${have___uint32_t}])

AS_IF(
    [test "x${have___uint32_t}" = "xno"],
    [
     AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM(
              [[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
              ]],
              [[
__uint32_t p = 2;
              ]])
         ],
         [
          have___uint32_t="yes"
          AC_DEFINE(
              [HAVE___UINT32_T_IN_SYS_TYPES_H], [1],
	      [Set to 1 if sys/types.h defines __uint32_t.])
         ],
         [have___uint32_t="no"])

     AC_MSG_CHECKING([for __uint32_t in sys/types.h])
     AC_MSG_RESULT([${have___uint32_t}])
    ])
AS_IF(
    [test "x${have___uint32_t}" = "xno"],
    [
     AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM(
              [[
#ifdef HAVE_SGIDEFS_H
# include <sgidefs.h>
#endif
              ]],
              [[
__uint32_t p = 2;
              ]])
         ],
         [
          have___uint32_t="yes"
          AC_DEFINE(
              [HAVE___UINT32_T_IN_SGIDEFS_H], [1],
	      [Set to 1 if sgidefs.h defines __uint32_t.])
         ],
         [have___uint32_t="no"])

     AC_MSG_CHECKING([for __uint32_t in sgidefs.h])
     AC_MSG_RESULT([${have___uint32_t}])
    ])

# __uint64_t

AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
         [[
         ]],
         [[
__uint64_t p = 2;
         ]])
    ],
    [
     have___uint64_t="yes"
     AC_DEFINE(
         [HAVE___UINT64_T], [1],
         [Set to 1 if the OS defines __uint64_t.])
    ],
    [have___uint64_t="no"])

AC_MSG_CHECKING([for __uint64_t is defined by the OS])
AC_MSG_RESULT([${have___uint64_t}])

AS_IF(
    [test "x${have___uint64_t}" = "xno"],
    [
     AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM(
              [[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
              ]],
              [[
__uint64_t p = 2;
              ]])
         ],
         [
          have___uint64_t="yes"
          AC_DEFINE(
              [HAVE___UINT64_T_IN_SYS_TYPES_H], [1],
	      [Set to 1 if sys/types.h defines __uint64_t.])
         ],
         [have___uint64_t="no"])

     AC_MSG_CHECKING([for __uint64_t in sys/types.h])
     AC_MSG_RESULT([${have___uint64_t}])
    ])
AS_IF(
    [test "x${have___uint64_t}" = "xno"],
    [
     AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM(
              [[
#ifdef HAVE_SGIDEFS_H
# include <sgidefs.h>
#endif
              ]],
              [[
__uint64_t p = 2;
              ]])
         ],
         [
          have___uint64_t="yes"
          AC_DEFINE(
              [HAVE___UINT64_T_IN_SGIDEFS_H], [1],
	      [Set to 1 if sgidefs.h defines __uint64_t.])
         ],
         [have___uint64_t="no"])

     AC_MSG_CHECKING([for __uint64_t in sgidefs.h])
     AC_MSG_RESULT([${have___uint64_t}])
    ])

# Elf64_Rela in elf.h
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
         [[
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LIBELF_H
# include <libelf.h>
#endif
#ifdef HAVE_LIBELF_LIBELF_H
# include <libelf/libelf.h>
#endif
         ]],
         [[
Elf64_Rela p;
p.r_offset = 1;
         ]])
    ],
    [
     AC_DEFINE([HAVE_ELF64_RELA], [1], [Set to 1 if Elf64_Rela defined in elf.h.])
     have_elf64_rela="yes"
    ],
    [have_elf64_rela="no"])

AS_IF(
    [test "x${have_elf64_rela}" = "xno"],
    [
     CPPFLAGS_save=${CPPFLAGS}
     CPPFLAGS="${CPPFLAGS} -D__LIBELF64"
     AC_LINK_IFELSE(
         [AC_LANG_PROGRAM(
              [[
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LIBELF_H
# include <libelf.h>
#endif
#ifdef HAVE_LIBELF_LIBELF_H
# include <libelf/libelf.h>
#endif
              ]],
              [[
Elf64_Rela p;
p.r_offset = 1;
              ]])
         ],
         [
          AC_DEFINE([HAVE_ELF64_RELA], [1], [Set to 1 if Elf64_Rela defined in elf.h.])
          have_elf64_rela="yes"
         ],
         [have_elf64_rela="no"])
     CPPFLAGS=${CPPFLAGS_save}
    ])

AC_MSG_CHECKING([for Elf64_Rela in elf.h])
AC_MSG_RESULT([${have_elf64_rela}])

# Elf64_Rel in elf.h
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
         [[
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LIBELF_H
# include <libelf.h>
#endif
#ifdef HAVE_LIBELF_LIBELF_H
# include <libelf/libelf.h>
#endif
         ]],
         [[
Elf64_Rel p;
p.r_info = 1;
         ]])
    ],
    [
     AC_DEFINE([HAVE_ELF64_R_INFO], [1], [Set to 1 if Elf64_Rel structure as r_info field.])
     have_elf64_rel="yes"
    ],
    [have_elf64_rel="no"])

AS_IF(
    [test "x${have_elf64_rel}" = "xno"],
    [
     CPPFLAGS_save=${CPPFLAGS}
     CPPFLAGS="${CPPFLAGS} -D__LIBELF64"
     AC_LINK_IFELSE(
         [AC_LANG_PROGRAM(
              [[
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LIBELF_H
# include <libelf.h>
#endif
#ifdef HAVE_LIBELF_LIBELF_H
# include <libelf/libelf.h>
#endif
              ]],
              [[
Elf64_Rel p;
p.r_info = 1;
              ]])
         ],
         [
          AC_DEFINE([HAVE_ELF64_R_INFO], [1], [Set to 1 if Elf64_Rel structure as r_info field.])
          have_elf64_rel="yes"
         ],
         [have_elf64_rel="no"])
     CPPFLAGS=${CPPFLAGS_save}
    ])

AC_MSG_CHECKING([for Elf64_Rel in elf.h])
AC_MSG_RESULT([${have_elf64_rel}])

# Elf64_Sym in elf.h
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
         [[
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LIBELF_H
# include <libelf.h>
#endif
#ifdef HAVE_LIBELF_LIBELF_H
# include <libelf/libelf.h>
#endif
         ]],
         [[
Elf64_Sym p;
p.st_info = 1;
         ]])
    ],
    [
     AC_DEFINE([HAVE_ELF64_SYM], [1], [Set to 1 if Elf64_Sym defined in elf.h.])
     have_elf64_sym="yes"
    ],
    [have_elf64_sym="no"])

AS_IF(
    [test "x${have_elf64_sym}" = "xno"],
    [
     CPPFLAGS_save=${CPPFLAGS}
     CPPFLAGS="${CPPFLAGS} -D__LIBELF64"
     AC_LINK_IFELSE(
         [AC_LANG_PROGRAM(
              [[
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LIBELF_H
# include <libelf.h>
#endif
#ifdef HAVE_LIBELF_LIBELF_H
# include <libelf/libelf.h>
#endif
              ]],
              [[
Elf64_Sym p;
p.st_info = 1;
              ]])
         ],
         [
          AC_DEFINE([HAVE_ELF64_SYM], [1], [Set to 1 if Elf64_Sym defined in elf.h.])
          have_elf64_sym="yes"
         ],
         [have_elf64_sym="no"])
     CPPFLAGS=${CPPFLAGS_save}
    ])

AC_MSG_CHECKING([for Elf64_Sym in elf.h])
AC_MSG_RESULT([${have_elf64_sym}])


### Checks for structures

# test if struct _Elf is used instead of struct Elf
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
         [[
#ifdef HAVE_LIBELF_H
# include <libelf.h>
#endif
#ifdef HAVE_LIBELF_LIBELF_H
# include <libelf/libelf.h>
#endif
/* This must be at global scope */
struct _Elf;
typedef struct _Elf Elf;
         ]],
         [[
struct _Elf *a = 0;
         ]])
    ],
    [
     have_struct__elf="yes"
     struct_elf="struct _Elf"
    ],
    [
     have_struct__elf="no"
     struct_elf="struct Elf"
    ])

AC_MSG_CHECKING([whether struct _Elf is used])
AC_MSG_RESULT([${have_struct__elf}])

AC_SUBST([struct_elf])


### Checks for compiler characteristics

AC_C_BIGENDIAN

# gcc accepts even totally bogus -Wno flags. Other compilers..no
# -Wno-long-long suppresses warnings on 'long long'
# -Wno-pedantic-ms-format
# compiler flags
AS_IF(
    [ test "x$enable_wall" = "xyes" ],
    [ 
       cxx_compiler_flags="-Wall -Wextra -Wpointer-arith -Wmissing-declarations -Wcomment -Wformat -Wpedantic -Wuninitialized -Wshadow -Wno-long-long"

       c_compiler_flags="${cxx_compiler_flags} -Wmissing-prototypes -Wdeclaration-after-statement -Wbad-function-cast -Wmissing-parameter-type -Wnested-externs"
    ]
    ,
    [
       c_compiler_flags=""
       cxx_compiler_flags=""
    ]
    )

DWARF_CHECK_CXX_COMPILER_FLAGS([${cxx_compiler_flags}])
DWARF_CHECK_C_COMPILER_FLAGS([${c_compiler_flags}])

# unused attribute
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
         [[
static unsigned int foo(unsigned int x, __attribute__ ((unused)) int y){
    unsigned int x2 = x + 1;
    return x2;
}
int goo() {
    unsigned int y = 0;
    y = foo(12, y);
}
         ]],
         [[
         ]])
    ],
    [
     have_unused="yes"
     AC_DEFINE(
         [HAVE_UNUSED_ATTRIBUTE], [1],
         [Set to 1 if __attribute__ ((unused)) is available.])
    ],
    [have_unused="no"])

AC_MSG_CHECKING([whether "unused" attribute is available])
AC_MSG_RESULT([${have_unused}])

# sanitize
AS_IF(
    [test "x${enable_sanitize}" = "xyes"],
    [
     CFLAGS_save=${CFLAGS}
     CFLAGS="${CFLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined"
     DWARF_CFLAGS=
     AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[]], [[]])],
         [
          enable_sanitize="yes"
          DWARF_CFLAGS="$DWARF_CFLAGS -fsanitize=address -fsanitize=leak -fsanitize=undefined"
          LDFLAGS="$LDFLAGS -fsanitize=address -fsanitize=leak -fsanitize=undefined"
         ],
         [enable_sanitize="no"])
     CFLAGS="${CFLAGS_save} ${DWARF_CFLAGS}" 
     DWARF_CFLAGS=
    ])

AC_MSG_CHECKING([whether sanitize options are used])
AC_MSG_RESULT([${enable_sanitize}])


### Checks for linker characteristics


### Checks for library functions

AC_FUNC_ALLOCA

# elf64_getehdr
CPPFLAGS_save=${CPPFLAGS}
CPPFLAGS="${CPPFLAGS}"
LIBS_save=${LIBS}
LIBS="${LIBS} ${DWARF_LIBS}"
AC_SEARCH_LIBS(
    [elf64_getehdr], [elf],
    [
     have_getehdr="yes"
     AC_DEFINE(
         [HAVE_ELF64_GETEHDR], [1],
	 [Set to 1 if the elf64_getehdr function is in libelf.])
     ],
     [have_getehdr="no"])
AC_SEARCH_LIBS(
    [elf64_getshdr], [elf],
    [
     have_getshdr="yes"
     AC_DEFINE(
         [HAVE_ELF64_GETSHDR], [1],
	 [Set to 1 if the elf64_getshdr function is in libelf.])
     ],
     [have_getshdr="no"])
CPPFLAGS=${CPPFLAGS_save}
LIBS=${LIBS_save}

AS_IF(
    [test "x${have_pc_zlib}" = "xno"  -a "x${have_zlib}" = "xyes"],
    [AC_SEARCH_LIBS([z], [have_zlib="yes"], [have_zlib="no"])])

AS_IF(
    [test "x${have_pc_zlib}" = "xyes"  -o "x${have_zlib}" = "xyes"],
    [
     have_zlib="yes"
     AC_DEFINE([HAVE_ZLIB], [1], [Set to 1 if zlib decompression is available.])
    ],
    [
     have_zlib="no"
    ])

AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
         [[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_REGEX_H
# include <regex.h>
#endif
         ]],
         [[
int i;
regex_t r;
int cflags = REG_EXTENDED;
const char *s = "abc";
i = regcomp(&r,s,cflags);
regfree(&r);
         ]])
    ],
    [
     AC_DEFINE([HAVE_REGEX], [1], [Set to 1 if regex is usable.])
     have_regex="yes"
    ],
    [have_regex="no"])

AC_MSG_CHECKING([for regex library])
AC_MSG_RESULT([${have_regex}])

### Checks for system services

AC_CONFIG_FILES([
Makefile
libdwarf/Makefile
dwarfdump/Makefile
dwarfgen/Makefile
dwarfexample/Makefile
])

### libdwarf needs to be adjusted to support struct Elf
### or struct _Elf, whichever the system defines in libelf.
AC_CONFIG_COMMANDS([libdwarf/libdwarf.h],
  [ sh $src/scripts/fixlibdwarfelf.sh $haself $src $top ],
  [ haself=${have_struct__elf}  top=${ac_pwd}   src=${srcdir} ]  )


AC_OUTPUT

echo
echo "$PACKAGE $VERSION"
echo
echo "Configuration Options Summary:"
echo
echo "  BuildOS..............: ${build_os}"
echo "  HostOS...............: ${host_os}"
echo
echo "  shared library.......: ${enable_shared}"
echo "  static library.......: ${enable_static}"
echo
echo "  zlib support.........: ${have_zlib}"
echo "  sanitize support.....: ${enable_sanitize}"
echo
echo "  libdwarf.............: always"
echo "    old frame column...: ${enable_oldframecol}"
echo "    names table........: ${enable_namestable}"
echo "    elf64_getehdr......: ${have_getehdr}"
echo "    elf64_getshdr......: ${have_getshdr}"
echo "    Elf64_Rela.........: ${have_elf64_rela}"
echo "    Elf64_Sym..........: ${have_elf64_sym}"
echo "    Elf spelled........: ${struct_elf}"
echo "    Windows path corr..: ${enable_windowspath}"
echo "    Nonstandardprintf..: ${enable_nonstandardprintf}"
echo "  dwarfdump............: always"
echo "    elf64_getehdr......: ${have_getehdr}"
echo "    Elf64_Rel (r_info).: ${have_elf64_rel}"
echo "    regex..............: ${have_regex}"
echo "  dwarfgen.............: ${enable_dwarfgen}"
echo "  dwarfexample.........: ${enable_dwarfexample}"
echo
echo "Compilation............: make (or gmake)"
echo "  CPPFLAGS.............: $CPPFLAGS"
echo "  CFLAGS...............: $CFLAGS ${c_compiler_flags}"
echo "  LDFLAGS..............: $LDFLAGS"
echo "  LIBS.................: $LIBS"
echo "  DWARF_LIBS used in linking libdwarf"
echo "  DWARF_LIBS...........: $DWARF_LIBS"
echo "  *FOR_BUILD used building executables built and run to generate" 
echo "  C source. Motivated by cross-compilation. "
echo "  Executables not part of the final products."
echo "  CPPFLAGS_FOR_BUILD...: $CPPFLAGS_FOR_BUILD"
echo "  CFLAGS_FOR_BUILD.....: $CFLAGS_FOR_BUILD ${c_compiler_flags}"
echo "  LDFLAGS_FOR_BUILD....: $LDFLAGS_FOR_BUILD"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo
