/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

/*
 * This file manages all the client-side state including:
 *  Pixel pack/unpack parameters
 *  Vertex arrays
 */


#include "cr_mem.h"
#include "state.h"
#include "state/cr_statetypes.h"
#include "state/cr_statefuncs.h"
#include "state_internals.h"

const CRPixelPackState crStateNativePixelPacking = {
   0, /* rowLength */
   0, /* skipRows */
   0, /* skipPixels */
   1, /* alignment */
   0, /* imageHeight */
   0, /* skipImages */
   GL_FALSE, /* swapBytes */
   GL_FALSE, /* psLSBFirst */
};


void crStateClientInitBits (CRClientBits *c) 
{
    int i;

    /* XXX why GLCLIENT_BIT_ALLOC? */
    c->v = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    c->n = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    c->c = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    c->s = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    c->i = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    for ( i = 0; i < CR_MAX_TEXTURE_UNITS; i++ )
        c->t[i] = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    c->e = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    c->f = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));

#ifdef CR_NV_vertex_program
    for ( i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++ )
        c->a[i] = (CRbitvalue *) crCalloc(GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
#endif
}

void crStateClientDestroyBits (CRClientBits *c) 
{
    int i;

    crFree(c->v);
    crFree(c->n);
    crFree(c->c);
    crFree(c->s);
    crFree(c->i);

    for ( i = 0; i < CR_MAX_TEXTURE_UNITS; i++ )
        crFree(c->t[i]);

    crFree(c->e);
    crFree(c->f);

#ifdef CR_NV_vertex_program
    for ( i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++ )
        crFree(c->a[i]);
#endif
}

static void crStateUnlockClientPointer(CRClientPointer* cp)
{
    if (cp->locked)
    {
#ifndef IN_GUEST
        if (cp->p) crFree(cp->p);
#endif
        cp->locked = GL_FALSE;
    }
}

void crStateClientDestroy(CRContext *g)
{
    CRClientState *c = &(g->client);
#ifdef CR_EXT_compiled_vertex_array
    if (c->array.locked)
    {
        unsigned int i;

        crStateUnlockClientPointer(&c->array.v);
        crStateUnlockClientPointer(&c->array.c);
        crStateUnlockClientPointer(&c->array.f);
        crStateUnlockClientPointer(&c->array.s);
        crStateUnlockClientPointer(&c->array.e);
        crStateUnlockClientPointer(&c->array.i);
        crStateUnlockClientPointer(&c->array.n);
        for (i = 0 ; i < CR_MAX_TEXTURE_UNITS ; i++)
        {
            crStateUnlockClientPointer(&c->array.t[i]);
        }
        for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++)
        {
            crStateUnlockClientPointer(&c->array.a[i]);
        }
    }
#endif
}

void crStateClientInit(CRContext *ctx)
{
    CRClientState *c = &(ctx->client);
    unsigned int i;

    /* pixel pack/unpack */
    c->unpack.rowLength   = 0;
    c->unpack.skipRows    = 0;
    c->unpack.skipPixels  = 0;
    c->unpack.skipImages  = 0;
    c->unpack.alignment   = 4;
    c->unpack.imageHeight = 0;
    c->unpack.swapBytes   = GL_FALSE;
    c->unpack.psLSBFirst  = GL_FALSE;
    c->pack.rowLength     = 0;
    c->pack.skipRows      = 0;
    c->pack.skipPixels    = 0;
    c->pack.skipImages    = 0;
    c->pack.alignment     = 4;
    c->pack.imageHeight   = 0;
    c->pack.swapBytes     = GL_FALSE;
    c->pack.psLSBFirst    = GL_FALSE;

    /* ARB multitexture */
    c->curClientTextureUnit = 0;

#ifdef CR_EXT_compiled_vertex_array
    c->array.lockFirst = 0;
    c->array.lockCount = 0;
    c->array.locked = GL_FALSE;
# ifdef IN_GUEST
    c->array.synced = GL_FALSE;
# endif
#endif

    /* vertex array */
    c->array.v.p = NULL;
    c->array.v.size = 4;
    c->array.v.type = GL_FLOAT;
    c->array.v.stride = 0;
    c->array.v.enabled = 0;
#ifdef CR_ARB_vertex_buffer_object
    c->array.v.buffer = ctx->bufferobject.arrayBuffer;
    if (c->array.v.buffer)
        ++c->array.v.buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
    c->array.v.locked = GL_FALSE;
    c->array.v.prevPtr = NULL;
    c->array.v.prevStride = 0;
#endif

    /* color array */
    c->array.c.p = NULL;
    c->array.c.size = 4;
    c->array.c.type = GL_FLOAT;
    c->array.c.stride = 0;
    c->array.c.enabled = 0;
#ifdef CR_ARB_vertex_buffer_object
    c->array.c.buffer = ctx->bufferobject.arrayBuffer;
    if (c->array.c.buffer)
        ++c->array.c.buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
    c->array.c.locked = GL_FALSE;
    c->array.c.prevPtr = NULL;
    c->array.c.prevStride = 0;
#endif

    /* fog array */
    c->array.f.p = NULL;
    c->array.f.size = 0;
    c->array.f.type = GL_FLOAT;
    c->array.f.stride = 0;
    c->array.f.enabled = 0;
#ifdef CR_ARB_vertex_buffer_object
    c->array.f.buffer = ctx->bufferobject.arrayBuffer;
    if (c->array.f.buffer)
        ++c->array.f.buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
    c->array.f.locked = GL_FALSE;
    c->array.f.prevPtr = NULL;
    c->array.f.prevStride = 0;
#endif

    /* secondary color array */
    c->array.s.p = NULL;
    c->array.s.size = 3;
    c->array.s.type = GL_FLOAT;
    c->array.s.stride = 0;
    c->array.s.enabled = 0;
#ifdef CR_ARB_vertex_buffer_object
    c->array.s.buffer = ctx->bufferobject.arrayBuffer;
    if (c->array.s.buffer)
        ++c->array.s.buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
    c->array.s.locked = GL_FALSE;
    c->array.s.prevPtr = NULL;
    c->array.s.prevStride = 0;
#endif

    /* edge flag array */
    c->array.e.p = NULL;
    c->array.e.size = 0;
    c->array.e.type = GL_FLOAT;
    c->array.e.stride = 0;
    c->array.e.enabled = 0;
#ifdef CR_ARB_vertex_buffer_object
    c->array.e.buffer = ctx->bufferobject.arrayBuffer;
    if (c->array.e.buffer)
        ++c->array.e.buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
    c->array.e.locked = GL_FALSE;
    c->array.e.prevPtr = NULL;
    c->array.e.prevStride = 0;
#endif

    /* color index array */
    c->array.i.p = NULL;
    c->array.i.size = 0;
    c->array.i.type = GL_FLOAT;
    c->array.i.stride = 0;
    c->array.i.enabled = 0;
#ifdef CR_ARB_vertex_buffer_object
    c->array.i.buffer = ctx->bufferobject.arrayBuffer;
    if (c->array.i.buffer)
        ++c->array.i.buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
    c->array.i.locked = GL_FALSE;
    c->array.i.prevPtr = NULL;
    c->array.i.prevStride = 0;
#endif

    /* normal array */
    c->array.n.p = NULL;
    c->array.n.size = 4;
    c->array.n.type = GL_FLOAT;
    c->array.n.stride = 0;
    c->array.n.enabled = 0;
#ifdef CR_ARB_vertex_buffer_object
    c->array.n.buffer = ctx->bufferobject.arrayBuffer;
    if (c->array.n.buffer)
        ++c->array.n.buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
    c->array.n.locked = GL_FALSE;
    c->array.n.prevPtr = NULL;
    c->array.n.prevStride = 0;
#endif

    /* texcoord arrays */
    for (i = 0 ; i < CR_MAX_TEXTURE_UNITS ; i++)
    {
        c->array.t[i].p = NULL;
        c->array.t[i].size = 4;
        c->array.t[i].type = GL_FLOAT;
        c->array.t[i].stride = 0;
        c->array.t[i].enabled = 0;
#ifdef CR_ARB_vertex_buffer_object
        c->array.t[i].buffer = ctx->bufferobject.arrayBuffer;
        if (c->array.t[i].buffer)
            ++c->array.t[i].buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
        c->array.t[i].locked = GL_FALSE;
        c->array.t[i].prevPtr = NULL;
        c->array.t[i].prevStride = 0;
#endif
    }

    /* generic vertex attributes */
#ifdef CR_NV_vertex_program
    for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++) {
        c->array.a[i].enabled = GL_FALSE;
        c->array.a[i].type = GL_FLOAT;
        c->array.a[i].size = 4;
        c->array.a[i].stride = 0;
#ifdef CR_ARB_vertex_buffer_object
        c->array.a[i].buffer = ctx->bufferobject.arrayBuffer;
        if (c->array.a[i].buffer)
            ++c->array.a[i].buffer->refCount;
#endif
#ifdef CR_EXT_compiled_vertex_array
        c->array.a[i].locked = GL_FALSE;
        c->array.a[i].prevPtr = NULL;
        c->array.a[i].prevStride = 0;
#endif
    }
#endif
}


/*
 * PixelStore functions are here, not in state_pixel.c because this
 * is client-side state, like vertex arrays.
 */

