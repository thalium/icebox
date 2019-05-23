#!/bin/bash

basedir=/boot/home/config/add-ons/
rm -f $basedir/input_server/devices/VBoxMouse
rm -f $basedir/kernel/drivers/bin/vboxdev
rm -f $basedir/kernel/drivers/dev/misc/vboxdev
rm -f $basedir/kernel/file_systems/vboxsf
rm -f $basedir/kernel/generic/vboxguest
rm -rf /boot/apps/VBoxAdditions

