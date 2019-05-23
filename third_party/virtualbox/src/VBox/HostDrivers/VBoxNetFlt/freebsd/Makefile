# $Id: Makefile $
## @file
# Makefile for the VirtualBox FreeBSD Host Driver.
#

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

KMOD = vboxnetflt

CFLAGS += -DRT_OS_FREEBSD -DIN_RING0 -DIN_RT_R0 -DIN_SUP_R0 -DVBOX -DRT_WITH_VBOX -Iinclude -I. -Ir0drv -w -DVBOX_WITH_HARDENING -DVIMAGE

.if (${MACHINE_ARCH} == "i386")
 CFLAGS += -DRT_ARCH_X86
.elif (${MACHINE_ARCH} == "amd64")
 CFLAGS += -DRT_ARCH_AMD64
.endif

SRCS = \
	VBoxNetFlt-freebsd.c \
	VBoxNetFlt.c \
	SUPR0IdcClient-freebsd.c \
	SUPR0IdcClient.c \
	SUPR0IdcClientComponent.c

SRCS += device_if.h bus_if.h opt_netgraph.h

.include <bsd.kmod.mk>

