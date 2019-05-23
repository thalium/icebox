/* $Id: server_getshaders.c $ */
/** @file
 * VBox OpenGL GLSL related get functions
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
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"

#include <iprt/assert.h>

#ifdef CR_OPENGL_VERSION_2_0

typedef struct _crGetActive_t
{
    GLsizei length;
    GLint   size;
    GLenum  type;
} crGetActive_t;

void SERVER_DISPATCH_APIENTRY crServerDispatchGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, char *name)
{
    crGetActive_t *pLocal;

    pLocal = (crGetActive_t*) crAlloc(bufSize+sizeof(crGetActive_t));
    if (!pLocal)
    {
        crGetActive_t zero;
        zero.length = 0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }
    /* zero out just the header to ensure it initially contains zero size values */
    memset(pLocal, 0, sizeof (*pLocal));
    cr_server.head_spu->dispatch_table.GetActiveAttrib(crStateGetProgramHWID(program), index, bufSize, &pLocal->length, &pLocal->size, &pLocal->type, (char*)&pLocal[1]);
    crServerReturnValue(pLocal, pLocal->length+1+sizeof(crGetActive_t));
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, char *name)
{
    crGetActive_t *pLocal;

    pLocal = (crGetActive_t*) crAlloc(bufSize+sizeof(crGetActive_t));
    if (!pLocal)
    {
        crGetActive_t zero;
        zero.length = 0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }
    /* zero out just the header to ensure it initially contains zero size values */
    memset(pLocal, 0, sizeof (*pLocal));
    cr_server.head_spu->dispatch_table.GetActiveUniform(crStateGetProgramHWID(program), index, bufSize, &pLocal->length, &pLocal->size, &pLocal->type, (char*)&pLocal[1]);
    crServerReturnValue(pLocal, pLocal->length+1+sizeof(crGetActive_t));
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders)
{
    GLsizei *pLocal;

    pLocal = (GLsizei*) crAlloc(maxCount*sizeof(GLuint)+sizeof(GLsizei));
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }
    /* initial (fallback )value */
    *pLocal = 0;
    cr_server.head_spu->dispatch_table.GetAttachedShaders(crStateGetProgramHWID(program), maxCount, pLocal, (GLuint*)&pLocal[1]);

    {
        GLsizei i;
        GLuint *ids=(GLuint*)&pLocal[1];

        for (i=0; i<*pLocal; ++i)
          ids[i] = crStateGLSLShaderHWIDtoID(ids[i]);
    }

    crServerReturnValue(pLocal, (*pLocal)*sizeof(GLuint)+sizeof(GLsizei));
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetAttachedObjectsARB(VBoxGLhandleARB containerObj, GLsizei maxCount, GLsizei * count, VBoxGLhandleARB * obj)
{
    GLsizei *pLocal;

    pLocal = (GLsizei*) crAlloc(maxCount*sizeof(VBoxGLhandleARB)+sizeof(GLsizei));
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }
    /* initial (fallback )value */
    *pLocal = 0;
    cr_server.head_spu->dispatch_table.GetAttachedObjectsARB(crStateGetProgramHWID(containerObj), maxCount, pLocal, (VBoxGLhandleARB*)&pLocal[1]);

    {
        GLsizei i;
        GLuint *ids=(GLuint*)&pLocal[1];

        for (i=0; i<*pLocal; ++i)
          ids[i] = crStateGLSLShaderHWIDtoID(ids[i]);
    }

    crServerReturnValue(pLocal, (*pLocal)*sizeof(VBoxGLhandleARB)+sizeof(GLsizei));
    crFree(pLocal);
}

AssertCompile(sizeof(GLsizei) == 4);

void SERVER_DISPATCH_APIENTRY crServerDispatchGetInfoLogARB(VBoxGLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * infoLog)
{
    GLsizei *pLocal;
    GLuint hwid;

    pLocal = (GLsizei*) crAlloc(maxLength+sizeof(GLsizei));
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }
    /* initial (fallback )value */
    *pLocal = 0;
    /** @todo recheck*/
    hwid = crStateGetProgramHWID(obj);
    if (!hwid) hwid = crStateGetShaderHWID(obj);
    cr_server.head_spu->dispatch_table.GetInfoLogARB(hwid, maxLength, pLocal, (char*)&pLocal[1]);
    CRASSERT((*pLocal) <= maxLength);
    crServerReturnValue(pLocal, (*pLocal)+sizeof(GLsizei));
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, char *infoLog)
{
    GLsizei *pLocal;

    pLocal = (GLsizei*) crAlloc(bufSize+sizeof(GLsizei));
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }
    /* initial (fallback )value */
    *pLocal = 0;
    cr_server.head_spu->dispatch_table.GetShaderInfoLog(crStateGetShaderHWID(shader), bufSize, pLocal, (char*)&pLocal[1]);
    crServerReturnValue(pLocal, pLocal[0]+sizeof(GLsizei));
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, char *infoLog)
{
    GLsizei *pLocal;

    pLocal = (GLsizei*) crAlloc(bufSize+sizeof(GLsizei));
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }
    /* initial (fallback )value */
    *pLocal = 0;
    cr_server.head_spu->dispatch_table.GetProgramInfoLog(crStateGetProgramHWID(program), bufSize, pLocal, (char*)&pLocal[1]);
    CRASSERT(pLocal[0] <= bufSize);
    crServerReturnValue(pLocal, pLocal[0]+sizeof(GLsizei));
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, char *source)
{
    GLsizei *pLocal;

    pLocal = (GLsizei*) crAlloc(bufSize+sizeof(GLsizei));
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }
    /* initial (fallback )value */
    *pLocal = 0;
    cr_server.head_spu->dispatch_table.GetShaderSource(crStateGetShaderHWID(shader), bufSize, pLocal, (char*)&pLocal[1]);
    CRASSERT(pLocal[0] <= bufSize);
    crServerReturnValue(pLocal, pLocal[0]+sizeof(GLsizei));
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchGetUniformsLocations(GLuint program, GLsizei maxcbData, GLsizei * cbData, GLvoid * pData)
{
    GLsizei *pLocal;

    (void) cbData;
    (void) pData;

    pLocal = (GLsizei*) crAlloc(maxcbData+sizeof(GLsizei));
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }

    /* initial (fallback )value */
    *pLocal = 0;
    crStateGLSLProgramCacheUniforms(program, maxcbData, pLocal, (char*)&pLocal[1]);

    crServerReturnValue(pLocal, (*pLocal)+sizeof(GLsizei));
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchGetAttribsLocations(GLuint program, GLsizei maxcbData, GLsizei * cbData, GLvoid * pData)
{
    GLsizei *pLocal;

    (void) cbData;
    (void) pData;

    pLocal = (GLsizei*) crAlloc(maxcbData+sizeof(GLsizei));
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }

    /* initial (fallback )value */
    *pLocal = 0;
    crStateGLSLProgramCacheAttribs(program, maxcbData, pLocal, (char*)&pLocal[1]);

    crServerReturnValue(pLocal, (*pLocal)+sizeof(GLsizei));
    crFree(pLocal);
}

