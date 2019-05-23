/* $Id: modulestub-r0drv-solaris.c $ */
/** @file
 * IPRT - Ring-0 Solaris stubs
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <sys/modctl.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static struct modlmisc g_rtModuleStubMisc =
{
    &mod_miscops,           /* extern from kernel */
    "platform agnostic module"
};


static struct modlinkage g_rtModuleStubModLinkage =
{
    MODREV_1,   /* loadable module system revision */
    {
        &g_rtModuleStubMisc,
        NULL    /* terminate array of linkage structures */
    }
};



int _init(void);
int _init(void)
{
    /* Disable auto unloading. */
    modctl_t *pModCtl = mod_getctl(&g_rtModuleStubModLinkage);
    if (pModCtl)
        pModCtl->mod_loadflags |= MOD_NOAUTOUNLOAD;

    return mod_install(&g_rtModuleStubModLinkage);
}


int _fini(void);
int _fini(void)
{
    return mod_remove(&g_rtModuleStubModLinkage);
}


int _info(struct modinfo *pModInfo);
int _info(struct modinfo *pModInfo)
{
    return mod_info(&g_rtModuleStubModLinkage, pModInfo);
}

