/* $Id: return-to-ring-3-aggregation-1.d $ */
/** @file
 * DTracing VBox - return to ring-3 aggregation test \#1.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#pragma D option quiet


vboxvmm:::r0-vmm-return-to-ring3-hm
{
    @g_aHmRcs[args[2]] = count();
}

vboxvmm:::r0-vmm-return-to-ring3-rc
{
    @g_aRawRcs[args[2]] = count();
}

END
{
    printa(" rcHm=%04d   %@8u times\n", @g_aHmRcs);
    printa(" rcRaw=%04d   %@8u times\n", @g_aRawRcs);
}

