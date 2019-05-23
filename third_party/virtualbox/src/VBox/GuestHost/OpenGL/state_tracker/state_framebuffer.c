/* $Id: state_framebuffer.c $ */

/** @file
 * VBox OpenGL: EXT_framebuffer_object state tracking
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

#include "state.h"
#include "state/cr_statetypes.h"
#include "state/cr_statefuncs.h"
#include "state_internals.h"
#include "cr_mem.h"

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferObjectInit(CRContext *ctx)
{
    CRFramebufferObjectState *fbo = &ctx->framebufferobject;

    fbo->readFB = NULL;
    fbo->drawFB = NULL;
    fbo->renderbuffer = NULL;
    ctx->shared->bFBOResyncNeeded = GL_FALSE;
}

void STATE_APIENTRY crStateGenFramebuffersEXT(GLsizei n, GLuint *buffers)
{
    CRContext *g = GetCurrentContext();
    crStateGenNames(g, g->shared->fbTable, n, buffers);
}

void STATE_APIENTRY crStateGenRenderbuffersEXT(GLsizei n, GLuint *buffers)
{
    CRContext *g = GetCurrentContext();
    crStateGenNames(g, g->shared->rbTable, n, buffers);
}

void crStateRegFramebuffers(GLsizei n, GLuint *buffers)
{
    CRContext *g = GetCurrentContext();
    crStateRegNames(g, g->shared->fbTable, n, buffers);
}

void crStateRegRenderbuffers(GLsizei n, GLuint *buffers)
{
    CRContext *g = GetCurrentContext();
    crStateRegNames(g, g->shared->rbTable, n, buffers);
}

static void crStateInitFrameBuffer(CRFramebufferObject *fbo);

static CRFramebufferObject *
crStateFramebufferAllocate(CRContext *ctx, GLuint name)
{
    CRFramebufferObject *buffer = (CRFramebufferObject*) crCalloc(sizeof(CRFramebufferObject));
    CRSTATE_CHECKERR_RET(!buffer, GL_OUT_OF_MEMORY, "crStateFramebufferAllocate", NULL);
    buffer->id = name;
#ifndef IN_GUEST
    diff_api.GenFramebuffersEXT(1, &buffer->hwid);
    if (!buffer->hwid)
    {
        crWarning("GenFramebuffersEXT failed!");
        crFree(buffer);
        return NULL;
    }
#else
    buffer->hwid = name;
#endif

    crStateInitFrameBuffer(buffer);
    crHashtableAdd(ctx->shared->fbTable, name, buffer);
    CR_STATE_SHAREDOBJ_USAGE_INIT(buffer);

    return buffer;
}

static CRRenderbufferObject *
crStateRenderbufferAllocate(CRContext *ctx, GLuint name)
{
    CRRenderbufferObject *buffer = (CRRenderbufferObject*) crCalloc(sizeof(CRRenderbufferObject));
    CRSTATE_CHECKERR_RET(!buffer, GL_OUT_OF_MEMORY, "crStateRenderbufferAllocate", NULL);
    buffer->id = name;
#ifndef IN_GUEST
    diff_api.GenRenderbuffersEXT(1, &buffer->hwid);
    if (!buffer->hwid)
    {
        crWarning("GenRenderbuffersEXT failed!");
        crFree(buffer);
        return NULL;
    }
#else
    buffer->hwid = name;
#endif

    buffer->internalformat = GL_RGBA;
    crHashtableAdd(ctx->shared->rbTable, name, buffer);
    CR_STATE_SHAREDOBJ_USAGE_INIT(buffer);

    return buffer;
}

void crStateFreeFBO(void *data)
{
    CRFramebufferObject *pObj = (CRFramebufferObject *)data;

#ifndef IN_GUEST
    if (diff_api.DeleteFramebuffersEXT)
    {
        diff_api.DeleteFramebuffersEXT(1, &pObj->hwid);
    }
#endif

    crFree(pObj);
}

void crStateFreeRBO(void *data)
{
    CRRenderbufferObject *pObj = (CRRenderbufferObject *)data;

#ifndef IN_GUEST
    if (diff_api.DeleteRenderbuffersEXT)
    {
        diff_api.DeleteRenderbuffersEXT(1, &pObj->hwid);
    }
#endif

    crFree(pObj);
}

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferObjectDestroy(CRContext *ctx)
{
    CRFramebufferObjectState *fbo = &ctx->framebufferobject;

    fbo->readFB = NULL;
    fbo->drawFB = NULL;
    fbo->renderbuffer = NULL;
}

DECLEXPORT(void) STATE_APIENTRY
crStateBindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;

    CRSTATE_CHECKERR(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end");
    CRSTATE_CHECKERR(target!=GL_RENDERBUFFER_EXT, GL_INVALID_ENUM, "invalid target");

    if (renderbuffer)
    {
        fbo->renderbuffer = (CRRenderbufferObject*) crHashtableSearch(g->shared->rbTable, renderbuffer);
        if (!fbo->renderbuffer)
        {
            CRSTATE_CHECKERR(!crHashtableIsKeyUsed(g->shared->rbTable, renderbuffer), GL_INVALID_OPERATION, "name is not a renderbuffer");
            fbo->renderbuffer = crStateRenderbufferAllocate(g, renderbuffer);
        }
        CR_STATE_SHAREDOBJ_USAGE_SET(fbo->renderbuffer, g);
    }
    else fbo->renderbuffer = NULL;
}

static void crStateCheckFBOAttachments(CRFramebufferObject *pFBO, GLuint rbo, GLenum target)
{
    CRFBOAttachmentPoint *ap;
    int u;

    if (!pFBO)
        return;

    for (u=0; u<CR_MAX_COLOR_ATTACHMENTS; ++u)
    {
        ap = &pFBO->color[u];
        if (ap->type==GL_RENDERBUFFER_EXT && ap->name==rbo)
        {
            crStateFramebufferRenderbufferEXT(target, u+GL_COLOR_ATTACHMENT0_EXT, 0, 0);
#ifdef IN_GUEST
            pFBO->status = GL_FRAMEBUFFER_UNDEFINED;
#endif
        }
    }

    ap = &pFBO->depth;
    if (ap->type==GL_RENDERBUFFER_EXT && ap->name==rbo)
    {
        crStateFramebufferRenderbufferEXT(target, GL_DEPTH_ATTACHMENT_EXT, 0, 0);
#ifdef IN_GUEST
        pFBO->status = GL_FRAMEBUFFER_UNDEFINED;
#endif
    }
    ap = &pFBO->stencil;
    if (ap->type==GL_RENDERBUFFER_EXT && ap->name==rbo)
    {
        crStateFramebufferRenderbufferEXT(target, GL_STENCIL_ATTACHMENT_EXT, 0, 0);
#ifdef IN_GUEST
        pFBO->status = GL_FRAMEBUFFER_UNDEFINED;
#endif
    }
}

static void ctStateRenderbufferRefsCleanup(CRContext *g, GLuint fboId, CRRenderbufferObject *rbo)
{
    CRFramebufferObjectState *fbo = &g->framebufferobject;

    if (fbo->renderbuffer==rbo)
    {
        fbo->renderbuffer = NULL;
    }

    /* check the attachments of current framebuffers */
    crStateCheckFBOAttachments(fbo->readFB, fboId, GL_READ_FRAMEBUFFER);
    crStateCheckFBOAttachments(fbo->drawFB, fboId, GL_DRAW_FRAMEBUFFER);

    CR_STATE_SHAREDOBJ_USAGE_CLEAR(rbo, g);
}