void STATE_APIENTRY crStatePixelStoref (GLenum pname, GLfloat param)
{

    /* The GL SPEC says I can do this on page 76. */
    switch( pname )
    {
        case GL_PACK_SWAP_BYTES:
        case GL_PACK_LSB_FIRST:
        case GL_UNPACK_SWAP_BYTES:
        case GL_UNPACK_LSB_FIRST:
            crStatePixelStorei( pname, param == 0.0f ? 0: 1 );
            break;
        default:
            crStatePixelStorei( pname, (GLint) param );
            break;
    }
}

void STATE_APIENTRY crStatePixelStorei (GLenum pname, GLint param)
{
    CRContext *g    = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "PixelStore{if} called in Begin/End");
        return;
    }

    FLUSH();

    switch(pname) {
        case GL_PACK_SWAP_BYTES:
            c->pack.swapBytes = (GLboolean) param;
            DIRTY(cb->pack, g->neg_bitid);
            break;
        case GL_PACK_LSB_FIRST:
            c->pack.psLSBFirst = (GLboolean) param;
            DIRTY(cb->pack, g->neg_bitid);
            break;
        case GL_PACK_ROW_LENGTH:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Row Length: %f", param);
                return;
            }
            c->pack.rowLength = param;
            DIRTY(cb->pack, g->neg_bitid);
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_PACK_IMAGE_HEIGHT:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Image Height: %f", param);
                return;
            }
            c->pack.imageHeight = param;
            DIRTY(cb->pack, g->neg_bitid);
            break;
#endif
        case GL_PACK_SKIP_IMAGES:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Skip Images: %f", param);
                return;
            }
            c->pack.skipImages = param;
            DIRTY(cb->pack, g->neg_bitid);
            break;
        case GL_PACK_SKIP_PIXELS:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Skip Pixels: %f", param);
                return;
            }
            c->pack.skipPixels = param;
            DIRTY(cb->pack, g->neg_bitid);
            break;
        case GL_PACK_SKIP_ROWS:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Row Skip: %f", param);
                return;
            }
            c->pack.skipRows = param;
            DIRTY(cb->pack, g->neg_bitid);
            break;
        case GL_PACK_ALIGNMENT:
            if (((GLint) param) != 1 && 
                    ((GLint) param) != 2 &&
                    ((GLint) param) != 4 &&
                    ((GLint) param) != 8) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Invalid Alignment: %f", param);
                return;
            }
            c->pack.alignment = param;
            DIRTY(cb->pack, g->neg_bitid);
            break;

        case GL_UNPACK_SWAP_BYTES:
            c->unpack.swapBytes = (GLboolean) param;
            DIRTY(cb->unpack, g->neg_bitid);
            break;
        case GL_UNPACK_LSB_FIRST:
            c->unpack.psLSBFirst = (GLboolean) param;
            DIRTY(cb->unpack, g->neg_bitid);
            break;
        case GL_UNPACK_ROW_LENGTH:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Row Length: %f", param);
                return;
            }
            c->unpack.rowLength = param;
            DIRTY(cb->unpack, g->neg_bitid);
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_UNPACK_IMAGE_HEIGHT:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Image Height: %f", param);
                return;
            }
            c->unpack.imageHeight = param;
            DIRTY(cb->unpack, g->neg_bitid);
            break;
#endif
        case GL_UNPACK_SKIP_IMAGES:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Skip Images: %f", param);
                return;
            }
            c->unpack.skipImages = param;
            DIRTY(cb->unpack, g->neg_bitid);
            break;
        case GL_UNPACK_SKIP_PIXELS:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Skip Pixels: %f", param);
                return;
            }
            c->unpack.skipPixels = param;
            DIRTY(cb->unpack, g->neg_bitid);
            break;
        case GL_UNPACK_SKIP_ROWS:
            if (param < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Negative Row Skip: %f", param);
                return;
            }
            c->unpack.skipRows = param;
            DIRTY(cb->unpack, g->neg_bitid);
            break;
        case GL_UNPACK_ALIGNMENT:
            if (((GLint) param) != 1 && 
                    ((GLint) param) != 2 &&
                    ((GLint) param) != 4 &&
                    ((GLint) param) != 8) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Invalid Alignment: %f", param);
                return;
            }
            c->unpack.alignment = param;
            DIRTY(cb->unpack, g->neg_bitid);
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Unknown glPixelStore Pname: %d", pname);
            return;
    }
    DIRTY(cb->dirty, g->neg_bitid);
}


static void setClientState(CRClientState *c, CRClientBits *cb, 
        CRbitvalue *neg_bitid, GLenum array, GLboolean state) 
{
    CRContext *g = GetCurrentContext();

    switch (array) 
    {
#ifdef CR_NV_vertex_program
        case GL_VERTEX_ATTRIB_ARRAY0_NV:
        case GL_VERTEX_ATTRIB_ARRAY1_NV:
        case GL_VERTEX_ATTRIB_ARRAY2_NV:
        case GL_VERTEX_ATTRIB_ARRAY3_NV:
        case GL_VERTEX_ATTRIB_ARRAY4_NV:
        case GL_VERTEX_ATTRIB_ARRAY5_NV:
        case GL_VERTEX_ATTRIB_ARRAY6_NV:
        case GL_VERTEX_ATTRIB_ARRAY7_NV:
        case GL_VERTEX_ATTRIB_ARRAY8_NV:
        case GL_VERTEX_ATTRIB_ARRAY9_NV:
        case GL_VERTEX_ATTRIB_ARRAY10_NV:
        case GL_VERTEX_ATTRIB_ARRAY11_NV:
        case GL_VERTEX_ATTRIB_ARRAY12_NV:
        case GL_VERTEX_ATTRIB_ARRAY13_NV:
        case GL_VERTEX_ATTRIB_ARRAY14_NV:
        case GL_VERTEX_ATTRIB_ARRAY15_NV:
            {
                const GLuint i = array - GL_VERTEX_ATTRIB_ARRAY0_NV;
                c->array.a[i].enabled = state;
            }
            break;
#endif
        case GL_VERTEX_ARRAY:
            c->array.v.enabled = state;
            break;
        case GL_COLOR_ARRAY:
            c->array.c.enabled = state;
            break;
        case GL_NORMAL_ARRAY:
            c->array.n.enabled = state;
            break;
        case GL_INDEX_ARRAY:
            c->array.i.enabled = state;
            break;
        case GL_TEXTURE_COORD_ARRAY:
            c->array.t[c->curClientTextureUnit].enabled = state;
            break;
        case GL_EDGE_FLAG_ARRAY:
            c->array.e.enabled = state;
            break;  
#ifdef CR_EXT_fog_coord
        case GL_FOG_COORDINATE_ARRAY_EXT:
            c->array.f.enabled = state;
            break;
#endif
#ifdef CR_EXT_secondary_color
        case GL_SECONDARY_COLOR_ARRAY_EXT:
            if( g->extensions.EXT_secondary_color ){
                c->array.s.enabled = state;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "Invalid Enum passed to Enable/Disable Client State: SECONDARY_COLOR_ARRAY_EXT - EXT_secondary_color is not enabled." );
                return;
            }
            break;
#endif
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "Invalid Enum passed to Enable/Disable Client State: 0x%x", array );
            return;
    }
    DIRTY(cb->dirty, neg_bitid);
    DIRTY(cb->enableClientState, neg_bitid);
}

void STATE_APIENTRY crStateEnableClientState (GLenum array) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    setClientState(c, cb, g->neg_bitid, array, GL_TRUE);
}

void STATE_APIENTRY crStateDisableClientState (GLenum array) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    setClientState(c, cb, g->neg_bitid, array, GL_FALSE);
}

static void
crStateClientSetPointer(CRClientPointer *cp, GLint size, 
                                                GLenum type, GLboolean normalized,
                                                GLsizei stride, const GLvoid *pointer) 
{
    CRContext *g = GetCurrentContext();

#ifdef CR_EXT_compiled_vertex_array
    crStateUnlockClientPointer(cp);
    cp->prevPtr = cp->p;
    cp->prevStride = cp->stride;
#endif

    cp->p = (unsigned char *) pointer;
    cp->size = size;
    cp->type = type;
    cp->normalized = normalized;

    /* Calculate the bytes per index for address calculation */
    cp->bytesPerIndex = size;
    switch (type) 
    {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            break;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
            cp->bytesPerIndex *= sizeof(GLshort);
            break;
        case GL_INT:
        case GL_UNSIGNED_INT:
            cp->bytesPerIndex *= sizeof(GLint);
            break;
        case GL_FLOAT:
            cp->bytesPerIndex *= sizeof(GLfloat);
            break;
        case GL_DOUBLE:
            cp->bytesPerIndex *= sizeof(GLdouble);
            break;
        default:
            crStateError( __LINE__, __FILE__, GL_INVALID_VALUE,
                                        "Unknown type of vertex array: %d", type );
            return;
    }

    /* 
     **  Note: If stride==0 then we set the 
     **  stride equal address offset
     **  therefore stride can never equal
     **  zero.
     */
    if (stride)
        cp->stride = stride;
    else
        cp->stride = cp->bytesPerIndex;

#ifdef CR_ARB_vertex_buffer_object
    if (cp->buffer)
    {
        --cp->buffer->refCount;
        CRASSERT(cp->buffer->refCount && cp->buffer->refCount < UINT32_MAX/2);
    }
    cp->buffer = g->bufferobject.arrayBuffer;
    if (cp->buffer)
        ++cp->buffer->refCount;
#endif
}

