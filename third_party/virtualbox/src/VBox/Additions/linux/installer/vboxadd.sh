#! /bin/sh
# $Id: vboxadd.sh $
## @file
# Linux Additions kernel module init script ($Revision: 131976 $)
#

#
# Copyright (C) 2006-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

# X-Start-Before is a Debian Addition which we use when converting to
# a systemd unit.  X-Service-Type is our own invention, also for systemd.

# chkconfig: 345 10 90
# description: VirtualBox Linux Additions kernel modules
#
### BEGIN INIT INFO
# Provides:       vboxadd
# Required-Start:
# Required-Stop:
# Default-Start:  2 3 4 5
# Default-Stop:   0 1 6
# X-Start-Before: display-manager
# X-Service-Type: oneshot
# Description:    VirtualBox Linux Additions kernel modules
### END INIT INFO

## @todo This file duplicates a lot of script with vboxdrv.sh.  When making
# changes please try to reduce differences between the two wherever possible.

# Testing:
# * Should fail if the configuration file is missing or missing INSTALL_DIR or
#   INSTALL_VER entries.
# * vboxadd user and vboxsf groups should be created if they do not exist - test
#   by removing them before installing.
# * Shared folders can be mounted and auto-mounts accessible to vboxsf group,
#   including on recent Fedoras with SELinux.
# * Setting INSTALL_NO_MODULE_BUILDS inhibits modules and module automatic
#   rebuild script creation; otherwise modules, user, group, rebuild script,
#   udev rule and shared folder mount helper should be created/set up.
# * Setting INSTALL_NO_MODULE_BUILDS inhibits module load and unload on start
#   and stop.
# * Uninstalling the Additions and re-installing them does not trigger warnings.

export LC_ALL=C
PATH=$PATH:/bin:/sbin:/usr/sbin
PACKAGE=VBoxGuestAdditions
MODPROBE=/sbin/modprobe
OLDMODULES="vboxguest vboxadd vboxsf vboxvfs vboxvideo"
SERVICE="VirtualBox Guest Additions"
QUICKSETUP=
## systemd logs information about service status, otherwise do that ourselves.
QUIET=
test -z "${TARGET_VER}" && TARGET_VER=`uname -r`
# Marker to ignore a particular kernel version which was already installed.
SKIPFILE_BASE=/var/lib/VBoxGuestAdditions/skip

setup_log()
{
    test -z "${LOG}" || return 0
    # Rotate log files
    LOG="/var/log/vboxadd-setup.log"
    mv "${LOG}.3" "${LOG}.4" 2>/dev/null
    mv "${LOG}.2" "${LOG}.3" 2>/dev/null
    mv "${LOG}.1" "${LOG}.2" 2>/dev/null
    mv "${LOG}" "${LOG}.1" 2>/dev/null
}

if $MODPROBE -c 2>/dev/null | grep -q '^allow_unsupported_modules  *0'; then
  MODPROBE="$MODPROBE --allow-unsupported-modules"
fi

# Check architecture
cpu=`uname -m`;
case "$cpu" in
  i[3456789]86|x86)
    cpu="x86"
    ldconfig_arch="(libc6)"
    lib_candidates="/usr/lib/i386-linux-gnu /usr/lib /lib"
    ;;
  x86_64|amd64)
    cpu="amd64"
    ldconfig_arch="(libc6,x86-64)"
    lib_candidates="/usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib /lib64 /lib"
    ;;
esac
for i in $lib_candidates; do
  if test -d "$i/VBoxGuestAdditions"; then
    lib_path=$i
    break
  fi
done

# Preamble for Gentoo
if [ "`which $0`" = "/sbin/rc" ]; then
    shift
fi

begin()
{
    test -n "${QUIET}" || echo "${SERVICE}: ${1}"
}

info()
{
    if test -z "${QUIET}"; then
        echo "${SERVICE}: $1" | fold -s
    else
        echo "$1" | fold -s
    fi
}

fail()
{
    log "${1}"
    echo "$1" >&2
    echo "The log file $LOG may contain further information." >&2
    exit 1
}

log()
{
    setup_log
    echo "${1}" >> "${LOG}"
}

module_build_log()
{
    log "Error building the module.  Build output follows."
    echo ""
    echo "${1}" >> "${LOG}"
}