DECLEXPORT(void) STATE_APIENTRY
crStateDeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers)
{
    CRContext *g = GetCurrentContext();
    /*CRFramebufferObjectState *fbo = &g->framebufferobject; - unused */
    int i;

    CRSTATE_CHECKERR(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end");
    CRSTATE_CHECKERR(n<0, GL_INVALID_OPERATION, "n<0");

    for (i = 0; i < n; i++)
    {
        if (renderbuffers[i])
        {
            CRRenderbufferObject *rbo;
            rbo = (CRRenderbufferObject*) crHashtableSearch(g->shared->rbTable, renderbuffers[i]);
            if (rbo)
            {
                int j;

                ctStateRenderbufferRefsCleanup(g, renderbuffers[i], rbo);
                CR_STATE_SHAREDOBJ_USAGE_FOREACH_USED_IDX(rbo, j)
                {
                    /* saved state version <= SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS does not have usage bits info,
                     * so on restore, we set mark bits as used.
                     * This is why g_pAvailableContexts[j] could be NULL
                     * also g_pAvailableContexts[0] will hold default context, which we should discard */
                    CRContext *ctx = g_pAvailableContexts[j];
                    if (j && ctx)
                    {
                        CRFramebufferObjectState *ctxFbo;
                        CRASSERT(ctx);
                        ctxFbo = &ctx->framebufferobject;
                        if (ctxFbo->renderbuffer==rbo)
                            crWarning("deleting RBO being used by another context %d", ctx->id);

                        ctStateRenderbufferRefsCleanup(ctx, renderbuffers[i], rbo);
                    }
                    else
                        CR_STATE_SHAREDOBJ_USAGE_CLEAR_IDX(rbo, j);
                }
                crHashtableDelete(g->shared->rbTable, renderbuffers[i], crStateFreeRBO);
            }
        }
    }
}

DECLEXPORT(void) STATE_APIENTRY
crStateRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRRenderbufferObject *rb = fbo->renderbuffer;

    CRSTATE_CHECKERR(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end");
    CRSTATE_CHECKERR(target!=GL_RENDERBUFFER_EXT, GL_INVALID_ENUM, "invalid target");
    CRSTATE_CHECKERR(!rb, GL_INVALID_OPERATION, "no bound renderbuffer");

    rb->width = width;
    rb->height = height;
    rb->internalformat = internalformat;
}

DECLEXPORT(void) STATE_APIENTRY
crStateGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRRenderbufferObject *rb = fbo->renderbuffer;

    CRSTATE_CHECKERR(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end");
    CRSTATE_CHECKERR(target!=GL_RENDERBUFFER_EXT, GL_INVALID_ENUM, "invalid target");
    CRSTATE_CHECKERR(!rb, GL_INVALID_OPERATION, "no bound renderbuffer");

    switch (pname)
    {
        case GL_RENDERBUFFER_WIDTH_EXT:
            *params = rb->width;
            break;
        case GL_RENDERBUFFER_HEIGHT_EXT:
            *params = rb->height;
            break;
        case GL_RENDERBUFFER_INTERNAL_FORMAT_EXT:
            *params = rb->internalformat;
            break;
        case GL_RENDERBUFFER_RED_SIZE_EXT:
        case GL_RENDERBUFFER_GREEN_SIZE_EXT:
        case GL_RENDERBUFFER_BLUE_SIZE_EXT:
        case GL_RENDERBUFFER_ALPHA_SIZE_EXT:
        case GL_RENDERBUFFER_DEPTH_SIZE_EXT:
        case GL_RENDERBUFFER_STENCIL_SIZE_EXT:
            CRSTATE_CHECKERR(GL_TRUE, GL_INVALID_OPERATION, "unimplemented");
            break;
        default:
            CRSTATE_CHECKERR(GL_TRUE, GL_INVALID_ENUM, "invalid pname");
    }
}

static void crStateInitFBOAttachmentPoint(CRFBOAttachmentPoint *fboap)
{
    fboap->type = GL_NONE;
    fboap->name = 0;
    fboap->level = 0;
    fboap->face = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    fboap->zoffset = 0;
}

static void crStateInitFrameBuffer(CRFramebufferObject *fbo)
{
    int i;

    for (i=0; i<CR_MAX_COLOR_ATTACHMENTS; ++i)
        crStateInitFBOAttachmentPoint(&fbo->color[i]);

    crStateInitFBOAttachmentPoint(&fbo->depth);
    crStateInitFBOAttachmentPoint(&fbo->stencil);

    fbo->readbuffer = GL_COLOR_ATTACHMENT0_EXT;
    fbo->drawbuffer[0] = GL_COLOR_ATTACHMENT0_EXT;

#ifdef IN_GUEST
    fbo->status = GL_FRAMEBUFFER_UNDEFINED;
#endif
}

static GLboolean crStateGetFBOAttachmentPoint(CRFramebufferObject *fb, GLenum attachment, CRFBOAttachmentPoint **ap)
{
    switch (attachment)
    {
        case GL_DEPTH_ATTACHMENT_EXT:
            *ap = &fb->depth;
            break;
        case GL_STENCIL_ATTACHMENT_EXT:
            *ap = &fb->stencil;
            break;
        default:
            if (attachment>=GL_COLOR_ATTACHMENT0_EXT && attachment<=GL_COLOR_ATTACHMENT15_EXT)
            {
                *ap = &fb->color[attachment-GL_COLOR_ATTACHMENT0_EXT];
            }
            else return GL_FALSE;
    }

    return GL_TRUE;
}

DECLEXPORT(void) STATE_APIENTRY
crStateBindFramebufferEXT(GLenum target, GLuint framebuffer)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRFramebufferObject *pFBO=NULL;

    CRSTATE_CHECKERR(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end");
    CRSTATE_CHECKERR(((target!=GL_FRAMEBUFFER_EXT) && (target!=GL_READ_FRAMEBUFFER) && (target!=GL_DRAW_FRAMEBUFFER)),
                         GL_INVALID_ENUM, "invalid target");

    if (framebuffer)
    {
        pFBO = (CRFramebufferObject*) crHashtableSearch(g->shared->fbTable, framebuffer);
        if (!pFBO)
        {
            CRSTATE_CHECKERR(!crHashtableIsKeyUsed(g->shared->fbTable, framebuffer), GL_INVALID_OPERATION, "name is not a framebuffer");
            pFBO = crStateFramebufferAllocate(g, framebuffer);
        }


        CR_STATE_SHAREDOBJ_USAGE_SET(pFBO, g);
    }

    /** @todo http://www.opengl.org/registry/specs/ARB/framebuffer_object.txt
     * FBO status might change when binding a different FBO here...but I doubt it happens.
     * So no status reset here until a proper check.
     */

    switch (target)
    {
        case GL_FRAMEBUFFER_EXT:
            fbo->readFB = pFBO;
            fbo->drawFB = pFBO;
            break;
        case GL_READ_FRAMEBUFFER:
            fbo->readFB = pFBO;
            break;
        case GL_DRAW_FRAMEBUFFER:
            fbo->drawFB = pFBO;
            break;
    }
}

static void ctStateFramebufferRefsCleanup(CRContext *g, CRFramebufferObject *fb)
{
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    if (fbo->readFB==fb)
    {
        fbo->readFB = NULL;
    }
    if (fbo->drawFB==fb)
    {
        fbo->drawFB = NULL;
    }

    CR_STATE_SHAREDOBJ_USAGE_CLEAR(fb, g);
}

DECLEXPORT(void) STATE_APIENTRY
crStateDeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers)
{
    CRContext *g = GetCurrentContext();
    int i;

    CRSTATE_CHECKERR(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end");
    CRSTATE_CHECKERR(n<0, GL_INVALID_OPERATION, "n<0");

    for (i = 0; i < n; i++)
    {
        if (framebuffers[i])
        {
            CRFramebufferObject *fb;
            fb = (CRFramebufferObject*) crHashtableSearch(g->shared->fbTable, framebuffers[i]);
            if (fb)
            {
                int j;

                ctStateFramebufferRefsCleanup(g, fb);

                CR_STATE_SHAREDOBJ_USAGE_FOREACH_USED_IDX(fb, j)
                {
                    /* saved state version <= SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS does not have usage bits info,
                     * so on restore, we set mark bits as used.
                     * This is why g_pAvailableContexts[j] could be NULL
                     * also g_pAvailableContexts[0] will hold default context, which we should discard */
                    CRContext *ctx = g_pAvailableContexts[j];
                    if (j && ctx)
                    {
                        CRFramebufferObjectState *ctxFbo;
                        CRASSERT(ctx);
                        ctxFbo = &ctx->framebufferobject;
                        if (ctxFbo->readFB==fb)
                            crWarning("deleting FBO being used as read buffer by another context %d", ctx->id);

                        if (ctxFbo->drawFB==fb)
                            crWarning("deleting FBO being used as draw buffer by another context %d", ctx->id);

                        ctStateFramebufferRefsCleanup(ctx, fb);
                    }
                    else
                        CR_STATE_SHAREDOBJ_USAGE_CLEAR_IDX(fb, j);
                }
                crHashtableDelete(g->shared->fbTable, framebuffers[i], crStateFreeFBO);
            }
        }
    }
}

/** @todo move this function somewhere else*/
/*return floor of base 2 log of x. log(0)==0*/
static unsigned int crLog2Floor(unsigned int x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x -= ((x >> 1) & 0x55555555);
    x  = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x  = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);
    return (x & 0x0000003f) - 1;
}

