# $Id: Makefile $
## @file
# Makefile for the VirtualBox Linux Host Driver.
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

# Linux kbuild sets this to our source directory if we are called from
# there
obj ?= $(CURDIR)
include $(obj)/Makefile.include.header

MOD_NAME = vboxdrv
MOD_OBJS = \
	linux/SUPDrv-linux.o \
	SUPDrv.o \
	SUPDrvGip.o \
	SUPDrvSem.o \
	SUPDrvTracer.o \
	SUPLibAll.o \
	r0drv/alloc-r0drv.o \
	r0drv/initterm-r0drv.o \
	r0drv/memobj-r0drv.o \
	r0drv/mpnotification-r0drv.o \
	r0drv/powernotification-r0drv.o \
	r0drv/linux/assert-r0drv-linux.o \
	r0drv/linux/alloc-r0drv-linux.o \
	r0drv/linux/initterm-r0drv-linux.o \
	r0drv/linux/memobj-r0drv-linux.o \
	r0drv/linux/memuserkernel-r0drv-linux.o \
	r0drv/linux/mp-r0drv-linux.o \
	r0drv/linux/mpnotification-r0drv-linux.o \
	r0drv/linux/process-r0drv-linux.o \
	r0drv/linux/rtStrFormatKernelAddress-r0drv-linux.o \
	r0drv/linux/semevent-r0drv-linux.o \
	r0drv/linux/semeventmulti-r0drv-linux.o \
	r0drv/linux/semfastmutex-r0drv-linux.o \
	r0drv/linux/semmutex-r0drv-linux.o \
	r0drv/linux/spinlock-r0drv-linux.o \
	r0drv/linux/thread-r0drv-linux.o \
	r0drv/linux/thread2-r0drv-linux.o \
	r0drv/linux/threadctxhooks-r0drv-linux.o \
	r0drv/linux/time-r0drv-linux.o \
	r0drv/linux/timer-r0drv-linux.o \
	r0drv/generic/semspinmutex-r0drv-generic.o \
	common/alloc/alloc.o \
	common/checksum/crc32.o \
	common/checksum/ipv4.o \
	common/checksum/ipv6.o \
	common/err/RTErrConvertFromErrno.o \
	common/err/RTErrConvertToErrno.o \
	common/err/errinfo.o \
	common/log/log.o \
	common/log/logellipsis.o \
	common/log/logrel.o \
	common/log/logrelellipsis.o \
	common/log/logcom.o \
	common/log/logformat.o \
	common/misc/RTAssertMsg1Weak.o \
	common/misc/RTAssertMsg2.o \
	common/misc/RTAssertMsg2Add.o \
	common/misc/RTAssertMsg2AddWeak.o \
	common/misc/RTAssertMsg2AddWeakV.o \
	common/misc/RTAssertMsg2Weak.o \
	common/misc/RTAssertMsg2WeakV.o \
	common/misc/assert.o \
	common/misc/handletable.o \
	common/misc/handletablectx.o \
	common/misc/thread.o \
	common/string/RTStrCat.o \
	common/string/RTStrCopy.o \
	common/string/RTStrCopyEx.o \
	common/string/RTStrCopyP.o \
	common/string/RTStrNCmp.o \
	common/string/RTStrNLen.o \
	common/string/stringalloc.o \
	common/string/strformat.o \
	common/string/strformatrt.o \
	common/string/strformattype.o \
	common/string/strprintf.o \
	common/string/strtonum.o \
	common/table/avlpv.o \
	common/time/time.o \
	r0drv/linux/RTLogWriteDebugger-r0drv-linux.o \
	generic/RTAssertShouldPanic-generic.o \
	generic/RTLogWriteStdErr-stub-generic.o \
	generic/RTLogWriteStdOut-stub-generic.o \
	generic/RTLogWriteUser-generic.o \
	generic/RTMpGetArraySize-generic.o \
	generic/RTMpGetCoreCount-generic.o \
	generic/RTSemEventWait-2-ex-generic.o \
	generic/RTSemEventWaitNoResume-2-ex-generic.o \
	generic/RTSemEventMultiWait-2-ex-generic.o \
	generic/RTSemEventMultiWaitNoResume-2-ex-generic.o \
	generic/RTTimerCreate-generic.o \
	generic/errvars-generic.o \
	generic/mppresent-generic.o \
	generic/uuid-generic.o \
	VBox/log-vbox.o
ifeq ($(BUILD_TARGET_ARCH),x86)
 MOD_OBJS += math/gcc/divdi3.o \
	math/gcc/moddi3.o \
	math/gcc/qdivrem.o \
	math/gcc/udivdi3.o \
	math/gcc/udivmoddi4.o \
	math/gcc/divdi3.o \
	math/gcc/umoddi3.o
endif
ifeq ($(BUILD_TARGET_ARCH),amd64)
 MOD_OBJS += common/alloc/heapsimple.o
endif
ifdef VBOX_WITH_NATIVE_DTRACE
 MOD_OBJS += SUPDrvDTrace.o
endif

MOD_INCL  = $(addprefix -I$(KBUILD_EXTMOD),/ /include /r0drv/linux)
ifdef VBOX_WITH_NATIVE_DTRACE
 MOD_INCL += -I/usr/include/linux -I/usr/include
endif
MOD_DEFS = -DRT_OS_LINUX -DIN_RING0 -DIN_RT_R0 -DIN_SUP_R0 -DVBOX \
	-DRT_WITH_VBOX -DVBOX_WITH_HARDENING -DSUPDRV_WITH_RELEASE_LOGGER \
	-DVBOX_WITH_EFLAGS_AC_SET_IN_VBOXDRV -DIPRT_WITH_EFLAGS_AC_PRESERVING \
    -Wno-declaration-after-statement
ifndef CONFIG_VBOXDRV_FIXEDMAJOR
 MOD_DEFS += -DCONFIG_VBOXDRV_AS_MISC
endif
ifdef VBOX_WITH_NATIVE_DTRACE
 MOD_DEFS += -DVBOX_WITH_NATIVE_DTRACE
endif
ifeq ($(BUILD_TARGET_ARCH),amd64)
 MOD_DEFS += -DRT_ARCH_AMD64
else
 MOD_DEFS += -DRT_ARCH_X86
endif
# must be consistent with Config.kmk!
MOD_DEFS += -DVBOX_WITH_64_BITS_GUESTS
ifdef VBOX_WITH_TEXT_MODMEM_HACK
 MOD_DEFS += -DRTMEMALLOC_EXEC_HEAP -DVBOX_WITH_TEXT_MODMEM_HACK
endif

# build defs
MOD_CFLAGS = -include $(KBUILD_EXTMOD)/include/VBox/SUPDrvMangling.h \
	-fno-omit-frame-pointer -fno-pie

include $(obj)/Makefile.include.footer

check: $(MODULE)
	@if ! readelf -p __ksymtab_strings vboxdrv.ko | grep -E "\[.*\]  *(RT|g_..*RT.*)"; then \
	    echo "All exported IPRT symbols are properly renamed!"; \
	 else \
	    echo "error: Some exported IPRT symbols was not properly renamed! See above." >&2; \
	    false; \
	 fi