dev=/dev/vboxguest
userdev=/dev/vboxuser
config=/var/lib/VBoxGuestAdditions/config
owner=vboxadd
group=1

if test -r $config; then
  . $config
else
  fail "Configuration file $config not found"
fi
test -n "$INSTALL_DIR" -a -n "$INSTALL_VER" ||
  fail "Configuration file $config not complete"

running_vboxguest()
{
    lsmod | grep -q "vboxguest[^_-]"
}

running_vboxadd()
{
    lsmod | grep -q "vboxadd[^_-]"
}

running_vboxsf()
{
    lsmod | grep -q "vboxsf[^_-]"
}

running_vboxvideo()
{
    lsmod | grep -q "vboxvideo[^_-]"
}

do_vboxguest_non_udev()
{
    if [ ! -c $dev ]; then
        maj=`sed -n 's;\([0-9]\+\) vboxguest;\1;p' /proc/devices`
        if [ ! -z "$maj" ]; then
            min=0
        else
            min=`sed -n 's;\([0-9]\+\) vboxguest;\1;p' /proc/misc`
            if [ ! -z "$min" ]; then
                maj=10
            fi
        fi
        test -n "$maj" || {
            rmmod vboxguest 2>/dev/null
            fail "Cannot locate the VirtualBox device"
        }

        mknod -m 0664 $dev c $maj $min || {
            rmmod vboxguest 2>/dev/null
            fail "Cannot create device $dev with major $maj and minor $min"
        }
    fi
    chown $owner:$group $dev 2>/dev/null || {
        rm -f $dev 2>/dev/null
        rm -f $userdev 2>/dev/null
        rmmod vboxguest 2>/dev/null
        fail "Cannot change owner $owner:$group for device $dev"
    }

    if [ ! -c $userdev ]; then
        maj=10
        min=`sed -n 's;\([0-9]\+\) vboxuser;\1;p' /proc/misc`
        if [ ! -z "$min" ]; then
            mknod -m 0666 $userdev c $maj $min || {
                rm -f $dev 2>/dev/null
                rmmod vboxguest 2>/dev/null
                fail "Cannot create device $userdev with major $maj and minor $min"
            }
            chown $owner:$group $userdev 2>/dev/null || {
                rm -f $dev 2>/dev/null
                rm -f $userdev 2>/dev/null
                rmmod vboxguest 2>/dev/null
                fail "Cannot change owner $owner:$group for device $userdev"
            }
        fi
    fi
}

start()
{
    begin "Starting."
    if test -z "${INSTALL_NO_MODULE_BUILDS}"; then
        setup --quick
        test -d /sys &&
            ps -A -o comm | grep -q '/*udevd$' 2>/dev/null ||
            no_udev=1
        running_vboxguest || {
            rm -f $dev || {
                fail "Cannot remove $dev"
            }
            rm -f $userdev || {
                fail "Cannot remove $userdev"
            }
            $MODPROBE vboxguest >/dev/null 2>&1 ||
                fail "modprobe vboxguest failed"
            case "$no_udev" in 1)
                sleep .5;;
            esac
        }
        case "$no_udev" in 1)
            do_vboxguest_non_udev;;
        esac

        running_vboxsf || {
            $MODPROBE vboxsf > /dev/null 2>&1 ||
                info "modprobe vboxsf failed"
        }
    fi  # INSTALL_NO_MODULE_BUILDS

    # Put the X.Org driver in place.  This is harmless if it is not needed.
    # Also set up the OpenGL library.
    myerr=`"${INSTALL_DIR}/init/vboxadd-x11" setup 2>&1`
    test -z "${myerr}" || log "${myerr}"

    # Mount all shared folders from /etc/fstab. Normally this is done by some
    # other startup script but this requires the vboxdrv kernel module loaded.
    # This isn't necessary anymore as the vboxsf module is autoloaded.
    # mount -a -t vboxsf

    return 0
}

stop()
{
    begin "Stopping."
    test -n "${INSTALL_NO_MODULE_BUILDS}" || setup --quick
    if test -r /etc/ld.so.conf.d/00vboxvideo.conf; then
        rm /etc/ld.so.conf.d/00vboxvideo.conf
        ldconfig
    fi
    if ! umount -a -t vboxsf 2>/dev/null; then
        fail "Cannot unmount vboxsf folders"
    fi
    test -n "${INSTALL_NO_MODULE_BUILDS}" ||
        info "You may need to restart your guest system to finish removing the guest drivers."
    return 0
}

