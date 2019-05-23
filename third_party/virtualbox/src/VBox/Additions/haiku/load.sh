#!/bin/bash

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

