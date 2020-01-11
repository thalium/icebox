/* $Id: server_glsl.cpp $ */
/** @file
 * VBox OpenGL - GLSL related functions
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

#ifdef CR_OPENGL_VERSION_2_0

void SERVER_DISPATCH_APIENTRY crServerDispatchShaderSource(GLuint shader, GLsizei count, const char ** string, const GLint * length)
{
    /*@todo?crStateShaderSource(shader...);*/
#ifdef DEBUG_misha
    GLenum err = cr_server.head_spu->dispatch_table.GetError();
#endif
    cr_server.head_spu->dispatch_table.ShaderSource(crStateGetShaderHWID(&cr_server.StateTracker, shader), count, string, length);
#ifdef DEBUG_misha
    err = cr_server.head_spu->dispatch_table.GetError();
    CRASSERT(err == GL_NO_ERROR);
#endif
    CR_SERVER_DUMP_SHADER_SOURCE(shader);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchCompileShader(GLuint shader)
{
#ifdef DEBUG_misha
    GLint iCompileStatus = GL_FALSE;
#endif
    crStateCompileShader(&cr_server.StateTracker, shader);
    cr_server.head_spu->dispatch_table.CompileShader(crStateGetShaderHWID(&cr_server.StateTracker, shader));
#ifdef DEBUG_misha
    cr_server.head_spu->dispatch_table.GetShaderiv(crStateGetShaderHWID(&cr_server.StateTracker, shader), GL_COMPILE_STATUS, &iCompileStatus);
    Assert(iCompileStatus == GL_TRUE);
#endif
    CR_SERVER_DUMP_COMPILE_SHADER(shader);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteShader(GLuint shader)
{
    GLuint shaderHW = crStateGetShaderHWID(&cr_server.StateTracker, shader);
    crStateDeleteShader(&cr_server.StateTracker, shader);
    if (shaderHW)
        cr_server.head_spu->dispatch_table.DeleteShader(shaderHW);
    else
        crWarning("crServerDispatchDeleteShader: hwid not found for shader(%d)", shader);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchAttachShader(GLuint program, GLuint shader)
{
    crStateAttachShader(&cr_server.StateTracker, program, shader);
    cr_server.head_spu->dispatch_table.AttachShader(crStateGetProgramHWID(&cr_server.StateTracker, program),
                                                    crStateGetShaderHWID(&cr_server.StateTracker, shader));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDetachShader(GLuint program, GLuint shader)
{
    crStateDetachShader(&cr_server.StateTracker, program, shader);
    cr_server.head_spu->dispatch_table.DetachShader(crStateGetProgramHWID(&cr_server.StateTracker, program),
                                                    crStateGetShaderHWID(&cr_server.StateTracker, shader));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchLinkProgram(GLuint program)
{
    crStateLinkProgram(&cr_server.StateTracker, program);
    cr_server.head_spu->dispatch_table.LinkProgram(crStateGetProgramHWID(&cr_server.StateTracker, program));
    CR_SERVER_DUMP_LINK_PROGRAM(program);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchUseProgram(GLuint program)
{
    crStateUseProgram(&cr_server.StateTracker, program);
    cr_server.head_spu->dispatch_table.UseProgram(crStateGetProgramHWID(&cr_server.StateTracker, program));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteProgram(GLuint program)
{
    GLuint hwId = crStateGetProgramHWID(&cr_server.StateTracker, program);
    crStateDeleteProgram(&cr_server.StateTracker, program);
    if (hwId)
        cr_server.head_spu->dispatch_table.DeleteProgram(hwId);
    else
        crWarning("crServerDispatchDeleteProgram: hwid not found for program(%d)", program);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchValidateProgram(GLuint program)
{
    crStateValidateProgram(&cr_server.StateTracker, program);
    cr_server.head_spu->dispatch_table.ValidateProgram(crStateGetProgramHWID(&cr_server.StateTracker, program));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchBindAttribLocation(GLuint program, GLuint index, const char * name)
{
    crStateBindAttribLocation(&cr_server.StateTracker, program, index, name);
    cr_server.head_spu->dispatch_table.BindAttribLocation(crStateGetProgramHWID(&cr_server.StateTracker, program), index, name);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteObjectARB(VBoxGLhandleARB obj)
{
    GLuint hwid =  crStateDeleteObjectARB(&cr_server.StateTracker, obj);

    if (hwid)
        cr_server.head_spu->dispatch_table.DeleteObjectARB(hwid);
    else
        crWarning("zero hwid for object %d", obj);
}

GLint SERVER_DISPATCH_APIENTRY crServerDispatchGetAttribLocation( GLuint program, const char * name )
{
    GLint retval;
    retval = cr_server.head_spu->dispatch_table.GetAttribLocation(crStateGetProgramHWID(&cr_server.StateTracker, program), name );
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}

VBoxGLhandleARB SERVER_DISPATCH_APIENTRY crServerDispatchGetHandleARB( GLenum pname )
{
    VBoxGLhandleARB retval;
    retval = cr_server.head_spu->dispatch_table.GetHandleARB(pname);
    if (pname==GL_PROGRAM_OBJECT_ARB)
    {
        retval = crStateGLSLProgramHWIDtoID(&cr_server.StateTracker, retval);
    }
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}

GLint SERVER_DISPATCH_APIENTRY crServerDispatchGetUniformLocation(GLuint program, const char * name)
{
    GLint retval;
    retval = cr_server.head_spu->dispatch_table.GetUniformLocation(crStateGetProgramHWID(&cr_server.StateTracker, program), name);
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetProgramiv( GLuint program, GLenum pname, GLint * params )
{
    GLint local_params[1] = {0};
    (void) params;
    cr_server.head_spu->dispatch_table.GetProgramiv(crStateGetProgramHWID(&cr_server.StateTracker, program), pname, local_params);
    crServerReturnValue( &(local_params[0]), 1*sizeof(GLint) );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetShaderiv( GLuint shader, GLenum pname, GLint * params )
{
    GLint local_params[1] = {0};
    (void) params;
    cr_server.head_spu->dispatch_table.GetShaderiv( crStateGetShaderHWID(&cr_server.StateTracker, shader), pname, local_params );
    crServerReturnValue( &(local_params[0]), 1*sizeof(GLint) );
}
#endif /* #ifdef CR_OPENGL_VERSION_2_0 */

/* XXXX Note: shared/separate Program ID numbers aren't totally implemented! */
GLuint crServerTranslateProgramID( GLuint id )
{
    if (!cr_server.sharedPrograms && id) {
        int client = cr_server.curClient->number;
        return id + client * 100000;
    }
    return id;
}


void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteProgramsARB(GLsizei n, const GLuint * programs)
{
    GLuint *pLocalProgs;
    GLint i;

    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint))
    {
        crError("crServerDispatchDeleteProgramsARB: parameter 'n' is out of range");
        return;
    }

    pLocalProgs = (GLuint *)crAlloc(n * sizeof(GLuint));

    if (!pLocalProgs) {
        crError("crServerDispatchDeleteProgramsARB: out of memory");
        return;
    }
    for (i = 0; i < n; i++) {
        pLocalProgs[i] = crServerTranslateProgramID(programs[i]);
    }
    crStateDeleteProgramsARB(&cr_server.StateTracker, n, pLocalProgs);
    cr_server.head_spu->dispatch_table.DeleteProgramsARB(n, pLocalProgs);
    crFree(pLocalProgs);
}


/** @todo will fail for progs loaded from snapshot */
GLboolean SERVER_DISPATCH_APIENTRY crServerDispatchIsProgramARB( GLuint program )
{
    GLboolean retval;
    program = crServerTranslateProgramID(program);
    retval = cr_server.head_spu->dispatch_table.IsProgramARB( program );
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}


GLboolean SERVER_DISPATCH_APIENTRY
crServerDispatchAreProgramsResidentNV(GLsizei n, const GLuint *programs,
                                                                            GLboolean *residences)
{
    GLboolean retval = GL_FALSE;
    GLboolean *res;
    GLsizei i;
    (void) residences;

    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint))
    {
        crError("crServerDispatchAreProgramsResidentNV: parameter 'n' is out of range");
        return GL_FALSE;
    }

    res = (GLboolean *)crCalloc(n * sizeof(GLboolean));

    if (!res) {
        crError("crServerDispatchAreProgramsResidentNV: out of memory");
        return GL_FALSE;
    }

    if (!cr_server.sharedTextureObjects) {
        GLuint *programs2 = (GLuint *) crCalloc(n * sizeof(GLuint));
        if (programs2)
        {
            for (i = 0; i < n; i++)
                programs2[i] = crServerTranslateProgramID(programs[i]);

            retval = cr_server.head_spu->dispatch_table.AreProgramsResidentNV(n, programs2, res);
            crFree(programs2);
        }
        else
        {
            crError("crServerDispatchAreProgramsResidentNV: out of memory");
        }
    }
    else {
        retval = cr_server.head_spu->dispatch_table.AreProgramsResidentNV(n, programs, res);
    }

    crServerReturnValue(res, n * sizeof(GLboolean));
    crFree(res);

    return retval; /* WILL PROBABLY BE IGNORED */
}

