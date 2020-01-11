/* $Id: unpack_shaders.cpp $ */
/** @file
 * VBox OpenGL DRI driver functions
 */

/*
 * Copyright (C) 2009-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "unpacker.h"
#include "cr_error.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_version.h"

void crUnpackExtendBindAttribLocation(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLuint);

    GLuint program   = READ_DATA(pState, 8, GLuint);
    GLuint index     = READ_DATA(pState, 12, GLuint);
    const char *name = DATA_POINTER(pState, 16, const char);

    CHECK_STRING_FROM_PTR_UPDATE_NO_SZ(pState, name);
    pState->pDispatchTbl->BindAttribLocation(program, index, name);
}

void crUnpackExtendShaderSource(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLsizei);

    GLint *length = NULL;
    GLuint shader = READ_DATA(pState, 8, GLuint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLint hasNonLocalLen = READ_DATA(pState, 16, GLsizei);
    GLint *pLocalLength = DATA_POINTER(pState, 20, GLint);
    char **ppStrings = NULL;
    GLsizei i, j, jUpTo;
    int pos, pos_check;

    if (count <= 0 || count >= INT32_MAX / sizeof(GLint) / 8)
    {
        crError("crUnpackExtendShaderSource: count %u is out of range", count);
        return;
    }
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pLocalLength, count, GLint);

    /** @todo More verification required here. */
    pos = 20 + count * sizeof(*pLocalLength);

    if (!DATA_POINTER_CHECK_SIZE(pState, pos))
    {
        crError("crUnpackExtendShaderSource: pos %d is out of range", pos);
        return;
    }

    if (hasNonLocalLen > 0)
    {
        length = DATA_POINTER(pState, pos, GLint);
        pos += count * sizeof(*length);
    }

    pos_check = pos;

    if (!DATA_POINTER_CHECK_SIZE(pState, pos_check))
    {
        crError("crUnpackExtendShaderSource: pos %d is out of range", pos);
        return;
    }

    for (i = 0; i < count; ++i)
    {
        if (pLocalLength[i] <= 0 || pos_check >= INT32_MAX - pLocalLength[i] || !DATA_POINTER_CHECK_SIZE(pState, pos_check))
        {
            crError("crUnpackExtendShaderSource: pos %d is out of range", pos_check);
            return;
        }

        pos_check += pLocalLength[i];

        if (!DATA_POINTER_CHECK_SIZE(pState, pos_check))
        {
            crError("crUnpackExtendShaderSource: pos %d is out of range", pos_check);
            return;
        }
    }

    ppStrings = (char **)crAlloc(count * sizeof(char*));
    if (!ppStrings) return;

    for (i = 0; i < count; ++i)
    {
        CHECK_BUFFER_SIZE_STATIC_UPDATE(pState, pos); /** @todo Free ppStrings on error. */
        ppStrings[i] = DATA_POINTER(pState, pos, char);
        pos += pLocalLength[i];
        if (!length)
        {
            pLocalLength[i] -= 1;
        }

        Assert(pLocalLength[i] > 0);
        jUpTo = i == count -1 ? pLocalLength[i] - 1 : pLocalLength[i];
        for (j = 0; j < jUpTo; ++j)
        {
            char *pString = ppStrings[i];

            if (pString[j] == '\0')
            {
                Assert(j == jUpTo - 1);
                pString[j] = '\n';
            }
        }
    }

//    pState->pDispatchTbl->ShaderSource(shader, count, ppStrings, length ? length : pLocalLength);
    pState->pDispatchTbl->ShaderSource(shader, 1, (const char**)ppStrings, 0);

    crFree(ppStrings);
}

void crUnpackExtendUniform1fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    const GLfloat *value = DATA_POINTER(pState, 16, const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count, GLfloat);
    pState->pDispatchTbl->Uniform1fv(location, count, value);
}

void crUnpackExtendUniform1iv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    const GLint *value = DATA_POINTER(pState, 16, const GLint);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count, GLint);
    pState->pDispatchTbl->Uniform1iv(location, count, value);
}

void crUnpackExtendUniform2fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    const GLfloat *value = DATA_POINTER(pState, 16, const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 2, GLfloat);
    pState->pDispatchTbl->Uniform2fv(location, count, value);
}

void crUnpackExtendUniform2iv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    const GLint *value = DATA_POINTER(pState, 16, const GLint);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 2, GLint);
    pState->pDispatchTbl->Uniform2iv(location, count, value);
}

