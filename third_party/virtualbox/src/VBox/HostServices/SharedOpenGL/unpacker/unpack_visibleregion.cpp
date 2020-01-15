/* $Id: unpack_visibleregion.cpp $ */
/** @file
 * VBox Packing VisibleRegion information
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

#include "unpacker.h"
#include "cr_error.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_version.h"

void crUnpackExtendWindowVisibleRegion(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLint);

    GLint window = READ_DATA(pState, 8, GLint );
    GLint cRects = READ_DATA(pState, 12, GLint );
    GLvoid *pRects = DATA_POINTER(pState, 16, GLvoid );

    if (cRects <= 0 || cRects >= INT32_MAX / sizeof(GLint) / 8)
    {
        crError("crUnpackExtendWindowVisibleRegion: parameter 'cRects' is out of range");
        return;
    }

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pRects, cRects * 4, GLint);

    pState->pDispatchTbl->WindowVisibleRegion( window, cRects, (GLint *)pRects );
}
