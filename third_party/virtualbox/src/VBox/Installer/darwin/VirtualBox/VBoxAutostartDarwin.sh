#!/bin/sh

#
# Copyright (C) 2012-2015 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

#
# Wrapper for the per user autostart daemon. Gets a list of all users
# and starts the VMs.
#

function vboxStartStopAllUserVms()
{
    # Go through the list and filter out all users without a shell and a
    # non existing home.
    for user in `dscl . -list /Users`
    do
        HOMEDIR=`dscl . -read /Users/${user} | grep NFSHomeDirectory | sed 's/NFSHomeDirectory: //g'`
        USERSHELL=`dscl . -read /Users/${user} | grep UserShell | sed 's/UserShell: //g'`

        # Check for known home directories and shells for daemons
        if [[   "${HOMEDIR}" == "/var/empty" || "${HOMEDIR}" == "/dev/null" || "${HOMEDIR}" == "/var/root"
             || "${USERSHELL}" == "/usr/bin/false" || "${USERSHELL}" == "/dev/null" || "${USERSHELL}" == "/usr/sbin/uucico" ]]
        then
            continue
        fi

        case "${1}" in
            start)
                # Start the daemon
                su ${user} -c "/Applications/VirtualBox.app/Contents/MacOS/VBoxAutostart --quiet --start --background --config ${CONFIG}"
                ;;
            stop)
                # Start the daemon
                su ${user} -c "/Applications/VirtualBox.app/Contents/MacOS/VBoxAutostart --quiet --stop --config ${CONFIG}"
                ;;
               *)
                echo "Usage: start|stop"
                exit 1
        esac
    done
}

function vboxStopAllUserVms()
{
    vboxStartStopAllUserVms "stop"
}

CONFIG=${1}
vboxStartStopAllUserVms "start"
trap vboxStopAllUserVms HUP KILL TERM


