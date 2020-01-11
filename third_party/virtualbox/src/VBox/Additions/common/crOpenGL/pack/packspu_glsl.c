/* $Id: packspu_glsl.c $ */

/** @file
 * VBox OpenGL GLSL related functions
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
#include "cr_net.h"
#include "packspu_proto.h"
#include "cr_mem.h"


GLuint PACKSPU_APIENTRY packspu_CreateProgram(void)
{
    GET_THREAD(thread);
    int writeback = 1;
    GLuint return_val = (GLuint) 0;
    if (!CRPACKSPU_IS_WDDM_CRHGSMI() && !(pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network))
    {
        crError("packspu_CreateProgram doesn't work when there's no actual network involved!\nTry using the simplequery SPU in your chain!");
    }
    crPackCreateProgram(&return_val, &writeback);
    packspuFlush((void *) thread);
    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    crStateCreateProgram(&pack_spu.StateTracker, return_val);

    return return_val;
}

static GLint packspu_GetUniformLocationUncached(GLuint program, const char * name)
{
    GET_THREAD(thread);
    int writeback = 1;
    GLint return_val = (GLint) 0;
    if (!CRPACKSPU_IS_WDDM_CRHGSMI() && !(pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network))
    {
        crError("packspu_GetUniformLocation doesn't work when there's no actual network involved!\nTry using the simplequery SPU in your chain!");
    }

    crPackGetUniformLocation(program, name, &return_val, &writeback);
    packspuFlush((void *) thread);
    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    return return_val;
}

GLint PACKSPU_APIENTRY packspu_GetUniformLocation(GLuint program, const char * name)
{
    if (!crStateIsProgramUniformsCached(&pack_spu.StateTracker, program))
    {
        GET_THREAD(thread);
        int writeback = 1;
        GLsizei maxcbData;
        GLsizei *pData;
        GLint mu;

        packspu_GetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &mu);
        maxcbData = 16*mu*sizeof(char);

        pData = (GLsizei *) crAlloc(maxcbData+sizeof(GLsizei));
        if (!pData)
        {
            crWarning("packspu_GetUniformLocation: not enough memory, fallback to single query");
            return packspu_GetUniformLocationUncached(program, name);
        }

        crPackGetUniformsLocations(program, maxcbData, pData, NULL, &writeback);

        packspuFlush((void *) thread);
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);

        crStateGLSLProgramCacheUniforms(&pack_spu.StateTracker, program, pData[0], &pData[1]);

        CRASSERT(crStateIsProgramUniformsCached(&pack_spu.StateTracker, program));

        crFree(pData);
    }

    /*crDebug("packspu_GetUniformLocation(%d, %s)=%i", program, name, crStateGetUniformLocation(program, name));*/
    return crStateGetUniformLocation(&pack_spu.StateTracker, program, name);
}

static GLint PACKSPU_APIENTRY packspu_GetAttribLocationUnchached( GLuint program, const char * name )
{
    GET_THREAD(thread);
    int writeback = 1;
    GLint return_val = (GLint) 0;
    if (!CRPACKSPU_IS_WDDM_CRHGSMI() && !(pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network))
    {
        crError( "packspu_GetAttribLocation doesn't work when there's no actual network involved!\nTry using the simplequery SPU in your chain!" );
    }

    crPackGetAttribLocation( program, name, &return_val, &writeback );
    packspuFlush( (void *) thread );
    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    return return_val;
}

