#!/bin/sh
# $Id: VirtualBoxStartup.sh $
## @file
# Startup service for loading the kernel extensions and select the set of VBox
# binaries that matches the kernel architecture.
#

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

if false; then
    . /etc/rc.common
else
    # Fake the startup item functions we're using.

    ConsoleMessage()
    {
        if [ "$1" != "-f" ]; then
            echo "$@"
        else
            shift
            echo "Fatal error: $@"
            exit 1;
        fi
    }

    RunService()
    {
        case "$1" in
            "start")
                StartService
                exit $?;
                ;;
            "stop")
                StopService
                exit $?;
                ;;
            "restart")
                RestartService
                exit $?;
                ;;
            "launchd")
                if RestartService; then
                    while true;
                    do
                        sleep 3600
                    done
                fi
                exit $?;
                ;;
             **)
                echo "Error: Unknown action '$1'"
                exit 1;
        esac
    }
fi


StartService()
{
    VBOX_RC=0
    VBOXDRV="VBoxDrv"
    VBOXUSB="VBoxUSB"

    #
    # Check that all the directories exist first.
    #
    if [ ! -d "/Library/Application Support/VirtualBox/${VBOXDRV}.kext" ]; then
        ConsoleMessage "Error: /Library/Application Support/VirtualBox/${VBOXDRV}.kext is missing"
        VBOX_RC=1
    fi
    if [ ! -d "/Library/Application Support/VirtualBox/${VBOXUSB}.kext" ]; then
        ConsoleMessage "Error: /Library/Application Support/VirtualBox/${VBOXUSB}.kext is missing"
        VBOX_RC=1
    fi
    if [ ! -d "/Library/Application Support/VirtualBox/VBoxNetFlt.kext" ]; then
        ConsoleMessage "Error: /Library/Application Support/VirtualBox/VBoxNetFlt.kext is missing"
        VBOX_RC=1
    fi
    if [ ! -d "/Library/Application Support/VirtualBox/VBoxNetAdp.kext" ]; then
        ConsoleMessage "Error: /Library/Application Support/VirtualBox/VBoxNetAdp.kext is missing"
        VBOX_RC=1
    fi

    #
    # Check that no drivers are currently running.
    # (Try stop the service if this is the case.)
    #
    if [ $VBOX_RC -eq 0 ]; then
        if kextstat -lb org.virtualbox.kext.VBoxDrv 2>&1 | grep -q org.virtualbox.kext.VBoxDrv; then
            ConsoleMessage "Error: ${VBOXDRV}.kext is already loaded"
            VBOX_RC=1
        fi
        if kextstat -lb org.virtualbox.kext.VBoxUSB 2>&1 | grep -q org.virtualbox.kext.VBoxUSB; then
            ConsoleMessage "Error: ${VBOXUSB}.kext is already loaded"
            VBOX_RC=1
        fi
        if kextstat -lb org.virtualbox.kext.VBoxNetFlt 2>&1 | grep -q org.virtualbox.kext.VBoxNetFlt; then
            ConsoleMessage "Error: VBoxNetFlt.kext is already loaded"
            VBOX_RC=1
        fi
        if kextstat -lb org.virtualbox.kext.VBoxNetAdp 2>&1 | grep -q org.virtualbox.kext.VBoxNetAdp; then
            ConsoleMessage "Error: VBoxNetAdp.kext is already loaded"
            VBOX_RC=1
        fi
    fi

    #
    # Load the drivers.
    #
    if [ $VBOX_RC -eq 0 ]; then
        ConsoleMessage "Loading ${VBOXDRV}.kext"
        if ! kextload "/Library/Application Support/VirtualBox/${VBOXDRV}.kext"; then
            ConsoleMessage "Error: Failed to load /Library/Application Support/VirtualBox/${VBOXDRV}.kext"
            VBOX_RC=1
        fi

        ConsoleMessage "Loading ${VBOXUSB}.kext"
        if ! kextload -d "/Library/Application Support/VirtualBox/${VBOXDRV}.kext" "/Library/Application Support/VirtualBox/${VBOXUSB}.kext"; then
            ConsoleMessage "Error: Failed to load /Library/Application Support/VirtualBox/${VBOXUSB}.kext"
            VBOX_RC=1
        fi

        ConsoleMessage "Loading VBoxNetFlt.kext"
        if ! kextload -d "/Library/Application Support/VirtualBox/${VBOXDRV}.kext" "/Library/Application Support/VirtualBox/VBoxNetFlt.kext"; then
            ConsoleMessage "Error: Failed to load /Library/Application Support/VirtualBox/VBoxNetFlt.kext"
            VBOX_RC=1
        fi

        ConsoleMessage "Loading VBoxNetAdp.kext"
        if ! kextload -d "/Library/Application Support/VirtualBox/${VBOXDRV}.kext" "/Library/Application Support/VirtualBox/VBoxNetAdp.kext"; then
            ConsoleMessage "Error: Failed to load /Library/Application Support/VirtualBox/VBoxNetAdp.kext"
            VBOX_RC=1
        fi

        if [ $VBOX_RC -ne 0 ]; then
            # unload the drivers (ignoring failures)
            kextunload -b org.virtualbox.kext.VBoxNetAdp
            kextunload -b org.virtualbox.kext.VBoxNetFlt
            kextunload -b org.virtualbox.kext.VBoxUSB
            kextunload -b org.virtualbox.kext.VBoxDrv
        fi
    fi

    #
    # Set the error on failure.
    #
    if [ "$VBOX_RC" -ne "0" ]; then
        ConsoleMessage -f VirtualBox
        exit $VBOX_RC
    fi
}


