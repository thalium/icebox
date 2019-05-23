#!/bin/sh
#
# VirtualBox Guest Additions kernel module control script for Solaris.
#
# Copyright (C) 2008-2012 Oracle Corporation
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

LC_ALL=C
export LC_ALL

LANG=C
export LANG

SILENTUNLOAD=""
MODNAME="vboxguest"
VFSMODNAME="vboxfs"
VMSMODNAME="vboxms"
MODDIR32="/usr/kernel/drv"
MODDIR64="/usr/kernel/drv/amd64"
VFSDIR32="/usr/kernel/fs"
VFSDIR64="/usr/kernel/fs/amd64"

abort()
{
    echo 1>&2 "## $1"
    exit 1
}

info()
{
    echo 1>&2 "$1"
}

check_if_installed()
{
    cputype=`isainfo -k`
    modulepath="$MODDIR32/$MODNAME"
    if test "$cputype" = "amd64"; then
        modulepath="$MODDIR64/$MODNAME"
    fi
    if test -f "$modulepath"; then
        return 0
    fi
    abort "VirtualBox kernel module ($MODNAME) NOT installed."
}

module_loaded()
{
    if test -z "$1"; then
        abort "missing argument to module_loaded()"
    fi

    modname=$1
    # modinfo should now work properly since we prevent module autounloading.
    loadentry=`/usr/sbin/modinfo | grep "$modname "`
    if test -z "$loadentry"; then
        return 1
    fi
    return 0
}

vboxguest_loaded()
{
    module_loaded $MODNAME
    return $?
}

vboxfs_loaded()
{
    module_loaded $VFSMODNAME
    return $?
}

vboxms_loaded()
{
    module_loaded $VMSMODNAME
    return $?
}

check_root()
{
    # the reason we don't use "-u" is that some versions of id are old and do not
    # support this option (eg. Solaris 10) and do not have a "--version" to check it either
    # so go with the uglier but more generic approach
    idbin=`which id`
    isroot=`$idbin | grep "uid=0"`
    if test -z "$isroot"; then
        abort "This program must be run with administrator privileges.  Aborting"
    fi
}

start_module()
{
    /usr/sbin/add_drv -i'pci80ee,cafe' -m'* 0666 root sys' $MODNAME
    if test ! vboxguest_loaded; then
        abort "Failed to load VirtualBox guest kernel module."
    elif test -c "/devices/pci@0,0/pci80ee,cafe@4:$MODNAME"; then
        info "VirtualBox guest kernel module loaded."
    else
        info "VirtualBox guest kernel module failed to attach."
    fi
}

stop_module()
{
    if vboxguest_loaded; then
        /usr/sbin/rem_drv $MODNAME || abort "Failed to unload VirtualBox guest kernel module."
        info "VirtualBox guest kernel module unloaded."
    elif test -z "$SILENTUNLOAD"; then
        info "VirtualBox guest kernel module not loaded."
    fi
}

start_vboxfs()
{
    if vboxfs_loaded; then
        info "VirtualBox FileSystem kernel module already loaded."
    else
        /usr/sbin/modload -p fs/$VFSMODNAME || abort "Failed to load VirtualBox FileSystem kernel module."
        if test ! vboxfs_loaded; then
            info "Failed to load VirtualBox FileSystem kernel module."
        else
            info "VirtualBox FileSystem kernel module loaded."
        fi
    fi
}

stop_vboxfs()
{
    if vboxfs_loaded; then
        vboxfs_mod_id=`/usr/sbin/modinfo | grep $VFSMODNAME | cut -f 1 -d ' ' `
        if test -n "$vboxfs_mod_id"; then
            /usr/sbin/modunload -i $vboxfs_mod_id || abort "Failed to unload VirtualBox FileSystem module."
            info "VirtualBox FileSystem kernel module unloaded."
        fi
    elif test -z "$SILENTUNLOAD"; then
        info "VirtualBox FileSystem kernel module not loaded."
    fi
}

start_vboxms()
{
    /usr/sbin/add_drv -m'* 0666 root sys' $VMSMODNAME
    if test ! vboxms_loaded; then
        abort "Failed to load VirtualBox pointer integration module."
    elif test -c "/devices/pseudo/$VMSMODNAME@0:$VMSMODNAME"; then
        info "VirtualBox pointer integration module loaded."
    else
        info "VirtualBox pointer integration module failed to attach."
    fi
}

stop_vboxms()
{
    if vboxms_loaded; then
        /usr/sbin/rem_drv $VMSMODNAME || abort "Failed to unload VirtualBox pointer integration module."
        info "VirtualBox pointer integration module unloaded."
    elif test -z "$SILENTUNLOAD"; then
        info "VirtualBox pointer integration module not loaded."
    fi
}

status_module()
{
    if vboxguest_loaded; then
        info "Running."
    else
        info "Stopped."
    fi
}

stop_all()
{
    stop_vboxms
    stop_vboxfs
    stop_module
    return 0
}

restart_all()
{
    stop_all
    start_module
    start_vboxfs
    start_vboxms
    return 0
}

check_root
check_if_installed

if test "$2" = "silentunload"; then
    SILENTUNLOAD="$2"
fi

case "$1" in
stopall)
    stop_all
    ;;
restartall)
    restart_all
    ;;
start)
    start_module
    start_vboxms
    ;;
stop)
    stop_vboxms
    stop_module
    ;;
status)
    status_module
    ;;
vfsstart)
    start_vboxfs
    ;;
vfsstop)
    stop_vboxfs
    ;;
vmsstart)
    start_vboxms
    ;;
vmsstop)
    stop_vboxms
    ;;
*)
    echo "Usage: $0 {start|stop|restart|status}"
    exit 1
esac

exit 0

