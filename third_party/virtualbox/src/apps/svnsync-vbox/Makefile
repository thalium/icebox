#
# Makefile
#
#
# Copyright (C) 2006-2015 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

CC = gcc
CFLAGS = -O2 -g -Wall -pedantic -Wno-long-long -Wno-trigraphs \
  -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
DEFS = -DVBOX -D_FILE_OFFSET_BITS=64
INCS = -I. -I/usr/include/subversion-1 -I/usr/include/apr-1.0
LIBS = -lsvn_ra-1 -lsvn_ra_local-1 \
  -lsvn_repos-1 -lsvn_fs-1 -lsvn_delta-1 \
  -lsvn_subr-1 -lapr-1 -laprutil-1 \
  -ldb

all: svnsync-vbox

svnsync-vbox: main.c
	$(CC) $(CFLAGS) $(DEFS) $(INCS) $(LIBS) -o $@ $<

svnsync-vbox: svn_private_config.h

svnsync-vbox: Makefile

