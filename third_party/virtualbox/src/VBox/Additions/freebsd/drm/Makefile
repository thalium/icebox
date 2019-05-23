# $Id: Makefile $
## @file
# Makefile for the VirtualBox FreeBSD Host Driver.
#

#
#
# Copyright (C) 2006-2010 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

KMOD = vboxvideo

SRCS = \
	vboxvideo_drm.c

SRCS += device_if.h bus_if.h pci_if.h opt_drm.h

.include <bsd.kmod.mk>

