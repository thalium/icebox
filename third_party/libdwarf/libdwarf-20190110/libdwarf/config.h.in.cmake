/* config.h.in.  Generated from configure.in by autoheader.  */

/* Define if building universal (internal helper macro) */
#cmakedefine AC_APPLE_UNIVERSAL_BUILD 1

/* Define to 1 if you have the <alloca.h> header file. */
#cmakedefine HAVE_ALLOCA_H 1

/* Define 1 if want to allow producer to build with 32/64bit section offsets
   per dwarf3 */
#cmakedefine HAVE_DWARF2_99_EXTENSION 1

/* Define to 1 if the elf64_getehdr function is in libelf.a. */
#cmakedefine HAVE_ELF64_GETEHDR 1

/* Define to 1 if the elf64_getshdr function is in libelf.a. */
#cmakedefine HAVE_ELF64_GETSHDR 1

/* Define 1 if Elf64_Rela defined. */
#cmakedefine HAVE_ELF64_RELA 1

/* Define 1 if Elf64_Sym defined. */
#cmakedefine HAVE_ELF64_SYM 1

/* Define to 1 if you have the <elfaccess.h> header file. */
#cmakedefine HAVE_ELFACCESS_H 1

/* Define to 1 if you have the <elf.h> header file. */
#cmakedefine HAVE_ELF_H 1

/* Define to 1 if you have the <sys/elf_86.h> header file. */
#cmakedefine HAVE_SYS_ELF_386_H 1

/* Define to 1 if you have the <sys/elf_amd64.h> header file. */
#cmakedefine HAVE_SYS_ELF_AMD64_H 1

/* Define to 1 if you have the <sys/elf_sparc.h> header file. */
#cmakedefine HAVE_SYS_ELF_SPARC_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <libelf.h> header file. */
#cmakedefine HAVE_LIBELF_H 1

/* Define to 1 if you have the <libelf/libelf.h> header file. */
#cmakedefine HAVE_LIBELF_LIBELF_H 1

/* Define 1 if off64 is defined via libelf with GNU_SOURCE. */
#cmakedefine HAVE_LIBELF_OFF64_OK 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define 1 if need nonstandard printf format for 64bit */
#cmakedefine HAVE_NONSTANDARD_PRINTF_64_FORMAT 1

/* Define 1 to default to old DW_FRAME_CFA_COL */
#cmakedefine HAVE_OLD_FRAME_CFA_COL 1

/* Define 1 if plain libelf builds. */
#cmakedefine HAVE_RAW_LIBELF_OK 1

/* Define 1 if R_IA_64_DIR32LSB is defined (might be enum value). */
#cmakedefine HAVE_R_IA_64_DIR32LSB 1

/* Define 1 if want producer to build with IRIX offset sizes */
#cmakedefine HAVE_SGI_IRIX_OFFSETS 1

/* Define 1 if we have the Windows specific header stdafx.h */
#cmakedefine HAVE_STDAFX_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define 1 if want producer to build with only 32bit section offsets */
#cmakedefine HAVE_STRICT_DWARF2_32BIT_OFFSET 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define 1 if libelf.h defines struct _Elf */
#cmakedefine HAVE_STRUCT_UNDERSCORE_ELF 1

/* Define to 1 if you have the <sys/ia64/elf.h> header file. */
#cmakedefine HAVE_SYS_IA64_ELF_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define 1 if __attribute__ ((unused)) compiles ok. */
#cmakedefine HAVE_UNUSED_ATTRIBUTE 1

/* Define 1 if want to allow Windows full path detection */
#cmakedefine HAVE_WINDOWS_PATH 1

/* Define 1 if zlib (decompression library) seems available. */
#cmakedefine HAVE_ZLIB 1

/* See if __uint32_t is predefined in the compiler. */
#cmakedefine HAVE___UINT32_T 1

/* Define 1 if __uint32_t is in sgidefs.h. */
#cmakedefine HAVE___UINT32_T_IN_SGIDEFS_H 1

/* Define 1 if sys/types.h defines __uint32_t. */
#cmakedefine HAVE___UINT32_T_IN_SYS_TYPES_H 1

/* See if __uint64_t is predefined in the compiler. */
#cmakedefine HAVE___UINT64_T 1

/* Define 1 if is in sgidefs.h. */
#cmakedefine HAVE___UINT64_T_IN_SGIDEFS_H 1

/* Define 1 if sys/types.h defines __uint64_t. */
#cmakedefine HAVE___UINT64_T_IN_SYS_TYPES_H 1

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT @PACKAGE_BUGREPORT@

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME @PACKAGE_NAME@

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING @PACKAGE_STRING@

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME @PACKAGE_TARNAME@

/* Define to the home page for this package. */
#cmakedefine PACKAGE_URL @PACKAGE_URL@

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION @PACKAGE_VERSION@

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#cmakedefine WORDS_BIGENDIAN 1
# endif
#endif
