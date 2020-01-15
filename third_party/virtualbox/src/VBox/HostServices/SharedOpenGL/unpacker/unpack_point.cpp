/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackExtendPointParameterfvARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER(pState, sizeof( int ) + 8, GLfloat );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLfloat);
    pState->pDispatchTbl->PointParameterfvARB( pname, params );
}

void crUnpackExtendPointParameteriv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 0, GLenum);

    GLenum pname = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLint *params = DATA_POINTER(pState, sizeof( int ) + 4, GLint );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLint);
    pState->pDispatchTbl->PointParameteriv( pname, params );
}

