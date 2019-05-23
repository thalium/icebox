/* $Id: unpack_framebuffer.c $ */
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

void crUnpackExtendDeleteFramebuffersEXT(void)
{
    GLsizei n = READ_DATA( 8, GLsizei );
    const GLuint *buffers = DATA_POINTER( 12, GLuint );
    cr_unpackDispatch.DeleteFramebuffersEXT( n, buffers );
}

void crUnpackExtendDeleteRenderbuffersEXT(void)
{
    GLsizei n = READ_DATA( 8, GLsizei );
    const GLuint *buffers = DATA_POINTER( 12, GLuint );
    cr_unpackDispatch.DeleteRenderbuffersEXT( n, buffers );
}
