#
# Makefile for the VirtualBox Linux Host PCI Driver.
#

#
#
# Copyright (C) 2011-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

# Linux kbuild sets this to our source directory if we are called from there
obj ?= $(CURDIR)
include $(obj)/Makefile.include.header

# override is required by the Debian guys
MOD_NAME = vboxpci
MOD_OBJS =                          	\
	linux/VBoxPci-linux.o     	\
	VBoxPci.o                 	\
        SUPR0IdcClient.o          	\
        SUPR0IdcClientComponent.o    	\
        linux/SUPR0IdcClient-linux.o

ifeq ($(BUILD_TARGET_ARCH),x86)
MOD_OBJS += math/gcc/divdi3.o       \
	math/gcc/moddi3.o 	\
	math/gcc/qdivrem.o 	\
	math/gcc/udivdi3.o 	\
	math/gcc/udivmoddi4.o 	\
        math/gcc/divdi3.o 	\
	math/gcc/umoddi3.o
endif

MOD_INCL = $(addprefix -I$(KBUILD_EXTMOD),/ /include /r0drv/linux)
MOD_DEFS = -DRT_OS_LINUX -DIN_RING0 -DIN_RT_R0 -DIN_SUP_R0 -DVBOX \
	-DRT_WITH_VBOX -DVBOX_WITH_HARDENING
ifeq ($(BUILD_TARGET_ARCH),amd64)
 MOD_DEFS  += -DRT_ARCH_AMD64
else
 MOD_DEFS  += -DRT_ARCH_X86
endif

# By default we use remap_pfn_range() kernel API to make kernel pages
# visible for userland. Unfortunately, it leads to situation that
# during debug session all structures on that page (such as PVM pointer)
# are not accessible to the debugger (see #3214).
# This code enables experimental support
# for vm_insert_page() kernel API, allowing to export kernel pages
# to the userland in more debugger-friendly way. Due to stability
# concerns, not enabled by default yet.
ifdef VBOX_USE_INSERT_PAGE
 MOD_DEFS += -DVBOX_USE_INSERT_PAGE
endif

# build defs
MOD_CFLAGS = -include $(KBUILD_EXTMOD)/include/VBox/SUPDrvMangling.h -fno-pie

include $(obj)/Makefile.include.footer
