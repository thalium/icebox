/* $Id: cr_glsl.h $ */

/** @file
 * VBox crOpenGL: GLSL related state info
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

#ifndef CR_STATE_GLSL_H
#define CR_STATE_GLSL_H

#include "cr_hash.h"
#include "state/cr_statetypes.h"
#include "state/cr_statefuncs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* We can't go the "easy" way of just extracting all the required data when taking snapshots.
   Shader objects might be modified *after* program linkage and wouldn't affect program until it's relinked.
   So we have to keep track of shaders statuses right before each program was linked as well as their "current" status.
*/

/*@todo: check rare case when successfully linked and active program is relinked with failure*/

typedef struct {
    GLuint      id, hwid;
    GLenum      type;               /*GL_VERTEX_SHADER or GL_FRAGMENT_SHADER*/
    GLchar*     source;             /*NULL after context loading unless in program's "active" hash*/
    GLboolean   compiled, deleted;
    GLuint      refCount;           /*valid only for shaders in CRGLSLState's hash*/
} CRGLSLShader;

typedef struct {
    GLchar* name;
    GLuint index;
} CRGLSLAttrib;

/*Note: active state will hold copies of shaders while current state references shaders in the CRGLSLState hashtable*/
/*@todo: probably don't need a hashtable here*/
typedef struct {
    CRHashTable  *attachedShaders;
    CRGLSLAttrib *pAttribs; /*several names could be bound to the same index*/
    GLuint        cAttribs;
} CRGLSLProgramState;

typedef struct{
    GLchar *name;
    GLenum  type;
    GLvoid *data;
#ifdef IN_GUEST
    GLint  location;
#endif
} CRGLSLUniform;

typedef struct {
    GLuint              id, hwid;
    GLboolean           validated, linked, deleted;
    CRGLSLProgramState  activeState, currentState;
    CRGLSLUniform      *pUniforms;
    GLuint              cUniforms;
#ifdef IN_GUEST
    CRGLSLAttrib        *pAttribs;
    GLuint              cAttribs;
    GLboolean           bUniformsSynced; /*uniforms info is updated since last link program call.*/
    GLboolean           bAttribsSynced; /*attribs info is updated since last link program call.*/
#endif
} CRGLSLProgram;

typedef struct {
    CRHashTable *shaders;
    CRHashTable *programs;

    CRGLSLProgram *activeProgram;

    /* Indicates that we have to resend GLSL data to GPU on first glMakeCurrent call with owning context */
    GLboolean   bResyncNeeded;
} CRGLSLState;

DECLEXPORT(void) STATE_APIENTRY crStateGLSLInit(CRContext *ctx);
DECLEXPORT(void) STATE_APIENTRY crStateGLSLDestroy(CRContext *ctx);
DECLEXPORT(void) STATE_APIENTRY crStateGLSLSwitch(CRContext *from, CRContext *to);

DECLEXPORT(GLuint) STATE_APIENTRY crStateGetShaderHWID(GLuint id);
DECLEXPORT(GLuint) STATE_APIENTRY crStateGetProgramHWID(GLuint id);
DECLEXPORT(GLuint) STATE_APIENTRY crStateGLSLProgramHWIDtoID(GLuint hwid);
DECLEXPORT(GLuint) STATE_APIENTRY crStateGLSLShaderHWIDtoID(GLuint hwid);

DECLEXPORT(GLint) STATE_APIENTRY crStateGetUniformSize(GLenum type);
DECLEXPORT(GLboolean) STATE_APIENTRY crStateIsIntUniform(GLenum type);

DECLEXPORT(GLuint) STATE_APIENTRY crStateCreateShader(GLuint id, GLenum type);
DECLEXPORT(GLuint) STATE_APIENTRY crStateCreateProgram(GLuint id);
DECLEXPORT(GLuint) STATE_APIENTRY crStateDeleteObjectARB( VBoxGLhandleARB obj );

DECLEXPORT(GLboolean) STATE_APIENTRY crStateIsProgramUniformsCached(GLuint program);
DECLEXPORT(GLboolean) STATE_APIENTRY crStateIsProgramAttribsCached(GLuint program);

#ifdef IN_GUEST
DECLEXPORT(void) STATE_APIENTRY crStateGLSLProgramCacheUniforms(GLuint program, GLsizei cbData, GLvoid *pData);
DECLEXPORT(void) STATE_APIENTRY crStateGLSLProgramCacheAttribs(GLuint program, GLsizei cbData, GLvoid *pData);
#else
DECLEXPORT(void) STATE_APIENTRY crStateGLSLProgramCacheUniforms(GLuint program, GLsizei maxcbData, GLsizei *cbData, GLvoid *pData);
DECLEXPORT(void) STATE_APIENTRY crStateGLSLProgramCacheAttribs(GLuint program, GLsizei maxcbData, GLsizei *cbData, GLvoid *pData);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_GLSL_H */