static GLuint crStateFramebufferGet(CRFramebufferObjectState *fbo, GLenum target, CRFramebufferObject **apFBOs)
{
    /** @todo Since this function returns not more than one FBO, callers can be cleaned up. */
    GLuint cPBOs = 0;
    switch (target)
    {
        case GL_READ_FRAMEBUFFER:
            cPBOs = 1;
            apFBOs[0] = fbo->readFB;
            break;
        /* OpenGL glFramebufferTexture, glFramebufferRenderbuffer, glFramebufferRenderbuffer specs:
         * "GL_FRAMEBUFFER is equivalent to GL_DRAW_FRAMEBUFFER."
         */
        case GL_FRAMEBUFFER:
        case GL_DRAW_FRAMEBUFFER:
            cPBOs = 1;
            apFBOs[0] = fbo->drawFB;
            break;
        default:
            crWarning("unexpected target value: 0x%x", target);
            cPBOs = 0;
            break;
    }

    return cPBOs;
}

static GLuint crStateFramebufferTextureCheck(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level,
                    CRFBOAttachmentPoint **aap, CRTextureObj **tobj)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRFramebufferObject *apFBOs[2];
    GLuint cFBOs = 0, i;
    GLuint maxtexsizelog2;

    CRSTATE_CHECKERR_RET(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end", 0);
    CRSTATE_CHECKERR_RET(((target!=GL_FRAMEBUFFER_EXT) && (target!=GL_READ_FRAMEBUFFER) && (target!=GL_DRAW_FRAMEBUFFER)),
                         GL_INVALID_ENUM, "invalid target", 0);

    cFBOs = crStateFramebufferGet(fbo, target, apFBOs);
    CRSTATE_CHECKERR_RET(!cFBOs, GL_INVALID_ENUM, "unexpected target", 0);
    for (i = 0; i < cFBOs; ++i)
    {
        CRSTATE_CHECKERR_RET(!apFBOs[i], GL_INVALID_OPERATION, "zero fbo bound", 0);
    }

    Assert(cFBOs);
    Assert(cFBOs <= 2);

    for (i = 0; i < cFBOs; ++i)
    {
        CRSTATE_CHECKERR_RET(!crStateGetFBOAttachmentPoint(apFBOs[i], attachment, &aap[i]), GL_INVALID_ENUM, "invalid attachment", 0);
    }

    if (!texture)
    {
        return cFBOs;
    }

    switch (textarget)
    {
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            maxtexsizelog2 = crLog2Floor(g->limits.maxCubeMapTextureSize);
            *tobj = crStateTextureGet(GL_TEXTURE_CUBE_MAP_ARB, texture);
            break;
        case GL_TEXTURE_RECTANGLE_ARB:
            maxtexsizelog2 = 0;
            *tobj = crStateTextureGet(textarget, texture);
            break;
        case GL_TEXTURE_3D:
            maxtexsizelog2 = crLog2Floor(g->limits.max3DTextureSize);
            *tobj = crStateTextureGet(textarget, texture);
            break;
        case GL_TEXTURE_2D:
        case GL_TEXTURE_1D:
            maxtexsizelog2 = crLog2Floor(g->limits.maxTextureSize);
            *tobj = crStateTextureGet(textarget, texture);
            break;
        default:
            CRSTATE_CHECKERR_RET(GL_TRUE, GL_INVALID_OPERATION, "invalid textarget", 0);
    }

    CRSTATE_CHECKERR_RET(!*tobj, GL_INVALID_OPERATION, "invalid textarget/texture combo", 0);

    if (GL_TEXTURE_RECTANGLE_ARB==textarget)
    {
        CRSTATE_CHECKERR_RET(level!=0, GL_INVALID_VALUE, "non zero mipmap level", 0);
    }

    CRSTATE_CHECKERR_RET(level<0, GL_INVALID_VALUE, "level<0", 0);
    CRSTATE_CHECKERR_RET((GLuint)level>maxtexsizelog2, GL_INVALID_VALUE, "level too big", 0);

#ifdef IN_GUEST
    for (i = 0; i < cFBOs; ++i)
    {
        if ((aap[i])->type!=GL_TEXTURE || (aap[i])->name!=texture || (aap[i])->level!=level)
        {
            apFBOs[i]->status = GL_FRAMEBUFFER_UNDEFINED;
        }
    }
#endif

    Assert(cFBOs);
    Assert(cFBOs <= 2);

    return cFBOs;
}

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    CRContext *g = GetCurrentContext();
    /*CRFramebufferObjectState *fbo = &g->framebufferobject; - unused */
    CRFBOAttachmentPoint *aap[2];
    GLuint cap, i;
    CRTextureObj *tobj;

    cap = crStateFramebufferTextureCheck(target, attachment, textarget, texture, level, aap, &tobj);
    if (!cap) return;

    if (!texture)
    {
        for (i = 0; i < cap; ++i)
        {
            crStateInitFBOAttachmentPoint(aap[i]);
        }
        return;
    }

    CRSTATE_CHECKERR(textarget!=GL_TEXTURE_1D, GL_INVALID_OPERATION, "textarget");

    CR_STATE_SHAREDOBJ_USAGE_SET(tobj, g);

    for (i = 0; i < cap; ++i)
    {
        crStateInitFBOAttachmentPoint(aap[i]);
        aap[i]->type = GL_TEXTURE;
        aap[i]->name = texture;
        aap[i]->level = level;
    }
}

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    CRContext *g = GetCurrentContext();
    /* CRFramebufferObjectState *fbo = &g->framebufferobject; - unused */
    CRFBOAttachmentPoint *aap[2];
    GLuint cap, i;
    CRTextureObj *tobj;

    cap = crStateFramebufferTextureCheck(target, attachment, textarget, texture, level, aap, &tobj);
    if (!cap) return;

    if (!texture)
    {
        for (i = 0; i < cap; ++i)
        {
            crStateInitFBOAttachmentPoint(aap[i]);
        }
        return;
    }

    CRSTATE_CHECKERR(GL_TEXTURE_1D==textarget || GL_TEXTURE_3D==textarget, GL_INVALID_OPERATION, "textarget");

    CR_STATE_SHAREDOBJ_USAGE_SET(tobj, g);

    for (i = 0; i < cap; ++i)
    {
        crStateInitFBOAttachmentPoint(aap[i]);
        aap[i]->type = GL_TEXTURE;
        aap[i]->name = texture;
        aap[i]->level = level;
        if (textarget!=GL_TEXTURE_2D && textarget!=GL_TEXTURE_RECTANGLE_ARB)
        {
            aap[i]->face = textarget;
        }
    }
}

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
    CRContext *g = GetCurrentContext();
    /* CRFramebufferObjectState *fbo = &g->framebufferobject; - unused */
    CRFBOAttachmentPoint *aap[2];
    GLuint cap, i;
    CRTextureObj *tobj;

    cap = crStateFramebufferTextureCheck(target, attachment, textarget, texture, level, aap, &tobj);
    if (!cap) return;

    if (!texture)
    {
        for (i = 0; i < cap; ++i)
        {
            crStateInitFBOAttachmentPoint(aap[i]);
        }
        return;
    }

    CRSTATE_CHECKERR(zoffset>((GLint)g->limits.max3DTextureSize-1), GL_INVALID_VALUE, "zoffset too big");
    CRSTATE_CHECKERR(textarget!=GL_TEXTURE_3D, GL_INVALID_OPERATION, "textarget");

    CR_STATE_SHAREDOBJ_USAGE_SET(tobj, g);

    for (i = 0; i < cap; ++i)
    {
        crStateInitFBOAttachmentPoint(aap[i]);
        aap[i]->type = GL_TEXTURE;
        aap[i]->name = texture;
        aap[i]->level = level;
        aap[i]->zoffset = zoffset;
    }
}

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRFramebufferObject *apFBOs[2];
    GLuint cFBOs, i;
    CRFBOAttachmentPoint *aap[2];
    CRRenderbufferObject *rb;
    (void)renderbuffertarget;

    CRSTATE_CHECKERR(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end");
    CRSTATE_CHECKERR(((target!=GL_FRAMEBUFFER_EXT) && (target!=GL_READ_FRAMEBUFFER) && (target!=GL_DRAW_FRAMEBUFFER)),
                         GL_INVALID_ENUM, "invalid target");
    cFBOs = crStateFramebufferGet(fbo, target, apFBOs);
    CRSTATE_CHECKERR(!cFBOs, GL_INVALID_OPERATION, "no fbo bound");
    for (i = 0; i < cFBOs; ++i)
    {
        CRSTATE_CHECKERR(!apFBOs[i], GL_INVALID_OPERATION, "zero fbo bound");
    }

    for (i = 0; i < cFBOs; ++i)
    {
        CRSTATE_CHECKERR(!crStateGetFBOAttachmentPoint(apFBOs[i], attachment, &aap[i]), GL_INVALID_ENUM, "invalid attachment");
    }

    if (!renderbuffer)
    {
        for (i = 0; i < cFBOs; ++i)
        {
#ifdef IN_GUEST
            if (&aap[i]->type!=GL_NONE)
            {
                apFBOs[i]->status = GL_FRAMEBUFFER_UNDEFINED;
            }
#endif
            crStateInitFBOAttachmentPoint(aap[i]);
        }
        return;
    }

    rb = (CRRenderbufferObject*) crHashtableSearch(g->shared->rbTable, renderbuffer);
    if (!rb)
    {
        CRSTATE_CHECKERR(!crHashtableIsKeyUsed(g->shared->rbTable, renderbuffer), GL_INVALID_OPERATION, "rb doesn't exist");
        rb = crStateRenderbufferAllocate(g, renderbuffer);
    }

    CR_STATE_SHAREDOBJ_USAGE_SET(rb, g);

    for (i = 0; i < cFBOs; ++i)
    {
#ifdef IN_GUEST
        if (aap[i]->type!=GL_RENDERBUFFER_EXT || aap[i]->name!=renderbuffer)
        {
            apFBOs[i]->status = GL_FRAMEBUFFER_UNDEFINED;
        }
#endif
        crStateInitFBOAttachmentPoint(aap[i]);
        aap[i]->type = GL_RENDERBUFFER_EXT;
        aap[i]->name = renderbuffer;
    }
}

