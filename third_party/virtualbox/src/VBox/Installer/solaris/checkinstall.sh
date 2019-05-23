#!/bin/sh
# $Id: checkinstall.sh $
## @file
#
# VirtualBox checkinstall script for Solaris.
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

infoprint()
{
    echo 1>&2 "$1"
}

errorprint()
{
    echo 1>&2 "## $1"
}

abort_error()
{
    errorprint "Please close all VirtualBox processes and re-run this installer."
    exit 1
}

checkdep_svr4()
{
    if test -z "$1"; then
        errorprint "Missing argument to checkdep_svr4"
        return 1
    fi
    $BIN_PKGINFO $BASEDIR_OPT "$1" >/dev/null 2>&1
    if test "$?" -eq 0; then
        return 0
    fi
    PKG_MISSING_SVR4="$PKG_MISSING_SVR4 $1"
    return 1
}

checkdep_ips()
{
    if test -z "$1"; then
        errorprint "Missing argument to checkdep_ips"
        return 1
    fi
    # using "list" without "-a" only lists installed pkgs which is what we need
    $BIN_PKG $BASEDIR_OPT list "$1" >/dev/null 2>&1
    if test "$?" -eq 0; then
        return 0
    fi
    PKG_MISSING_IPS="$PKG_MISSING_IPS $1"
    return 1
}

checkdep_ips_either()
{
    if test -z "$1" || test -z "$2"; then
        errorprint "Missing argument to checkdep_ips_either"
        return 1
    fi
    # using "list" without "-a" only lists installed pkgs which is what we need
    $BIN_PKG $BASEDIR_OPT list "$1" >/dev/null 2>&1
    if test "$?" -eq 0; then
        return 0
    fi
    $BIN_PKG $BASEDIR_OPT list "$2" >/dev/null 2>&1
    if test "$?" -eq 0; then
        return 0
    fi
    PKG_MISSING_IPS="$PKG_MISSING_IPS $1 or $2"
    return 1
}

disable_service()
{
    if test -z "$1" || test -z "$2"; then
        errorprint "Missing argument to disable_service"
        return 1
    fi
    servicefound=`$BIN_SVCS -H "$1" 2> /dev/null | grep '^online'`
    if test ! -z "$servicefound"; then
        infoprint "$2 ($1) is still enabled. Disabling..."
        $BIN_SVCADM disable -s "$1"
        # Don't delete the service, handled by manifest class action
        # /usr/sbin/svccfg delete $1
    fi
}

# find_bin_path()
# !! failure is always fatal
find_bin_path()
{
    if test -z "$1"; then
        errorprint "missing argument to find_bin_path()"
        exit 1
    fi

    binfilename=`basename $1`
    binfilepath=`which $binfilename 2> /dev/null`
    if test -x "$binfilepath"; then
        echo "$binfilepath"
        return 0
    else
        errorprint "$1 missing or is not an executable"
        exit 1
    fi
}

# find_bins()
# !! failure is always fatal
find_mandatory_bins()
{
    # Search only for binaries that might be in different locations
    if test ! -x "$BIN_SVCS"; then
        BIN_SVCS=`find_bin_path "$BIN_SVCS"`
    fi

    if test ! -x "$BIN_SVCADM"; then
        BIN_SVCADM=`find_bin_path "$BIN_SVCADM"`
    fi
}


#
# Begin execution
#

# Nothing to check for remote install
REMOTE_INST=0
if test "x${PKG_INSTALL_ROOT:=/}" != "x/"; then
    BASEDIR_OPT="-R $PKG_INSTALL_ROOT"
    REMOTE_INST=1
fi

# Nothing to check for non-global zones
currentzone=`zonename`
if test "x$currentzone" != "xglobal"; then
    exit 0
fi

PKG_MISSING_IPS=""
PKG_MISSING_SVR4=""
BIN_PKGINFO=/usr/bin/pkginfo
BIN_PKG=/usr/bin/pkg
BIN_SVCS=/usr/bin/svcs
BIN_SVCADM=/usr/sbin/svcadm

# Check non-optional binaries
find_mandatory_bins

infoprint "Checking package dependencies..."

if test -x "$BIN_PKG"; then
    checkdep_ips_either "runtime/python-26" "runtime/python-27"
    checkdep_ips_either "system/library/iconv/utf-8" "system/library/iconv/iconv-core"
    checkdep_ips_either "system/library/gcc/gcc-c++-runtime" "system/library/gcc-45-runtime"
    checkdep_ips_either "system/library/gcc/gcc-c-runtime" "system/library/gcc-45-runtime"
else
    PKG_MISSING_IPS="runtime/python-26 system/library/iconv/utf-8 system/library/gcc/gcc-c++-runtime system/library/gcc/gcc-c-runtime"
fi
if test -x "$BIN_PKGINFO"; then
    checkdep_svr4 "SUNWPython"
    checkdep_svr4 "SUNWPython-devel"
    checkdep_svr4 "SUNWuiu8"
else
    PKG_MISSING_SVR4="SUNWPython SUNWPython-devel SUNWuiu8"
