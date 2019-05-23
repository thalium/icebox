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

TARGET=`readlink -e -- "${0}"` || exit 1 # The GNU-specific way.
MY_DIR="${TARGET%/[!/]*}"
MY_GROUPNAME="vboxusers"

#
# vboxusers membership check.
#
if ! (id -Gn | grep -w ${MY_GROUPNAME}); then
    echo "loadall.sh: you're not a member of vboxusers...";
    # Create the group.
    if ! getent group ${MY_GROUPNAME}; then
        set -e
        sudo groupadd -r -f ${MY_GROUPNAME}
        set +e
    fi
    # Add ourselves.
    MY_USER=`id -un`
    if [ -z "${MY_USER}" ]; then echo "loadall.sh: cannot figure user name"; exit 1; fi
    sudo usermod -a -G ${MY_GROUPNAME} "${MY_USER}";

    # Require relogon.
    echo "loadall.sh: You must log on again to load the new group membership."
    exit 1
fi

#
# Normal action.
#
if [ ${#} -eq 0 ]; then
    if [ -f "${MY_DIR}/src/vboxdrv/vboxdrv.ko" ]; then
        echo "Cleaning build folder."
        make -C "${MY_DIR}/src" clean > /dev/null
    fi
    sudo "${MY_DIR}/vboxdrv.sh" setup

#
# Unload and clean up when '-u' is given.
#
elif [ ${#} -eq 1  -a "x${1}" = x-u ]; then
    sudo "${MY_DIR}/vboxdrv.sh" cleanup

#
# Invalid syntax.
#
else
    echo "Usage: loadall.sh [-u]"
    exit 1
fi

