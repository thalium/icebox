#!/bin/sh
# $Id: preremove.sh $
## @file
# VirtualBox preremove script for Solaris.
#

#
# VirtualBox preremove script for Solaris.
#
# Copyright (C) 2007-2015 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

currentzone=`zonename`
if test "x$currentzone" = "xglobal"; then
    echo "Removing VirtualBox services and drivers..."
    ${PKG_INSTALL_ROOT:=/}/opt/VirtualBox/vboxconfig.sh --preremove
    if test "$?" -eq 0; then
        echo "Done."
        exit 0
    fi
    echo 1>&2 "## Failed."
    exit 1
fi

exit 0

