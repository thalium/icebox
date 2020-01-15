/* $Id: server_framebuffer.cpp $ */
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

#include "cr_spu.h"
#include "chromium.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"
#include "cr_unpack.h"

void SERVER_DISPATCH_APIENTRY
crServerDispatchGenFramebuffersEXT(GLsizei n, GLuint *framebuffers)
{
    GLuint *local_buffers;
    (void) framebuffers;

    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint))
    {
        crError("crServerDispatchGenFramebuffersEXT: parameter 'n' is out of range");
        return;
    }

    local_buffers = (GLuint *)crCalloc(n * sizeof(*local_buffers));

    crStateGenFramebuffersEXT(&cr_server.StateTracker, n, local_buffers);

    crServerReturnValue(local_buffers, n * sizeof(*local_buffers));
    crFree(local_buffers);
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchGenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers)
{
    GLuint *local_buffers;
    (void) renderbuffers;

    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint))
    {
        crError("crServerDispatchGenRenderbuffersEXT: parameter 'n' is out of range");
        return;
    }

    local_buffers = (GLuint *)crCalloc(n * sizeof(*local_buffers));

    crStateGenRenderbuffersEXT(&cr_server.StateTracker, n, local_buffers);

    crServerReturnValue(local_buffers, n * sizeof(*local_buffers));
    crFree(local_buffers);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    crStateFramebufferTexture1DEXT(&cr_server.StateTracker, target, attachment, textarget, texture, level);
    cr_server.head_spu->dispatch_table.FramebufferTexture1DEXT(target, attachment, textarget, crStateGetTextureHWID(&cr_server.StateTracker, texture), level);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    crStateFramebufferTexture2DEXT(&cr_server.StateTracker, target, attachment, textarget, texture, level);
    cr_server.head_spu->dispatch_table.FramebufferTexture2DEXT(target, attachment, textarget, crStateGetTextureHWID(&cr_server.StateTracker, texture), level);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
    crStateFramebufferTexture3DEXT(&cr_server.StateTracker, target, attachment, textarget, texture, level, zoffset);
    cr_server.head_spu->dispatch_table.FramebufferTexture3DEXT(target, attachment, textarget, crStateGetTextureHWID(&cr_server.StateTracker, texture), level, zoffset);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchBindFramebufferEXT(GLenum target, GLuint framebuffer)
{
#ifdef DEBUG_misha
    GLint rfb = 0, dfb = 0;
#endif
    crStateBindFramebufferEXT(&cr_server.StateTracker, target, framebuffer);

    if (0==framebuffer)
    {
        CRContext *ctx = crStateGetCurrent(&cr_server.StateTracker);
        if (ctx->buffer.drawBuffer == GL_FRONT || ctx->buffer.drawBuffer == GL_FRONT_LEFT || ctx->buffer.drawBuffer == GL_FRONT_RIGHT)
            cr_server.curClient->currentMural->bFbDraw = GL_TRUE;
    }

    if (0==framebuffer && crServerIsRedirectedToFBO())
    {
        CRMuralInfo *mural = cr_server.curClient->currentMural;
        if (target == GL_FRAMEBUFFER)
        {
            GLuint idDrawFBO = CR_SERVER_FBO_FOR_IDX(mural, mural->iCurDrawBuffer);
            GLuint idReadFBO = CR_SERVER_FBO_FOR_IDX(mural, mural->iCurReadBuffer);
            if (idDrawFBO == idReadFBO)
                cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_FRAMEBUFFER, idDrawFBO);
            else
            {
                cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_READ_FRAMEBUFFER, idReadFBO);
                cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, idDrawFBO);
            }
        }
        else if (target == GL_READ_FRAMEBUFFER)
        {
            GLuint idReadFBO = CR_SERVER_FBO_FOR_IDX(mural, mural->iCurReadBuffer);
            cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_READ_FRAMEBUFFER, idReadFBO);
        }
        else if (target == GL_DRAW_FRAMEBUFFER)
        {
            GLuint idDrawFBO = CR_SERVER_FBO_FOR_IDX(mural, mural->iCurDrawBuffer);
            cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, idDrawFBO);
        }
        else
        {
            crWarning("unknown target %d", target);
        }
