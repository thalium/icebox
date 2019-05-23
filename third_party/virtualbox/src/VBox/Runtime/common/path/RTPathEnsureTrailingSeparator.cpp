/* $Id: RTPathEnsureTrailingSeparator.cpp $ */
/** @file
 * IPRT - RTPathEnsureTrailingSeparator
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#include <iprt/string.h>
#include <iprt/ctype.h>



RTDECL(size_t) RTPathEnsureTrailingSeparator(char *pszPath, size_t cbPath)
{
    size_t off = strlen(pszPath);
    if (off > 0)
    {
        char ch = pszPath[off - 1];
        if (RTPATH_IS_SLASH(ch) || RTPATH_IS_VOLSEP(ch))
            return off;
        if (off + 2 <= cbPath)
        {
            pszPath[off++] = RTPATH_SLASH;
            pszPath[off]   = '\0';
            return off;
        }
    }
    else if (off + 3 <= cbPath)
    {
        pszPath[off++] = '.';
        pszPath[off++] = RTPATH_SLASH;
        pszPath[off]   = '\0';
        return off;
    }

    return 0;
}
RT_EXPORT_SYMBOL(RTPathEnsureTrailingSeparator);

