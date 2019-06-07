###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com
###This code is public domain and can be freely used or copied.

MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = dwarfdump

noinst_PROGRAMS = tag_attr_build tag_tree_build

### tag_attr_build

dwarf_names.c: 
	cp $(top_builddir)/libdwarf/dwarf_names.c .

tag_attr_build_SOURCES = \
dwarf_names.c \
common.c \
common.h \
dwarf_tsearchbal.c \
dwarf_tsearch.h \
dwgetopt.c \
dwgetopt.h \
esb.c \
esb.h \
makename.c \
makename.h \
naming.c \
naming.h \
sanitized.c \
sanitized.h \
tag_attr.c \
tag_common.c \
tag_common.h

tag_attr_build_CFLAGS = $(DWARF_CFLAGS_WARN)
tag_attr_build_CPPFLAGS = -DTRIVIAL_NAMING \
-I$(top_builddir) \
-I$(top_srcdir)/libdwarf  \
-I$(top_builddir)/libdwarf

$(tag_attr_build_OBJECTS):CC=$(CC_FOR_BUILD)
$(tag_attr_build_OBJECTS):CFLAGS=$(CFLAGS_FOR_BUILD)
$(tag_attr_build_OBJECTS):CPPFLAGS=$(CPPFLAGS_FOR_BUILD)
tag_attr_build$(BUILD_EXEEXT):LDFLAGS=$(LDFLAGS_FOR_BUILD)
tag_attr_build$(BUILD_EXEEXT):CC=$(CC_FOR_BUILD)
tag_attr_build$(BUILD_EXEEXT):CFLAGS=$(CFLAGS_FOR_BUILD)
tag_attr_build$(BUILD_EXEEXT):CPPFLAGS=$(CPPFLAGS_FOR_BUILD)

### tag_tree_build

tag_tree_build_SOURCES = \
dwarf_names.c \
common.c \
common.h \
dwarf_tsearchbal.c \
dwarf_tsearch.h \
dwgetopt.c \
dwgetopt.h \
esb.c \
esb.h \
makename.c \
makename.h \
sanitized.c \
sanitized.h \
tag_common.c \
tag_common.h \
tag_tree.c

tag_tree_build_CFLAGS = $(DWARF_CFLAGS_WARN) 

tag_tree_build_CPPFLAGS = @CPPFLAGS@ -DTRIVIAL_NAMING \
-I$(top_builddir) \
-I$(top_srcdir)/libdwarf \
-I$(top_builddir)/libdwarf

$(tag_tree_build_OBJECTS):CC=$(CC_FOR_BUILD) 
$(tag_tree_build_OBJECTS):CFLAGS=$(CFLAGS_FOR_BUILD)
$(tag_tree_build_OBJECTS):CPPFLAGS=$(CPPFLAGS_FOR_BUILD)
tag_tree_build$(BUILD_EXEEXT):CC=$(CC_FOR_BUILD)
tag_tree_build$(BUILD_EXEEXT):LDFLAGS=$(LDFLAGS_FOR_BUILD)
tag_tree_build$(BUILD_EXEEXT):CFLAGS=$(CFLAGS_FOR_BUILD)
tag_tree_build$(BUILD_EXEEXT):CPPFLAGS=$(CPPFLAGS_FOR_BUILD)


BUILT_SOURCES = \
tmp-ta-table.h \
tmp-ta-ext-table.h \
tmp-tt-table.h \
tmp-tt-ext-table.h

gn_verbose = $(gn_verbose_@AM_V@)
gn_verbose_ = $(gn_verbose_@AM_DEFAULT_V@)
gn_verbose_0 = @echo "  GEN     " $@;

tag_attr.list:
tag_attr_ext.list:
tag_tree.list:
tag_tree_ext.list:

tmp-tt-table.h: tag_tree.list tag_tree_build$(BUILD_EXEEXT)
	$(AM_V_at)rm -f tmp-t1.c
	$(AM_V_at)cp $(top_srcdir)/dwarfdump/tag_tree.list tmp-t1.c
	$(AM_V_at)$(CC) $(CFLAGS) $(CPPFLAGS) -I$(top_srcdir)/libdwarf -E tmp-t1.c > tmp-tag-tree-build1.tmp
	$(gn_verbose)./tag_tree_build$(BUILD_EXEEXT) -s -i tmp-tag-tree-build1.tmp -o tmp-tt-table.h

tmp-ta-table.h: tag_attr.list tag_attr_build$(BUILD_EXEEXT)
	$(AM_V_at)rm -f tmp-t2.c
	$(AM_V_at)cp $(top_srcdir)/dwarfdump/tag_attr.list tmp-t2.c
	$(AM_V_at)$(CC) $(CFLAGS) $(CPPFLAGS) -I$(top_srcdir)/libdwarf  -E tmp-t2.c > tmp-tag-attr-build2.tmp
	$(gn_verbose)./tag_attr_build$(BUILD_EXEEXT) -s -i tmp-tag-attr-build2.tmp -o tmp-ta-table.h

tmp-ta-ext-table.h: tag_attr_ext.list tag_attr_build$(BUILD_EXEEXT)
	$(AM_V_at)rm -f tmp-t3.c
	$(AM_V_at)cp $(top_srcdir)/dwarfdump/tag_attr_ext.list tmp-t3.c
	$(AM_V_at)$(CC) $(CFLAGS) $(CPPFLAGS) -I$(top_srcdir)/libdwarf -E tmp-t3.c > tmp-tag-attr-build3.tmp
	$(gn_verbose)./tag_attr_build$(BUILD_EXEEXT) -e -i tmp-tag-attr-build3.tmp -o tmp-ta-ext-table.h

tmp-tt-ext-table.h: tag_tree_ext.list tag_tree_build$(EXEEXT)
	$(AM_V_at)rm -f tmp-t4.c
	$(AM_V_at)cp $(top_srcdir)/dwarfdump/tag_tree_ext.list tmp-t4.c
	$(AM_V_at)$(CC) $(CFLAGS) $(CPPFLAGS) -I$(top_srcdir)/libdwarf  -E tmp-t4.c > tmp-tag-tree-build4.tmp
	$(gn_verbose)./tag_tree_build$(BUILD_EXEEXT) -e -i tmp-tag-tree-build4.tmp -o tmp-tt-ext-table.h

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
uri.c \
uri.h \
warningcontrol.h

dwarfdump_CPPFLAGS =  -I$(top_srcdir)/libdwarf \
-I$(top_builddir)/libdwarf

dwarfdump_CFLAGS = $(DWARF_CFLAGS_WARN) "-DCONFPREFIX=@prefix@/share/dwarfdump" 

dwarfdump_LDADD = \
$(top_builddir)/libdwarf/libdwarf.la \
@DWARF_LIBS@

TESTS = ./runtests.sh
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
getopttest.c \
testesb.c \
configure.cmake \
config.h.in.cmake \
$(dwarfdumpdev_DATA) \
tag_attr.list \
tag_attr_ext.list \
tag_tree.list \
tag_tree_ext.list

CLEANFILES = \
dwarf_names.c \
tmp-ta-table.h \
tmp-ta-ext-table.h \
tmp-tt-table.h \
tmp-tt-ext-table.h \
tmp-t1.c \
tmp-t2.c \
tmp-t3.c \
tmp-t4.c \
tmp-tag-tree-build1.tmp \
tmp-tag-attr-build2.tmp \
tmp-tag-attr-build3.tmp \
tmp-tag-tree-build4.tmp

