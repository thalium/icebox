#
# Makefile for the VirtualBox FreeBSD Guest Drivers.
#

#
#
# Copyright (C) 2009-2010 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

KBUILD_VERBOSE =

all:
	@echo "=== Building 'vboxguest' module ==="
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxguest
	@if [ -f vboxguest/vboxguest.ko ]; then \
	    cp vboxguest/vboxguest.ko .; \
	 fi
	@echo
	@if [ -d vboxvfs ]; then \
	    echo "=== Building 'vboxvfs' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxvfs; \
	    if [ -f vboxvfs/vboxvfs.ko ]; then \
	        cp vboxvfs/vboxvfs.ko .; \
	    fi; \
	fi


install:
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxguest install
	@if [ -d vboxvfs ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxvfs install; \
	fi

clean:
	@$(MAKE) -C vboxguest clean
	@if [ -d vboxvfs ]; then \
	    $(MAKE) -C vboxvfs clean; \
	fi
	rm -f vboxguest.*o vboxvfs.*o

load:
	@/sbin/kldunload vboxvfs || true
	@/sbin/kldunload vboxguest || true
	@/sbin/kldload ./vboxguest.ko
	@if [ -f vboxvfs.ko ]; then /sbin/kldload ./vboxvfs.ko; fi