fi

if test "x$PKG_MISSING_IPS" != "x" && test "x$PKG_MISSING_SVR4" != "x"; then
    if test ! -x "$BIN_PKG" && test ! -x "$BIN_PKGINFO"; then
        errorprint "Missing or non-executable binaries: pkg ($BIN_PKG) and pkginfo ($BIN_PKGINFO)."
        errorprint "Cannot check for dependencies."
        errorprint ""
        errorprint "Please install one of the required packaging system."
        exit 1
    fi
    errorprint "Missing packages: "
    errorprint "IPS : $PKG_MISSING_IPS"
    errorprint "SVr4: $PKG_MISSING_SVR4"
    errorprint ""
    errorprint "Please install either the IPS or SVr4 packages before installing VirtualBox."
    exit 1
else
    infoprint "Done."
fi

# Nothing more to do for remote installs
if test "$REMOTE_INST" -eq 1; then
    exit 0
fi

# Check & disable running services
disable_service "svc:/application/virtualbox/zoneaccess"  "VirtualBox zone access service"
disable_service "svc:/application/virtualbox/webservice"  "VirtualBox web service"
disable_service "svc:/application/virtualbox/autostart"   "VirtualBox auto-start service"
disable_service "svc:/application/virtualbox/balloonctrl" "VirtualBox balloon-control service"

# Check if VBoxSVC is currently running
VBOXSVC_PID=`ps -eo pid,fname | grep VBoxSVC | grep -v grep | awk '{ print $1 }'`
if test ! -z "$VBOXSVC_PID" && test "$VBOXSVC_PID" -ge 0; then
    errorprint "VirtualBox's VBoxSVC (pid $VBOXSVC_PID) still appears to be running."
    abort_error
fi

# Check if VBoxNetDHCP is currently running
VBOXNETDHCP_PID=`ps -eo pid,fname | grep VBoxNetDHCP | grep -v grep | awk '{ print $1 }'`
if test ! -z "$VBOXNETDHCP_PID" && test "$VBOXNETDHCP_PID" -ge 0; then
    errorprint "VirtualBox's VBoxNetDHCP (pid $VBOXNETDHCP_PID) still appears to be running."
    abort_error
fi

# Check if VBoxNetNAT is currently running
VBOXNETNAT_PID=`ps -eo pid,fname | grep VBoxNetNAT | grep -v grep | awk '{ print $1 }'`
if test ! -z "$VBOXNETNAT_PID" && test "$VBOXNETNAT_PID" -ge 0; then
    errorprint "VirtualBox's VBoxNetNAT (pid $VBOXNETNAT_PID) still appears to be running."
    abort_error
fi

# Check if vboxnet is still plumbed, if so try unplumb it
BIN_IFCONFIG=`which ifconfig 2> /dev/null`
if test -x "$BIN_IFCONFIG"; then
    vboxnetup=`$BIN_IFCONFIG vboxnet0 >/dev/null 2>&1`
    if test "$?" -eq 0; then
        infoprint "VirtualBox NetAdapter is still plumbed"
        infoprint "Trying to remove old NetAdapter..."
        $BIN_IFCONFIG vboxnet0 unplumb
        if test "$?" -ne 0; then
            errorprint "VirtualBox NetAdapter 'vboxnet0' couldn't be unplumbed (probably in use)."
            abort_error
        fi
    fi
    vboxnetup=`$BIN_IFCONFIG vboxnet0 inet6 >/dev/null 2>&1`
    if test "$?" -eq 0; then
        infoprint "VirtualBox NetAdapter (Ipv6) is still plumbed"
        infoprint "Trying to remove old NetAdapter..."
        $BIN_IFCONFIG vboxnet0 inet6 unplumb
        if test "$?" -ne 0; then
            errorprint "VirtualBox NetAdapter 'vboxnet0' IPv6 couldn't be unplumbed (probably in use)."
            abort_error
        fi
    fi
fi

# Make sure that SMF has finished removing any services left over from a
# previous installation which may interfere with installing new ones.
# This is only relevant on Solaris 11 for SysV packages.
#
# See BugDB 14838646 for the original problem and @bugref{7866} for
# follow up fixes.
for i in 1 2 3 4 5 6 7 8 9 10; do
    $BIN_SVCS -H "svc:/application/virtualbox/autostart"   >/dev/null 2>&1 ||
    $BIN_SVCS -H "svc:/application/virtualbox/webservice"  >/dev/null 2>&1 ||
    $BIN_SVCS -H "svc:/application/virtualbox/zoneaccess"  >/dev/null 2>&1 ||
    $BIN_SVCS -H "svc:/application/virtualbox/balloonctrl" >/dev/null 2>&1 || break
    if test "${i}" = "1"; then
        printf "Waiting for services from previous installation to be removed."
    elif test "${i}" = "10"; then
        printf "\nWarning!!! Some service(s) still appears to be present"
    else
        printf "."
    fi
    sleep 1
done
test "${i}" = "1" || printf "\n"

exit 0

