# $Revision: 91752 $
## @file makefile.tstCAPIGlue
# Makefile for sample program illustrating use of C binding for COM/XPCOM.
#

#
# Copyright (C) 2009-2014 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

PATH_SDK      = ../../..
CAPI_INC      = -I$(PATH_SDK)/bindings/c/include
ifeq ($(BUILD_PLATFORM),win)
PLATFORM_INC  = -I$(PATH_SDK)/bindings/mscom/include
PLATFORM_LIB  = $(PATH_SDK)/bindings/mscom/lib
else
PLATFORM_INC  = -I$(PATH_SDK)/bindings/xpcom/include
PLATFORM_LIB  = $(PATH_SDK)/bindings/xpcom/lib
endif
GLUE_DIR      = $(PATH_SDK)/bindings/c/glue
GLUE_INC      = -I$(GLUE_DIR)

CC            = gcc
CFLAGS        = -g -Wall

.PHONY: all
all: tstCAPIGlue

.PHONY: clean
clean:
	rm -f tstCAPIGlue.o VBoxCAPIGlue.o VirtualBox_i.o tstCAPIGlue

tstCAPIGlue: tstCAPIGlue.o VBoxCAPIGlue.o VirtualBox_i.o
	$(CC) -o $@ $^ -ldl -lpthread

tstCAPIGlue.o: tstCAPIGlue.c
	$(CC) $(CFLAGS) $(CAPI_INC) $(PLATFORM_INC) $(GLUE_INC) -o $@ -c $<

VBoxCAPIGlue.o: $(GLUE_DIR)/VBoxCAPIGlue.c
	$(CC) $(CFLAGS) $(CAPI_INC) $(PLATFORM_INC) $(GLUE_INC) -o $@ -c $<

VirtualBox_i.o: $(PLATFORM_LIB)/VirtualBox_i.c
	$(CC) $(CFLAGS) $(CAPI_INC) $(PLATFORM_INC) $(GLUE_INC) -o $@ -c $<