void STATE_APIENTRY crStateVertexPointer(GLint size, GLenum type, 
        GLsizei stride, const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if (size != 2 && size != 3 && size != 4)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glVertexPointer: invalid size: %d", size);
        return;
    }
    if (type != GL_SHORT && type != GL_INT &&
            type != GL_FLOAT && type != GL_DOUBLE)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glVertexPointer: invalid type: 0x%x", type);
        return;
    }
    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glVertexPointer: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.v), size, type, GL_FALSE, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->v, g->neg_bitid);
}

void STATE_APIENTRY crStateColorPointer(GLint size, GLenum type, 
        GLsizei stride, const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if (size != 3 && size != 4)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glColorPointer: invalid size: %d", size);
        return;
    }
    if (type != GL_BYTE && type != GL_UNSIGNED_BYTE &&
            type != GL_SHORT && type != GL_UNSIGNED_SHORT &&
            type != GL_INT && type != GL_UNSIGNED_INT &&
            type != GL_FLOAT && type != GL_DOUBLE)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glColorPointer: invalid type: 0x%x", type);
        return;
    }
    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glColorPointer: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.c), size, type, GL_TRUE, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->c, g->neg_bitid);
}

void STATE_APIENTRY crStateSecondaryColorPointerEXT(GLint size,
        GLenum type, GLsizei stride, const GLvoid *p)
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if ( !g->extensions.EXT_secondary_color )
    {
        crError( "glSecondaryColorPointerEXT called but EXT_secondary_color is disabled." );
        return;
    }

    /*Note: According to opengl spec, only size==3 should be accepted here.
     *But it turns out that most drivers accept size==4 here as well, and 4th value
     *could even be accessed in shaders code.
     *Having a strict check here, leads to difference between guest and host gpu states, which
     *in turn could lead to crashes when using server side VBOs.
     *@todo: add error reporting to state's VBO related functions and abort dispatching to
     *real gpu on any failure to prevent other possible issues.
     */

    if ((size != 3) && (size != 4))
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glSecondaryColorPointerEXT: invalid size: %d", size);
        return;
    }
    if (type != GL_BYTE && type != GL_UNSIGNED_BYTE &&
            type != GL_SHORT && type != GL_UNSIGNED_SHORT &&
            type != GL_INT && type != GL_UNSIGNED_INT &&
            type != GL_FLOAT && type != GL_DOUBLE)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glSecondaryColorPointerEXT: invalid type: 0x%x", type);
        return;
    }
    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glSecondaryColorPointerEXT: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.s), size, type, GL_TRUE, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->s, g->neg_bitid);
}

void STATE_APIENTRY crStateIndexPointer(GLenum type, GLsizei stride, 
        const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if (type != GL_SHORT && type != GL_INT && type != GL_UNSIGNED_BYTE &&
            type != GL_FLOAT && type != GL_DOUBLE)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glIndexPointer: invalid type: 0x%x", type);
        return;
    }
    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glIndexPointer: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.i), 1, type, GL_TRUE, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->i, g->neg_bitid);
}

void STATE_APIENTRY crStateNormalPointer(GLenum type, GLsizei stride, 
        const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if (type != GL_BYTE && type != GL_SHORT &&
            type != GL_INT && type != GL_FLOAT &&
            type != GL_DOUBLE)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glNormalPointer: invalid type: 0x%x", type);
        return;
    }
    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glNormalPointer: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.n), 3, type, GL_TRUE, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->n, g->neg_bitid);
}

void STATE_APIENTRY crStateTexCoordPointer(GLint size, GLenum type, 
        GLsizei stride, const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if (size != 1 && size != 2 && size != 3 && size != 4)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glTexCoordPointer: invalid size: %d", size);
        return;
    }
    if (type != GL_SHORT && type != GL_INT &&
            type != GL_FLOAT && type != GL_DOUBLE)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexCoordPointer: invalid type: 0x%x", type);
        return;
    }
    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glTexCoordPointer: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.t[c->curClientTextureUnit]), size, type, GL_FALSE, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->t[c->curClientTextureUnit], g->neg_bitid);
}

void STATE_APIENTRY crStateEdgeFlagPointer(GLsizei stride, const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glTexCoordPointer: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.e), 1, GL_UNSIGNED_BYTE, GL_FALSE, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->e, g->neg_bitid);
}

void STATE_APIENTRY crStateFogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if (type != GL_BYTE && type != GL_UNSIGNED_BYTE &&
            type != GL_SHORT && type != GL_UNSIGNED_SHORT &&
            type != GL_INT && type != GL_UNSIGNED_INT &&
            type != GL_FLOAT && type != GL_DOUBLE)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glFogCoordPointerEXT: invalid type: 0x%x", type);
        return;
    }
    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glFogCoordPointerEXT: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.f), 1, type, GL_FALSE, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->f, g->neg_bitid);
}


void STATE_APIENTRY crStateVertexAttribPointerNV(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *p) 
{
    GLboolean normalized = GL_FALSE;
    /* Extra error checking for NV arrays */
    if (type != GL_UNSIGNED_BYTE && type != GL_SHORT &&
            type != GL_FLOAT && type != GL_DOUBLE) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glVertexAttribPointerNV: invalid type: 0x%x", type);
        return;
    }
    crStateVertexAttribPointerARB(index, size, type, normalized, stride, p);
}


void STATE_APIENTRY crStateVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    FLUSH();

    if (index >= CR_MAX_VERTEX_ATTRIBS)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glVertexAttribPointerARB: invalid index: %d", (int) index);
        return;
    }
    if (size != 1 && size != 2 && size != 3 && size != 4)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glVertexAttribPointerARB: invalid size: %d", size);
        return;
    }
    if (type != GL_BYTE && type != GL_UNSIGNED_BYTE &&
            type != GL_SHORT && type != GL_UNSIGNED_SHORT &&
            type != GL_INT && type != GL_UNSIGNED_INT &&
            type != GL_FLOAT && type != GL_DOUBLE)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glVertexAttribPointerARB: invalid type: 0x%x", type);
        return;
    }
    if (stride < 0) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glVertexAttribPointerARB: stride was negative: %d", stride);
        return;
    }

    crStateClientSetPointer(&(c->array.a[index]), size, type, normalized, stride, p);
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);
    DIRTY(cb->a[index], g->neg_bitid);
}


void STATE_APIENTRY crStateGetVertexAttribPointervNV(GLuint index, GLenum pname, GLvoid **pointer)
{
    CRContext *g = GetCurrentContext();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetVertexAttribPointervNV called in Begin/End");
        return;
    }

    if (index >= CR_MAX_VERTEX_ATTRIBS) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glGetVertexAttribPointervNV(index)");
        return;
    }

    if (pname != GL_ATTRIB_ARRAY_POINTER_NV) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetVertexAttribPointervNV(pname)");
        return;
    }

    *pointer = g->client.array.a[index].p;
}


void STATE_APIENTRY crStateGetVertexAttribPointervARB(GLuint index, GLenum pname, GLvoid **pointer)
{
    crStateGetVertexAttribPointervNV(index, pname, pointer);
}



/* 
** Currently I treat Interleaved Arrays as if the 
** user uses them as separate arrays.
** Certainly not the most efficient method but it 
** lets me use the same glDrawArrays method.
*/
void STATE_APIENTRY crStateInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *p) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);
    CRClientPointer *cp;
    unsigned char *base = (unsigned char *) p;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glInterleavedArrays called in begin/end");
        return;
    }

    FLUSH();

    if (stride < 0)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glInterleavedArrays: stride < 0: %d", stride);
        return;
    }

    switch (format) 
    {
        case GL_T4F_C4F_N3F_V4F:
        case GL_T2F_C4F_N3F_V3F:
        case GL_C4F_N3F_V3F:
        case GL_T4F_V4F:
        case GL_T2F_C3F_V3F:
        case GL_T2F_N3F_V3F:
        case GL_C3F_V3F:
        case GL_N3F_V3F:
        case GL_T2F_C4UB_V3F:
        case GL_T2F_V3F:
        case GL_C4UB_V3F:
        case GL_V3F:
        case GL_C4UB_V2F:
        case GL_V2F:
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glInterleavedArrays: Unrecognized format: %d", format);
            return;
    }

    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->clientPointer, g->neg_bitid);

/* p, size, type, stride, enabled, bytesPerIndex */
/*
**  VertexPointer 
*/
    
    cp = &(c->array.v);
    cp->type = GL_FLOAT;
    cp->enabled = GL_TRUE;

#ifdef CR_EXT_compiled_vertex_array
    crStateUnlockClientPointer(cp);
