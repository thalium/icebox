###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com
###This code is public domain and can be freely used or copied.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libdwarf dwarfdump
if HAVE_DWARFGEN
SUBDIRS += dwarfgen
endif
if HAVE_DWARFEXAMPLE
SUBDIRS += dwarfexample
endif

DISTCLEANFILES = \
libdwarf/libdwarf.h

MAINTAINERCLEANFILES = \
Makefile.in \
aclocal.m4 \
ar-lib \
compile \
config.guess \
config.h.in \
config.h.in~ \
config.sub \
configure \
depcomp \
install-sh \
ltmain.sh \
m4/libtool.m4 \
m4/ltoptions.m4 \
m4/ltsugar.m4 \
m4/ltversion.m4 \
m4/lt~obsolete.m4 \
missing \
$(distdir).tar.gz \
$(distdir).tar.xz

EXTRA_DIST = \
CMakeLists.txt \
README.md \
appveyor.yml \
dwarfdump/runtests.sh \
libdwarf/runtests.sh \
bugxml \
tsearch \
cmake \
ChangeLog \
ChangeLog2018 \
configure.cmake \
codingstyle.txt \
scripts/ChangeLog \
scripts/ChangeLog2018 \
scripts/BLDLIBDWARF \
scripts/BLDLIBDWARFTAR \
scripts/BLDTESTDIR \
scripts/CLEANUP \
scripts/CPTARTOWEBDIR \
scripts/CPTOPUBLIC \
scripts/CREATINGARELEASE \
scripts/GETTOTOP \
scripts/fixlibdwarfelf.sh \
scripts/REBLDLIBDWARF \
scripts/SETUP_MASTER_TREE \
scripts/UPD.awk \
scripts/UPDATEDWARFDUMPVERSION.sh \
tsearch/ChangeLog \
tsearch/ChangeLog2014 \
tsearch/ChangeLog2015 \
tsearch/ChangeLog2016 \
tsearch/ChangeLog2017 \
tsearch/ChangeLog2018 \
tsearch/config.h \
tsearch/dwarf_incl.h \
tsearch/dwarf_tsearchbal.c \
tsearch/dwarf_tsearchbin.c \
tsearch/dwarf_tsearch.c \
tsearch/dwarf_tsearchepp.c \
tsearch/dwarf_tsearch.h \
tsearch/dwarf_tsearchhash.c \
tsearch/dwarf_tsearchred.c \
tsearch/ESSAY.txt \
tsearch/Makefile \
tsearch/README \
tsearch/RUNTEST \
tsearch/scripts \
tsearch/tsearch.c \
tsearch/tsearchlibtimes.csv \
tsearch/tsearchlibtimes.ods \
tsearch/tsearch_tester.c 


# AUTHORS, ChangeLog and COPYING must be present
