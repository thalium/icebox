#!/bin/bash
## @file
# For development, builds and loads all the host drivers.
#

#
# Copyright (C) 2010-2017 Oracle Corporation
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

TARGET=`readlink -e -- "${0}"` || exit 1  # The GNU-specific way.
MY_DIR="${TARGET%/[!/]*}"

set -e

# Parse parameters.
OPT_UNLOAD_ONLY=
if [ ${#} -ge 1 -a "${1}" = "-u" ]; then
    OPT_UNLOAD_ONLY=yes
    shift
fi
if [ ${#} -ge 1 -a '(' "${1}" = "-h"  -o  "${1}" = "--help" ')' ]; then
    echo "usage: load.sh [-u] [make arguments]"
    exit 1
fi

# Unload, but keep the udev rules.
sudo "${MY_DIR}/vboxdrv.sh" stop

if [ -z "${OPT_UNLOAD_ONLY}" ]; then
    # Build and load.
    MAKE_JOBS=`grep vendor_id /proc/cpuinfo | wc -l`
    if [ "${MAKE_JOBS}" -le "0" ]; then MAKE_JOBS=1; fi
    make "-j${MAKE_JOBS}" -C "${MY_DIR}/src/vboxdrv" "$@"

    echo "Installing SUPDrv (aka VBoxDrv/vboxdrv)"
    sudo /sbin/insmod "${MY_DIR}/src/vboxdrv/vboxdrv.ko"
fi

