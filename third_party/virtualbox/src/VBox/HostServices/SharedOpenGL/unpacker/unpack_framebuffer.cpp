/* $Id: unpack_framebuffer.cpp $ */
/** @file
 * VBox OpenGL: EXT_framebuffer_object
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#include "cr_string.h"
#include "cr_version.h"

void crUnpackExtendDeleteFramebuffersEXT(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLsizei);

    GLsizei n = READ_DATA(pState, 8, GLsizei );
    const GLuint *buffers = DATA_POINTER(pState, 12, GLuint );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, buffers, n, GLuint);
    pState->pDispatchTbl->DeleteFramebuffersEXT( n, buffers );
}

void crUnpackExtendDeleteRenderbuffersEXT(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLsizei);

    GLsizei n = READ_DATA(pState, 8, GLsizei );
    const GLuint *buffers = DATA_POINTER(pState, 12, GLuint );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, buffers, n, GLuint);
    pState->pDispatchTbl->DeleteRenderbuffersEXT( n, buffers );
}
