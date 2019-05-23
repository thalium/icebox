/* $Id: DBGPlugIns.h $ */
/** @file
 * DBGPlugIns - Debugger Plug-Ins.
 *
 * This is just a temporary static wrapper for what may eventually
 * become some fancy dynamic stuff.
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

#ifndef ___Debugger_DBGPlugIns_h
#define ___Debugger_DBGPlugIns_h

#include <VBox/vmm/dbgf.h>

RT_C_DECLS_BEGIN

extern const DBGFOSREG g_DBGDiggerFreeBsd;
extern const DBGFOSREG g_DBGDiggerDarwin;
extern const DBGFOSREG g_DBGDiggerLinux;
extern const DBGFOSREG g_DBGDiggerOS2;
extern const DBGFOSREG g_DBGDiggerSolaris;
extern const DBGFOSREG g_DBGDiggerWinNt;

RT_C_DECLS_END

#endif

