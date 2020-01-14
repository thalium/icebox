###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com
###This code is public domain and can be freely used or copied.

MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = dwarfdump

dwarfdump_SOURCES = \
addrmap.c \
addrmap.h \
checkutil.c \
checkutil.h \
command_options.c \
command_options.h \
common.c \
common.h \
compiler_info.c \
compiler_info.h \
defined_types.h \
dwarfdump.c \
dwarfdump-tt-table.h\
dwarfdump-ta-table.h\
dwarfdump-tt-ext-table.h\
dwarfdump-ta-ext-table.h\
dwarf_names.c \
dwarf_names.h \
dwarf_tsearchbal.c \
dwarf_tsearch.h \
dwconf.c \
dwconf.h \
dwconf_using_functions.h \
dwgetopt.c \
dwgetopt.h \
esb.c \
esb.h \
esb_using_functions.h \
glflags.c \
glflags.h \
globals.h \
helpertree.c \
helpertree.h \
macrocheck.c \
macrocheck.h \
makename.c \
makename.h \
naming.c \
naming.h \
print_abbrevs.c \
print_aranges.c \
print_debugfission.c \
print_die.c \
print_dnames.c \
print_frames.c \
print_frames.h \
print_gdbindex.c \
print_lines.c \
print_locs.c \
print_macro.c \
print_macros.c \
print_pubnames.c \
print_ranges.c \
print_reloc.c \
print_reloc.h \
print_section_groups.c \
print_sections.c \
print_sections.h \
print_static_funcs.c \
print_static_vars.c \
print_str_offsets.c \
print_strings.c \
print_types.c \
print_weaknames.c \
sanitized.c \
sanitized.h \
section_bitmaps.c \
section_bitmaps.h \
strstrnocase.c \
true_section_name.c \
tag_common.h \
uri.c \
uri.h \
warningcontrol.h

dwarfdump_CPPFLAGS =  -I$(top_srcdir)/libdwarf \
-I$(top_builddir)/libdwarf

dwarfdump_CFLAGS = $(DWARF_CFLAGS_WARN) "-DCONFPREFIX=@prefix@/share/dwarfdump" 

dwarfdump_LDADD = \
$(top_builddir)/libdwarf/libdwarf.la \
@DWARF_LIBS@

TESTS = runtests.sh
AM_TESTS_ENVIRONMENT = DWTOPSRCDIR='$(top_srcdir)'; export DWTOPSRCDIR;

man_MANS = dwarfdump.1

dwarfdumpdevdir = $(datadir)/dwarfdump
dwarfdumpdev_DATA = dwarfdump.conf

EXTRA_DIST = \
$(man_MANS) \
COPYING \
DWARFDUMPCOPYRIGHT \
GPL.txt \
ChangeLog \
ChangeLog2006 \
ChangeLog2007 \
ChangeLog2008 \
ChangeLog2009 \
ChangeLog2010 \
ChangeLog2011 \
ChangeLog2012 \
ChangeLog2013 \
ChangeLog2014 \
ChangeLog2015 \
ChangeLog2016 \
ChangeLog2017 \
ChangeLog2018 \
CMakeLists.txt \
NEWS \
README \
CODINGSTYLE \
print_reloc_decls.h \
makename_test.c  \
section_bitmaps_test.c \
helpertree_test.c \
print_reloc_test.c \
getopttest.c \
tag_common.c \
testesb.c \
test-mach-o-32.dSYM \
test-mach-o-32.base \
testobjLE32PE.exe \
testuriLE64ELf.obj \
testobjLE32PE.base \
testuriLE64ELf.base \
$(dwarfdumpdev_DATA) \
tag_attr.c \
tag_tree.c \
tag_attr.list \
tag_attr_ext.list \
tag_tree.list \
tag_tree_ext.list

CLEANFILES = 

