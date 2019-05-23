/* $Id: cr_framebuffer.h $ */

/** @file
 * VBox crOpenGL: FBO related state info
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


#ifndef CR_STATE_FRAMEBUFFEROBJECT_H
#define CR_STATE_FRAMEBUFFEROBJECT_H

#include "cr_hash.h"
#include "state/cr_statetypes.h"
#include "state/cr_statefuncs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CR_MAX_COLOR_ATTACHMENTS 16

typedef struct {
    GLenum  type; /*one of GL_NONE GL_TEXTURE GL_RENDERBUFFER_EXT*/
    GLuint  name;
    GLint   level;
    GLint   face;
    GLint   zoffset;
} CRFBOAttachmentPoint;

typedef struct {
    GLuint                  id, hwid;
    CRFBOAttachmentPoint    color[CR_MAX_COLOR_ATTACHMENTS];
    CRFBOAttachmentPoint    depth;
    CRFBOAttachmentPoint    stencil;
    GLenum                  readbuffer;
    /*@todo: we don't support drawbufferS yet, so it's a stub*/
    GLenum                  drawbuffer[1];
#ifdef IN_GUEST
    GLenum                  status;
#endif
    /* bitfield representing the object usage. 1 means the object is used by the context with the given bitid */
    CRbitvalue             ctxUsage[CR_MAX_BITARRAY];
} CRFramebufferObject;

typedef struct {
    GLuint   id, hwid;
    GLsizei  width, height;
    GLenum   internalformat;
    GLuint   redBits, greenBits, blueBits, alphaBits, depthBits, stencilBits;
    /* bitfield representing the object usage. 1 means the object is used by the context with the given bitid */
    CRbitvalue             ctxUsage[CR_MAX_BITARRAY];
} CRRenderbufferObject;

typedef struct {
    CRFramebufferObject     *readFB, *drawFB;
    CRRenderbufferObject    *renderbuffer;
} CRFramebufferObjectState;

DECLEXPORT(void) STATE_APIENTRY crStateFramebufferObjectInit(CRContext *ctx);
DECLEXPORT(void) STATE_APIENTRY crStateFramebufferObjectDestroy(CRContext *ctx);
DECLEXPORT(void) STATE_APIENTRY crStateFramebufferObjectSwitch(CRContext *from, CRContext *to);

DECLEXPORT(void) STATE_APIENTRY crStateFramebufferObjectDisableHW(CRContext *ctx, GLuint idDrawFBO, GLuint idReadFBO);
DECLEXPORT(void) STATE_APIENTRY crStateFramebufferObjectReenableHW(CRContext *fromCtx, CRContext *toCtx, GLuint idDrawFBO, GLuint idReadFBO);

DECLEXPORT(GLuint) STATE_APIENTRY crStateGetFramebufferHWID(GLuint id);
DECLEXPORT(GLuint) STATE_APIENTRY crStateGetRenderbufferHWID(GLuint id);

DECLEXPORT(void) STATE_APIENTRY crStateBindRenderbufferEXT(GLenum target, GLuint renderbuffer);
DECLEXPORT(void) STATE_APIENTRY crStateDeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers);
DECLEXPORT(void) STATE_APIENTRY crStateRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
DECLEXPORT(void) STATE_APIENTRY crStateGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params);
DECLEXPORT(void) STATE_APIENTRY crStateBindFramebufferEXT(GLenum target, GLuint framebuffer);
DECLEXPORT(void) STATE_APIENTRY crStateDeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers);
DECLEXPORT(void) STATE_APIENTRY crStateFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
DECLEXPORT(void) STATE_APIENTRY crStateFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
DECLEXPORT(void) STATE_APIENTRY crStateFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
DECLEXPORT(void) STATE_APIENTRY crStateFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
DECLEXPORT(void) STATE_APIENTRY crStateGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint *params);
DECLEXPORT(void) STATE_APIENTRY crStateGenerateMipmapEXT(GLenum target);

DECLEXPORT(GLuint) STATE_APIENTRY crStateFBOHWIDtoID(GLuint hwid);
DECLEXPORT(GLuint) STATE_APIENTRY crStateRBOHWIDtoID(GLuint hwid);

DECLEXPORT(void) crStateRegFramebuffers(GLsizei n, GLuint *buffers);
DECLEXPORT(void) crStateRegRenderbuffers(GLsizei n, GLuint *buffers);

#ifdef IN_GUEST
DECLEXPORT(GLenum) STATE_APIENTRY crStateCheckFramebufferStatusEXT(GLenum target);
DECLEXPORT(GLenum) STATE_APIENTRY crStateSetFramebufferStatus(GLenum target, GLenum status);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_FRAMEBUFFEROBJECT_H */
