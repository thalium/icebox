cmake_minimum_required(VERSION 3.2)

project(libdwarf)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(LibdwarfMacros)

# view folders on supported IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# used when finding libelf
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)

# this project has tests
enable_testing()

# always include project's folder to includes
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

find_package(LibElf REQUIRED)

list(APPEND CMAKE_REQUIRED_INCLUDES ${LIBELF_INCLUDE_DIRS})

include(configure.cmake)

if(nonshared)
	set(DWARF_TARGETS dwarf-static)
	set(DWARF_TYPES STATIC)
	set(dwarf-target dwarf-static)
endif()
if(shared)
	list(APPEND DWARF_TARGETS dwarf-shared)
	list(APPEND DWARF_TYPES SHARED)
	set(dwarf-target dwarf-shared)
endif()

add_subdirectory(libdwarf)
add_subdirectory(dwarfexample)
add_subdirectory(dwarfdump)
add_subdirectory(dwarfgen)

add_custom_target(dd
	DEPENDS ${DWARF_TARGETS} dwarfdump)