DECLEXPORT(void) STATE_APIENTRY
crStateGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint *params)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRFramebufferObject *apFBOs[2];
    GLint cFBOs = 0, i;
    CRFBOAttachmentPoint *ap;

    CRSTATE_CHECKERR(g->current.inBeginEnd, GL_INVALID_OPERATION, "called in begin/end");
    CRSTATE_CHECKERR(((target!=GL_FRAMEBUFFER_EXT) && (target!=GL_READ_FRAMEBUFFER) && (target!=GL_DRAW_FRAMEBUFFER)),
                         GL_INVALID_ENUM, "invalid target");

    cFBOs = crStateFramebufferGet(fbo, target, apFBOs);

    CRSTATE_CHECKERR(!cFBOs, GL_INVALID_OPERATION, "no fbo bound");
    for (i = 0; i < cFBOs; ++i)
    {
        CRSTATE_CHECKERR(!apFBOs[i], GL_INVALID_OPERATION, "zero fbo bound");
    }

    if(cFBOs != 1)
    {
        crWarning("different FBPs attached to draw and read buffers, returning info for the read buffer");
    }

    for (i = 0; i < 1; ++i)
    {
        CRSTATE_CHECKERR(!crStateGetFBOAttachmentPoint(apFBOs[i], attachment, &ap), GL_INVALID_ENUM, "invalid attachment");

        switch (pname)
        {
            case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT:
                *params = ap->type;
                break;
            case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT:
                CRSTATE_CHECKERR(ap->type!=GL_RENDERBUFFER_EXT && ap->type!=GL_TEXTURE, GL_INVALID_ENUM, "can't query object name when it's not bound")
                *params = ap->name;
                break;
            case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT:
                CRSTATE_CHECKERR(ap->type!=GL_TEXTURE, GL_INVALID_ENUM, "not a texture");
                *params = ap->level;
                break;
            case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT:
                CRSTATE_CHECKERR(ap->type!=GL_TEXTURE, GL_INVALID_ENUM, "not a texture");
                *params = ap->face;
                break;
            case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT:
                CRSTATE_CHECKERR(ap->type!=GL_TEXTURE, GL_INVALID_ENUM, "not a texture");
                *params = ap->zoffset;
                break;
            default:
                CRSTATE_CHECKERR(GL_TRUE, GL_INVALID_ENUM, "invalid pname");
        }
    }
}

