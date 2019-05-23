#!/bin/bash
## @file
# For development, loads all the host drivers.
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
        echo "loadall.sh: $drv - $STATE"
    else
        echo "loadall.sh: $drv - not configured, probably."
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
# Invoke the installers.
#
if test "$1" != "-u" -a "$1" != "--uninstall"; then
    INSTALLERS="SUPInstall.exe USBInstall.exe";
    VER=`cmd.exe /c ver`
    VER=`echo "$VER" | kmk_sed -e 's/^.*\[[^0-9]* \(.*\)\]/\1/' -e '/^$/d`
    case "$VER" in
        6.*|10.*|11.*|12.*)
            INSTALLERS="$INSTALLERS NetLwfInstall.exe"; #NetAdp6Install.exe - also busted?
            ;;
        *)
            INSTALLERS="$INSTALLERS NetFltInstall.exe"; #NetAdpInstall.exe; - busted
            ;;
    esac

    for inst in $INSTALLERS;
    do
        if test -f ${MY_DIR}/$inst; then
            ${MY_DIR}/$inst
        fi
    done
fi

echo "loadall.sh: Successfully installed all drivers"
exit 0