static GLint __GetUniformSize(GLuint program, GLint location)
{
    GLint  size = 0;
    GLenum type = 0;

    /** @todo check if index and location is the same*/
    cr_server.head_spu->dispatch_table.GetActiveUniform(crStateGetProgramHWID(program), location, 0, NULL, &size, &type, NULL);

    return crStateGetUniformSize(type);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetUniformfv(GLuint program, GLint location, GLfloat *params)
{
    int size = __GetUniformSize(program, location) * sizeof(GLfloat);
    GLfloat *pLocal;

    pLocal = (GLfloat*) crAlloc(size);
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }

    cr_server.head_spu->dispatch_table.GetUniformfv(crStateGetProgramHWID(program), location, pLocal);

    crServerReturnValue(pLocal, size);
    crFree(pLocal);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetUniformiv(GLuint program, GLint location, GLint *params)
{
    int size = __GetUniformSize(program, location) * sizeof(GLint);
    GLint *pLocal;

    pLocal = (GLint*) crAlloc(size);
    if (!pLocal)
    {
        GLsizei zero=0;
        crServerReturnValue(&zero, sizeof(zero));
        return;
    }

    cr_server.head_spu->dispatch_table.GetUniformiv(crStateGetProgramHWID(program), location, pLocal);

    crServerReturnValue(pLocal, size);
    crFree(pLocal);
}

GLuint SERVER_DISPATCH_APIENTRY crServerDispatchCreateShader(GLenum type)
{
    GLuint retval, hwVal;
    hwVal = cr_server.head_spu->dispatch_table.CreateShader(type);
    retval = crStateCreateShader(hwVal, type);
    crServerReturnValue(&retval, sizeof(retval));
    return retval; /* ignored */
}

GLuint SERVER_DISPATCH_APIENTRY crServerDispatchCreateProgram(void)
{
    GLuint retval, hwVal;
    hwVal = cr_server.head_spu->dispatch_table.CreateProgram();
    retval = crStateCreateProgram(hwVal);
    crServerReturnValue(&retval, sizeof(retval));
    return retval; /* ignored */
}

GLboolean SERVER_DISPATCH_APIENTRY crServerDispatchIsShader(GLuint shader)
{
    GLboolean retval;
    retval = cr_server.head_spu->dispatch_table.IsShader(crStateGetShaderHWID(shader));
    crServerReturnValue(&retval, sizeof(retval));
    return retval; /* ignored */
}

GLboolean SERVER_DISPATCH_APIENTRY crServerDispatchIsProgram(GLuint program)
{
    GLboolean retval;
    retval = cr_server.head_spu->dispatch_table.IsProgram(crStateGetProgramHWID(program));
    crServerReturnValue(&retval, sizeof(retval));
    return retval; /* ignored */
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetObjectParameterfvARB( VBoxGLhandleARB obj, GLenum pname, GLfloat * params )
{
    GLfloat local_params[1];
    GLuint hwid = crStateGetProgramHWID(obj);
    (void) params;

    if (!hwid)
    {
        hwid = crStateGetShaderHWID(obj);
        if (!hwid)
        {
            crWarning("Unknown object %i, in crServerDispatchGetObjectParameterfvARB", obj);
        }
    }

    cr_server.head_spu->dispatch_table.GetObjectParameterfvARB( hwid, pname, local_params );
    crServerReturnValue( &(local_params[0]), 1*sizeof(GLfloat) );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetObjectParameterivARB( VBoxGLhandleARB obj, GLenum pname, GLint * params )
{
    GLint local_params[1];
    GLuint hwid = crStateGetProgramHWID(obj);
    if (!hwid)
    {
        hwid = crStateGetShaderHWID(obj);
        if (!hwid)
        {
            crWarning("Unknown object %i, in crServerDispatchGetObjectParameterivARB", obj);
        }
    }

    (void) params;
    cr_server.head_spu->dispatch_table.GetObjectParameterivARB( hwid, pname, local_params );
    crServerReturnValue( &(local_params[0]), 1*sizeof(GLint) );
}
#endif /* #ifdef CR_OPENGL_VERSION_2_0 */
