/* $Id: VBoxAPI-start.d $ */
/** @file
 * VBoxAPI - Static dtrace probes.
 */

/*
 * Copyright (C) 2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

provider vboxapi
{
    /* Manually defined probes: */
    probe machine__state__changed(void *a_pMachine, int a_enmNewState, int a_enmOldState, const char *pszMachineUuid);

    /* The following probes are automatically generated and changes with the API: */