restart()
{
    stop && start
    return 0
}

## Update the initramfs.  Debian and Ubuntu put the graphics driver in, and
# need the touch(1) command below.  Everyone else that I checked just need
# the right module alias file from depmod(1) and only use the initramfs to
# load the root filesystem, not the boot splash.  update-initramfs works
# for the first two and dracut for every one else I checked.  We are only
# interested in distributions recent enough to use the KMS vboxvideo driver.
update_initramfs()
{
    ## kernel version to update for.
    version="${1}"
    depmod "${version}"
    rm -f "/lib/modules/${version}/initrd/vboxvideo"
    test ! -d "/lib/modules/${version}/initrd" ||
        test ! -f "/lib/modules/${version}/misc/vboxvideo.ko" ||
        touch "/lib/modules/${version}/initrd/vboxvideo"

    # Systems without systemd-inhibit probably don't need their initramfs
    # rebuild here anyway.
    type systemd-inhibit >/dev/null 2>&1 || return
    if type dracut >/dev/null 2>&1; then
        systemd-inhibit --why="Installing VirtualBox Guest Additions" \
            dracut -f --kver "${version}"
    elif type update-initramfs >/dev/null 2>&1; then
        systemd-inhibit --why="Installing VirtualBox Guest Additions" \
            update-initramfs -u -k "${version}"
    fi
}

# Remove any existing VirtualBox guest kernel modules from the disk, but not
# from the kernel as they may still be in use
cleanup_modules()
{
    # Needed for Ubuntu and Debian, see update_initramfs
    rm -f /lib/modules/*/initrd/vboxvideo
    for i in /lib/modules/*/misc; do
        KERN_VER="${i%/misc}"
        KERN_VER="${KERN_VER#/lib/modules/}"
        unset do_update
        for j in ${OLDMODULES}; do
            test -f "${i}/${j}.ko" && do_update=1 && rm -f "${i}/${j}.ko"
        done
        test -z "$do_update" || update_initramfs "$KERN_VER"
        # Remove empty /lib/modules folders which may have been kept around
        rmdir -p "${i}" 2>/dev/null || true
        unset keep
        for j in /lib/modules/"${KERN_VER}"/*; do
            name="${j##*/}"
            test -d "${name}" || test "${name%%.*}" != modules && keep=1
        done
        if test -z "${keep}"; then
            rm -rf /lib/modules/"${KERN_VER}"
            rm -f /boot/initrd.img-"${KERN_VER}"
        fi
    done
    for i in ${OLDMODULES}; do
        # We no longer support DKMS, remove any leftovers.
        rm -rf "/var/lib/dkms/${i}"*
    done
    rm -f /etc/depmod.d/vboxvideo-upstream.conf
    rm -f "$SKIPFILE_BASE"-*
}

# Build and install the VirtualBox guest kernel modules
setup_modules()
{
    KERN_VER="$1"
    test -n "$KERN_VER" || return 1
    test ! -f /lib/modules/"$KERN_VER"/misc/vboxguest.ko || return 0
    test ! -f /lib/modules/"$KERN_VER"/misc/vboxguest.o || return 0
    test -d /lib/modules/"$KERN_VER"/build || return 0
    test ! -f "$SKIPFILE_BASE"-"$KERN_VER" || return 0
    export KERN_VER
    info "Building the modules for kernel $KERN_VER."

    log "Building the main Guest Additions module for kernel $KERN_VER."
    if ! myerr=`$BUILDINTMP \
        --save-module-symvers /tmp/vboxguest-Module.symvers \
        --module-source $MODULE_SRC/vboxguest \
        --no-print-directory install 2>&1`; then
        # If check_module_dependencies.sh fails it prints a message itself.
        module_build_log "$myerr"
        "${INSTALL_DIR}"/other/check_module_dependencies.sh 2>&1 &&
            info "Look at $LOG to find out what went wrong"
        return 0
    fi
    log "Building the shared folder support module."
    if ! myerr=`$BUILDINTMP \
        --use-module-symvers /tmp/vboxguest-Module.symvers \
        --module-source $MODULE_SRC/vboxsf \
        --no-print-directory install 2>&1`; then
        module_build_log "$myerr"
        info  "Look at $LOG to find out what went wrong"
        return 0
    fi
    log "Building the graphics driver module."
    if ! myerr=`$BUILDINTMP \
        --use-module-symvers /tmp/vboxguest-Module.symvers \
        --module-source $MODULE_SRC/vboxvideo \
        --no-print-directory install 2>&1`; then
        module_build_log "$myerr"
        info "Look at $LOG to find out what went wrong"
    fi
    [ -d /etc/depmod.d ] || mkdir /etc/depmod.d
    echo "override vboxguest * misc" > /etc/depmod.d/vboxvideo-upstream.conf
    echo "override vboxsf * misc" >> /etc/depmod.d/vboxvideo-upstream.conf
    echo "override vboxvideo * misc" >> /etc/depmod.d/vboxvideo-upstream.conf
    update_initramfs "${KERN_VER}"
    return 0
}

