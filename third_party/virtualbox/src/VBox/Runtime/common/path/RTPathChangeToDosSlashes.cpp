/* $Id: RTPathChangeToDosSlashes.cpp $ */
/** @file
 * IPRT - RTPathChangeToDosSlashes
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#include "internal/iprt.h"
#include <iprt/path.h>


/**
 * Changes all the slashes in the specified path to DOS style.
 *
 * Unless @a fForce is set, nothing will be done when on a UNIX flavored system
 * since paths wont work with DOS style slashes there.
 *
 * @returns @a pszPath.
 * @param   pszPath             The path to modify.
 * @param   fForce              Whether to force the conversion on non-DOS OSes.
 */
RTDECL(char *) RTPathChangeToDosSlashes(char *pszPath, bool fForce)
{
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    RT_NOREF_PV(fForce);
#else
    if (fForce)
#endif
    {
        char    ch;
        char   *psz = pszPath;
        while ((ch = *psz) != '\0')
        {
            if (ch == '/')
                *psz = '\\';
            psz++;
        }
    }
    return pszPath;
}

