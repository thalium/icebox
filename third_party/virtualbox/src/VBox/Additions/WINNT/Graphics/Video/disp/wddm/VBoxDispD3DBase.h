/* $Id: VBoxDispD3DBase.h $ */
/** @file
 * VBoxVideo Display D3D Base Include
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

#ifndef ___VBoxDispD3DBase_h
#define ___VBoxDispD3DBase_h

/* Include nt-and-windows.h here so we get NT_SUCCESS, but don't try if
   something windowsy is already included because that'll cause conflicts. */
#ifndef STATUS_WAIT_0
# include <iprt/nt/nt-and-windows.h>
#endif

#include <d3d9types.h>
//#include <d3dtypes.h>
#include <D3dumddi.h>
#include <d3dhal.h>

#endif