create_vbox_user()
{
    # This is the LSB version of useradd and should work on recent
    # distributions
    useradd -d /var/run/vboxadd -g 1 -r -s /bin/false vboxadd >/dev/null 2>&1 || true
    # And for the others, we choose a UID ourselves
    useradd -d /var/run/vboxadd -g 1 -u 501 -o -s /bin/false vboxadd >/dev/null 2>&1 || true

}

create_udev_rule()
{
    # Create udev description file
    if [ -d /etc/udev/rules.d ]; then
        udev_call=""
        udev_app=`which udevadm 2> /dev/null`
        if [ $? -eq 0 ]; then
            udev_call="${udev_app} version 2> /dev/null"
        else
            udev_app=`which udevinfo 2> /dev/null`
            if [ $? -eq 0 ]; then
                udev_call="${udev_app} -V 2> /dev/null"
            fi
        fi
        udev_fix="="
        if [ "${udev_call}" != "" ]; then
            udev_out=`${udev_call}`
            udev_ver=`expr "$udev_out" : '[^0-9]*\([0-9]*\)'`
            if [ "$udev_ver" = "" -o "$udev_ver" -lt 55 ]; then
               udev_fix=""
            fi
        fi
        ## @todo 60-vboxadd.rules -> 60-vboxguest.rules ?
        echo "KERNEL=${udev_fix}\"vboxguest\", NAME=\"vboxguest\", OWNER=\"vboxadd\", MODE=\"0660\"" > /etc/udev/rules.d/60-vboxadd.rules
        echo "KERNEL=${udev_fix}\"vboxuser\", NAME=\"vboxuser\", OWNER=\"vboxadd\", MODE=\"0666\"" >> /etc/udev/rules.d/60-vboxadd.rules
        # Make sure the new rule is noticed.
        udevadm control --reload >/dev/null 2>&1 || true
        udevcontrol reload_rules >/dev/null 2>&1 || true
    fi
}

create_module_rebuild_script()
{
    # And a post-installation script for rebuilding modules when a new kernel
    # is installed.
    mkdir -p /etc/kernel/postinst.d /etc/kernel/prerm.d
    cat << EOF > /etc/kernel/postinst.d/vboxadd
#!/bin/sh
# This only works correctly on Debian derivatives - Red Hat calls it before
# installing the right header files.
/sbin/rcvboxadd quicksetup "\${1}"
exit 0
EOF
    cat << EOF > /etc/kernel/prerm.d/vboxadd
#!/bin/sh
for i in ${OLDMODULES}; do rm -f /lib/modules/"\${1}"/misc/"\${i}".ko; done
rmdir -p /lib/modules/"\$1"/misc 2>/dev/null
exit 0
EOF
    chmod 0755 /etc/kernel/postinst.d/vboxadd /etc/kernel/prerm.d/vboxadd
}

shared_folder_setup()
{
    # Add a group "vboxsf" for Shared Folders access
    # All users which want to access the auto-mounted Shared Folders have to
    # be added to this group.
    groupadd -r -f vboxsf >/dev/null 2>&1

    # Put the mount.vboxsf mount helper in the right place.
    ## @todo It would be nicer if the kernel module just parsed parameters
    # itself instead of needing a separate binary to do that.
    ln -sf "${INSTALL_DIR}/other/mount.vboxsf" /sbin
    # SELinux security context for the mount helper.
    if test -e /etc/selinux/config; then
        # This is correct.  semanage maps this to the real path, and it aborts
        # with an error, telling you what you should have typed, if you specify
        # the real path.  The "chcon" is there as a back-up for old guests.
        command -v semanage > /dev/null &&
            semanage fcontext -a -t mount_exec_t "${INSTALL_DIR}/other/mount.vboxsf"
        chcon -t mount_exec_t "${INSTALL_DIR}/other/mount.vboxsf" 2>/dev/null
    fi
}

