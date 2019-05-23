/* $Id: vmexit-rip-aggregation-1.d $ */
/** @file
 * DTracing VBox - vmexit rip aggregation test \#1.
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


vboxvmm:::r0-hmsvm-vmexit,vboxvmm:::r0-hmvmx-vmexit
{
    /*printf("cs:rip=%02x:%08llx", args[1]->cs.Sel, args[1]->rip.rip);*/
    @g_aRips[args[1]->rip.rip] = count();
    /*@g_aRips[args[0]->cpum.s.Guest.rip.rip] = count(); - alternative access route */
}

END
{
    printa(" rip=%#018llx   %@4u times\n", @g_aRips);
}

