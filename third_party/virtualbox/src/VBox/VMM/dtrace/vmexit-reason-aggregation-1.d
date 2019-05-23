/* $Id: vmexit-reason-aggregation-1.d $ */
/** @file
 * DTracing VBox - vmexit reason aggregation test \#1.
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


vboxvmm:::r0-hmsvm-vmexit
{
    @g_aSvmExits[args[2]] = count();
}

vboxvmm:::r0-hmvmx-vmexit-noctx
{
    @g_aVmxExits[args[2]] = count();
}

END
{
    printa(" svmexit=%#04llx   %@10u times\n", @g_aSvmExits);
    printa(" vmxexit=%#04llx   %@10u times\n", @g_aVmxExits);
}

