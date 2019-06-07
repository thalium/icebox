/* config.h.in.  Generated from configure.in by autoheader.  */

/* Define if building universal (internal helper macro) */
#cmakedefine AC_APPLE_UNIVERSAL_BUILD 1

/* Define to 1 if you have the <elf.h> header file. */
#cmakedefine HAVE_ELF_H 1

/* Define 1 if intptr_t defined in C99 stdint.h */
#cmakedefine HAVE_INTPTR_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <libelf.h> header file. */
#cmakedefine HAVE_LIBELF_H 1

/* Define to 1 if you have the <libelf/libelf.h> header file. */
#cmakedefine HAVE_LIBELF_LIBELF_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the <sgidefs.h> header file. */
#cmakedefine HAVE_SGIDEFS_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define 1 if zlib (decompression library) seems available. */
#cmakedefine HAVE_ZLIB 1

/* Define 1 if __uint32_t is in sgidefs.h. */
#cmakedefine HAVE___UINT32_T_IN_SGIDEFS_H 1

/* Define 1 if is in sgidefs.h. */
#cmakedefine HAVE___UINT64_T_IN_SGIDEFS_H 1

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
