/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_mem.h"
#include "cr_error.h"


void crUnpackExtendGetBufferSubDataARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 28 + sizeof(CRNetworkPointer));

    GLenum target = READ_DATA(pState, 8, GLenum );
    GLintptrARB offset = READ_DATA(pState, 12, GLuint );
    GLsizeiptrARB size = READ_DATA(pState, 16, GLuint );

    SET_RETURN_PTR(pState, 20);
    SET_WRITEBACK_PTR(pState, 28);

    pState->pDispatchTbl->GetBufferSubDataARB( target, offset, size, NULL );
}


void crUnpackExtendBufferDataARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, sizeof(int) + 20);

    GLenum target      = READ_DATA(pState, sizeof(int) + 4, GLenum);
    GLsizeiptrARB size = READ_DATA(pState, sizeof(int) + 8, GLuint);
    GLenum usage       = READ_DATA(pState, sizeof(int) + 12, GLenum);
    GLboolean hasdata  = READ_DATA(pState, sizeof(int) + 16, GLint);
    GLvoid *data       = DATA_POINTER(pState, sizeof(int) + 20, GLvoid);

    if (hasdata)
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, data, size, GLubyte);
    pState->pDispatchTbl->BufferDataARB(target, size, hasdata ? data:NULL, usage);
}


void crUnpackExtendBufferSubDataARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, sizeof(int) + 16);

    GLenum target = READ_DATA(pState, sizeof(int) + 4, GLenum );
    GLintptrARB offset = READ_DATA(pState, sizeof(int) + 8, GLuint );
    GLsizeiptrARB size = READ_DATA(pState, sizeof(int) + 12, GLuint );
    GLvoid *data = DATA_POINTER(pState, sizeof(int) + 16, GLvoid );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, data, size, GLubyte);
    pState->pDispatchTbl->BufferSubDataARB( target, offset, size, data );
}


void crUnpackExtendDeleteBuffersARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 8 + sizeof(GLsizei));

    GLsizei n = READ_DATA(pState, 8, GLsizei );
    const GLuint *buffers = DATA_POINTER(pState, 12, GLuint );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, buffers, n, GLuint);
    pState->pDispatchTbl->DeleteBuffersARB( n, buffers );
}

