# $Id: Makefile $
## @file
# Makefile for the VirtualBox FreeBSD Host Driver.
#

#
# Copyright (C) 2006-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#
# The contents of this file may alternatively be used under the terms
# of the Common Development and Distribution License Version 1.0
# (CDDL) only, as it comes in the "COPYING.CDDL" file of the
# VirtualBox OSE distribution, in which case the provisions of the
# CDDL are applicable instead of those of the GPL.
#
# You may elect to license modified versions of this file under the
# terms and conditions of either the GPL or the CDDL or both.
#

KMOD = vboxnetadp

CFLAGS += -DRT_OS_FREEBSD -DIN_RING0 -DIN_RT_R0 -DIN_SUP_R0 -DVBOX -DRT_WITH_VBOX -Iinclude -I. -Ir0drv -w -DVBOX_WITH_HARDENING -DVIMAGE

.if (${MACHINE_ARCH} == "i386")
 CFLAGS += -DRT_ARCH_X86
.elif (${MACHINE_ARCH} == "amd64")
 CFLAGS += -DRT_ARCH_AMD64
.endif

SRCS = \
	VBoxNetAdp-freebsd.c \
	VBoxNetAdp.c

SRCS += device_if.h bus_if.h

.include <bsd.kmod.mk>

