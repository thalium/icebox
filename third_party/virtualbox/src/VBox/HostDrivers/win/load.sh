#!/bin/bash
## @file
# For development, loads the support driver.
#

#
# Copyright (C) 2010-2015 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

if test -n "$Path" -a -z "$PATH"; then
    export PATH="$Path"
fi

MY_DIR=`cd "${0}/.." && cmd /c cd | kmk_sed -e 's,\\\\,/,g' `
if [ ! -d "${MY_DIR}" ]; then
    echo "Cannot find ${MY_DIR} or it's not a directory..."
    exit 1;
fi
echo MY_DIR=$MY_DIR

set -e
cd "$MY_DIR"
set +e


#
# Query the status of the drivers.
#
for drv in VBoxNetAdp VBoxNetAdp6 VBoxNetFlt VBoxNetLwf VBoxUSBMon VBoxUSB VBoxDrv;
do
    if sc query $drv > /dev/null; then
        STATE=`sc query $drv \
               | kmk_sed -e '/^ *STATE /!d' -e 's/^[^:]*: [0-9 ]*//' \
                         -e 's/  */ /g' \
                         -e 's/^ *//g' \
                         -e 's/ *$//g' \
              `
        echo "load.sh: $drv - $STATE"
    else
        echo "load.sh: $drv - not configured, probably."
    fi
done

set -e
set -x

#
# Invoke the uninstallers.
#
for uninst in NetAdpUninstall.exe NetAdp6Uninstall.exe USBUninstall.exe NetFltUninstall.exe NetLwfUninstall.exe SUPUninstall.exe;
do
    if test -f ${MY_DIR}/$uninst; then
        ${MY_DIR}/$uninst
    fi
done

#
# Invoke the installer.
#
if test "$1" != "-u" -a "$1" != "--uninstall"; then
    for inst in SUPInstall.exe;
    do
        if test -f ${MY_DIR}/$inst; then
            ${MY_DIR}/$inst
        fi
    done
fi

echo "load.sh: Successfully installed SUPDrv (aka VBoxDrv)"
exit 0

