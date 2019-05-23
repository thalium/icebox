/* $Id: DynLoadLibSolaris.cpp $ */
/** @file
 * Dynamically load libraries for Solaris hosts.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "DynLoadLibSolaris.h"

#include <iprt/err.h>
#include <iprt/ldr.h>


/** -=-=-=-=-= LIB DLPI -=-=-=-=-=-=- **/

/**
 * Global pointer to the libdlpi module. This should only be set once all needed libraries
 * and symbols have been successfully loaded.
 */
static RTLDRMOD g_hLibDlpi = NIL_RTLDRMOD;

/**
 * Whether we have tried to load libdlpi yet.  This flag should only be set
 * to "true" after we have either loaded both libraries and all symbols which we need,
 * or failed to load something and unloaded.
 */
static bool g_fCheckedForLibDlpi = false;

/** All the symbols we need from libdlpi.
 * @{
 */
int (*g_pfnLibDlpiWalk)(dlpi_walkfunc_t *, void *, uint_t);
int (*g_pfnLibDlpiOpen)(const char *, dlpi_handle_t *, uint_t);
void (*g_pfnLibDlpiClose)(dlpi_handle_t);
/** @} */

bool VBoxSolarisLibDlpiFound(void)
{
    RTLDRMOD hLibDlpi;

    if (g_fCheckedForLibDlpi)
        return g_hLibDlpi != NIL_RTLDRMOD;
    g_fCheckedForLibDlpi = true;
    int rc = RTLdrLoad(LIB_DLPI, &hLibDlpi);
    if (RT_SUCCESS(rc))
    {
        /*
         * Unfortunately; we cannot make use of dlpi_get_physaddr because it requires us to
         * open the VNIC/link which requires root permissions :/
         */
        rc  = RTLdrGetSymbol(hLibDlpi, "dlpi_walk", (void **)&g_pfnLibDlpiWalk);
        rc |= RTLdrGetSymbol(hLibDlpi, "dlpi_close", (void **)&g_pfnLibDlpiClose);
        rc |= RTLdrGetSymbol(hLibDlpi, "dlpi_open", (void **)&g_pfnLibDlpiOpen);
        if (RT_SUCCESS(rc))
        {
            g_hLibDlpi = hLibDlpi;
            return true;
        }

        RTLdrClose(hLibDlpi);
    }
    hLibDlpi = NIL_RTLDRMOD;
    return false;
}