#ifdef DEBUG_misha
        cr_server.head_spu->dispatch_table.GetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &rfb);
        cr_server.head_spu->dispatch_table.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &dfb);
        if (GL_FRAMEBUFFER_EXT == target)
        {
            Assert(rfb == CR_SERVER_FBO_FOR_IDX(mural, mural->iCurReadBuffer));
            Assert(dfb == CR_SERVER_FBO_FOR_IDX(mural, mural->iCurDrawBuffer));
        }
        else if (GL_READ_FRAMEBUFFER_EXT == target)
        {
            Assert(rfb == CR_SERVER_FBO_FOR_IDX(mural, mural->iCurReadBuffer));
        }
        else if (GL_DRAW_FRAMEBUFFER_EXT == target)
        {
            Assert(dfb == CR_SERVER_FBO_FOR_IDX(mural, mural->iCurDrawBuffer));
        }
        else
        {
            Assert(0);
        }
#endif
    }
    else
    {
        cr_server.head_spu->dispatch_table.BindFramebufferEXT(target, crStateGetFramebufferHWID(&cr_server.StateTracker, framebuffer));
#ifdef DEBUG_misha
        cr_server.head_spu->dispatch_table.GetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &rfb);
        cr_server.head_spu->dispatch_table.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &dfb);
        if (GL_FRAMEBUFFER_EXT == target)
        {
            Assert(rfb == crStateGetFramebufferHWID(&cr_server.StateTracker, framebuffer));
            Assert(dfb == crStateGetFramebufferHWID(&cr_server.StateTracker, framebuffer));
        }
        else if (GL_READ_FRAMEBUFFER_EXT == target)
        {
            Assert(rfb == crStateGetFramebufferHWID(&cr_server.StateTracker, framebuffer));
        }
        else if (GL_DRAW_FRAMEBUFFER_EXT == target)
        {
            Assert(dfb == crStateGetFramebufferHWID(&cr_server.StateTracker, framebuffer));
        }
        else
        {
            Assert(0);
        }
#endif
    }
}

void SERVER_DISPATCH_APIENTRY crServerDispatchBindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
        crStateBindRenderbufferEXT(&cr_server.StateTracker, target, renderbuffer);
        cr_server.head_spu->dispatch_table.BindRenderbufferEXT(target, crStateGetRenderbufferHWID(&cr_server.StateTracker, renderbuffer));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteFramebuffersEXT(GLsizei n, const GLuint * framebuffers)
{
    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint))
    {
        crError("crStateDeleteFramebuffersEXT: parameter 'n' is out of range");
        return;
    }

    crStateDeleteFramebuffersEXT(&cr_server.StateTracker, n, framebuffers);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteRenderbuffersEXT(GLsizei n, const GLuint * renderbuffers)
{
    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint))
    {
        crError("glDeleteRenderbuffersEXT: parameter 'n' is out of range");
        return;
    }

    crStateDeleteRenderbuffersEXT(&cr_server.StateTracker, n, renderbuffers);
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
        crStateFramebufferRenderbufferEXT(&cr_server.StateTracker, target, attachment, renderbuffertarget, renderbuffer);
        cr_server.head_spu->dispatch_table.FramebufferRenderbufferEXT(target, attachment, renderbuffertarget, crStateGetRenderbufferHWID(&cr_server.StateTracker, renderbuffer));
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint * params)
{
        GLint local_params[1];
        (void) params;
        crStateGetFramebufferAttachmentParameterivEXT(&cr_server.StateTracker, target, attachment, pname, local_params);

        crServerReturnValue(&(local_params[0]), 1*sizeof(GLint));
}

GLboolean SERVER_DISPATCH_APIENTRY crServerDispatchIsFramebufferEXT( GLuint framebuffer )
{
    /* since GenFramebuffers/Renderbuffers issued to host ogl only on bind + some other ops, the host drivers may not know about them
     * so use state data*/
    GLboolean retval = crStateIsFramebufferEXT(&cr_server.StateTracker, framebuffer);
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}

GLboolean SERVER_DISPATCH_APIENTRY crServerDispatchIsRenderbufferEXT( GLuint renderbuffer )
{
    /* since GenFramebuffers/Renderbuffers issued to host ogl only on bind + some other ops, the host drivers may not know about them
     * so use state data*/
    GLboolean retval = crStateIsRenderbufferEXT(&cr_server.StateTracker, renderbuffer);
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}