DECLEXPORT(GLboolean) STATE_APIENTRY crStateIsFramebufferEXT( GLuint framebuffer )
{
    CRContext *g = GetCurrentContext();

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glIsFramebufferEXT called in begin/end");
        return GL_FALSE;
    }

    return framebuffer ? crHashtableIsKeyUsed(g->shared->fbTable, framebuffer) : GL_FALSE;
}

DECLEXPORT(GLboolean)  STATE_APIENTRY crStateIsRenderbufferEXT( GLuint renderbuffer )
{
    CRContext *g = GetCurrentContext();


    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glIsRenderbufferEXT called in begin/end");
        return GL_FALSE;
    }

    return renderbuffer ? crHashtableIsKeyUsed(g->shared->rbTable, renderbuffer) : GL_FALSE;
}

DECLEXPORT(void) STATE_APIENTRY
crStateGenerateMipmapEXT(GLenum target)
{
    (void)target;
    /** @todo */
}

static void crStateSyncRenderbuffersCB(unsigned long key, void *data1, void *data2)
{
    CRRenderbufferObject *pRBO = (CRRenderbufferObject*) data1;
    (void)key; (void)data2;

    diff_api.GenRenderbuffersEXT(1, &pRBO->hwid);

    if (pRBO->width && pRBO->height)
    {
        diff_api.BindRenderbufferEXT(GL_RENDERBUFFER_EXT, pRBO->hwid);
        diff_api.RenderbufferStorageEXT(GL_RENDERBUFFER_EXT, pRBO->internalformat, pRBO->width, pRBO->height);
    }
}