void crUnpackExtendUniform3fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    const GLfloat *value = DATA_POINTER(pState, 16, const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 3, GLfloat);
    pState->pDispatchTbl->Uniform3fv(location, count, value);
}

void crUnpackExtendUniform3iv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    const GLint *value = DATA_POINTER(pState, 16, const GLint);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 3, GLint);
    pState->pDispatchTbl->Uniform3iv(location, count, value);
}

void crUnpackExtendUniform4fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    const GLfloat *value = DATA_POINTER(pState, 16, const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 4, GLfloat);
    pState->pDispatchTbl->Uniform4fv(location, count, value);
}

void crUnpackExtendUniform4iv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLsizei);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    const GLint *value = DATA_POINTER(pState, 16, const GLint);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 4, GLint);
    pState->pDispatchTbl->Uniform4iv(location, count, value);
}

void crUnpackExtendUniformMatrix2fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 2 * 2, GLfloat);
    pState->pDispatchTbl->UniformMatrix2fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix3fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 3 * 3, GLfloat);
    pState->pDispatchTbl->UniformMatrix3fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix4fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 4 * 4, GLfloat);
    pState->pDispatchTbl->UniformMatrix4fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix2x3fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 2 * 3, GLfloat);
    pState->pDispatchTbl->UniformMatrix2x3fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix3x2fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 3 * 2, GLfloat);
    pState->pDispatchTbl->UniformMatrix3x2fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix2x4fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 2 * 4, GLfloat);
    pState->pDispatchTbl->UniformMatrix2x4fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix4x2fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 4 * 2, GLfloat);
    pState->pDispatchTbl->UniformMatrix4x2fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix3x4fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 3 * 4, GLfloat);
    pState->pDispatchTbl->UniformMatrix3x4fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix4x3fv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLboolean);

    GLint location = READ_DATA(pState, 8, GLint);
    GLsizei count = READ_DATA(pState, 12, GLsizei);
    GLboolean transpose = READ_DATA(pState, 16, GLboolean);
    const GLfloat *value = DATA_POINTER(pState, 16+sizeof(GLboolean), const GLfloat);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, value, count * 4 * 3, GLfloat);
    pState->pDispatchTbl->UniformMatrix4x3fv(location, count, transpose, value);
}

void crUnpackExtendDrawBuffers(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLsizei);

    GLsizei n = READ_DATA(pState, 8, GLsizei);
    const GLenum *bufs = DATA_POINTER(pState, 8+sizeof(GLsizei), const GLenum);

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, bufs, n, GLenum);
    pState->pDispatchTbl->DrawBuffers(n, bufs);
}

void crUnpackExtendGetActiveAttrib(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 28, CRNetworkPointer);

    GLuint program = READ_DATA(pState, 8, GLuint);
    GLuint index = READ_DATA(pState, 12, GLuint);
    GLsizei bufSize = READ_DATA(pState, 16, GLsizei);
    SET_RETURN_PTR(pState, 20);
    SET_WRITEBACK_PTR(pState, 28);
    pState->pDispatchTbl->GetActiveAttrib(program, index, bufSize, NULL, NULL, NULL, NULL);
}

void crUnpackExtendGetActiveUniform(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 28, CRNetworkPointer);

    GLuint program = READ_DATA(pState, 8, GLuint);
    GLuint index = READ_DATA(pState, 12, GLuint);
    GLsizei bufSize = READ_DATA(pState, 16, GLsizei);
    SET_RETURN_PTR(pState, 20);
    SET_WRITEBACK_PTR(pState, 28);
    pState->pDispatchTbl->GetActiveUniform(program, index, bufSize, NULL, NULL, NULL, NULL);
}

void crUnpackExtendGetAttachedShaders(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    GLuint program = READ_DATA(pState, 8, GLuint);
    GLsizei maxCount = READ_DATA(pState, 12, GLsizei);
    SET_RETURN_PTR(pState, 16);
    SET_WRITEBACK_PTR(pState, 24);
    pState->pDispatchTbl->GetAttachedShaders(program, maxCount, NULL, NULL);
}

void crUnpackExtendGetAttachedObjectsARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    VBoxGLhandleARB containerObj = READ_DATA(pState, 8, VBoxGLhandleARB);
    GLsizei maxCount = READ_DATA(pState, 12, GLsizei);
    SET_RETURN_PTR(pState, 16);
    SET_WRITEBACK_PTR(pState, 24);
    pState->pDispatchTbl->GetAttachedObjectsARB(containerObj, maxCount, NULL, NULL);
}

void crUnpackExtendGetInfoLogARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    VBoxGLhandleARB obj = READ_DATA(pState, 8, VBoxGLhandleARB);
    GLsizei maxLength = READ_DATA(pState, 12, GLsizei);
    SET_RETURN_PTR(pState, 16);
    SET_WRITEBACK_PTR(pState, 24);
    pState->pDispatchTbl->GetInfoLogARB(obj, maxLength, NULL, NULL);
}

void crUnpackExtendGetProgramInfoLog(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    GLuint program = READ_DATA(pState, 8, GLuint);
    GLsizei bufSize = READ_DATA(pState, 12, GLsizei);
    SET_RETURN_PTR(pState, 16);
    SET_WRITEBACK_PTR(pState, 24);
    pState->pDispatchTbl->GetProgramInfoLog(program, bufSize, NULL, NULL);
}

void crUnpackExtendGetShaderInfoLog(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    GLuint shader = READ_DATA(pState, 8, GLuint);
    GLsizei bufSize = READ_DATA(pState, 12, GLsizei);
    SET_RETURN_PTR(pState, 16);
    SET_WRITEBACK_PTR(pState, 24);
    pState->pDispatchTbl->GetShaderInfoLog(shader, bufSize, NULL, NULL);
}

void crUnpackExtendGetShaderSource(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    GLuint shader = READ_DATA(pState, 8, GLuint);
    GLsizei bufSize = READ_DATA(pState, 12, GLsizei);
    SET_RETURN_PTR(pState, 16);
    SET_WRITEBACK_PTR(pState, 24);
    pState->pDispatchTbl->GetShaderSource(shader, bufSize, NULL, NULL);
}

void crUnpackExtendGetAttribLocation(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLuint);

    unsigned packet_length = (unsigned)READ_DATA(pState, 0, int);
    GLuint program = READ_DATA(pState, 8, GLuint);
    const char *name = DATA_POINTER(pState, 12, const char);

    size_t cchStr = CHECK_STRING_FROM_PTR_UPDATE_NO_RETURN(pState, name);
    if (RT_UNLIKELY(cchStr == ~(size_t)0U || packet_length != cchStr + 2 * sizeof(CRNetworkPointer)))
    {
        crError("crUnpackExtendGetAttribLocation: packet_length is corrupt");
        return;
    }

    CHECK_BUFFER_SIZE_STATIC_UPDATE(pState, packet_length);
    SET_RETURN_PTR(pState, packet_length-16);
    SET_WRITEBACK_PTR(pState, packet_length-8);
    pState->pDispatchTbl->GetAttribLocation(program, name);
}

void crUnpackExtendGetUniformLocation(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLuint);

    unsigned packet_length = (unsigned)READ_DATA(pState, 0, int);
    GLuint program = READ_DATA(pState, 8, GLuint);
    const char *name = DATA_POINTER(pState, 12, const char);

    size_t cchStr = CHECK_STRING_FROM_PTR_UPDATE_NO_RETURN(pState, name);
    if (RT_UNLIKELY(cchStr == ~(size_t)0U || packet_length != cchStr + 2 * sizeof(CRNetworkPointer)))
    {
        crError("crUnpackExtendGetUniformLocation: packet_length is corrupt");
        return;
    }

    CHECK_BUFFER_SIZE_STATIC_UPDATE(pState, packet_length);
    SET_RETURN_PTR(pState, packet_length-16);
    SET_WRITEBACK_PTR(pState, packet_length-8);
    pState->pDispatchTbl->GetUniformLocation(program, name);
}

void crUnpackExtendGetUniformsLocations(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    GLuint program = READ_DATA(pState, 8, GLuint);
    GLsizei maxcbData = READ_DATA(pState, 12, GLsizei);
    SET_RETURN_PTR(pState, 16);
    SET_WRITEBACK_PTR(pState, 24);
    pState->pDispatchTbl->GetUniformsLocations(program, maxcbData, NULL, NULL);
}

void crUnpackExtendGetAttribsLocations(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    GLuint program = READ_DATA(pState, 8, GLuint);
    GLsizei maxcbData = READ_DATA(pState, 12, GLsizei);
    SET_RETURN_PTR(pState, 16);
    SET_WRITEBACK_PTR(pState, 24);
    pState->pDispatchTbl->GetAttribsLocations(program, maxcbData, NULL, NULL);
}
