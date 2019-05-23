#!/bin/sh
## @file
# Oracle VM VirtualBox startup script, Solaris hosts.
#

#
# Copyright (C) 2006-2015 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

CURRENT_ISA=`isainfo -k`
if test "$CURRENT_ISA" = "amd64"; then
    INSTALL_DIR="/opt/VirtualBox/amd64"
else
    INSTALL_DIR="/opt/VirtualBox/i386"
fi

APP=`basename $0`
case "$APP" in
    VirtualBox|virtualbox)
        exec "$INSTALL_DIR/VirtualBox" "$@"
        ;;
    VBoxManage|vboxmanage)
        exec "$INSTALL_DIR/VBoxManage" "$@"
        ;;
    VBoxSDL|vboxsdl)
        exec "$INSTALL_DIR/VBoxSDL" "$@"
        ;;
    VBoxVRDP|VBoxHeadless|vboxheadless)
        exec "$INSTALL_DIR/VBoxHeadless" "$@"
        ;;
    VBoxBugReport|vboxbugreport)
        exec "$INSTALL_DIR/VBoxBugReport" "$@"
        ;;
    VBoxBalloonCtrl|vboxballoonctrl)
        exec "$INSTALL_DIR/VBoxBalloonCtrl" "$@"
        ;;
    VBoxAutostart|vboxautostart)
        exec "$INSTALL_DIR/VBoxAutostart" "$@"
        ;;
    VBoxDTrace|vboxdtrace)
        exec "$INSTALL_DIR/VBoxDTrace" "$@"
        ;;
    vboxwebsrv)
        exec "$INSTALL_DIR/vboxwebsrv" "$@"
        ;;
    VBoxQtconfig)
        exec "$INSTALL_DIR/VBoxQtconfig" "$@"
        ;;
    vbox-img)
        exec "$INSTALL_DIR/vbox-img" "$0"
        ;;
    *)
        echo "Unknown application - $APP"
        exit 1
        ;;
esac
exit 0

