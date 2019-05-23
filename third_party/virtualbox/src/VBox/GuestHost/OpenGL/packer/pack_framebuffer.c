/* $Id: pack_framebuffer.c $ */

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

#include "packer.h"
#include "cr_error.h"
#include "cr_string.h"

void PACK_APIENTRY
crPackDeleteRenderbuffersEXT(GLsizei n, const GLuint * renderbuffers)
{
    unsigned char *data_ptr;
    int packet_length = sizeof(GLenum) + sizeof(n) + n*sizeof(*renderbuffers);

    if (!renderbuffers)
        return;

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, CR_DELETERENDERBUFFERSEXT_EXTEND_OPCODE);
    WRITE_DATA(4, GLsizei, n);
    crMemcpy(data_ptr + 8, renderbuffers, n* sizeof(*renderbuffers));
    crHugePacket(CR_EXTEND_OPCODE, data_ptr);
    crPackFree(data_ptr);
}

void PACK_APIENTRY
crPackDeleteFramebuffersEXT(GLsizei n, const GLuint * framebuffers)
{
    unsigned char *data_ptr;
    int packet_length = sizeof(GLenum) + sizeof(n) + n*sizeof(*framebuffers);

    if (!framebuffers)
        return;

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, CR_DELETEFRAMEBUFFERSEXT_EXTEND_OPCODE);
    WRITE_DATA(4, GLsizei, n);
    crMemcpy(data_ptr + 8, framebuffers, n* sizeof(*framebuffers));
    crHugePacket(CR_EXTEND_OPCODE, data_ptr);
    crPackFree(data_ptr);
}

void PACK_APIENTRY
crPackDeleteRenderbuffersEXTSWAP(GLsizei n, const GLuint * renderbuffers)
{
    (void) n;
    (void) renderbuffers;
    crError ("No swap version");
}

void PACK_APIENTRY
crPackDeleteFramebuffersEXTSWAP(GLsizei n, const GLuint * framebuffers)
{
    (void) n;
    (void) framebuffers;
    crError ("No swap version");
}
