/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_mem.h"
#include "cr_error.h"


void crUnpackExtendGetBufferSubDataARB( void )
{
    GLenum target = READ_DATA( 8, GLenum );
    GLintptrARB offset = READ_DATA( 12, GLuint );
    GLsizeiptrARB size = READ_DATA( 16, GLuint );

    SET_RETURN_PTR( 20 );
    SET_WRITEBACK_PTR( 28 );

    cr_unpackDispatch.GetBufferSubDataARB( target, offset, size, NULL );
}


void crUnpackExtendBufferDataARB( void )
{
    GLenum target      = READ_DATA(sizeof(int) + 4, GLenum);
    GLsizeiptrARB size = READ_DATA(sizeof(int) + 8, GLuint);
    GLenum usage       = READ_DATA(sizeof(int) + 12, GLenum);
    GLboolean hasdata  = READ_DATA(sizeof(int) + 16, GLint);
    GLvoid *data       = DATA_POINTER(sizeof(int) + 20, GLvoid);

    cr_unpackDispatch.BufferDataARB(target, size, hasdata ? data:NULL, usage);
}


void crUnpackExtendBufferSubDataARB( void )
{
    GLenum target = READ_DATA( sizeof(int) + 4, GLenum );
    GLintptrARB offset = READ_DATA( sizeof(int) + 8, GLuint );
    GLsizeiptrARB size = READ_DATA( sizeof(int) + 12, GLuint );
    GLvoid *data = DATA_POINTER( sizeof(int) + 16, GLvoid );

    cr_unpackDispatch.BufferSubDataARB( target, offset, size, data );
}


void crUnpackExtendDeleteBuffersARB(void)
{
    GLsizei n = READ_DATA( 8, GLsizei );
    const GLuint *buffers = DATA_POINTER( 12, GLuint );
    cr_unpackDispatch.DeleteBuffersARB( n, buffers );
}

