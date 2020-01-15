/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackMaterialfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum face = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER(pState, sizeof( int ) + 8, GLfloat );

    switch (pname)
    {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_AMBIENT_AND_DIFFUSE:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLfloat);
            break;
        case GL_SHININESS:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLfloat);
            break;
        case GL_COLOR_INDEXES:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 3, GLfloat);
            break;
        default:
            crError("crUnpackMaterialfv: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->Materialfv( face, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackMaterialiv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum face = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER(pState, sizeof( int ) + 8, GLint );

    switch (pname)
    {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_AMBIENT_AND_DIFFUSE:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLint);
            break;
        case GL_SHININESS:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLint);
            break;
        case GL_COLOR_INDEXES:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 3, GLint);
            break;
        default:
            crError("crUnpackMaterialfv: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->Materialiv( face, pname, params );
    INCR_VAR_PTR(pState);
}