static void crStateSyncAP(CRFBOAttachmentPoint *pAP, GLenum ap, CRContext *ctx)
{
    CRRenderbufferObject *pRBO;
    CRTextureObj *tobj;

    switch (pAP->type)
    {
        case GL_TEXTURE:
            CRASSERT(pAP->name!=0);

            tobj = (CRTextureObj *) crHashtableSearch(ctx->shared->textureTable, pAP->name);
            if (tobj)
            {
                CRASSERT(!tobj->id || tobj->hwid);

                switch (tobj->target)
                {
                    case GL_TEXTURE_1D:
                        diff_api.FramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT, ap, tobj->target, crStateGetTextureObjHWID(tobj), pAP->level);
                        break;
                    case GL_TEXTURE_2D:
                    case GL_TEXTURE_RECTANGLE_ARB:
                        diff_api.FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, ap, tobj->target, crStateGetTextureObjHWID(tobj), pAP->level);
                        break;
                    case GL_TEXTURE_CUBE_MAP_ARB:
                        diff_api.FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, ap, pAP->face, crStateGetTextureObjHWID(tobj), pAP->level);
                        break;
                    case GL_TEXTURE_3D:
                        diff_api.FramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT, ap, tobj->target, crStateGetTextureObjHWID(tobj), pAP->level, pAP->zoffset);
                        break;
                    default:
                        crWarning("Unexpected textarget %d", tobj->target);
                }
            }
            else
            {
                crWarning("Unknown texture id %d", pAP->name);
            }
            break;
        case GL_RENDERBUFFER_EXT:
            pRBO = (CRRenderbufferObject*) crHashtableSearch(ctx->shared->rbTable, pAP->name);
            diff_api.FramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, ap, GL_RENDERBUFFER_EXT, pRBO->hwid);
            break;
        case GL_NONE:
            /* Intentionally left blank */
            break;
        default: crWarning("Invalid attachment point type %d (ap: %i)", pAP->type, ap);
    }
}

