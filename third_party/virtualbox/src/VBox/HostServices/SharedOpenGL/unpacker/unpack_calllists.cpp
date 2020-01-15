/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackCallLists(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof( int ) + 4, GLenum);

    GLint n = READ_DATA(pState, sizeof( int ) + 0, GLint );
    GLenum type = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLvoid *lists = DATA_POINTER(pState, sizeof( int ) + 8, GLvoid );

    switch (type)
    {
        case GL_BYTE:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, n, GLbyte);
            break;
        case GL_UNSIGNED_BYTE:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, n, GLubyte);
            break;
        case GL_SHORT:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, n, GLshort);
            break;
        case GL_UNSIGNED_SHORT:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, n, GLushort);
            break;
        case GL_INT:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, n, GLint);
            break;
        case GL_UNSIGNED_INT:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, n, GLuint);
            break;
        case GL_FLOAT:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, n, GLfloat);
            break;
        case GL_2_BYTES:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, 2 * n, GLbyte);
            break;
        case GL_3_BYTES:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, 3 * n, GLbyte);
            break;
        case GL_4_BYTES:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, lists, 4 * n, GLbyte);
            break;
        default:
            crError("crUnpackCallLists: Invalid type (%#x) passed!\n", type);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->CallLists( n, type, lists );
    INCR_VAR_PTR(pState);
}
