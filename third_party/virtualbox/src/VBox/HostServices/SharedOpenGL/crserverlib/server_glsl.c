/* $Id: server_glsl.c $ */
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
    cr_server.head_spu->dispatch_table.ShaderSource(crStateGetShaderHWID(shader), count, string, length);
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
    crStateCompileShader(shader);
    cr_server.head_spu->dispatch_table.CompileShader(crStateGetShaderHWID(shader));
#ifdef DEBUG_misha
    cr_server.head_spu->dispatch_table.GetShaderiv(crStateGetShaderHWID(shader), GL_COMPILE_STATUS, &iCompileStatus);
    Assert(iCompileStatus == GL_TRUE);
#endif
    CR_SERVER_DUMP_COMPILE_SHADER(shader);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteShader(GLuint shader)
{
    GLuint shaderHW = crStateGetShaderHWID(shader);
    crStateDeleteShader(shader);
    if (shaderHW)
        cr_server.head_spu->dispatch_table.DeleteShader(shaderHW);
    else
        crWarning("crServerDispatchDeleteShader: hwid not found for shader(%d)", shader);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchAttachShader(GLuint program, GLuint shader)
{
    crStateAttachShader(program, shader);
    cr_server.head_spu->dispatch_table.AttachShader(crStateGetProgramHWID(program), crStateGetShaderHWID(shader));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDetachShader(GLuint program, GLuint shader)
{
    crStateDetachShader(program, shader);
    cr_server.head_spu->dispatch_table.DetachShader(crStateGetProgramHWID(program), crStateGetShaderHWID(shader));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchLinkProgram(GLuint program)
{
    crStateLinkProgram(program);
    cr_server.head_spu->dispatch_table.LinkProgram(crStateGetProgramHWID(program));
    CR_SERVER_DUMP_LINK_PROGRAM(program);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchUseProgram(GLuint program)
{
    crStateUseProgram(program);
    cr_server.head_spu->dispatch_table.UseProgram(crStateGetProgramHWID(program));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteProgram(GLuint program)
{
    GLuint hwId = crStateGetProgramHWID(program);
    crStateDeleteProgram(program);
    if (hwId)
        cr_server.head_spu->dispatch_table.DeleteProgram(hwId);
    else
        crWarning("crServerDispatchDeleteProgram: hwid not found for program(%d)", program);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchValidateProgram(GLuint program)
{
    crStateValidateProgram(program);
    cr_server.head_spu->dispatch_table.ValidateProgram(crStateGetProgramHWID(program));
}

void SERVER_DISPATCH_APIENTRY crServerDispatchBindAttribLocation(GLuint program, GLuint index, const char * name)
{
    crStateBindAttribLocation(program, index, name);
    cr_server.head_spu->dispatch_table.BindAttribLocation(crStateGetProgramHWID(program), index, name);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteObjectARB(VBoxGLhandleARB obj)
{
    GLuint hwid =  crStateDeleteObjectARB(obj);

    if (hwid)
        cr_server.head_spu->dispatch_table.DeleteObjectARB(hwid);
    else
        crWarning("zero hwid for object %d", obj);
}

GLint SERVER_DISPATCH_APIENTRY crServerDispatchGetAttribLocation( GLuint program, const char * name )
{
    GLint retval;
    retval = cr_server.head_spu->dispatch_table.GetAttribLocation(crStateGetProgramHWID(program), name );
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}

VBoxGLhandleARB SERVER_DISPATCH_APIENTRY crServerDispatchGetHandleARB( GLenum pname )
{
    VBoxGLhandleARB retval;
    retval = cr_server.head_spu->dispatch_table.GetHandleARB(pname);
    if (pname==GL_PROGRAM_OBJECT_ARB)
    {
        retval = crStateGLSLProgramHWIDtoID(retval);
    }
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}

GLint SERVER_DISPATCH_APIENTRY crServerDispatchGetUniformLocation(GLuint program, const char * name)
{
    GLint retval;
    retval = cr_server.head_spu->dispatch_table.GetUniformLocation(crStateGetProgramHWID(program), name);
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetProgramiv( GLuint program, GLenum pname, GLint * params )
{
    GLint local_params[1];
    (void) params;
    cr_server.head_spu->dispatch_table.GetProgramiv(crStateGetProgramHWID(program), pname, local_params);
    crServerReturnValue( &(local_params[0]), 1*sizeof(GLint) );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetShaderiv( GLuint shader, GLenum pname, GLint * params )
{
    GLint local_params[1];
    (void) params;
    cr_server.head_spu->dispatch_table.GetShaderiv( crStateGetShaderHWID(shader), pname, local_params );
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

    if (n >= UINT32_MAX / sizeof(GLuint))
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
    crStateDeleteProgramsARB(n, pLocalProgs);
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
    GLboolean retval;
    GLboolean *res = (GLboolean *) crAlloc(n * sizeof(GLboolean));
    GLsizei i;

    (void) residences;

    if (!cr_server.sharedTextureObjects) {
        GLuint *programs2 = (GLuint *) crAlloc(n * sizeof(GLuint));
        for (i = 0; i < n; i++)
            programs2[i] = crServerTranslateProgramID(programs[i]);
        retval = cr_server.head_spu->dispatch_table.AreProgramsResidentNV(n, programs2, res);
        crFree(programs2);
    }
    else {
        retval = cr_server.head_spu->dispatch_table.AreProgramsResidentNV(n, programs, res);
    }

    crServerReturnValue(res, n * sizeof(GLboolean));
    crFree(res);

    return retval; /* WILL PROBABLY BE IGNORED */
}