#endif

    switch (format) 
    {
        case GL_T4F_C4F_N3F_V4F:
            cp->p = base+4*sizeof(GLfloat)+4*sizeof(GLfloat)+3*sizeof(GLfloat);
            cp->size = 4;
            break;
        case GL_T2F_C4F_N3F_V3F:
            cp->p = base+2*sizeof(GLfloat)+4*sizeof(GLfloat)+3*sizeof(GLfloat);
            cp->size = 3;
            break;
        case GL_C4F_N3F_V3F:
            cp->p = base+4*sizeof(GLfloat)+3*sizeof(GLfloat);
            cp->size = 3;
            break;
        case GL_T4F_V4F:
            cp->p = base+4*sizeof(GLfloat);
            cp->size = 4;
            break;
        case GL_T2F_C3F_V3F:
            cp->p = base+2*sizeof(GLfloat)+3*sizeof(GLfloat);
            cp->size = 3;
            break;
        case GL_T2F_N3F_V3F:
            cp->p = base+2*sizeof(GLfloat)+3*sizeof(GLfloat);
            cp->size = 3;
            break;
        case GL_C3F_V3F:
            cp->p = base+3*sizeof(GLfloat);
            cp->size = 3;
            break;
        case GL_N3F_V3F:
            cp->p = base+3*sizeof(GLfloat);
            cp->size = 3;
            break;
        case GL_T2F_C4UB_V3F:
            cp->p = base+2*sizeof(GLfloat)+4*sizeof(GLubyte);
            cp->size = 3;
            break;
        case GL_T2F_V3F:
            cp->p = base+2*sizeof(GLfloat);
            cp->size = 3;
            break;
        case GL_C4UB_V3F:
            cp->p = base+4*sizeof(GLubyte);
            cp->size = 3;
            break;
        case GL_V3F:
            cp->p = base;
            cp->size = 3;
            break;
        case GL_C4UB_V2F:
            cp->p = base+4*sizeof(GLubyte);
            cp->size = 2;
            break;
        case GL_V2F:
            cp->p = base;
            cp->size = 2;
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glInterleavedArrays: Unrecognized format: %d", format);
            return;
    }

    cp->bytesPerIndex = cp->size * sizeof (GLfloat);

    if (stride==0)
        stride = cp->bytesPerIndex + (cp->p - base);
    cp->stride = stride;

/*
**  NormalPointer
*/

    cp = &(c->array.n);
    cp->enabled = GL_TRUE;
    cp->stride = stride;
#ifdef CR_EXT_compiled_vertex_array
    crStateUnlockClientPointer(cp);
#endif

    switch (format) 
    {
        case GL_T4F_C4F_N3F_V4F:
            cp->p = base+4*sizeof(GLfloat)+4*sizeof(GLfloat);
            break;
        case GL_T2F_C4F_N3F_V3F:
            cp->p = base+2*sizeof(GLfloat)+4*sizeof(GLfloat);
            break;
        case GL_C4F_N3F_V3F:
            cp->p = base+4*sizeof(GLfloat);
            break;
        case GL_T2F_N3F_V3F:
            cp->p = base+2*sizeof(GLfloat);
            break;
        case GL_N3F_V3F:
            cp->p = base;
            break;
        case GL_T4F_V4F:
        case GL_T2F_C3F_V3F:
        case GL_C3F_V3F:
        case GL_T2F_C4UB_V3F:
        case GL_T2F_V3F:
        case GL_C4UB_V3F:
        case GL_V3F:
        case GL_C4UB_V2F:
        case GL_V2F:
            cp->enabled = GL_FALSE;
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glInterleavedArrays: Unrecognized format: %d", format);
            return;
    }

    if (cp->enabled) 
    {
        cp->type = GL_FLOAT;
        cp->size = 3;
        cp->bytesPerIndex = cp->size * sizeof (GLfloat);
    }
    
/*
**  ColorPointer
*/

    cp = &(c->array.c);
    cp->enabled = GL_TRUE;
    cp->stride = stride;
#ifdef CR_EXT_compiled_vertex_array
    crStateUnlockClientPointer(cp);
#endif

    switch (format) 
    {
        case GL_T4F_C4F_N3F_V4F:
            cp->size = 4;
            cp->type = GL_FLOAT;
            cp->bytesPerIndex = cp->size * sizeof(GLfloat);
            cp->p = base+4*sizeof(GLfloat);
            break;
        case GL_T2F_C4F_N3F_V3F:
            cp->size = 4;
            cp->type = GL_FLOAT;
            cp->bytesPerIndex = cp->size * sizeof(GLfloat);
            cp->p = base+2*sizeof(GLfloat);
            break;
        case GL_C4F_N3F_V3F:
            cp->size = 4;
            cp->type = GL_FLOAT;
            cp->bytesPerIndex = cp->size * sizeof(GLfloat);
            cp->p = base;
            break;
        case GL_T2F_C3F_V3F:
            cp->size = 3;
            cp->type = GL_FLOAT;
            cp->bytesPerIndex = cp->size * sizeof(GLfloat);
            cp->p = base+2*sizeof(GLfloat);
            break;
        case GL_C3F_V3F:
            cp->size = 3;
            cp->type = GL_FLOAT;
            cp->bytesPerIndex = cp->size * sizeof(GLfloat);
            cp->p = base;
            break;
        case GL_T2F_C4UB_V3F:
            cp->size = 4;
            cp->type = GL_UNSIGNED_BYTE;
            cp->bytesPerIndex = cp->size * sizeof(GLubyte);
            cp->p = base+2*sizeof(GLfloat);
            break;
        case GL_C4UB_V3F:
            cp->size = 4;
            cp->type = GL_UNSIGNED_BYTE;
            cp->bytesPerIndex = cp->size * sizeof(GLubyte);
            cp->p = base;
            break;
        case GL_C4UB_V2F:
            cp->size = 4;
            cp->type = GL_UNSIGNED_BYTE;
            cp->bytesPerIndex = cp->size * sizeof(GLubyte);
            cp->p = base;
            break;
        case GL_T2F_N3F_V3F:
        case GL_N3F_V3F:
        case GL_T4F_V4F:
        case GL_T2F_V3F:
        case GL_V3F:
        case GL_V2F:
            cp->enabled = GL_FALSE;
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glInterleavedArrays: Unrecognized format: %d", format);
            return;
    }

/*
**  TexturePointer
*/

    cp = &(c->array.t[c->curClientTextureUnit]);
    cp->enabled = GL_TRUE;
    cp->stride = stride;
#ifdef CR_EXT_compiled_vertex_array
    crStateUnlockClientPointer(cp);
#endif

    switch (format) 
    {
        case GL_T4F_C4F_N3F_V4F:
            cp->size = 4;
            cp->p = base;
            break;
        case GL_T2F_C4F_N3F_V3F:
            cp->size = 3;
            cp->p = base;
            break;
        case GL_T2F_C3F_V3F:
        case GL_T2F_N3F_V3F:
            cp->size = 3;
            cp->p = base;
            break;
        case GL_T2F_C4UB_V3F:
            cp->size = 3;
            cp->p = base;
            break;
        case GL_T4F_V4F:
            cp->size = 4;
            cp->p = base;
            break;
        case GL_T2F_V3F:
            cp->size = 3;
            cp->p = base;
            break;
        case GL_C4UB_V3F:
        case GL_C4UB_V2F:
        case GL_C3F_V3F:
        case GL_C4F_N3F_V3F:
        case GL_N3F_V3F:
        case GL_V3F:
        case GL_V2F:
            cp->enabled = GL_FALSE;
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glInterleavedArrays: Unrecognized format: %d", format);
            return;
    }

    if (cp->enabled) 
    {
        cp->type = GL_FLOAT;
        cp->bytesPerIndex = cp->size * sizeof (GLfloat);
    }   
}

void STATE_APIENTRY crStateGetPointerv(GLenum pname, GLvoid * * params) 
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                "GetPointerv called in begin/end");
        return;
    }

    switch (pname) 
    {
        case GL_VERTEX_ARRAY_POINTER:
            *params = (GLvoid *) c->array.v.p;
            break;
        case GL_COLOR_ARRAY_POINTER:
            *params = (GLvoid *) c->array.c.p;
            break;
        case GL_NORMAL_ARRAY_POINTER:
            *params = (GLvoid *) c->array.n.p;
            break;
        case GL_INDEX_ARRAY_POINTER:
            *params = (GLvoid *) c->array.i.p;
            break;
        case GL_TEXTURE_COORD_ARRAY_POINTER:
            *params = (GLvoid *) c->array.t[c->curClientTextureUnit].p;
            break;
        case GL_EDGE_FLAG_ARRAY_POINTER:
            *params = (GLvoid *) c->array.e.p;
            break;
#ifdef CR_EXT_fog_coord
        case GL_FOG_COORDINATE_ARRAY_POINTER_EXT:
            *params = (GLvoid *) c->array.f.p;
            break;
#endif
#ifdef CR_EXT_secondary_color
        case GL_SECONDARY_COLOR_ARRAY_POINTER_EXT:
            if( g->extensions.EXT_secondary_color ){
                *params = (GLvoid *) c->array.s.p;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "Invalid Enum passed to glGetPointerv: SECONDARY_COLOR_ARRAY_EXT - EXT_secondary_color is not enabled." );
                return;
            }
            break;
#endif
        case GL_FEEDBACK_BUFFER_POINTER:
        case GL_SELECTION_BUFFER_POINTER:
            /* do nothing - API switching should pick this up */
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "glGetPointerv: invalid pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY crStatePushClientAttrib( GLbitfield mask )
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glPushClientAttrib called in Begin/End");
        return;
    }

    if (c->attribStackDepth == CR_MAX_CLIENT_ATTRIB_STACK_DEPTH - 1) {
        crStateError(__LINE__, __FILE__, GL_STACK_OVERFLOW,
                                 "glPushClientAttrib called with a full stack!" );
        return;
    }

    FLUSH();

    c->pushMaskStack[c->attribStackDepth++] = mask;

    if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
        c->pixelPackStoreStack[c->pixelStoreStackDepth] = c->pack;
        c->pixelUnpackStoreStack[c->pixelStoreStackDepth] = c->unpack;
        c->pixelStoreStackDepth++;
    }
    if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
        c->vertexArrayStack[c->vertexArrayStackDepth] = c->array;
        c->vertexArrayStackDepth++;
    }

    /* dirty? - no, because we haven't really changed any state */
}


