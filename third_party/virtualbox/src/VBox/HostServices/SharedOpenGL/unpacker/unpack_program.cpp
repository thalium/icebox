/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_version.h"


void crUnpackExtendProgramParameter4dvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 40, GLdouble);

	GLenum target = READ_DATA(pState, 8, GLenum);
	GLuint index = READ_DATA(pState, 12, GLuint);
	GLdouble params[4];
	params[0] = READ_DOUBLE(pState, 16);
	params[1] = READ_DOUBLE(pState, 24);
	params[2] = READ_DOUBLE(pState, 32);
	params[3] = READ_DOUBLE(pState, 40);
	pState->pDispatchTbl->ProgramParameter4dvNV(target, index, params);
}


void crUnpackExtendProgramParameter4fvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 28, GLfloat);

	GLenum target = READ_DATA(pState, 8, GLenum);
	GLuint index = READ_DATA(pState, 12, GLuint);
	GLfloat params[4];
	params[0] = READ_DATA(pState, 16, GLfloat);
	params[1] = READ_DATA(pState, 20, GLfloat);
	params[2] = READ_DATA(pState, 24, GLfloat);
	params[3] = READ_DATA(pState, 28, GLfloat);
	pState->pDispatchTbl->ProgramParameter4fvNV(target, index, params);
}


void crUnpackExtendProgramParameters4dvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLuint);

    GLenum target = READ_DATA(pState, 8, GLenum);
    GLuint index = READ_DATA(pState, 12, GLuint);
    GLuint num = READ_DATA(pState, 16, GLuint);

    if (num <= 0 || num >= INT32_MAX / (4 * sizeof(GLdouble)))
    {
        crError("crUnpackExtendProgramParameters4dvNV: parameter 'num' is out of range");
        pState->rcUnpack = VERR_INVALID_PARAMETER;
        return;
    }

    GLdouble *params = DATA_POINTER(pState, 20, GLdouble);
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4 * num, GLdouble);

    pState->pDispatchTbl->ProgramParameters4dvNV(target, index, num, params);
}


void crUnpackExtendProgramParameters4fvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLuint);

    GLenum target = READ_DATA(pState, 8, GLenum);
    GLuint index = READ_DATA(pState, 12, GLuint);
    GLuint num = READ_DATA(pState, 16, GLuint);

    if (num <= 0 || num >= INT32_MAX / (4 * sizeof(GLfloat)))
    {
        crError("crUnpackExtendProgramParameters4fvNV: parameter 'num' is out of range");
        pState->rcUnpack = VERR_INVALID_PARAMETER;
        return;
    }

    GLfloat *params = DATA_POINTER(pState, 20, GLfloat);
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4 * num, GLfloat);

    pState->pDispatchTbl->ProgramParameters4fvNV(target, index, num, params);
}


void crUnpackExtendAreProgramsResidentNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLsizei);
    GLsizei n = READ_DATA(pState, 8, GLsizei);
    const GLuint *programs = DATA_POINTER(pState, 12, const GLuint);

    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint) / 4 || !DATA_POINTER_CHECK_SIZE(pState, 20 + n * sizeof(GLuint)))
    {
        crError("crUnpackExtendAreProgramsResidentNV: %d is out of range", n);
        pState->rcUnpack = VERR_INVALID_PARAMETER;
        return;
    }
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, programs, n, GLuint);
    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 20 + n * sizeof(GLuint), CRNetworkPointer);

    SET_RETURN_PTR(pState, 12 + n * sizeof(GLuint));
    SET_WRITEBACK_PTR(pState, 20 + n * sizeof(GLuint));
    (void) pState->pDispatchTbl->AreProgramsResidentNV(n, programs, NULL);
}


void crUnpackExtendLoadProgramNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLsizei);

    GLenum target = READ_DATA(pState, 8, GLenum);
    GLuint id = READ_DATA(pState, 12, GLuint);
    GLsizei len = READ_DATA(pState, 16, GLsizei);

    GLvoid *program = DATA_POINTER(pState, 20, GLvoid);
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, program, len, GLubyte);
    pState->pDispatchTbl->LoadProgramNV(target, id, len, (GLubyte *)program);
}


void crUnpackExtendExecuteProgramNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 20, GLfloat);

	GLenum target = READ_DATA(pState, 8, GLenum);
	GLuint id = READ_DATA(pState, 12, GLuint);
	GLfloat params[4];
	params[0] = READ_DATA(pState, 16, GLfloat);
	params[1] = READ_DATA(pState, 20, GLfloat);
	params[2] = READ_DATA(pState, 24, GLfloat);
	params[3] = READ_DATA(pState, 28, GLfloat);
	pState->pDispatchTbl->ExecuteProgramNV(target, id, params);
}

void crUnpackExtendRequestResidentProgramsNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLsizei);

	GLsizei n = READ_DATA(pState, 8, GLsizei);
	crError("RequestResidentProgramsNV needs to be special cased!");
	pState->pDispatchTbl->RequestResidentProgramsNV(n, NULL);
}


