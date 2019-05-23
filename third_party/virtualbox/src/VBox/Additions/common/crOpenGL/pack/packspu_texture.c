/* $Id: packspu_texture.c $ */

/** @file
 * VBox OpenGL DRI driver functions
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

#include "packspu.h"
#include "cr_packfunctions.h"
#include "cr_glstate.h"
#include "packspu_proto.h"

void PACKSPU_APIENTRY packspu_ActiveTextureARB(GLenum texture)
{
    crStateActiveTextureARB(texture);
    crPackActiveTextureARB(texture);
}

void PACKSPU_APIENTRY packspu_BindTexture(GLenum target, GLuint texture)
{
    crStateBindTexture(target, texture);
    crPackBindTexture(target, texture);
}

void PACKSPU_APIENTRY packspu_DeleteTextures(GLsizei n, const GLuint * textures)
{
    crStateDeleteTextures(n, textures);
    crPackDeleteTextures(n, textures);
}

void PACKSPU_APIENTRY packspu_GenTextures( GLsizei n, GLuint * textures )
{
    GET_THREAD(thread);
    int writeback = 1;
    unsigned int i;
    if (!CRPACKSPU_IS_WDDM_CRHGSMI() && !(pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network))
    {
        crError( "packspu_GenTextures doesn't work when there's no actual network involved!\nTry using the simplequery SPU in your chain!" );
    }
    if (pack_spu.swap)
    {
        crPackGenTexturesSWAP( n, textures, &writeback );
    }
    else
    {
        crPackGenTextures( n, textures, &writeback );
    }
    packspuFlush( (void *) thread );
    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    if (pack_spu.swap)
    {
        for (i = 0 ; i < (unsigned int) n ; i++)
        {
            textures[i] = SWAP32(textures[i]);
        }
    }

    crStateRegTextures(n, textures);
}