void STATE_APIENTRY crStatePopClientAttrib( void )
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);
    CRbitvalue mask;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glPopClientAttrib called in Begin/End");
        return;
    }

    if (c->attribStackDepth == 0) {
        crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW,
                                 "glPopClientAttrib called with an empty stack!" );
        return;
    }

    FLUSH();

    mask = c->pushMaskStack[--c->attribStackDepth];

    if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
        if (c->pixelStoreStackDepth == 0) {
            crError("bug in glPopClientAttrib (pixel store) ");
            return;
        }
        c->pixelStoreStackDepth--;
        c->pack = c->pixelPackStoreStack[c->pixelStoreStackDepth];
        c->unpack = c->pixelUnpackStoreStack[c->pixelStoreStackDepth];
        DIRTY(cb->pack, g->neg_bitid);
    }

    if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
        if (c->vertexArrayStackDepth == 0) {
            crError("bug in glPopClientAttrib (vertex array) ");
            return;
        }
        c->vertexArrayStackDepth--;
        c->array = c->vertexArrayStack[c->vertexArrayStackDepth];
        DIRTY(cb->clientPointer, g->neg_bitid);
    }

    DIRTY(cb->dirty, g->neg_bitid);
}

static void crStateLockClientPointer(CRClientPointer* cp)
{
    crStateUnlockClientPointer(cp);
    if (cp->enabled)
    {
        cp->locked = GL_TRUE;
    }
}

static GLboolean crStateCanLockClientPointer(CRClientPointer* cp)
{
    return !(cp->enabled && cp->buffer && cp->buffer->id);
}

void STATE_APIENTRY crStateLockArraysEXT(GLint first, GLint count)
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    int i;

    for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
    {
        if (!crStateCanLockClientPointer(crStateGetClientPointerByIndex(i, &c->array)))
        {
            break;
        }
    }
    if (i<CRSTATECLIENT_MAX_VERTEXARRAYS)
    {
        crDebug("crStateLockArraysEXT ignored because array %i have a bound VBO", i);
        return;
    }

    c->array.locked = GL_TRUE;
    c->array.lockFirst = first;
    c->array.lockCount = count;
#ifdef IN_GUEST
    c->array.synced = GL_FALSE;
#endif

    for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
    {
        crStateLockClientPointer(crStateGetClientPointerByIndex(i, &c->array));
    }
}

void STATE_APIENTRY crStateUnlockArraysEXT()
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    int i;

    if (!c->array.locked)
    {
        crDebug("crStateUnlockArraysEXT ignored because arrays aren't locked");
        return;
    }

    c->array.locked = GL_FALSE;
#ifdef IN_GUEST
    c->array.synced = GL_FALSE;
#endif

    for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
    {
        crStateUnlockClientPointer(crStateGetClientPointerByIndex(i, &c->array));
    }
}

void STATE_APIENTRY crStateVertexArrayRangeNV(GLsizei length, const GLvoid *pointer)
{
  /* XXX todo */
    crWarning("crStateVertexArrayRangeNV not implemented");
    (void)length; (void)pointer;
}


void STATE_APIENTRY crStateFlushVertexArrayRangeNV(void)
{
  /* XXX todo */
    crWarning("crStateFlushVertexArrayRangeNV not implemented");
}

/*Returns if the given clientpointer could be used on server side directly*/
#define CRSTATE_IS_SERVER_CP(cp) (!(cp).enabled || !(cp).p || ((cp).buffer && (cp).buffer->id) || ((cp).locked))

#if defined(DEBUG) && 0
static void crStateDumpClientPointer(CRClientPointer *cp, const char *name, int i)
{
  if (i<0 && cp->enabled)
  {
    crDebug("CP(%s): enabled:%d ptr:%p buffer:%p buffer.name:%i locked: %i %s",
            name, cp->enabled, cp->p, cp->buffer, cp->buffer ? cp->buffer->id : ~(GLuint)0, (int)cp->locked,
            CRSTATE_IS_SERVER_CP(*cp) ? "":"!FAIL!");
  }
  else if (0==i || cp->enabled)
  {
    crDebug("CP(%s%i): enabled:%d ptr:%p buffer:%p buffer.name:%i locked: %i %s",
            name, i, cp->enabled, cp->p, cp->buffer, cp->buffer ? cp->buffer->id : ~(GLuint)0, (int)cp->locked,
            CRSTATE_IS_SERVER_CP(*cp) ? "":"!FAIL!");
  }
}
#endif

#ifdef DEBUG_misha
/* debugging */
/*# define CR_NO_SERVER_ARRAYS*/
#endif

/*
 * Determine if the enabled arrays all live on the server
 * (via GL_ARB_vertex_buffer_object).
 */
GLboolean crStateUseServerArrays(void)
{
#if defined(CR_ARB_vertex_buffer_object) && !defined(CR_NO_SERVER_ARRAYS)
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    int i;
    GLboolean res;

    res =    CRSTATE_IS_SERVER_CP(c->array.v)
          && CRSTATE_IS_SERVER_CP(c->array.n)
          && CRSTATE_IS_SERVER_CP(c->array.c)
          && CRSTATE_IS_SERVER_CP(c->array.i)
          && CRSTATE_IS_SERVER_CP(c->array.e)
          && CRSTATE_IS_SERVER_CP(c->array.s)
          && CRSTATE_IS_SERVER_CP(c->array.f);

    if (res)
    {
        for (i = 0; (unsigned int)i < g->limits.maxTextureUnits; i++)
            if (!CRSTATE_IS_SERVER_CP(c->array.t[i]))
            {
                res = GL_FALSE;
                break;    
            }
    }

    if (res)
    {
        for (i = 0; (unsigned int)i < g->limits.maxVertexProgramAttribs; i++)
            if (!CRSTATE_IS_SERVER_CP(c->array.a[i]))
            {
                res = GL_FALSE;
                break;    
            }
    }

#if defined(DEBUG) && 0
    if (!res)
    {
        crStateDumpClientPointer(&c->array.v, "v", -1);
        crStateDumpClientPointer(&c->array.n, "n", -1);
        crStateDumpClientPointer(&c->array.c, "c", -1);
        crStateDumpClientPointer(&c->array.i, "i", -1);
        crStateDumpClientPointer(&c->array.e, "e", -1);
        crStateDumpClientPointer(&c->array.s, "s", -1);
        crStateDumpClientPointer(&c->array.f, "f", -1);
        for (i = 0; (unsigned int)i < g->limits.maxTextureUnits; i++)
            crStateDumpClientPointer(&c->array.t[i], "tex", i);
        for (i = 0; (unsigned int)i < g->limits.maxVertexProgramAttribs; i++)
            crStateDumpClientPointer(&c->array.a[i], "attrib", i);
        crDebug("crStateUseServerArrays->%d", res);
    }
#endif

    return res;
#else
    return GL_FALSE;
#endif
}

GLuint crStateNeedDummyZeroVertexArray(CRContext *g, CRCurrentStatePointers *current, GLfloat *pZva)
{
#if defined(CR_ARB_vertex_buffer_object)
    CRClientState *c = &(g->client);
    int i;
    GLuint zvMax = 0;

    if (c->array.a[0].enabled)
        return 0;

    for (i = 1; (unsigned int)i < g->limits.maxVertexProgramAttribs; i++)
    {
        if (c->array.a[i].enabled)
        {
            if (c->array.a[i].buffer && c->array.a[i].buffer->id)
            {
                GLuint cElements = c->array.a[i].buffer->size / c->array.a[i].stride;
                if (zvMax < cElements)
                    zvMax = cElements;
            }
            else
            {
                zvMax = ~0;
                break;
            }
        }
    }

    if (zvMax)
    {
        Assert(!c->array.v.enabled);

        crStateCurrentRecoverNew(g, current);

        crMemcpy(pZva, &g->current.vertexAttrib[0][0], sizeof (*pZva) * 4);
    }

    return zvMax;
#else
    return GL_FALSE;
#endif
}


/**
 * Determine if there's a server-side array element buffer.
 * Called by glDrawElements() in packing SPUs to determine if glDrawElements
 * should be evaluated (unrolled) locally or if glDrawElements should be
 * packed and sent to the server.
 */
GLboolean
crStateUseServerArrayElements(void)
{
#ifdef CR_ARB_vertex_buffer_object
    CRContext *g = GetCurrentContext();
    if (g->bufferobject.elementsBuffer &&
            g->bufferobject.elementsBuffer->id > 0)
        return GL_TRUE;
    else
        return GL_FALSE;
#else
    return GL_FALSE;
#endif
}

#define CR_BUFFER_HWID(_p) ((_p) ? (_p)->hwid : 0)

void
crStateClientDiff(CRClientBits *cb, CRbitvalue *bitID,
                                    CRContext *fromCtx, CRContext *toCtx)
{
    CRClientState *from = &(fromCtx->client);
    const CRClientState *to = &(toCtx->client);
    GLint curClientTextureUnit = from->curClientTextureUnit;
    int i;
    GLint idHwArrayBuffer = CR_BUFFER_HWID(toCtx->bufferobject.arrayBuffer);
    const GLint idHwInitialBuffer = idHwArrayBuffer;

#ifdef DEBUG_misha
    {
        GLint tstHwBuffer = -1;
        diff_api.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &tstHwBuffer);
        CRASSERT(idHwInitialBuffer == tstHwBuffer);
    }
