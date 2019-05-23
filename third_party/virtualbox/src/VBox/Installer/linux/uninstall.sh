#!/bin/sh
#
# Oracle VM VirtualBox
# VirtualBox linux uninstallation script

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

# The below is GNU-specific.  See VBox.sh for the longer Solaris/OS X version.
TARGET=`readlink -e -- "${0}"` || exit 1
MY_PATH="${TARGET%/[!/]*}"
. "${MY_PATH}/routines.sh"

if [ -z "$ro_LOG_FILE" ]; then
    create_log "/var/log/vbox-uninstall.log"
fi

if [ -z "VBOX_NO_UNINSTALL_MESSAGE" ]; then
    info "Uninstalling VirtualBox"
    log "Uninstalling VirtualBox"
    log ""
fi

check_root

[ -z "$CONFIG_DIR" ]    && CONFIG_DIR="/etc/vbox"
[ -z "$CONFIG" ]        && CONFIG="vbox.cfg"
[ -z "$CONFIG_FILES" ]  && CONFIG_FILES="filelist"
[ -z "$DEFAULT_FILES" ] && DEFAULT_FILES=`pwd`/deffiles

# Find previous installation
if [ -r $CONFIG_DIR/$CONFIG ]; then
    . $CONFIG_DIR/$CONFIG
    PREV_INSTALLATION=$INSTALL_DIR
fi

# Remove previous installation
if [ "$PREV_INSTALLATION" = "" ]; then
    log "Unable to find a VirtualBox installation, giving up."
    abort "Couldn't find a VirtualBox installation to uninstall."
fi

# Do pre-removal common to all installer types, currently service script
# clean-up.
"${MY_PATH}/prerm-common.sh" || exit 1

# Remove kernel module installed
if [ -z "$VBOX_DONT_REMOVE_OLD_MODULES" ]; then
    rm -f /usr/src/vboxhost-$INSTALL_VER 2> /dev/null
    rm -f /usr/src/vboxdrv-$INSTALL_VER 2> /dev/null
    rm -f /usr/src/vboxnetflt-$INSTALL_VER 2> /dev/null
    rm -f /usr/src/vboxnetadp-$INSTALL_VER 2> /dev/null
    rm -f /usr/src/vboxpci-$INSTALL_VER 2> /dev/null
fi

# Remove symlinks
rm -f \
  /usr/bin/VirtualBox \
  /usr/bin/VBoxManage \
  /usr/bin/VBoxSDL \
  /usr/bin/VBoxVRDP \
  /usr/bin/VBoxHeadless \
  /usr/bin/VBoxDTrace \
  /usr/bin/VBoxBugReport \
  /usr/bin/VBoxBalloonCtrl \
  /usr/bin/VBoxAutostart \
  /usr/bin/VBoxNetDHCP \
  /usr/bin/VBoxNetNAT \
  /usr/bin/vboxwebsrv \
  /usr/bin/vbox-img \
  /usr/bin/VBoxAddIF \
  /usr/bin/VBoxDeleteIf \
  /usr/bin/VBoxTunctl \
  /usr/bin/virtualbox \
  /usr/share/pixmaps/VBox.png \
  /usr/share/pixmaps/virtualbox.png \
  /usr/share/applications/virtualbox.desktop \
  /usr/share/mime/packages/virtualbox.xml \
  /usr/bin/rdesktop-vrdp \
  /usr/bin/virtualbox \
  /usr/bin/vboxmanage \
  /usr/bin/vboxsdl \
  /usr/bin/vboxheadless \
  /usr/bin/vboxdtrace \
  /usr/bin/vboxbugreport \
  $PREV_INSTALLATION/components/VBoxVMM.so \
  $PREV_INSTALLATION/components/VBoxREM.so \
  $PREV_INSTALLATION/components/VBoxRT.so \
  $PREV_INSTALLATION/components/VBoxDDU.so \
  $PREV_INSTALLATION/components/VBoxXPCOM.so \
  2> /dev/null

cwd=`pwd`
if [ -f $PREV_INSTALLATION/src/Makefile ]; then
    cd $PREV_INSTALLATION/src
    make clean > /dev/null 2>&1
fi
if [ -f $PREV_INSTALLATION/src/vboxdrv/Makefile ]; then
    cd $PREV_INSTALLATION/src/vboxdrv
    make clean > /dev/null 2>&1
fi
if [ -f $PREV_INSTALLATION/src/vboxnetflt/Makefile ]; then
    cd $PREV_INSTALLATION/src/vboxnetflt
    make clean > /dev/null 2>&1
fi
if [ -f $PREV_INSTALLATION/src/vboxnetadp/Makefile ]; then
    cd $PREV_INSTALLATION/src/vboxnetadp
    make clean > /dev/null 2>&1
fi
if [ -f $PREV_INSTALLATION/src/vboxpci/Makefile ]; then
    cd $PREV_INSTALLATION/src/vboxpci
    make clean > /dev/null 2>&1
fi
cd $PREV_INSTALLATION
if [ -r $CONFIG_DIR/$CONFIG_FILES ]; then
    rm -f `cat $CONFIG_DIR/$CONFIG_FILES` 2> /dev/null
elif [ -n "$DEFAULT_FILES" -a -r "$DEFAULT_FILES" ]; then
    DEFAULT_FILE_NAMES=""
    . $DEFAULT_FILES
    for i in "$DEFAULT_FILE_NAMES"; do
        rm -f $i 2> /dev/null
    done
fi
for file in `find $PREV_INSTALLATION 2> /dev/null`; do
    rmdir -p $file 2> /dev/null
done
cd $cwd
mkdir -p $PREV_INSTALLATION 2> /dev/null # The above actually removes the current directory and parents!
rmdir $PREV_INSTALLATION 2> /dev/null
rm -r $CONFIG_DIR/$CONFIG 2> /dev/null

if [ -z "$VBOX_NO_UNINSTALL_MESSAGE" ]; then
    rm -r $CONFIG_DIR/$CONFIG_FILES 2> /dev/null
    rmdir $CONFIG_DIR 2> /dev/null
    [ -n "$INSTALL_REV" ] && INSTALL_REV=" r$INSTALL_REV"
    info "VirtualBox $INSTALL_VER$INSTALL_REV has been removed successfully."
    log "Successfully $INSTALL_VER$INSTALL_REV removed VirtualBox."
fi
update-mime-database /usr/share/mime >/dev/null 2>&1
