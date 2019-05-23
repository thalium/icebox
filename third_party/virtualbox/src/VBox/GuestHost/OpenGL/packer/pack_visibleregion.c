/* $Id: pack_visibleregion.c $ */
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

#include "packer.h"
#include "cr_opcodes.h"
#include "cr_error.h"

#ifdef WINDOWS
# include <iprt/win/windows.h>
#endif

void PACK_APIENTRY crPackWindowVisibleRegion( CR_PACKER_CONTEXT_ARGDECL GLint window, GLint cRects, const GLint * pRects )
{
    GLint i, size, cnt;

    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    size = 16 + cRects * 4 * sizeof(GLint);
    CR_GET_BUFFERED_POINTER( pc, size );
    WRITE_DATA( 0, GLint, size );
    WRITE_DATA( 4, GLenum, CR_WINDOWVISIBLEREGION_EXTEND_OPCODE );
    WRITE_DATA( 8, GLint, window );
    WRITE_DATA( 12, GLint, cRects );

    cnt = 16;
    for (i=0; i<cRects; ++i)
    {
        WRITE_DATA(cnt, GLint, (GLint) pRects[4*i+0]);
        WRITE_DATA(cnt+4, GLint, (GLint) pRects[4*i+1]);
        WRITE_DATA(cnt+8, GLint, (GLint) pRects[4*i+2]);
        WRITE_DATA(cnt+12, GLint, (GLint) pRects[4*i+3]);
        cnt += 16;
    }
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackWindowVisibleRegionSWAP( CR_PACKER_CONTEXT_ARGDECL  GLint window, GLint cRects, const GLint * pRects )
{
    RT_NOREF3(window, cRects, pRects); CR_PACKER_CONTEXT_ARG_NOREF();
    crError( "crPackWindowVisibleRegionSWAP unimplemented and shouldn't be called" );
}

