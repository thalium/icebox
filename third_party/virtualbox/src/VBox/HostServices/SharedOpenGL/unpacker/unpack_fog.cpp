/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackFogfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLfloat);

    GLenum pname = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLfloat *params = DATA_POINTER(pState, sizeof( int ) + 4, GLfloat );

    /*
     * GL_FOG_COLOR requires 4 elements values while all others require only
     * 1 element.
     */
    if (pname == GL_FOG_COLOR)
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLfloat);
    else
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLfloat);

    pState->pDispatchTbl->Fogfv( pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackFogiv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLint);
    GLenum pname = READ_DATA(pState,  sizeof( int ) + 0, GLenum );
    GLint *params = DATA_POINTER(pState,  sizeof( int ) + 4, GLint );

    /*
     * GL_FOG_COLOR requires 4 elements values while all others require only
     * 1 element.
     */
    if (pname == GL_FOG_COLOR)
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLint);
    else
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLint);

    pState->pDispatchTbl->Fogiv( pname, params );
    INCR_VAR_PTR(pState);
}