#endif


    if (CHECKDIRTY(cb->clientPointer, bitID)) {
        /* one or more vertex pointers is dirty */
        if (CHECKDIRTY(cb->v, bitID)) {
            if (from->array.v.size != to->array.v.size ||
                    from->array.v.type != to->array.v.type ||
                    from->array.v.stride != to->array.v.stride ||
                    from->array.v.p != to->array.v.p ||
                    from->array.v.buffer != to->array.v.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.v.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.VertexPointer(to->array.v.size, to->array.v.type,
                                                             to->array.v.stride, to->array.v.p);
                from->array.v.size = to->array.v.size;
                from->array.v.type = to->array.v.type;
                from->array.v.stride = to->array.v.stride;
                from->array.v.p = to->array.v.p;
                from->array.v.buffer = to->array.v.buffer;
            }
            CLEARDIRTY2(cb->v, bitID);
        }
        /* normal */
        if (CHECKDIRTY(cb->n, bitID)) {
            if (from->array.n.type != to->array.n.type ||
                    from->array.n.stride != to->array.n.stride ||
                    from->array.n.p != to->array.n.p ||
                    from->array.n.buffer != to->array.n.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.n.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.NormalPointer(to->array.n.type,
                                                             to->array.n.stride, to->array.n.p);
                from->array.n.type = to->array.n.type;
                from->array.n.stride = to->array.n.stride;
                from->array.n.p = to->array.n.p;
                from->array.n.buffer = to->array.n.buffer;
            }
            CLEARDIRTY2(cb->n, bitID);
        }
        /* color */
        if (CHECKDIRTY(cb->c, bitID)) {
            if (from->array.c.size != to->array.c.size ||
                    from->array.c.type != to->array.c.type ||
                    from->array.c.stride != to->array.c.stride ||
                    from->array.c.p != to->array.c.p ||
                    from->array.c.buffer != to->array.c.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.c.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.ColorPointer(to->array.c.size, to->array.c.type,
                                                            to->array.c.stride, to->array.c.p);
                from->array.c.size = to->array.c.size;
                from->array.c.type = to->array.c.type;
                from->array.c.stride = to->array.c.stride;
                from->array.c.p = to->array.c.p;
                from->array.c.buffer = to->array.c.buffer;
            }
            CLEARDIRTY2(cb->c, bitID);
        }
        /* index */
        if (CHECKDIRTY(cb->i, bitID)) {
            if (from->array.i.type != to->array.i.type ||
                    from->array.i.stride != to->array.i.stride ||
                    from->array.i.p != to->array.i.p ||
                    from->array.i.buffer != to->array.i.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.i.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.IndexPointer(to->array.i.type,
                                                            to->array.i.stride, to->array.i.p);
                from->array.i.type = to->array.i.type;
                from->array.i.stride = to->array.i.stride;
                from->array.i.p = to->array.i.p;
                from->array.i.buffer = to->array.i.buffer;
            }
            CLEARDIRTY2(cb->i, bitID);
        }
        /* texcoords */
        for (i = 0; (unsigned int)i < toCtx->limits.maxTextureUnits; i++) {
            if (CHECKDIRTY(cb->t[i], bitID)) {
                if (from->array.t[i].size != to->array.t[i].size ||
                        from->array.t[i].type != to->array.t[i].type ||
                        from->array.t[i].stride != to->array.t[i].stride ||
                        from->array.t[i].p != to->array.t[i].p ||
                        from->array.t[i].buffer != to->array.t[i].buffer) {
                    GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.t[i].buffer);
                    if (idHwArrayBufferUsed != idHwArrayBuffer)
                    {
                        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                        idHwArrayBuffer = idHwArrayBufferUsed;
                    }
                    diff_api.ClientActiveTextureARB(GL_TEXTURE0_ARB + i);
                    curClientTextureUnit = i;
                    diff_api.TexCoordPointer(to->array.t[i].size, to->array.t[i].type,
                                                                to->array.t[i].stride, to->array.t[i].p);
                    from->array.t[i].size = to->array.t[i].size;
                    from->array.t[i].type = to->array.t[i].type;
                    from->array.t[i].stride = to->array.t[i].stride;
                    from->array.t[i].p = to->array.t[i].p;
                    from->array.t[i].buffer = to->array.t[i].buffer;
                }
                CLEARDIRTY2(cb->t[i], bitID);
            }
        }
        /* edge flag */
        if (CHECKDIRTY(cb->e, bitID)) {
            if (from->array.e.stride != to->array.e.stride ||
                    from->array.e.p != to->array.e.p ||
                    from->array.e.buffer != to->array.e.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.e.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.EdgeFlagPointer(to->array.e.stride, to->array.e.p);
                from->array.e.stride = to->array.e.stride;
                from->array.e.p = to->array.e.p;
                from->array.e.buffer = to->array.e.buffer;
            }
            CLEARDIRTY2(cb->e, bitID);
        }
        /* secondary color */
        if (CHECKDIRTY(cb->s, bitID)) {
            if (from->array.s.size != to->array.s.size ||
                    from->array.s.type != to->array.s.type ||
                    from->array.s.stride != to->array.s.stride ||
                    from->array.s.p != to->array.s.p ||
                    from->array.s.buffer != to->array.s.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.s.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.SecondaryColorPointerEXT(to->array.s.size, to->array.s.type,
                                                                                    to->array.s.stride, to->array.s.p);
                from->array.s.size = to->array.s.size;
                from->array.s.type = to->array.s.type;
                from->array.s.stride = to->array.s.stride;
                from->array.s.p = to->array.s.p;
                from->array.s.buffer = to->array.s.buffer;
            }
            CLEARDIRTY2(cb->s, bitID);
        }
        /* fog coord */
        if (CHECKDIRTY(cb->f, bitID)) {
            if (from->array.f.type != to->array.f.type ||
                    from->array.f.stride != to->array.f.stride ||
                    from->array.f.p != to->array.f.p ||
                    from->array.f.buffer != to->array.f.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.f.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.FogCoordPointerEXT(to->array.f.type,
                                                                        to->array.f.stride, to->array.f.p);
                from->array.f.type = to->array.f.type;
                from->array.f.stride = to->array.f.stride;
                from->array.f.p = to->array.f.p;
                from->array.f.buffer = to->array.f.buffer;
            }
            CLEARDIRTY2(cb->f, bitID);
        }
#if defined(CR_NV_vertex_program) || defined(CR_ARB_vertex_program)
        /* vertex attributes */
        for (i = 0; (unsigned int)i < toCtx->limits.maxVertexProgramAttribs; i++) {
            if (CHECKDIRTY(cb->a[i], bitID)) {
                if (from->array.a[i].size != to->array.a[i].size ||
                        from->array.a[i].type != to->array.a[i].type ||
                        from->array.a[i].stride != to->array.a[i].stride ||
                        from->array.a[i].normalized != to->array.a[i].normalized ||
                        from->array.a[i].p != to->array.a[i].p ||
                        from->array.a[i].buffer != to->array.a[i].buffer) {
                    GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.a[i].buffer);
                    if (idHwArrayBufferUsed != idHwArrayBuffer)
                    {
                        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                        idHwArrayBuffer = idHwArrayBufferUsed;
                    }
                    diff_api.VertexAttribPointerARB(i, to->array.a[i].size,
                                                                                    to->array.a[i].type,
                                                                                    to->array.a[i].normalized,
                                                                                    to->array.a[i].stride,
                                                                                    to->array.a[i].p);
                    from->array.a[i].size = to->array.a[i].size;
                    from->array.a[i].type = to->array.a[i].type;
                    from->array.a[i].stride = to->array.a[i].stride;
                    from->array.a[i].normalized = to->array.a[i].normalized;
                    from->array.a[i].p = to->array.a[i].p;
                    from->array.a[i].buffer = to->array.a[i].buffer;
                }
                CLEARDIRTY2(cb->a[i], bitID);
            }
        }
