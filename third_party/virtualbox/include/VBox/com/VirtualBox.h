/** @file
 * MS COM / XPCOM Abstraction Layer - VirtualBox COM Library definitions.
 *
 * @note This is the main header file that COM/XPCOM clients include; however,
 *       it is only a wrapper around another platform-dependent include file
 *       that contains the real COM/XPCOM interface declarations.  That other
 *       include file is generated automatically at build time from
 *       /src/VBox/Main/idl/VirtualBox.xidl, which contains all the VirtualBox
 *       interfaces; the include file is called VirtualBox.h on Windows hosts
 *       and VirtualBox_XPCOM.h on Linux hosts.  The build process places it in
 *       out/{platform}/bin/sdk/include, from where it gets
 *       included by the rest of the VirtualBox code.
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

#ifndef ___VBox_com_VirtualBox_h
#define ___VBox_com_VirtualBox_h

// generated VirtualBox COM library definition file
#if !defined(VBOXCOM_NOINCLUDE)
# if !defined(VBOX_WITH_XPCOM)
#  include <VirtualBox.h>
# else
#  include <VirtualBox_XPCOM.h>
# endif
#endif

// for convenience
#include "VBox/com/defs.h"
#include "VBox/com/ptr.h"

#endif