void crUnpackExtendProgramLocalParameter4fvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 28, GLfloat);

	GLenum target = READ_DATA(pState, 8, GLenum);
	GLuint index = READ_DATA(pState, 12, GLuint);
	GLfloat params[4];
	params[0] = READ_DATA(pState, 16, GLfloat);
	params[1] = READ_DATA(pState, 20, GLfloat);
	params[2] = READ_DATA(pState, 24, GLfloat);
	params[3] = READ_DATA(pState, 28, GLfloat);
	pState->pDispatchTbl->ProgramLocalParameter4fvARB(target, index, params);
}


void crUnpackExtendProgramLocalParameter4dvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 40, GLdouble);

	GLenum target = READ_DATA(pState, 8, GLenum);
	GLuint index = READ_DATA(pState, 12, GLuint);
	GLdouble params[4];
	params[0] = READ_DOUBLE(pState, 16);
	params[1] = READ_DOUBLE(pState, 24);
	params[2] = READ_DOUBLE(pState, 32);
	params[3] = READ_DOUBLE(pState, 40);
	pState->pDispatchTbl->ProgramLocalParameter4dvARB(target, index, params);
}



void crUnpackExtendProgramNamedParameter4dvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 40, GLdouble);

    GLuint id = READ_DATA(pState, 8, GLuint);
    GLsizei len = READ_DATA(pState, 12, GLsizei);
    GLdouble *params = DATA_POINTER(pState, 16, GLdouble);
    GLubyte *name = DATA_POINTER(pState, 48, GLubyte);
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, name, len, GLubyte);
    pState->pDispatchTbl->ProgramNamedParameter4dvNV(id, len, name, params);
}

void crUnpackExtendProgramNamedParameter4dNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 40, GLdouble);

    GLuint id = READ_DATA(pState, 8, GLuint);
    GLsizei len = READ_DATA(pState, 12, GLsizei);
    GLdouble params[4];
    params[0] = READ_DOUBLE(pState, 16);
    params[1] = READ_DOUBLE(pState, 24);
    params[2] = READ_DOUBLE(pState, 32);
    params[3] = READ_DOUBLE(pState, 40);

    GLubyte *name = DATA_POINTER(pState, 48, GLubyte);
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, name, len, GLubyte);
    pState->pDispatchTbl->ProgramNamedParameter4dNV(id, len, name, params[0], params[1], params[2], params[3]);
}

void crUnpackExtendProgramNamedParameter4fNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 28, GLfloat);

    GLenum id = READ_DATA(pState, 8, GLuint);
    GLsizei len = READ_DATA(pState, 12, GLsizei);
    GLfloat params[4];
    params[0] = READ_DATA(pState, 16, GLfloat);
    params[1] = READ_DATA(pState, 20, GLfloat);
    params[2] = READ_DATA(pState, 24, GLfloat);
    params[3] = READ_DATA(pState, 28, GLfloat);

    GLubyte *name = DATA_POINTER(pState, 32, GLubyte);
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, name, len, GLubyte);
    pState->pDispatchTbl->ProgramNamedParameter4fNV(id, len, name, params[0], params[1], params[2], params[3]);
}

void crUnpackExtendProgramNamedParameter4fvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 28, GLfloat);

    GLenum id = READ_DATA(pState, 8, GLuint);
    GLsizei len = READ_DATA(pState, 12, GLsizei);
    GLfloat *params = DATA_POINTER(pState, 16, GLfloat);
    GLubyte *name = DATA_POINTER(pState, 32, GLubyte);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, name, len, GLubyte);
    pState->pDispatchTbl->ProgramNamedParameter4fvNV(id, len, name, params);
}

void crUnpackExtendGetProgramNamedParameterdvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLuint id = READ_DATA(pState, 8, GLuint);
    GLsizei len = READ_DATA(pState, 12, GLsizei);
    const GLubyte *name = DATA_POINTER(pState, 16, GLubyte);

    if (len <= 0 || len >= INT32_MAX / 4)
    {
        crError("crUnpackExtendGetProgramNamedParameterdvNV: len %d is out of range", len);
        pState->rcUnpack = VERR_INVALID_PARAMETER;
        return;
    }
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, name, len, GLubyte);
    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 16 + len + sizeof(CRNetworkPointer), CRNetworkPointer);

    SET_RETURN_PTR(pState, 16+len);
    SET_WRITEBACK_PTR(pState, 16+len+8);
    pState->pDispatchTbl->GetProgramNamedParameterdvNV(id, len, name, NULL);
}

void crUnpackExtendGetProgramNamedParameterfvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLuint id = READ_DATA(pState, 8, GLuint);
    GLsizei len = READ_DATA(pState, 12, GLsizei);
    const GLubyte *name = DATA_POINTER(pState, 16, GLubyte);

    if (len <= 0 || len >= INT32_MAX / 4)
    {
        crError("crUnpackExtendGetProgramNamedParameterfvNV: len %d is out of range", len);
        pState->rcUnpack = VERR_INVALID_PARAMETER;
        return;
    }

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, name, len, GLubyte);
    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 16 + len + sizeof(CRNetworkPointer), CRNetworkPointer);
    SET_RETURN_PTR(pState, 16+len);
    SET_WRITEBACK_PTR(pState, 16+len+8);
    pState->pDispatchTbl->GetProgramNamedParameterfvNV(id, len, name, NULL);
}

void crUnpackExtendProgramStringARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLsizei);

    GLenum target = READ_DATA(pState, 8, GLenum);
    GLenum format = READ_DATA(pState, 12, GLuint);
    GLsizei len = READ_DATA(pState, 16, GLsizei);
    GLvoid *program = DATA_POINTER(pState, 20, GLvoid);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, program, len, GLubyte);
    pState->pDispatchTbl->ProgramStringARB(target, format, len, program);
}

void crUnpackExtendGetProgramStringARB(PCrUnpackerState pState)
{
    RT_NOREF(pState);
}

void crUnpackExtendProgramEnvParameter4dvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 40, GLdouble);

	GLenum target = READ_DATA(pState, 8, GLenum);
	GLuint index = READ_DATA(pState, 12, GLuint);
	GLdouble params[4];
	params[0] = READ_DOUBLE(pState, 16);
	params[1] = READ_DOUBLE(pState, 24);
	params[2] = READ_DOUBLE(pState, 32);
	params[3] = READ_DOUBLE(pState, 40);
	pState->pDispatchTbl->ProgramEnvParameter4dvARB(target, index, params);
}

void crUnpackExtendProgramEnvParameter4fvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 28, GLfloat);

	GLenum target = READ_DATA(pState, 8, GLenum);
	GLuint index = READ_DATA(pState, 12, GLuint);
	GLfloat params[4];
	params[0] = READ_DATA(pState, 16, GLfloat);
	params[1] = READ_DATA(pState, 20, GLfloat);
	params[2] = READ_DATA(pState, 24, GLfloat);
	params[3] = READ_DATA(pState, 28, GLfloat);
	pState->pDispatchTbl->ProgramEnvParameter4fvARB(target, index, params);
}

void crUnpackExtendDeleteProgramsARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLsizei);

    GLsizei n = READ_DATA(pState, 8, GLsizei);
    const GLuint *programs = DATA_POINTER(pState, 12, GLuint);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, programs, n, GLuint);
    pState->pDispatchTbl->DeleteProgramsARB(n, programs);
}

void crUnpackVertexAttrib4NbvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 8);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLbyte *v = DATA_POINTER(pState, 4, const GLbyte);

    pState->pDispatchTbl->VertexAttrib4NbvARB(index, v);
    INCR_DATA_PTR(pState, 8);
}

void crUnpackVertexAttrib4NivARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 20);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLint *v = DATA_POINTER(pState, 4, const GLint);

    pState->pDispatchTbl->VertexAttrib4NivARB(index, v);
    INCR_DATA_PTR(pState, 20);
}

void crUnpackVertexAttrib4NsvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 12);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLshort *v = DATA_POINTER(pState, 4, const GLshort);

    pState->pDispatchTbl->VertexAttrib4NsvARB(index, v);
    INCR_DATA_PTR(pState, 12);
}

void crUnpackVertexAttrib4NubvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 8);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLubyte *v = DATA_POINTER(pState, 4, const GLubyte);

    pState->pDispatchTbl->VertexAttrib4NubvARB(index, v);
    INCR_DATA_PTR(pState, 8);
}

void crUnpackVertexAttrib4NuivARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 20);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLuint *v = DATA_POINTER(pState, 4, const GLuint);

    pState->pDispatchTbl->VertexAttrib4NuivARB(index, v);
    INCR_DATA_PTR(pState, 20);
}

void crUnpackVertexAttrib4NusvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 12);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLushort *v = DATA_POINTER(pState, 4, const GLushort);

    pState->pDispatchTbl->VertexAttrib4NusvARB(index, v);
    INCR_DATA_PTR(pState, 12);
}

void crUnpackVertexAttrib4bvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 8);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLbyte *v = DATA_POINTER(pState, 4, const GLbyte);

    pState->pDispatchTbl->VertexAttrib4bvARB(index, v);
    INCR_DATA_PTR(pState, 8);
}

void crUnpackVertexAttrib4ivARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 20);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLint *v = DATA_POINTER(pState, 4, const GLint);

    pState->pDispatchTbl->VertexAttrib4ivARB(index, v);
    INCR_DATA_PTR(pState, 20);
}

void crUnpackVertexAttrib4ubvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 8);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLubyte *v = DATA_POINTER(pState, 4, const GLubyte);

    pState->pDispatchTbl->VertexAttrib4ubvARB(index, v);
    INCR_DATA_PTR(pState, 8);
}

void crUnpackVertexAttrib4uivARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 20);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLuint *v = DATA_POINTER(pState, 4, const GLuint);

    pState->pDispatchTbl->VertexAttrib4uivARB(index, v);
    INCR_DATA_PTR(pState, 20);
}

void crUnpackVertexAttrib4usvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 12);

    GLuint index = READ_DATA(pState, 0, GLuint);
    const GLushort *v = DATA_POINTER(pState, 4, const GLushort);

    pState->pDispatchTbl->VertexAttrib4usvARB(index, v);
    INCR_DATA_PTR(pState, 12);
}
