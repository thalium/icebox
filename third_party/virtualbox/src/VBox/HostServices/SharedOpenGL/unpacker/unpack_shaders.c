/* $Id: unpack_shaders.c $ */
/** @file
 * VBox OpenGL DRI driver functions
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

#include "unpacker.h"
#include "cr_error.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_version.h"

void crUnpackExtendBindAttribLocation(void)
{
    GLuint program   = READ_DATA(8, GLuint);
    GLuint index     = READ_DATA(12, GLuint);
    const char *name = DATA_POINTER(16, const char);

    cr_unpackDispatch.BindAttribLocation(program, index, name);
}

void crUnpackExtendShaderSource(void)
{
    GLint *length = NULL;
    GLuint shader = READ_DATA(8, GLuint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLint hasNonLocalLen = READ_DATA(16, GLsizei);
    GLint *pLocalLength = DATA_POINTER(20, GLint);
    char **ppStrings = NULL;
    GLsizei i, j, jUpTo;
    int pos, pos_check;

    if (count >= UINT32_MAX / sizeof(char *) / 4)
    {
        crError("crUnpackExtendShaderSource: count %u is out of range", count);
        return;
    }

    pos = 20 + count * sizeof(*pLocalLength);

    if (hasNonLocalLen > 0)
    {
        length = DATA_POINTER(pos, GLint);
        pos += count * sizeof(*length);
    }

    pos_check = pos; 

    if (!DATA_POINTER_CHECK(pos_check))
    {
        crError("crUnpackExtendShaderSource: pos %d is out of range", pos_check);
        return;
    }

    for (i = 0; i < count; ++i)
    {
        if (pLocalLength[i] <= 0 || pos_check >= INT32_MAX - pLocalLength[i] || !DATA_POINTER_CHECK(pos_check))
        {
            crError("crUnpackExtendShaderSource: pos %d is out of range", pos_check);
            return;
        }

        pos_check += pLocalLength[i];
    }

    ppStrings = crAlloc(count * sizeof(char*));
    if (!ppStrings) return;

    for (i = 0; i < count; ++i)
    {
        ppStrings[i] = DATA_POINTER(pos, char);
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

//    cr_unpackDispatch.ShaderSource(shader, count, ppStrings, length ? length : pLocalLength);
    cr_unpackDispatch.ShaderSource(shader, 1, (const char**)ppStrings, 0);

    crFree(ppStrings);
}

void crUnpackExtendUniform1fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    const GLfloat *value = DATA_POINTER(16, const GLfloat);
    cr_unpackDispatch.Uniform1fv(location, count, value);
}

void crUnpackExtendUniform1iv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    const GLint *value = DATA_POINTER(16, const GLint);
    cr_unpackDispatch.Uniform1iv(location, count, value);
}

void crUnpackExtendUniform2fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    const GLfloat *value = DATA_POINTER(16, const GLfloat);
    cr_unpackDispatch.Uniform2fv(location, count, value);
}

void crUnpackExtendUniform2iv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    const GLint *value = DATA_POINTER(16, const GLint);
    cr_unpackDispatch.Uniform2iv(location, count, value);
}

void crUnpackExtendUniform3fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    const GLfloat *value = DATA_POINTER(16, const GLfloat);
    cr_unpackDispatch.Uniform3fv(location, count, value);
}

void crUnpackExtendUniform3iv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    const GLint *value = DATA_POINTER(16, const GLint);
    cr_unpackDispatch.Uniform3iv(location, count, value);
}

void crUnpackExtendUniform4fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    const GLfloat *value = DATA_POINTER(16, const GLfloat);
    cr_unpackDispatch.Uniform4fv(location, count, value);
}

void crUnpackExtendUniform4iv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    const GLint *value = DATA_POINTER(16, const GLint);
    cr_unpackDispatch.Uniform4iv(location, count, value);
}

void crUnpackExtendUniformMatrix2fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix2fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix3fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix3fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix4fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix4fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix2x3fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix2x3fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix3x2fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix3x2fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix2x4fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix2x4fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix4x2fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix4x2fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix3x4fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix3x4fv(location, count, transpose, value);
}

void crUnpackExtendUniformMatrix4x3fv(void)
{
    GLint location = READ_DATA(8, GLint);
    GLsizei count = READ_DATA(12, GLsizei);
    GLboolean transpose = READ_DATA(16, GLboolean);
    const GLfloat *value = DATA_POINTER(16+sizeof(GLboolean), const GLfloat);
    cr_unpackDispatch.UniformMatrix4x3fv(location, count, transpose, value);
}

void crUnpackExtendDrawBuffers(void)
{
    GLsizei n = READ_DATA(8, GLsizei);
    const GLenum *bufs = DATA_POINTER(8+sizeof(GLsizei), const GLenum);
    cr_unpackDispatch.DrawBuffers(n, bufs);
}

void crUnpackExtendGetActiveAttrib(void)
{
    GLuint program = READ_DATA(8, GLuint);
    GLuint index = READ_DATA(12, GLuint);
    GLsizei bufSize = READ_DATA(16, GLsizei);
    SET_RETURN_PTR(20);
    SET_WRITEBACK_PTR(28);
    cr_unpackDispatch.GetActiveAttrib(program, index, bufSize, NULL, NULL, NULL, NULL);
}

void crUnpackExtendGetActiveUniform(void)
{
    GLuint program = READ_DATA(8, GLuint);
    GLuint index = READ_DATA(12, GLuint);
    GLsizei bufSize = READ_DATA(16, GLsizei);
    SET_RETURN_PTR(20);
    SET_WRITEBACK_PTR(28);
    cr_unpackDispatch.GetActiveUniform(program, index, bufSize, NULL, NULL, NULL, NULL);
}

void crUnpackExtendGetAttachedShaders(void)
{
    GLuint program = READ_DATA(8, GLuint);
    GLsizei maxCount = READ_DATA(12, GLsizei);
    SET_RETURN_PTR(16);
    SET_WRITEBACK_PTR(24);
    cr_unpackDispatch.GetAttachedShaders(program, maxCount, NULL, NULL);
}

void crUnpackExtendGetAttachedObjectsARB(void)
{
        VBoxGLhandleARB containerObj = READ_DATA(8, VBoxGLhandleARB);
        GLsizei maxCount = READ_DATA(12, GLsizei);
        SET_RETURN_PTR(16);
        SET_WRITEBACK_PTR(24);
        cr_unpackDispatch.GetAttachedObjectsARB(containerObj, maxCount, NULL, NULL);
}

void crUnpackExtendGetInfoLogARB(void)
{
        VBoxGLhandleARB obj = READ_DATA(8, VBoxGLhandleARB);
        GLsizei maxLength = READ_DATA(12, GLsizei);
        SET_RETURN_PTR(16);
        SET_WRITEBACK_PTR(24);
        cr_unpackDispatch.GetInfoLogARB(obj, maxLength, NULL, NULL);
}

void crUnpackExtendGetProgramInfoLog(void)
{
    GLuint program = READ_DATA(8, GLuint);
    GLsizei bufSize = READ_DATA(12, GLsizei);
    SET_RETURN_PTR(16);
    SET_WRITEBACK_PTR(24);
    cr_unpackDispatch.GetProgramInfoLog(program, bufSize, NULL, NULL);
}

void crUnpackExtendGetShaderInfoLog(void)
{
    GLuint shader = READ_DATA(8, GLuint);
    GLsizei bufSize = READ_DATA(12, GLsizei);
    SET_RETURN_PTR(16);
    SET_WRITEBACK_PTR(24);
    cr_unpackDispatch.GetShaderInfoLog(shader, bufSize, NULL, NULL);
}

void crUnpackExtendGetShaderSource(void)
{
    GLuint shader = READ_DATA(8, GLuint);
    GLsizei bufSize = READ_DATA(12, GLsizei);
    SET_RETURN_PTR(16);
    SET_WRITEBACK_PTR(24);
    cr_unpackDispatch.GetShaderSource(shader, bufSize, NULL, NULL);
}

void crUnpackExtendGetAttribLocation(void)
{
    int packet_length = READ_DATA(0, int);
    GLuint program = READ_DATA(8, GLuint);
    const char *name = DATA_POINTER(12, const char);
    SET_RETURN_PTR(packet_length-16);
    SET_WRITEBACK_PTR(packet_length-8);
    cr_unpackDispatch.GetAttribLocation(program, name);
}

void crUnpackExtendGetUniformLocation(void)
{
    int packet_length = READ_DATA(0, int);
    GLuint program = READ_DATA(8, GLuint);
    const char *name = DATA_POINTER(12, const char);
    SET_RETURN_PTR(packet_length-16);
    SET_WRITEBACK_PTR(packet_length-8);
    cr_unpackDispatch.GetUniformLocation(program, name);
}

void crUnpackExtendGetUniformsLocations(void)
{
        GLuint program = READ_DATA(8, GLuint);
        GLsizei maxcbData = READ_DATA(12, GLsizei);
        SET_RETURN_PTR(16);
        SET_WRITEBACK_PTR(24);
        cr_unpackDispatch.GetUniformsLocations(program, maxcbData, NULL, NULL);
}

void crUnpackExtendGetAttribsLocations(void)
{
    GLuint program = READ_DATA(8, GLuint);
    GLsizei maxcbData = READ_DATA(12, GLsizei);
    SET_RETURN_PTR(16);
    SET_WRITEBACK_PTR(24);
    cr_unpackDispatch.GetAttribsLocations(program, maxcbData, NULL, NULL);
}
