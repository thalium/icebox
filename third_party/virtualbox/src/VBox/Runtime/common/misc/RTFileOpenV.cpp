/* $Id: RTFileOpenV.cpp $ */
/** @file
 * IPRT - RTFileOpenV.
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
#include <iprt/file.h>
#include "internal/iprt.h"

#include <iprt/err.h>
#include <iprt/param.h>
#include <iprt/string.h>


RTR3DECL(int) RTFileOpenV(PRTFILE pFile, uint64_t fOpen, const char *pszFilenameFmt, va_list va)
{
    char szFilename[RTPATH_MAX];
    size_t cchFilename = RTStrPrintfV(szFilename, sizeof(szFilename), pszFilenameFmt, va);
    if (cchFilename >= sizeof(szFilename) - 1)
        return VERR_FILENAME_TOO_LONG;
    return RTFileOpen(pFile, szFilename, fOpen);
}
RT_EXPORT_SYMBOL(RTFileOpenV);

