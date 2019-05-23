/** $Id: VDScriptChecker.cpp $ */
/** @file
 *
 * VBox HDD container test utility - scripting engine, type and context checker.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#define LOGGROUP LOGGROUP_DEFAULT
#include <iprt/list.h>
#include <iprt/mem.h>
#include <iprt/assert.h>

#include <VBox/log.h>

#include "VDScriptAst.h"
#include "VDScriptInternal.h"

DECLHIDDEN(int) vdScriptCtxCheck(PVDSCRIPTCTXINT pThis)
{
    RT_NOREF1(pThis);
    return VERR_NOT_IMPLEMENTED;
}
