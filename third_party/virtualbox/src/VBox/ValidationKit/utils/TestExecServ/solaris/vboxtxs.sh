#!/bin/sh
## @file
# VirtualBox Test Execution Service Architecture Wrapper for Solaris.
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

# 1. Change directory to the script directory (usually /opt/VBoxTest/).
set -x
MY_DIR=`dirname "$0"`
cd "${MY_DIR}"

# 2. Determine the architecture.
MY_ARCH=`isainfo -k`
case "${MY_ARCH}" in
    amd64)
        MY_ARCH=amd64
        ;;
    i386)
        MY_ARCH=x86
        ;;
    *)
        echo "vboxtxs.sh: Unsupported architecture '${MY_ARCH}' returned by isainfo -k." >&2
        exit 2;
        ;;
esac

# 3. Exec the service.
exec "./${MY_ARCH}/TestExecService" \
    --cdrom="/cdrom/cdrom0/" \
    --scratch="/var/tmp/VBoxTest/" \
    --no-display-output \
    $*
exit 3;

