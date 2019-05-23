#!/bin/bash
# $Id: load.sh $
## @file
# Driver load script.
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

outdir=out/haiku.x86/debug/bin/additions
instdir=/boot/apps/VBoxAdditions


# vboxguest
mkdir -p ~/config/add-ons/kernel/generic/
cp $outdir/vboxguest ~/config/add-ons/kernel/generic/

# vboxdev
mkdir -p ~/config/add-ons/kernel/drivers/dev/misc/
cp $outdir/vboxdev ~/config/add-ons/kernel/drivers/bin/
ln -sf ../../bin/vboxdev ~/config/add-ons/kernel/drivers/dev/misc

# VBoxMouse
cp $outdir/VBoxMouse        ~/config/add-ons/input_server/devices/
cp $outdir/VBoxMouseFilter  ~/config/add-ons/input_server/filters/

# Services
mkdir -p $instdir
cp $outdir/VBoxService $instdir/
cp $outdir/VBoxTray    $instdir/
cp $outdir/VBoxControl $instdir/
ln -sf $instdir/VBoxService ~/config/boot/launch

