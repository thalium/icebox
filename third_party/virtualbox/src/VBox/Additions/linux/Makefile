#
# Makefile for the VirtualBox Linux Guest Drivers.
#

#
# Copyright (C) 2009-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

ifneq ($(KBUILD_EXTMOD),)

# Building from kBuild (make -C <kernel_directory> M=`pwd`).
# KBUILD_EXTMOD is set to $(M) in this case.

obj-m = vboxguest/ vboxsf/ vboxvideo/

else # ! KBUILD_EXTMOD

KBUILD_VERBOSE =

vboxguest:
	@echo "=== Building 'vboxguest' module ==="
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxguest
	@if [ -f vboxguest/vboxguest.ko ]; then \
	    cp vboxguest/vboxguest.ko .; \
	 else \
	    cp vboxguest/vboxguest.o .; \
	 fi
	@echo

vboxsf: vboxguest
	@if [ -d vboxsf ]; then \
	    if [ -f vboxguest/Module.symvers ]; then \
	        cp vboxguest/Module.symvers vboxsf; \
	    fi; \
	    echo "=== Building 'vboxsf' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxsf || exit 1; \
	    if [ -f vboxsf/vboxsf.ko ]; then \
	        cp vboxsf/vboxsf.ko .; \
	    else \
	        cp vboxsf/vboxsf.o .; \
	    fi; \
	    echo; \
	fi

vboxvideo:
	@if [ -d vboxvideo ]; then \
	    if [ -f vboxguest/Module.symvers ]; then \
	        cp vboxguest/Module.symvers vboxvideo; \
	    fi; \
	    echo "=== Building 'vboxvideo' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxvideo || exit 1; \
	    if [ -f vboxvideo/vboxvideo.ko ]; then \
	        cp vboxvideo/vboxvideo.ko .; \
	    elif [ -f vboxvideo/vboxvideo.o ]; then \
	        cp vboxvideo/vboxvideo.o .; \
	    fi; \
	    echo; \
	fi

all: vboxguest vboxsf vboxvideo

install:
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxguest install
	@if [ -d vboxsf ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxsf install; \
	fi
	@if [ -d vboxvideo ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxvideo install; \
	fi

clean:
	@$(MAKE) -C vboxguest clean
	@if [ -d vboxsf ]; then \
	    $(MAKE) -C vboxsf clean; \
	fi
	@if [ -d vboxvideo ]; then \
	    $(MAKE) -C vboxvideo clean; \
	fi
	rm -f vboxguest.*o vboxsf.*o vboxvideo.*o

check:
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxguest check

load:
	@/sbin/rmmod vboxvideo || true
	@/sbin/rmmod vboxvfs || true
	@/sbin/rmmod vboxsf || true
	@/sbin/rmmod vboxguest || true
	@/sbin/insmod vboxguest.ko
	@if [ -f vboxsf.ko ]; then /sbin/insmod vboxsf.ko; fi
	@if [ -f vboxvideo.ko ]; then /sbin/insmod vboxvideo.ko; fi

.PHONY: vboxguest vboxsf vboxvideo all install clean check load

endif # ! KBUILD_EXTMOD