StopService()
{
    VBOX_RC=0
    VBOXDRV="VBoxDrv"
    VBOXUSB="VBoxUSB"

    if kextstat -lb org.virtualbox.kext.VBoxUSB 2>&1 | grep -q org.virtualbox.kext.VBoxUSB; then
        ConsoleMessage "Unloading ${VBOXUSB}.kext"
        if ! kextunload -m org.virtualbox.kext.VBoxUSB; then
            ConsoleMessage "Error: Failed to unload VBoxUSB.kext"
            VBOX_RC=1
        fi
    fi

    if kextstat -lb org.virtualbox.kext.VBoxNetFlt 2>&1 | grep -q org.virtualbox.kext.VBoxNetFlt; then
        ConsoleMessage "Unloading VBoxNetFlt.kext"
        if ! kextunload -m org.virtualbox.kext.VBoxNetFlt; then
            ConsoleMessage "Error: Failed to unload VBoxNetFlt.kext"
            VBOX_RC=1
        fi
    fi

    if kextstat -lb org.virtualbox.kext.VBoxNetAdp 2>&1 | grep -q org.virtualbox.kext.VBoxNetAdp; then
        ConsoleMessage "Unloading VBoxNetAdp.kext"
        if ! kextunload -m org.virtualbox.kext.VBoxNetAdp; then
            ConsoleMessage "Error: Failed to unload VBoxNetAdp.kext"
            VBOX_RC=1
        fi
    fi

    # This must come last because of dependencies.
    if kextstat -lb org.virtualbox.kext.VBoxDrv 2>&1 | grep -q org.virtualbox.kext.VBoxDrv; then
        ConsoleMessage "Unloading ${VBOXDRV}.kext"
        if ! kextunload -m org.virtualbox.kext.VBoxDrv; then
            ConsoleMessage "Error: Failed to unload VBoxDrv.kext"
            VBOX_RC=1
        fi
    fi

    # Set the error on failure.
    if [ "$VBOX_RC" -ne "0" ]; then
        ConsoleMessage -f VirtualBox
        exit $VBOX_RC
    fi
}


RestartService()
{
    StopService
    StartService
}


RunService "$1"

