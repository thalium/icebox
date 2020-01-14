# - Try to find libelf
# Referenced by the find_package() command from the top-level
# CMakeLists.txt.
# Once done this will define
#
#  LIBELF_FOUND - system has libelf
#  LIBELF_INCLUDE_DIRS - the libelf include directory
#  LIBELF_LIBRARIES - Link these to use libelf
#  LIBELF_DEFINITIONS - Compiler switches required for using libelf
#
# This module reads hints about search locations from variables:
#
#  LIBELF_ROOT - Preferred installation prefix
#
#  Copyright (c) 2008 Bernhard Walle <bernhard.walle@gmx.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (NOT DWARF_WITH_LIBELF)
	return()
endif ()

if (LIBELF_LIBRARIES AND LIBELF_INCLUDE_DIRS)
  set (LibElf_FIND_QUIETLY TRUE)
endif (LIBELF_LIBRARIES AND LIBELF_INCLUDE_DIRS)

find_path (LIBELF_INCLUDE_DIRS
  NAMES
    libelf/libelf.h libelf.h
  HINTS
    ${LIBELF_ROOT}
  PATH_SUFFIXES
    include
	libelf/include
)
if (NOT LIBELF_INCLUDE_DIRS)
	set (DWARF_WITH_LIBELF OFF)
	return ()
endif()

find_library (LIBELF_LIBRARIES
  NAMES
    elf libelf
  HINTS
    ${LIBELF_ROOT}
  PATH_SUFFIXES 
    lib
	libelf/lib
)
if (NOT LIBELF_LIBRARIES)
	 set (DWARF_WITH_LIBELF OFF)
	 return ()
endif()

include (FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set LIBELF_FOUND to TRUE if all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibElf DEFAULT_MSG
  LIBELF_LIBRARIES
  LIBELF_INCLUDE_DIRS)

set(CMAKE_REQUIRED_LIBRARIES elf)
include(CheckCSourceCompiles)
check_c_source_compiles("#include <libelf.h>
int main() {
  Elf *e = (Elf*)0;
  size_t sz;
  elf_getshdrstrndx(e, &sz);
  return 0;
}" ELF_GETSHDRSTRNDX)
unset(CMAKE_REQUIRED_LIBRARIES)

mark_as_advanced(LIBELF_INCLUDE_DIRS LIBELF_LIBRARIES ELF_GETSHDRSTRNDX)
