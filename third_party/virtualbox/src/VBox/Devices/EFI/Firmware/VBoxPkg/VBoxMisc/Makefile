# $Id: Makefile $
#* @file
# Makefile - assembling the iso image for experimenting with EFI.
#/
#
#
# Copyright (C) 2009-2015 Oracle Corporation
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
#/

#
# Here we add modules to be burn on iso-image 
#
SOURCES += VBoxInterceptorDxe.efi
SOURCES += VBoxMimicryDxe.efi

ifeq ($(BUILD_ARCH), IA32)
ARCH_SUFFIX=
else
ARCH_SUFFIX=$(BUILD_ARCH)
endif

BUILD_ARCH ?= IA32
BUILD_DIR=$(WORKSPACE)/Build/VBoxPkg$(ARCH_SUFFIX)/DEBUG_UNIXGCC/$(BUILD_ARCH)

ISO_DIR=efi-app
FILES_TO_ISO=$(foreach file, $(SOURCES), $(addprefix $(ISO_DIR)/, $(file)))

DEST=efi-app.iso
all:${DEST}

$(DEST):$(FILES_TO_ISO)
	mkisofs -R -o $@ $(basename $@)

$(ISO_DIR)/%.efi:$(BUILD_DIR)/%.efi
	cp $< $@

clean:
	$(RM) -rf ${DEST} $(FILES_TO_ISO)
