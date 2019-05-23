#!/bin/sh
# $Id: pkginstall.sh $
## @file
#
# VirtualBox postinstall script for Solaris.
#
# If you just installed VirtualBox using IPS/pkg(5), you should run this
# script once to avoid rebooting the system before using VirtualBox.
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

if test "$1" != "--srv4"; then
    # IPS package
    echo "Checking for older & partially installed bits..."
    ISIPS="--ips"
else
    # SRv4 package
    echo "Checking for older bits..."
    ISIPS=""
fi

# pkgadd -v
if test "$1" = "--sh-trace" || test "$2" = "--sh-trace" || test "$3" = "--sh-trace"; then
    set -x
fi
DEBUGOPT=`set -o 2>/dev/null | sed -ne 's/^xtrace *on$/--sh-trace/p'` # propagate pkgadd -v

# If PKG_INSTALL_ROOT is undefined or NULL, redefine to '/' and carry on.
${PKG_INSTALL_ROOT:=/}/opt/VirtualBox/vboxconfig.sh --preremove --fatal ${ISIPS} ${DEBUGOPT}

if test "$?" -eq 0; then
    echo "Installing new ones..."
    $PKG_INSTALL_ROOT/opt/VirtualBox/vboxconfig.sh --postinstall ${DEBUGOPT}
    rc=$?
    if test "$rc" -ne 0; then
        echo 1>&2 "## Completed but with errors."
        rc=1
    else
        if test "$1" != "--srv4"; then
            echo "Post installation completed successfully!"
        fi
    fi
else
    echo 1>&2 "## ERROR!! Failed to remove older/partially installed bits."
    rc=1
fi

exit "$rc"