GLint PACKSPU_APIENTRY packspu_GetAttribLocation(GLuint program, const char * name)
{
    if (!(CR_VBOX_CAP_GETATTRIBSLOCATIONS & g_u32VBoxHostCaps))
        return packspu_GetAttribLocationUnchached(program, name);

    if (!crStateIsProgramAttribsCached(&pack_spu.StateTracker, program))
    {
        GET_THREAD(thread);
        int writeback = 1;
        GLsizei maxcbData;
        GLsizei *pData;
        GLint mu;

        packspu_GetIntegerv(GL_MAX_VERTEX_ATTRIBS, &mu);
        maxcbData = 4*32*mu*sizeof(char);

        pData = (GLsizei *) crAlloc(maxcbData+sizeof(GLsizei));
        if (!pData)
        {
            crWarning("packspu_GetAttribLocation: not enough memory, fallback to single query");
            return packspu_GetAttribLocationUnchached(program, name);
        }

        crPackGetAttribsLocations(program, maxcbData, pData, NULL, &writeback);

        packspuFlush((void *) thread);
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);

        crStateGLSLProgramCacheAttribs(&pack_spu.StateTracker, program, pData[0], &pData[1]);

        CRASSERT(crStateIsProgramAttribsCached(&pack_spu.StateTracker, program));

        crFree(pData);
    }

    /*crDebug("packspu_GetAttribLocation(%d, %s)=%i", program, name, crStateGetAttribLocation(program, name));*/
    return crStateGetAttribLocation(&pack_spu.StateTracker, program, name);
}

void PACKSPU_APIENTRY packspu_GetUniformsLocations(GLuint program, GLsizei maxcbData, GLsizei * cbData, GLvoid * pData)
{
    (void) program;
    (void) maxcbData;
    (void) cbData;
    (void) pData;
    WARN(("packspu_GetUniformsLocations shouldn't be called directly"));
}

void PACKSPU_APIENTRY packspu_GetAttribsLocations(GLuint program, GLsizei maxcbData, GLsizei * cbData, GLvoid * pData)
{
    (void) program;
    (void) maxcbData;
    (void) cbData;
    (void) pData;
    WARN(("packspu_GetAttribsLocations shouldn't be called directly"));
}

void PACKSPU_APIENTRY packspu_DeleteProgram(GLuint program)
{
    crStateDeleteProgram(&pack_spu.StateTracker, program);
    crPackDeleteProgram(program);
}

void PACK_APIENTRY packspu_DeleteObjectARB(VBoxGLhandleARB obj)
{
    GLuint hwid = crStateGetProgramHWID(&pack_spu.StateTracker, obj);

    CRASSERT(obj);

    /* we do not track shader creation inside guest since it is not needed currently.
     * this is why we only care about programs here */
    if (hwid)
    {
        crStateDeleteProgram(&pack_spu.StateTracker, obj);
    }

    crPackDeleteObjectARB(obj);
}

#ifdef VBOX_WITH_CRPACKSPU_DUMPER
static void packspu_RecCheckInitRec()
{
    if (pack_spu.Recorder.pDumper)
        return;

    crDmpDbgPrintInit(&pack_spu.Dumper);

    crRecInit(&pack_spu.Recorder, NULL /*pBlitter: we do not support blitter operations here*/, &pack_spu.self, &pack_spu.Dumper.Base);
}
#endif

void PACKSPU_APIENTRY packspu_LinkProgram(GLuint program)
{
#ifdef VBOX_WITH_CRPACKSPU_DUMPER
    GLint linkStatus = 0;
#endif

    crStateLinkProgram(&pack_spu.StateTracker, program);
    crPackLinkProgram(program);

#ifdef VBOX_WITH_CRPACKSPU_DUMPER
    pack_spu.self.GetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linkStatus);
    Assert(linkStatus);
    if (!linkStatus)
    {
        CRContext *ctx = crStateGetCurrent(&pack_spu.StateTracker);
        packspu_RecCheckInitRec();
        crRecDumpProgram(&pack_spu.Recorder, ctx, program, program);
    }
#endif
}

void PACKSPU_APIENTRY packspu_CompileShader(GLuint shader)
{
#ifdef VBOX_WITH_CRPACKSPU_DUMPER
    GLint compileStatus = 0;
#endif

//    crStateCompileShader(shader);
    crPackCompileShader(shader);

#ifdef VBOX_WITH_CRPACKSPU_DUMPER
    pack_spu.self.GetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compileStatus);
    Assert(compileStatus);
    if (!compileStatus)
    {
        CRContext *ctx = crStateGetCurrent(&pack_spu.StateTracker);
        packspu_RecCheckInitRec();
        crRecDumpShader(&pack_spu.Recorder, ctx, shader, shader);
    }
#endif
}