# setup_script
setup()
{
    export BUILD_TYPE
    export USERNAME

    test x"$1" != x--quick || QUICKSETUP=true
    test -n "$QUICKSETUP" || cleanup
    MODULE_SRC="$INSTALL_DIR/src/vboxguest-$INSTALL_VER"
    BUILDINTMP="$MODULE_SRC/build_in_tmp"
    # chcon is needed on old Fedora/Redhat systems.  No one remembers which.
    test ! -e /etc/selinux/config ||
        chcon -t bin_t "$BUILDINTMP" 2>/dev/null

    if test -z "$INSTALL_NO_MODULE_BUILDS"; then
        if test -z "$QUICKSETUP"; then
            info "Building the VirtualBox Guest Additions kernel modules.  This may take a while."
            info "To build modules for other installed kernels, run"
            info "  /sbin/rcvboxadd quicksetup <version>"
            for setupi in /lib/modules/*; do
                KERN_VER="${setupi##*/}"
                # For a full setup, mark kernels we do not want to build.
                touch "$SKIPFILE_BASE"-"$KERN_VER"
            done
        fi
        # That is, we mark all but the requested kernel.
        rm -f "$SKIPFILE_BASE"-"$TARGET_VER"
        test -d /lib/modules/"$TARGET_VER"/build || test -n "$QUICKSETUP" ||
            info "Kernel headers not found for target kernel $TARGET_VER. \
Please install them and execute
  /sbin/rcvboxadd setup"
        for setupi in /lib/modules/*; do
            KERN_VER="${setupi##*/}"
            setup_modules "$KERN_VER"
        done
        depmod
    fi
    create_vbox_user
    create_udev_rule
    test -n "${INSTALL_NO_MODULE_BUILDS}" || create_module_rebuild_script
    test -z "$QUICKSETUP" || return 0
    shared_folder_setup
    if  running_vboxguest || running_vboxadd; then
        info "Running kernel modules will not be replaced until the system is restarted"
    fi
    return 0
}

# cleanup_script
cleanup()
{
    if test -z "${INSTALL_NO_MODULE_BUILDS}"; then
        # Delete old versions of VBox modules.
        cleanup_modules
        depmod

        # Remove old module sources
        for i in $OLDMODULES; do
          rm -rf /usr/src/$i-*
        done
    fi

    # Clean-up X11-related bits
    "${INSTALL_DIR}/init/vboxadd-x11" cleanup

    # Remove other files
    if test -z "${INSTALL_NO_MODULE_BUILDS}"; then
        rm -f /etc/kernel/postinst.d/vboxadd /etc/kernel/prerm.d/vboxadd
        rmdir -p /etc/kernel/postinst.d /etc/kernel/prerm.d 2>/dev/null
    fi
    rm /sbin/mount.vboxsf 2>/dev/null
    rm /etc/udev/rules.d/60-vboxadd.rules 2>/dev/null
    udevadm control --reload >/dev/null 2>&1 || true
    udevcontrol reload_rules >/dev/null 2>&1 || true
}

dmnstatus()
{
    if running_vboxguest; then
        echo "The VirtualBox Additions are currently running."
    else
        echo "The VirtualBox Additions are not currently running."
    fi
}

case "$2" in quiet)
    QUIET=yes;;
esac
case "$1" in
start)
    start
    ;;
stop)
    stop
    ;;
restart)
    restart
    ;;
setup)
    setup
    start
    ;;
quicksetup)
    test -z "$2" || test ! -d /lib/modules/"$2"/build || TARGET_VER="$2"
    setup --quick
    ;;
cleanup)
    cleanup
    ;;
status)
    dmnstatus
    ;;
*)
    echo "Usage: $0 {start|stop|restart|status|setup|quicksetup|cleanup} [quiet]"
    exit 1
esac

exit
