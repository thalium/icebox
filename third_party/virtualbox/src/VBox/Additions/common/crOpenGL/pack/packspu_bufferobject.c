/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_error.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "packspu.h"
#include "packspu_proto.h"

static void packspu_GetHostBufferSubDataARB( GLenum target, GLintptrARB offset, GLsizeiptrARB size, void * data )
{
    GET_THREAD(thread);
    int writeback = 1;

    crPackGetBufferSubDataARB(target, offset, size, data, &writeback);

    packspuFlush((void *) thread);

    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
}

void * PACKSPU_APIENTRY
packspu_MapBufferARB( GLenum target, GLenum access )
{
    GET_CONTEXT(ctx);
    void *buffer;
    CRBufferObject *pBufObj;

    CRASSERT(GL_TRUE == ctx->clientState->bufferobject.retainBufferData);
    buffer = crStateMapBufferARB(target, access);

#ifdef CR_ARB_pixel_buffer_object
    if (buffer)
    {
        pBufObj = crStateGetBoundBufferObject(target, &ctx->clientState->bufferobject);
        CRASSERT(pBufObj);

        if (pBufObj->bResyncOnRead && 
            access != GL_WRITE_ONLY_ARB)
        {
            /*fetch data from host side*/
            packspu_GetHostBufferSubDataARB(target, 0, pBufObj->size, buffer);
        }
    }
#endif

    return buffer;
}

void PACKSPU_APIENTRY packspu_GetBufferSubDataARB( GLenum target, GLintptrARB offset, GLsizeiptrARB size, void * data )
{
    GET_CONTEXT(ctx);

#ifdef CR_ARB_pixel_buffer_object
    CRBufferObject *pBufObj;

    pBufObj = crStateGetBoundBufferObject(target, &ctx->clientState->bufferobject);

    if (pBufObj && pBufObj->bResyncOnRead)
    {
        packspu_GetHostBufferSubDataARB(target, offset, size, data);
        return;
    }
#endif
    
    crStateGetBufferSubDataARB(target, offset, size, data);
}


GLboolean PACKSPU_APIENTRY
packspu_UnmapBufferARB( GLenum target )
{
    GET_CONTEXT(ctx);

#if CR_ARB_vertex_buffer_object
    CRBufferObject *bufObj;

    bufObj = crStateGetBoundBufferObject(target, &ctx->clientState->bufferobject);

    /* send new buffer contents to server */
    crPackBufferDataARB( target, bufObj->size, bufObj->pointer, bufObj->usage );
#endif

    CRASSERT(GL_TRUE == ctx->clientState->bufferobject.retainBufferData);
    crStateUnmapBufferARB( target );

    return GL_TRUE;
}


void PACKSPU_APIENTRY
packspu_BufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage)
{
    /*crDebug("packspu_BufferDataARB size:%d", size);*/
    crStateBufferDataARB(target, size, data, usage);
    crPackBufferDataARB(target, size, data, usage);
}

void PACKSPU_APIENTRY
packspu_BufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data)
{
    /*crDebug("packspu_BufferSubDataARB size:%d", size);*/
    crStateBufferSubDataARB(target, offset, size, data);
    crPackBufferSubDataARB(target, offset, size, data);
}


void PACKSPU_APIENTRY
packspu_GetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params)
{
    crStateGetBufferPointervARB( target, pname, params );
}


void PACKSPU_APIENTRY
packspu_GetBufferParameterivARB( GLenum target, GLenum pname, GLint * params )
{
    crStateGetBufferParameterivARB( target, pname, params );
}

/*
 * Need to update our local state for vertex arrays.
 */
void PACKSPU_APIENTRY
packspu_BindBufferARB( GLenum target, GLuint buffer )
{
    crStateBindBufferARB(target, buffer);
    crPackBindBufferARB(target, buffer);
}

void PACKSPU_APIENTRY packspu_GenBuffersARB( GLsizei n, GLuint * buffer )
{
    GET_THREAD(thread);
    int writeback = 1;
    if (!CRPACKSPU_IS_WDDM_CRHGSMI() && !(pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network))
    {
        crError( "packspu_GenBuffersARB doesn't work when there's no actual network involved!\nTry using the simplequery SPU in your chain!" );
    }
    if (pack_spu.swap)
    {
        crPackGenBuffersARBSWAP( n, buffer, &writeback );
    }
    else
    {
        crPackGenBuffersARB( n, buffer, &writeback );
    }
    packspuFlush( (void *) thread );
    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);

    crStateRegBuffers(n, buffer);
}

void PACKSPU_APIENTRY packspu_DeleteBuffersARB( GLsizei n, const GLuint * buffer )
{
    crStateDeleteBuffersARB( n, buffer );
    crPackDeleteBuffersARB(n, buffer);
}
