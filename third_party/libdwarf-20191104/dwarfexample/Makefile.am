###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com
###This code is public domain and can be freely used or copied.

MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = simplereader frame1 findfuncbypc

simplereader_SOURCES = simplereader.c
simplereader_CPPFLAGS = -I$(top_srcdir)/libdwarf \
  -I$(top_builddir)/libdwarf
simplereader_CFLAGS = $(DWARF_CFLAGS_WARN)
simplereader_LDADD = $(top_builddir)/libdwarf/libdwarf.la \
$(DWARF_LIBS)

frame1_SOURCES = frame1.c
frame1_CPPFLAGS = -I$(top_srcdir)/libdwarf \
  -I$(top_builddir)/libdwarf
frame1_CFLAGS = $(DWARF_CFLAGS_WARN)
frame1_LDADD = $(top_builddir)/libdwarf/libdwarf.la \
$(DWARF_LIBS)

findfuncbypc_SOURCES = findfuncbypc.c
findfuncbypc_CPPFLAGS = -I$(top_srcdir)/libdwarf \
  -I$(top_builddir)/libdwarf
findfuncbypc_CFLAGS = $(DWARF_CFLAGS_WARN)
findfuncbypc_LDADD = $(top_builddir)/libdwarf/libdwarf.la \
$(DWARF_LIBS)

EXTRA_DIST = \
ChangeLog \
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
$(dwarfexample_DATA)
