#!/usr/bin/perl -w
# $Revision: 83575 $
#
# Restore xorg.conf while removing Guest Additions.
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


my $os_type=`uname -s`;
my @cfg_files = ("/etc/X11/xorg.conf-4", "/etc/X11/xorg.conf", "/etc/X11/.xorg.conf", "/etc/xorg.conf",
                 "/usr/etc/X11/xorg.conf-4", "/usr/etc/X11/xorg.conf", "/usr/lib/X11/xorg.conf-4",
                 "/usr/lib/X11/xorg.conf", "/etc/X11/XF86Config-4", "/etc/X11/XF86Config",
                 "/etc/XF86Config", "/usr/X11R6/etc/X11/XF86Config-4", "/usr/X11R6/etc/X11/XF86Config",
                 "/usr/X11R6/lib/X11/XF86Config-4", "/usr/X11R6/lib/X11/XF86Config");
my $CFG;
my $BAK;

my $config_count = 0;
my $vboxpresent = "vboxvideo";

foreach $cfg (@cfg_files)
{
    if (open(CFG, $cfg))
    {
        @array=<CFG>;
        close(CFG);

        foreach $line (@array)
        {
            if ($line =~ /$vboxpresent/)
            {
                if (open(BAK, $cfg.".bak"))
                {
                    close(BAK);
                    rename $cfg.".bak", $cfg;
                }
                else
                {
                    # On Solaris just delete existing conf if backup is not found (Possible on distros like Indiana)
                    if ($os_type =~ 'SunOS')
                    {
                        unlink $cfg
                    }
                    else
                    {
                        die "Failed to restore xorg.conf! Your existing config. still uses VirtualBox drivers!!";
                    }
                }
            }
        }
        $config_count++;
    }
}

$config_count != 0 or die "Could not find backed-up xorg.conf to restore it.";