#endif
    }

    if (idHwArrayBuffer != idHwInitialBuffer)
    {
        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwInitialBuffer);
    }

    if (CHECKDIRTY(cb->enableClientState, bitID)) {
        /* update vertex array enable/disable flags */
        glAble able[2];
        able[0] = diff_api.DisableClientState;
        able[1] = diff_api.EnableClientState;
        if (from->array.v.enabled != to->array.v.enabled) {
            able[to->array.v.enabled](GL_VERTEX_ARRAY);
            from->array.v.enabled = to->array.v.enabled;
        }
        if (from->array.n.enabled != to->array.n.enabled) {
            able[to->array.n.enabled](GL_NORMAL_ARRAY);
            from->array.n.enabled = to->array.n.enabled;
        }
        if (from->array.c.enabled != to->array.c.enabled) {
            able[to->array.c.enabled](GL_COLOR_ARRAY);
            from->array.c.enabled = to->array.c.enabled;
        }
        if (from->array.i.enabled != to->array.i.enabled) {
            able[to->array.i.enabled](GL_INDEX_ARRAY);
            from->array.i.enabled = to->array.i.enabled;
        }
        for (i = 0; (unsigned int)i < toCtx->limits.maxTextureUnits; i++) {
            if (from->array.t[i].enabled != to->array.t[i].enabled) {
                diff_api.ClientActiveTextureARB(GL_TEXTURE0_ARB + i);
                curClientTextureUnit = i;
                able[to->array.t[i].enabled](GL_TEXTURE_COORD_ARRAY);
                from->array.t[i].enabled = to->array.t[i].enabled;
            }
        }
        if (from->array.e.enabled != to->array.e.enabled) {
            able[to->array.e.enabled](GL_EDGE_FLAG_ARRAY);
            from->array.e.enabled = to->array.e.enabled;
        }
        if (from->array.s.enabled != to->array.s.enabled) {
            able[to->array.s.enabled](GL_SECONDARY_COLOR_ARRAY_EXT);
            from->array.s.enabled = to->array.s.enabled;
        }
        if (from->array.f.enabled != to->array.f.enabled) {
            able[to->array.f.enabled](GL_FOG_COORDINATE_ARRAY_EXT);
            from->array.f.enabled = to->array.f.enabled;
        }
        for (i = 0; (unsigned int)i < toCtx->limits.maxVertexProgramAttribs; i++) {
            if (from->array.a[i].enabled != to->array.a[i].enabled) {
                if (to->array.a[i].enabled)
                    diff_api.EnableVertexAttribArrayARB(i);
                else 
                    diff_api.DisableVertexAttribArrayARB(i);
                from->array.a[i].enabled = to->array.a[i].enabled;
            }
        }
        CLEARDIRTY2(cb->enableClientState, bitID);
    }

    if (to->curClientTextureUnit != curClientTextureUnit)
    {
        diff_api.ClientActiveTextureARB(GL_TEXTURE0_ARB + to->curClientTextureUnit);
    }
}


void
crStateClientSwitch(CRClientBits *cb, CRbitvalue *bitID,
                                        CRContext *fromCtx, CRContext *toCtx)
{
    const CRClientState *from = &(fromCtx->client);
    const CRClientState *to = &(toCtx->client);
    GLint curClientTextureUnit = from->curClientTextureUnit;
    int i;
    GLint idHwArrayBuffer = CR_BUFFER_HWID(toCtx->bufferobject.arrayBuffer);
    const GLint idHwInitialBuffer = idHwArrayBuffer;

#ifdef DEBUG_misha
    {
        GLint tstHwBuffer = -1;
        diff_api.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &tstHwBuffer);
        CRASSERT(idHwInitialBuffer == tstHwBuffer);
    }
#endif

    if (CHECKDIRTY(cb->clientPointer, bitID)) {
        /* one or more vertex pointers is dirty */
        if (CHECKDIRTY(cb->v, bitID)) {
            if (from->array.v.size != to->array.v.size ||
                    from->array.v.type != to->array.v.type ||
                    from->array.v.stride != to->array.v.stride ||
                    from->array.v.p != to->array.v.p ||
                    from->array.v.buffer != to->array.v.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.v.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.VertexPointer(to->array.v.size, to->array.v.type,
                                                             to->array.v.stride, to->array.v.p);
                FILLDIRTY(cb->v);
                FILLDIRTY(cb->clientPointer);
                FILLDIRTY(cb->dirty);
            }
            CLEARDIRTY2(cb->v, bitID);
        }
        /* normal */
        if (CHECKDIRTY(cb->n, bitID)) {
            if (from->array.n.type != to->array.n.type ||
                    from->array.n.stride != to->array.n.stride ||
                    from->array.n.p != to->array.n.p ||
                    from->array.n.buffer != to->array.n.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.n.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.NormalPointer(to->array.n.type,
                                                             to->array.n.stride, to->array.n.p);
                FILLDIRTY(cb->n);
                FILLDIRTY(cb->clientPointer);
                FILLDIRTY(cb->dirty);
            }
            CLEARDIRTY2(cb->n, bitID);
        }
        /* color */
        if (CHECKDIRTY(cb->c, bitID)) {
            if (from->array.c.size != to->array.c.size ||
                    from->array.c.type != to->array.c.type ||
                    from->array.c.stride != to->array.c.stride ||
                    from->array.c.p != to->array.c.p ||
                    from->array.c.buffer != to->array.c.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.c.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.ColorPointer(to->array.c.size, to->array.c.type,
                                                            to->array.c.stride, to->array.c.p);
                FILLDIRTY(cb->c);
                FILLDIRTY(cb->clientPointer);
                FILLDIRTY(cb->dirty);
            }
            CLEARDIRTY2(cb->c, bitID);
        }
        /* index */
        if (CHECKDIRTY(cb->i, bitID)) {
            if (from->array.i.type != to->array.i.type ||
                    from->array.i.stride != to->array.i.stride ||
                    from->array.i.p != to->array.i.p ||
                    from->array.i.buffer != to->array.i.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.i.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.IndexPointer(to->array.i.type,
                                                            to->array.i.stride, to->array.i.p);
                FILLDIRTY(cb->i);
                FILLDIRTY(cb->dirty);
                FILLDIRTY(cb->clientPointer);
            }
            CLEARDIRTY2(cb->i, bitID);
        }
        /* texcoords */
        for (i = 0; (unsigned int)i < toCtx->limits.maxTextureUnits; i++) {
            if (CHECKDIRTY(cb->t[i], bitID)) {
                if (from->array.t[i].size != to->array.t[i].size ||
                        from->array.t[i].type != to->array.t[i].type ||
                        from->array.t[i].stride != to->array.t[i].stride ||
                        from->array.t[i].p != to->array.t[i].p ||
                        from->array.t[i].buffer != to->array.t[i].buffer) {
                    GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.t[i].buffer);
                    if (idHwArrayBufferUsed != idHwArrayBuffer)
                    {
                        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                        idHwArrayBuffer = idHwArrayBufferUsed;
                    }
                    diff_api.ClientActiveTextureARB(GL_TEXTURE0_ARB + i);
                    curClientTextureUnit = i;
                    diff_api.TexCoordPointer(to->array.t[i].size, to->array.t[i].type,
                                                                to->array.t[i].stride, to->array.t[i].p);
                    FILLDIRTY(cb->t[i]);
                    FILLDIRTY(cb->clientPointer);
                    FILLDIRTY(cb->dirty);
                }
                CLEARDIRTY2(cb->t[i], bitID);
            }
        }
        /* edge flag */
        if (CHECKDIRTY(cb->e, bitID)) {
            if (from->array.e.stride != to->array.e.stride ||
                    from->array.e.p != to->array.e.p ||
                    from->array.e.buffer != to->array.e.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.e.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.EdgeFlagPointer(to->array.e.stride, to->array.e.p);
                FILLDIRTY(cb->e);
                FILLDIRTY(cb->clientPointer);
                FILLDIRTY(cb->dirty);
            }
            CLEARDIRTY2(cb->e, bitID);
        }
        /* secondary color */
        if (CHECKDIRTY(cb->s, bitID)) {
            if (from->array.s.size != to->array.s.size ||
                    from->array.s.type != to->array.s.type ||
                    from->array.s.stride != to->array.s.stride ||
                    from->array.s.p != to->array.s.p ||
                    from->array.s.buffer != to->array.s.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.s.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.SecondaryColorPointerEXT(to->array.s.size, to->array.s.type,
                                                                                    to->array.s.stride, to->array.s.p);
                FILLDIRTY(cb->s);
                FILLDIRTY(cb->clientPointer);
                FILLDIRTY(cb->dirty);
            }
            CLEARDIRTY2(cb->s, bitID);
        }
        /* fog coord */
        if (CHECKDIRTY(cb->f, bitID)) {
            if (from->array.f.type != to->array.f.type ||
                    from->array.f.stride != to->array.f.stride ||
                    from->array.f.p != to->array.f.p ||
                    from->array.f.buffer != to->array.f.buffer) {
                GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.f.buffer);
                if (idHwArrayBufferUsed != idHwArrayBuffer)
                {
                    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                    idHwArrayBuffer = idHwArrayBufferUsed;
                }
                diff_api.FogCoordPointerEXT(to->array.f.type,
                                                                        to->array.f.stride, to->array.f.p);
                FILLDIRTY(cb->f);
                FILLDIRTY(cb->clientPointer);
                FILLDIRTY(cb->dirty);
            }
            CLEARDIRTY2(cb->f, bitID);
        }
#if defined(CR_NV_vertex_program) || defined(CR_ARB_vertex_program)
        /* vertex attributes */
        for (i = 0; (unsigned int)i < toCtx->limits.maxVertexProgramAttribs; i++) {
            if (CHECKDIRTY(cb->a[i], bitID)) {
                if (from->array.a[i].size != to->array.a[i].size ||
                        from->array.a[i].type != to->array.a[i].type ||
                        from->array.a[i].stride != to->array.a[i].stride ||
                        from->array.a[i].normalized != to->array.a[i].normalized ||
                        from->array.a[i].p != to->array.a[i].p ||
                        from->array.a[i].buffer != to->array.a[i].buffer) {
                    GLint idHwArrayBufferUsed = CR_BUFFER_HWID(to->array.a[i].buffer);
                    if (idHwArrayBufferUsed != idHwArrayBuffer)
                    {
                        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwArrayBufferUsed);
                        idHwArrayBuffer = idHwArrayBufferUsed;
                    }
                    diff_api.VertexAttribPointerARB(i, to->array.a[i].size,
                                                                                    to->array.a[i].type,
                                                                                    to->array.a[i].normalized,
                                                                                    to->array.a[i].stride,
                                                                                    to->array.a[i].p);
                    FILLDIRTY(cb->a[i]);
                    FILLDIRTY(cb->clientPointer);
                    FILLDIRTY(cb->dirty);
                }
                CLEARDIRTY2(cb->a[i], bitID);
            }
        }
