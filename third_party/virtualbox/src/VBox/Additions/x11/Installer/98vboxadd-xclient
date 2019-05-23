#!/bin/sh
## @file
# Start the Guest Additions X11 Client
#

#
# Copyright (C) 2007-2012 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

# Sanity check: if non-writeable PID-files are present in the user home
# directory VBoxClient will fail to start.
for i in $HOME/.vboxclient-*.pid; do
    test -w $i || rm -f $i
done

if ! test -c /dev/vboxguest 2>/dev/null; then
   # Do not start if the kernel module is not present.
   notify-send "VBoxClient: the VirtualBox kernel service is not running.  Exiting."
elif test -z "${SSH_CONNECTION}"; then
   # This script can also be triggered by a connection over SSH, which is not
   # what we had in mind, so we do not start VBoxClient in that case.  We do
   # not use "exit" here as this script is "source"d, not executed.
  /usr/bin/VBoxClient --clipboard
  /usr/bin/VBoxClient --checkhostversion
  /usr/bin/VBoxClient --display
  /usr/bin/VBoxClient --seamless
  /usr/bin/VBoxClient --draganddrop
fi
