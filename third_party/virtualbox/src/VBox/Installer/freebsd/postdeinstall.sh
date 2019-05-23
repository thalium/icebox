#!/bin/sh
## @file
#
# VirtualBox postdeinstall script for FreeBSD.
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

echo "Removing kernel modules, please wait..."

kldunload vboxnetadp
kldunload vboxnetflt
kldunload vboxdrv
rm /boot/kernel/vboxnetflt.ko
rm /boot/kernel/vboxnetadp.ko
rm /boot/kernel/vboxdrv.ko
kldxref -R /boot

echo "Kernel modules successfully removed."

exit 0

