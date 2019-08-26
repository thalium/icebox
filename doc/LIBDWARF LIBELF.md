updated on April 2019
<br><br>

### libdwarf

download from https://www.prevanders.net/dwarf.html

Documentations (not last version of API) :
- http://netbsd.gw.com/cgi-bin/man-cgi?dwarf
- libdwarf-20190110/libdwarf/\*.pdf


/!\ Edits :
- libdwarf/config.h takes from retdec project (3.5)
- undefine HAVE_UNISTD_H in libdwarf/config.h
- add to libdwarf/config.h :
	- #define \_\_LIBELF_INTERNAL\_\_ 1
	- typedef long long ssize_t;
	- #ifdef \_\_linux__ #define HAVE___UINT64_T_IN_SYS_TYPES_H 1 #endif

<br><br>Note on this website :<br>
By mid-2019 (maybe earlier) the libdwarf functions dwarf_elf_init() and dwarf_elf_init_b() will return errors in all cases. Use dwarf_init(),dwarf_init_b(), or dwarf_init_path() instead (dwarf_init_path() was added in October 2018). This change allows us to delete use of libelf and Elf.h from libdwarf and dwarfdump and that deletion will simplify building libdwarf and dwarfdump for everyone.
<br><br>

### libelf

download from http://web.archive.org/web/20160505220855/http://www.mr511.de/software

/!\ Edits :
- lib/sys_elf.h and lib/config.h take from retdec project (3.5)
- delete reference to bcopy in lib/private.h

The website seems to be offline since the end of 2018.