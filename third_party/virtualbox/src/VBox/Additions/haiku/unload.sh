#!/bin/bash
# $Id: unload.sh $
## @file
# Driver unload script.
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

basedir=/boot/home/config/add-ons/
rm -f $basedir/input_server/devices/VBoxMouse
rm -f $basedir/kernel/drivers/bin/vboxdev
rm -f $basedir/kernel/drivers/dev/misc/vboxdev
rm -f $basedir/kernel/file_systems/vboxsf
rm -f $basedir/kernel/generic/vboxguest
rm -rf /boot/apps/VBoxAdditions

