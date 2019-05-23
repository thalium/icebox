/** @file
 *
 * Shared Folders:
 * Handles helper functions header.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __SHFLHANDLE__H
#define __SHFLHANDLE__H

#include "shfl.h"
#include <VBox/shflsvc.h>
#include <iprt/dir.h>

#define SHFL_HF_TYPE_MASK       (0x000000FF)
#define SHFL_HF_TYPE_DIR        (0x00000001)
#define SHFL_HF_TYPE_FILE       (0x00000002)
#define SHFL_HF_TYPE_VOLUME     (0x00000004)
#define SHFL_HF_TYPE_DONTUSE    (0x00000080)

#define SHFL_HF_VALID           (0x80000000)

#define SHFLHANDLE_MAX          (4096)

typedef struct _SHFLHANDLEHDR
{
    uint32_t u32Flags;
} SHFLHANDLEHDR;

#define ShflHandleType(__Handle) BIT_FLAG(((SHFLHANDLEHDR *)(__Handle))->u32Flags, SHFL_HF_TYPE_MASK)

typedef struct _SHFLFILEHANDLE
{
    SHFLHANDLEHDR Header;
    SHFLROOT root; /* Where the handle has been opened. */
    union
    {
        struct
        {
            RTFILE        Handle;
        } file;
        struct
        {
            PRTDIR        Handle;
            PRTDIR        SearchHandle;
            PRTDIRENTRYEX pLastValidEntry; /* last found file in a directory search */
        } dir;
    };
} SHFLFILEHANDLE;


SHFLHANDLE      vbsfAllocDirHandle(PSHFLCLIENTDATA pClient);
SHFLHANDLE      vbsfAllocFileHandle(PSHFLCLIENTDATA pClient);
void            vbsfFreeFileHandle (PSHFLCLIENTDATA pClient, SHFLHANDLE hHandle);


int         vbsfInitHandleTable();
int         vbsfFreeHandleTable();
SHFLHANDLE  vbsfAllocHandle(PSHFLCLIENTDATA pClient, uint32_t uType,
                            uintptr_t pvUserData);
SHFLFILEHANDLE *vbsfQueryFileHandle(PSHFLCLIENTDATA pClient,
                                    SHFLHANDLE handle);
SHFLFILEHANDLE *vbsfQueryDirHandle(PSHFLCLIENTDATA pClient, SHFLHANDLE handle);
uint32_t        vbsfQueryHandleType(PSHFLCLIENTDATA pClient,
                                    SHFLHANDLE handle);

#endif /* __SHFLHANDLE__H */