static void crStateSyncFramebuffersCB(unsigned long key, void *data1, void *data2)
{
    CRFramebufferObject *pFBO = (CRFramebufferObject*) data1;
    CRContext *ctx = (CRContext*) data2;
    GLint i;
    (void)key;

    diff_api.GenFramebuffersEXT(1, &pFBO->hwid);

    diff_api.BindFramebufferEXT(GL_FRAMEBUFFER_EXT, pFBO->hwid);

    for (i=0; i<CR_MAX_COLOR_ATTACHMENTS; ++i)
    {
        crStateSyncAP(&pFBO->color[i], GL_COLOR_ATTACHMENT0_EXT+i, ctx);
    }

    crStateSyncAP(&pFBO->depth, GL_DEPTH_ATTACHMENT_EXT, ctx);
    crStateSyncAP(&pFBO->stencil, GL_STENCIL_ATTACHMENT_EXT, ctx);
}

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferObjectSwitch(CRContext *from, CRContext *to)
{
    if (to->shared->bFBOResyncNeeded)
    {
        to->shared->bFBOResyncNeeded = GL_FALSE;

        crHashtableWalk(to->shared->rbTable, crStateSyncRenderbuffersCB, NULL);
        crHashtableWalk(to->shared->fbTable, crStateSyncFramebuffersCB, to);

        if (to->framebufferobject.drawFB==to->framebufferobject.readFB)
        {
            diff_api.BindFramebufferEXT(GL_FRAMEBUFFER_EXT, to->framebufferobject.drawFB?
                to->framebufferobject.drawFB->hwid:0);
        }
        else
        {
            diff_api.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, to->framebufferobject.drawFB?
                to->framebufferobject.drawFB->hwid:0);

            diff_api.BindFramebufferEXT(GL_READ_FRAMEBUFFER, to->framebufferobject.readFB?
                to->framebufferobject.readFB->hwid:0);
        }

        diff_api.BindRenderbufferEXT(GL_RENDERBUFFER_EXT, to->framebufferobject.renderbuffer?
            to->framebufferobject.renderbuffer->hwid:0);
    }
    else
    {
        if (to->framebufferobject.drawFB!=from->framebufferobject.drawFB
            || to->framebufferobject.readFB!=from->framebufferobject.readFB)
        {
            if (to->framebufferobject.drawFB==to->framebufferobject.readFB)
            {
                diff_api.BindFramebufferEXT(GL_FRAMEBUFFER_EXT, to->framebufferobject.drawFB?
                    to->framebufferobject.drawFB->hwid:0);
            }
            else
            {
                diff_api.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, to->framebufferobject.drawFB?
                    to->framebufferobject.drawFB->hwid:0);

                diff_api.BindFramebufferEXT(GL_READ_FRAMEBUFFER, to->framebufferobject.readFB?
                    to->framebufferobject.readFB->hwid:0);
            }

            diff_api.DrawBuffer(to->framebufferobject.drawFB?to->framebufferobject.drawFB->drawbuffer[0]:to->buffer.drawBuffer);
            diff_api.ReadBuffer(to->framebufferobject.readFB?to->framebufferobject.readFB->readbuffer:to->buffer.readBuffer);
        }

        if (to->framebufferobject.renderbuffer!=from->framebufferobject.renderbuffer)
        {
            diff_api.BindRenderbufferEXT(GL_RENDERBUFFER_EXT, to->framebufferobject.renderbuffer?
                to->framebufferobject.renderbuffer->hwid:0);
        }
    }
}

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferObjectDisableHW(CRContext *ctx, GLuint idDrawFBO, GLuint idReadFBO)
{
    GLenum idDrawBuffer = 0, idReadBuffer = 0;

    if (ctx->framebufferobject.drawFB || idDrawFBO)
    {
        diff_api.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
        idDrawBuffer = ctx->buffer.drawBuffer;
    }

    if (ctx->framebufferobject.readFB || idReadFBO)
    {
        diff_api.BindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);
        idReadBuffer = ctx->buffer.readBuffer;
    }

    if (idDrawBuffer)
        diff_api.DrawBuffer(idDrawBuffer);
    if (idReadBuffer)
        diff_api.ReadBuffer(idReadBuffer);

    if (ctx->framebufferobject.renderbuffer)
        diff_api.BindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
}

