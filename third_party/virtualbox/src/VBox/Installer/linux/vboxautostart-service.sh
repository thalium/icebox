#!/bin/sh
# $Id: vboxautostart-service.sh $
## @file
# VirtualBox autostart service init script.
#

#
# Copyright (C) 2012-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

# chkconfig: 345 35 65
# description: VirtualBox autostart service
#
### BEGIN INIT INFO
# Provides:       vboxautostart-service
# Required-Start: vboxdrv
# Required-Stop:  vboxdrv
# Default-Start:  2 3 4 5
# Default-Stop:   0 1 6
# Description:    VirtualBox autostart service
### END INIT INFO

PATH=$PATH:/bin:/sbin:/usr/sbin
SCRIPTNAME=vboxautostart-service.sh

[ -f /etc/debian_release -a -f /lib/lsb/init-functions ] || NOLSB=yes
[ -f /etc/vbox/vbox.cfg ] && . /etc/vbox/vbox.cfg

if [ -n "$INSTALL_DIR" ]; then
    binary="$INSTALL_DIR/VBoxAutostart"
else
    binary="/usr/lib/virtualbox/VBoxAutostart"
fi

# silently exit if the package was uninstalled but not purged,
# applies to Debian packages only (but shouldn't hurt elsewhere)
[ ! -f /etc/debian_release -o -x $binary ] || exit 0

[ -r /etc/default/virtualbox ] && . /etc/default/virtualbox

# Preamble for Gentoo
if [ "`which $0`" = "/sbin/rc" ]; then
    shift
fi

begin_msg()
{
    test -n "${2}" && echo "${SCRIPTNAME}: ${1}."
    logger -t "${SCRIPTNAME}" "${1}."
}

succ_msg()
{
    logger -t "${SCRIPTNAME}" "${1}."
}

fail_msg()
{
    echo "${SCRIPTNAME}: failed: ${1}." >&2
    logger -t "${SCRIPTNAME}" "failed: ${1}."
}

start_daemon() {
    usr="$1"
    shift
    su - $usr -c "$*"
}

if which start-stop-daemon >/dev/null 2>&1; then
    start_daemon() {
        usr="$1"
        shift
        bin="$1"
        shift
        start-stop-daemon --chuid $usr --start --exec $bin -- $@
    }
fi

vboxdrvrunning() {
    lsmod | grep -q "vboxdrv[^_-]"
}

start() {
    [ -z "$VBOXAUTOSTART_DB" ] && exit 0
    [ -z "$VBOXAUTOSTART_CONFIG" ] && exit 0
    begin_msg "Starting VirtualBox VMs configured for autostart" console;
    vboxdrvrunning || {
        fail_msg "VirtualBox kernel module not loaded!"
        exit 0
    }
    PARAMS="--background --start --config $VBOXAUTOSTART_CONFIG"

    # prevent inheriting this setting to VBoxSVC
    unset VBOX_RELEASE_LOG_DEST

    for user in `ls $VBOXAUTOSTART_DB/*.start`
    do
        start_daemon `basename $user | sed -ne "s/\(.*\).start/\1/p"` $binary $PARAMS > /dev/null 2>&1
    done

    return $RETVAL
}

stop() {
    [ -z "$VBOXAUTOSTART_DB" ] && exit 0
    [ -z "$VBOXAUTOSTART_CONFIG" ] && exit 0

    PARAMS="--stop --config $VBOXAUTOSTART_CONFIG"

    # prevent inheriting this setting to VBoxSVC
    unset VBOX_RELEASE_LOG_DEST

    for user in `ls $VBOXAUTOSTART_DB/*.stop`
    do
        start_daemon `basename $user | sed -ne "s/\(.*\).stop/\1/p"` $binary $PARAMS > /dev/null 2>&1
    done

    return $RETVAL
}

case "$1" in
start)
    start
    ;;
stop)
    stop
    ;;
*)
    echo "Usage: $0 {start|stop}"
    exit 1
esac

exit $RETVAL
