/* $Id: VBoxDD2R0.cpp $ */
/** @file
 * VBoxDD2R0 - Built-in drivers & devices (part 2), ring-0 module.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/types.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#if defined(RT_OS_SOLARIS) && defined(IN_RING0)
/* Dependency information for the native solaris loader. */
extern "C" { char _depends_on[] = "vboxdrv VMMR0.r0"; }
#endif

