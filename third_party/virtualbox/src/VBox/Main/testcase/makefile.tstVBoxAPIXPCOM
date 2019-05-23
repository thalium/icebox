#
# tstVBoxAPIXPCOM makefile
#
#
# Copyright (C) 2006-2014 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

PATH_XPCOM     = ..
PATH_BIN       = ../../../../

# This setting must be the same as used when building VBoxXPCOM.so.
# If you get a lot of unresolved symbols, try commenting it out.
VBOX_WITH_XPCOM_NAMESPACE_CLEANUP=1

PATH_XPCOM_IDL = $(PATH_XPCOM)/idl
INCS_XPCOM     = $(PATH_XPCOM)/include \
                 $(PATH_XPCOM)/include/nsprpub \
                 $(PATH_XPCOM)/include/string \
                 $(PATH_XPCOM)/include/xpcom \
                 $(PATH_XPCOM)/include/ipcd

ifdef VBOX_WITH_XPCOM_NAMESPACE_CLEANUP
 DEFS_XPCOM   += VBOX_WITH_XPCOM_NAMESPACE_CLEANUP
endif

# Adjust this to match your platform, pick from RT_OS_LINUX, RT_OS_WINDOWS,
# RT_OS_DARWIN, RT_OS_SOLARIS...
DEFS_XPCOM += RT_OS_LINUX


#
# Link with the public XPCOM libraries
#
tstVBoxAPIXPCOM: tstVBoxAPIXPCOM.o
	g++ -g -o $@ $^ \
		$(PATH_BIN)/VBoxXPCOM.so \
		-Wl,-rpath $(PATH_BIN)/ \
		-ldl -lpthread

tstVBoxAPIXPCOM.o: tstVBoxAPIXPCOM.cpp
	g++ -c -g -fshort-wchar $(addprefix -I, $(INCS_XPCOM)) $(addprefix -D, $(DEFS_XPCOM)) -o $@ tstVBoxAPIXPCOM.cpp

clean:
	rm -f tstVBoxAPIXPCOM tstVBoxAPIXPCOM.o