#endif
    }

    if (idHwArrayBuffer != idHwInitialBuffer)
    {
        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, idHwInitialBuffer);
    }

    if (CHECKDIRTY(cb->enableClientState, bitID)) {
        /* update vertex array enable/disable flags */
        glAble able[2];
        able[0] = diff_api.DisableClientState;
        able[1] = diff_api.EnableClientState;
        if (from->array.v.enabled != to->array.v.enabled) {
            able[to->array.v.enabled](GL_VERTEX_ARRAY);
            FILLDIRTY(cb->enableClientState);
            FILLDIRTY(cb->dirty);
        }
        if (from->array.n.enabled != to->array.n.enabled) {
            able[to->array.n.enabled](GL_NORMAL_ARRAY);
            FILLDIRTY(cb->enableClientState);
            FILLDIRTY(cb->dirty);
        }
        if (from->array.c.enabled != to->array.c.enabled) {
            able[to->array.c.enabled](GL_COLOR_ARRAY);
            FILLDIRTY(cb->enableClientState);
            FILLDIRTY(cb->dirty);
        }
        if (from->array.i.enabled != to->array.i.enabled) {
            able[to->array.i.enabled](GL_INDEX_ARRAY);
            FILLDIRTY(cb->enableClientState);
            FILLDIRTY(cb->dirty);
        }
        for (i = 0; (unsigned int)i < toCtx->limits.maxTextureUnits; i++) {
            if (from->array.t[i].enabled != to->array.t[i].enabled) {
                diff_api.ClientActiveTextureARB(GL_TEXTURE0_ARB + i);
                curClientTextureUnit = i;
                able[to->array.t[i].enabled](GL_TEXTURE_COORD_ARRAY);
                FILLDIRTY(cb->enableClientState);
                FILLDIRTY(cb->dirty);
            }
        }
        if (from->array.e.enabled != to->array.e.enabled) {
            able[to->array.e.enabled](GL_EDGE_FLAG_ARRAY);
            FILLDIRTY(cb->enableClientState);
            FILLDIRTY(cb->dirty);
        }
        if (from->array.s.enabled != to->array.s.enabled) {
            able[to->array.s.enabled](GL_SECONDARY_COLOR_ARRAY_EXT);
            FILLDIRTY(cb->enableClientState);
            FILLDIRTY(cb->dirty);
        }
        if (from->array.f.enabled != to->array.f.enabled) {
            able[to->array.f.enabled](GL_FOG_COORDINATE_ARRAY_EXT);
            FILLDIRTY(cb->enableClientState);
            FILLDIRTY(cb->dirty);
        }
        for (i = 0; (unsigned int)i < toCtx->limits.maxVertexProgramAttribs; i++) {
            if (from->array.a[i].enabled != to->array.a[i].enabled) {
                if (to->array.a[i].enabled)
                    diff_api.EnableVertexAttribArrayARB(i);
                else
                    diff_api.DisableVertexAttribArrayARB(i);
                FILLDIRTY(cb->enableClientState);
                FILLDIRTY(cb->dirty);
            }
        }
        CLEARDIRTY2(cb->enableClientState, bitID);
    }

    if (to->curClientTextureUnit != curClientTextureUnit)
    {
        diff_api.ClientActiveTextureARB(GL_TEXTURE0_ARB + to->curClientTextureUnit);
    }

    if (CHECKDIRTY(cb->unpack, bitID))
    {
        if (from->unpack.rowLength != to->unpack.rowLength)
        {
            diff_api.PixelStorei(GL_UNPACK_ROW_LENGTH, to->unpack.rowLength);
            FILLDIRTY(cb->unpack);
            FILLDIRTY(cb->dirty);
        }
        if (from->unpack.skipRows != to->unpack.skipRows)
        {
            diff_api.PixelStorei(GL_UNPACK_SKIP_ROWS, to->unpack.skipRows);
            FILLDIRTY(cb->unpack);
            FILLDIRTY(cb->dirty);
        }
        if (from->unpack.skipPixels != to->unpack.skipPixels)
        {
            diff_api.PixelStorei(GL_UNPACK_SKIP_PIXELS, to->unpack.skipPixels);
            FILLDIRTY(cb->unpack);
            FILLDIRTY(cb->dirty);
        }
        if (from->unpack.alignment != to->unpack.alignment)
        {
            diff_api.PixelStorei(GL_UNPACK_ALIGNMENT, to->unpack.alignment);
            FILLDIRTY(cb->unpack);
            FILLDIRTY(cb->dirty);
        }
        if (from->unpack.imageHeight != to->unpack.imageHeight)
        {
            diff_api.PixelStorei(GL_UNPACK_IMAGE_HEIGHT, to->unpack.imageHeight);
            FILLDIRTY(cb->unpack);
            FILLDIRTY(cb->dirty);
        }
        if (from->unpack.skipImages != to->unpack.skipImages)
        {
            diff_api.PixelStorei(GL_UNPACK_SKIP_IMAGES, to->unpack.skipImages);
            FILLDIRTY(cb->unpack);
            FILLDIRTY(cb->dirty);
        }
        if (from->unpack.swapBytes != to->unpack.swapBytes)
        {
            diff_api.PixelStorei(GL_UNPACK_SWAP_BYTES, to->unpack.swapBytes);
            FILLDIRTY(cb->unpack);
            FILLDIRTY(cb->dirty);
        }
        if (from->unpack.psLSBFirst != to->unpack.psLSBFirst)
        {
            diff_api.PixelStorei(GL_UNPACK_LSB_FIRST, to->unpack.psLSBFirst);
            FILLDIRTY(cb->unpack);
            FILLDIRTY(cb->dirty);
        }
        CLEARDIRTY2(cb->unpack, bitID);
    }

    if (CHECKDIRTY(cb->pack, bitID))
    {
        if (from->pack.rowLength != to->pack.rowLength)
        {
            diff_api.PixelStorei(GL_PACK_ROW_LENGTH, to->pack.rowLength);
            FILLDIRTY(cb->pack);
            FILLDIRTY(cb->dirty);
        }
        if (from->pack.skipRows != to->pack.skipRows)
        {
            diff_api.PixelStorei(GL_PACK_SKIP_ROWS, to->pack.skipRows);
            FILLDIRTY(cb->pack);
            FILLDIRTY(cb->dirty);
        }
        if (from->pack.skipPixels != to->pack.skipPixels)
        {
            diff_api.PixelStorei(GL_PACK_SKIP_PIXELS, to->pack.skipPixels);
            FILLDIRTY(cb->pack);
            FILLDIRTY(cb->dirty);
        }
        if (from->pack.alignment != to->pack.alignment)
        {
            diff_api.PixelStorei(GL_PACK_ALIGNMENT, to->pack.alignment);
            FILLDIRTY(cb->pack);
            FILLDIRTY(cb->dirty);
        }
        if (from->pack.imageHeight != to->pack.imageHeight)
        {
            diff_api.PixelStorei(GL_PACK_IMAGE_HEIGHT, to->pack.imageHeight);
            FILLDIRTY(cb->pack);
            FILLDIRTY(cb->dirty);
        }
        if (from->pack.skipImages != to->pack.skipImages)
        {
            diff_api.PixelStorei(GL_PACK_SKIP_IMAGES, to->pack.skipImages);
            FILLDIRTY(cb->pack);
            FILLDIRTY(cb->dirty);
        }
        if (from->pack.swapBytes != to->pack.swapBytes)
        {
            diff_api.PixelStorei(GL_PACK_SWAP_BYTES, to->pack.swapBytes);
            FILLDIRTY(cb->pack);
            FILLDIRTY(cb->dirty);
        }
        if (from->pack.psLSBFirst != to->pack.psLSBFirst)
        {
            diff_api.PixelStorei(GL_PACK_LSB_FIRST, to->pack.psLSBFirst);
            FILLDIRTY(cb->pack);
            FILLDIRTY(cb->dirty);
        }
        CLEARDIRTY2(cb->pack, bitID);
    }

    CLEARDIRTY2(cb->dirty, bitID);
}

CRClientPointer* crStateGetClientPointerByIndex(int index, CRVertexArrays *array)
{
    CRASSERT(array && index>=0 && index<CRSTATECLIENT_MAX_VERTEXARRAYS);

    if (array == NULL || index < 0 || index >= CRSTATECLIENT_MAX_VERTEXARRAYS)
    {
        return NULL;
    }

    if (index < 7)
    {
        switch (index)
        {
            case 0: return &array->v;
            case 1: return &array->c;
            case 2: return &array->f;
            case 3: return &array->s;
            case 4: return &array->e;
            case 5: return &array->i;
            case 6: return &array->n;
        }
    }
    else if (index<(7+CR_MAX_TEXTURE_UNITS))
    {
        return &array->t[index-7];
    }
    else
    {
        return &array->a[index-7-CR_MAX_TEXTURE_UNITS];
    }

    /*silence the compiler warning*/
    CRASSERT(false);
    return NULL;
}
