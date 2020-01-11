/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "chromium.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "server_dispatch.h"
#include "server.h"
#include "cr_unpack.h"

void * SERVER_DISPATCH_APIENTRY
crServerDispatchMapBufferARB( GLenum target, GLenum access )
{
    RT_NOREF(target, access);
	return NULL;
}

GLboolean SERVER_DISPATCH_APIENTRY
crServerDispatchUnmapBufferARB( GLenum target )
{
    RT_NOREF(target);
	return GL_FALSE;
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchGenBuffersARB(GLsizei n, GLuint *buffers)
{
    GLuint *local_buffers;
    (void) buffers;

    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint))
    {
        crError("crServerDispatchGenBuffersARB: parameter 'n' is out of range");
        return;
    }

    local_buffers = (GLuint *)crCalloc(n * sizeof(*local_buffers));

    if (!local_buffers)
    {
        crError("crServerDispatchGenBuffersARB: out of memory");
        return;
    }

    crStateGenBuffersARB(&cr_server.StateTracker, n, local_buffers);

    crServerReturnValue( local_buffers, n * sizeof(*local_buffers) );
    crFree( local_buffers );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteBuffersARB( GLsizei n, const GLuint * buffer )
{
    if (n <= 0 || n >= INT32_MAX / sizeof(GLuint))
    {
        crError("glDeleteBuffersARB: parameter 'n' is out of range");
        return;
    }

    crStateDeleteBuffersARB(&cr_server.StateTracker, n, buffer );
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchGetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params)
{
	crError( "glGetBufferPointervARB isn't *ever* allowed to be on the wire!" );
	(void) target;
	(void) pname;
	(void) params;
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchGetBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, void * data)
{
    void *b;

    RT_NOREF(data);

    if (size <= 0 || size >= INT32_MAX / 2)
    {
        crError("crServerDispatchGetBufferSubDataARB: size is out of range");
        return;
    }

    b = crCalloc(size);

    if (b) {
        cr_server.head_spu->dispatch_table.GetBufferSubDataARB( target, offset, size, b );

        crServerReturnValue( b, size );
        crFree( b );
    }
    else {
        crError("Out of memory in crServerDispatchGetBufferSubDataARB");
    }
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchBindBufferARB(GLenum target, GLuint buffer)
{
    crStateBindBufferARB(&cr_server.StateTracker, target, buffer);
    cr_server.head_spu->dispatch_table.BindBufferARB(target, crStateGetBufferHWID(&cr_server.StateTracker, buffer));
}

GLboolean SERVER_DISPATCH_APIENTRY
crServerDispatchIsBufferARB(GLuint buffer)
{
    /* since GenBuffersARB issued to host ogl only on bind + some other ops, the host drivers may not know about them
     * so use state data*/
    GLboolean retval = crStateIsBufferARB(&cr_server.StateTracker, buffer);
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}