DECLEXPORT(void) STATE_APIENTRY
crStateFramebufferObjectReenableHW(CRContext *fromCtx, CRContext *toCtx, GLuint idDrawFBO, GLuint idReadFBO)
{
    GLuint idReadBuffer = 0, idDrawBuffer = 0;
    if (!fromCtx)
        fromCtx = toCtx; /* <- in case fromCtx is zero, set it to toCtx to ensure framebuffer state gets re-enabled correctly */

    if ((fromCtx->framebufferobject.drawFB) /* <- the FBO state was reset in crStateFramebufferObjectDisableHW */
            && fromCtx->framebufferobject.drawFB == toCtx->framebufferobject.drawFB)  /* .. and it was NOT restored properly in crStateFramebufferObjectSwitch */
    {
        diff_api.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, toCtx->framebufferobject.drawFB->hwid);
        idDrawBuffer = toCtx->framebufferobject.drawFB->drawbuffer[0];
    }
    else if (idDrawFBO && !toCtx->framebufferobject.drawFB)
    {
        diff_api.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, idDrawFBO);
        idDrawBuffer = GL_COLOR_ATTACHMENT0;
    }

    if ((fromCtx->framebufferobject.readFB) /* <- the FBO state was reset in crStateFramebufferObjectDisableHW */
            && fromCtx->framebufferobject.readFB == toCtx->framebufferobject.readFB) /* .. and it was NOT restored properly in crStateFramebufferObjectSwitch */
    {
        diff_api.BindFramebufferEXT(GL_READ_FRAMEBUFFER, toCtx->framebufferobject.readFB->hwid);
        idReadBuffer = toCtx->framebufferobject.readFB->readbuffer;
    }
    else if (idReadFBO && !toCtx->framebufferobject.readFB)
    {
        diff_api.BindFramebufferEXT(GL_READ_FRAMEBUFFER, idReadFBO);
        idReadBuffer = GL_COLOR_ATTACHMENT0;
    }

    if (idDrawBuffer)
        diff_api.DrawBuffer(idDrawBuffer);
    if (idReadBuffer)
        diff_api.ReadBuffer(idReadBuffer);

    if (fromCtx->framebufferobject.renderbuffer /* <- the FBO state was reset in crStateFramebufferObjectDisableHW */
            && fromCtx->framebufferobject.renderbuffer==toCtx->framebufferobject.renderbuffer) /* .. and it was NOT restored properly in crStateFramebufferObjectSwitch */
    {
        diff_api.BindRenderbufferEXT(GL_RENDERBUFFER_EXT, toCtx->framebufferobject.renderbuffer->hwid);
    }
}


DECLEXPORT(GLuint) STATE_APIENTRY crStateGetFramebufferHWID(GLuint id)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObject *pFBO = (CRFramebufferObject*) crHashtableSearch(g->shared->fbTable, id);
#if 0 /*def DEBUG_misha*/
    crDebug("FB id(%d) hw(%d)", id, pFBO ? pFBO->hwid : 0);
#endif
    return pFBO ? pFBO->hwid : 0;
}

DECLEXPORT(GLuint) STATE_APIENTRY crStateGetRenderbufferHWID(GLuint id)
{
    CRContext *g = GetCurrentContext();
    CRRenderbufferObject *pRBO = (CRRenderbufferObject*) crHashtableSearch(g->shared->rbTable, id);

    return pRBO ? pRBO->hwid : 0;
}

static void crStateCheckFBOHWIDCB(unsigned long key, void *data1, void *data2)
{
    CRFramebufferObject *pFBO = (CRFramebufferObject *) data1;
    crCheckIDHWID_t *pParms = (crCheckIDHWID_t*) data2;
    (void) key;

    if (pFBO->hwid==pParms->hwid)
        pParms->id = pFBO->id;
}

static void crStateCheckRBOHWIDCB(unsigned long key, void *data1, void *data2)
{
    CRRenderbufferObject *pRBO = (CRRenderbufferObject *) data1;
    crCheckIDHWID_t *pParms = (crCheckIDHWID_t*) data2;
    (void) key;

    if (pRBO->hwid==pParms->hwid)
        pParms->id = pRBO->id;
}

DECLEXPORT(GLuint) STATE_APIENTRY crStateFBOHWIDtoID(GLuint hwid)
{
    CRContext *g = GetCurrentContext();
    crCheckIDHWID_t parms;

    parms.id = hwid;
    parms.hwid = hwid;

    crHashtableWalk(g->shared->fbTable, crStateCheckFBOHWIDCB, &parms);
    return parms.id;
}

DECLEXPORT(GLuint) STATE_APIENTRY crStateRBOHWIDtoID(GLuint hwid)
{
    CRContext *g = GetCurrentContext();
    crCheckIDHWID_t parms;

    parms.id = hwid;
    parms.hwid = hwid;

    crHashtableWalk(g->shared->rbTable, crStateCheckRBOHWIDCB, &parms);
    return parms.id;
}

#ifdef IN_GUEST
DECLEXPORT(GLenum) STATE_APIENTRY crStateCheckFramebufferStatusEXT(GLenum target)
{
    GLenum status = GL_FRAMEBUFFER_UNDEFINED;
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRFramebufferObject *pFBO=NULL;

    switch (target)
    {
        case GL_FRAMEBUFFER_EXT:
            pFBO = fbo->drawFB;
            break;
        case GL_READ_FRAMEBUFFER:
            pFBO = fbo->readFB;
            break;
        case GL_DRAW_FRAMEBUFFER:
            pFBO = fbo->drawFB;
            break;
    }

    if (pFBO) status = pFBO->status;

    return status;
}

DECLEXPORT(GLenum) STATE_APIENTRY crStateSetFramebufferStatus(GLenum target, GLenum status)
{
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRFramebufferObject *pFBO=NULL;

    switch (target)
    {
        case GL_FRAMEBUFFER_EXT:
            pFBO = fbo->drawFB;
            break;
        case GL_READ_FRAMEBUFFER:
            pFBO = fbo->readFB;
            break;
        case GL_DRAW_FRAMEBUFFER:
            pFBO = fbo->drawFB;
            break;
    }

    if (pFBO) pFBO->status = status;

    return status;
}
#endif
