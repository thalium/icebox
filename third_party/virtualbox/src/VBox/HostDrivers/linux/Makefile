#
# Makefile for the VirtualBox Linux Host Drivers.
#

#
#
# Copyright (C) 2008-2015 Oracle Corporation
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

obj-m = vboxdrv/
ifneq ($(wildcard $(KBUILD_EXTMOD)/vboxnetflt/Makefile),)
 obj-m += vboxnetflt/
endif
ifneq ($(wildcard $(KBUILD_EXTMOD)/vboxnetadp/Makefile),)
 obj-m += vboxnetadp/
endif
ifneq ($(wildcard $(KBUILD_EXTMOD)/vboxpci/Makefile),)
 obj-m += vboxpci/
endif
else # ! KBUILD_EXTMOD

# convenience Makefile without KBUILD_EXTMOD

KBUILD_VERBOSE =

all:
	@echo "=== Building 'vboxdrv' module ==="
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxdrv
	@cp vboxdrv/vboxdrv.ko .
	@echo
	@if [ -d vboxnetflt ]; then \
	    if [ -f vboxdrv/Module.symvers ]; then \
	        cp vboxdrv/Module.symvers vboxnetflt; \
	    fi; \
	    echo "=== Building 'vboxnetflt' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetflt || exit 1; \
	    cp vboxnetflt/vboxnetflt.ko .; \
	    echo; \
	fi
	@if [ -d vboxnetadp ]; then \
	    if [ -f vboxdrv/Module.symvers ]; then \
	        cp vboxdrv/Module.symvers vboxnetadp; \
	    fi; \
	    echo "=== Building 'vboxnetadp' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetadp || exit 1; \
	    cp vboxnetadp/vboxnetadp.ko .; \
	    echo; \
	fi
	@if [ -d vboxpci ]; then \
	    if [ -f vboxdrv/Module.symvers ]; then \
	        cp vboxdrv/Module.symvers vboxpci; \
	    fi; \
	    echo "=== Building 'vboxpci' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxpci || exit 1; \
	    cp vboxpci/vboxpci.ko .; \
	    echo; \
	fi

install:
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxdrv install
	@if [ -d vboxnetflt ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetflt install; \
	fi
	@if [ -d vboxnetadp ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetadp install; \
	fi
	@if [ -d vboxpci ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxpci install; \
	fi

clean:
	@$(MAKE) -C vboxdrv clean
	@if [ -d vboxnetflt ]; then \
	    $(MAKE) -C vboxnetflt clean; \
	fi
	@if [ -d vboxnetadp ]; then \
	    $(MAKE) -C vboxnetadp clean; \
	fi
	@if [ -d vboxpci ]; then \
	    $(MAKE) -C vboxpci clean; \
	fi
	rm -f vboxdrv.ko vboxnetflt.ko vboxnetadp.ko vboxpci.ko 

check:
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxdrv check

unload:
	@for module in vboxpci vboxnetadp vboxnetflt vboxdrv; do \
		if grep "^$$module " /proc/modules >/dev/null; then \
			echo "Removing previously installed $$module module"; \
			/sbin/rmmod $$module; \
		fi; \
	done

load: unload
	@for module in vboxdrv vboxnetflt vboxnetadp vboxpci; do \
		if test -f $$module.ko; then \
			echo "Installing $$module module"; \
			/sbin/insmod $$module.ko; \
		fi; \
	done

endif # ! KBUILD_EXTMOD
