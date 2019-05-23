/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "state/cr_statetypes.h"
#include "state/cr_statefuncs.h"
#include "state_internals.h"
#include "cr_mem.h"
#include "cr_string.h"


static CRBufferObject *AllocBufferObject(GLuint name)
{
    CRBufferObject *b = crCalloc(sizeof(CRBufferObject));
    if (b) {
        b->refCount = 1;
        b->id = name;
        b->hwid = name;
        b->usage = GL_STATIC_DRAW_ARB;
        b->access = GL_READ_WRITE_ARB;
        b->bResyncOnRead = GL_FALSE;
        CR_STATE_SHAREDOBJ_USAGE_INIT(b);
    }
    return b;
}

void STATE_APIENTRY crStateGenBuffersARB(GLsizei n, GLuint *buffers)
{
    CRContext *g = GetCurrentContext();
    crStateGenNames(g, g->shared->buffersTable, n, buffers);
}

void crStateRegBuffers(GLsizei n, GLuint *buffers)
{
    CRContext *g = GetCurrentContext();
    crStateRegNames(g, g->shared->buffersTable, n, buffers);
}

GLboolean crStateIsBufferBoundForCtx(CRContext *g, GLenum target)
{
    CRBufferObjectState *b = &(g->bufferobject);

    switch (target)
    {
        case GL_ARRAY_BUFFER_ARB:
            return b->arrayBuffer->id!=0;
        case GL_ELEMENT_ARRAY_BUFFER_ARB:
            return b->elementsBuffer->id!=0;
#ifdef CR_ARB_pixel_buffer_object
        case GL_PIXEL_PACK_BUFFER_ARB:
            return b->packBuffer->id!=0;
        case GL_PIXEL_UNPACK_BUFFER_ARB:
            return b->unpackBuffer->id!=0;
#endif
        default:
            return GL_FALSE;
    }
}

GLboolean crStateIsBufferBound(GLenum target)
{
    CRContext *g = GetCurrentContext();
    return crStateIsBufferBoundForCtx(g, target);
}

CRBufferObject *crStateGetBoundBufferObject(GLenum target, CRBufferObjectState *b)
{
    switch (target)
    {
        case GL_ARRAY_BUFFER_ARB:
            return b->arrayBuffer;
        case GL_ELEMENT_ARRAY_BUFFER_ARB:
            return b->elementsBuffer;
#ifdef CR_ARB_pixel_buffer_object
        case GL_PIXEL_PACK_BUFFER_ARB:
            return b->packBuffer;
        case GL_PIXEL_UNPACK_BUFFER_ARB:
            return b->unpackBuffer;
#endif
        default:
            return NULL;
    }
}

DECLEXPORT(GLboolean) STATE_APIENTRY crStateIsBufferARB( GLuint buffer )
{
    CRContext *g = GetCurrentContext();

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glIsBufferARB called in begin/end");
        return GL_FALSE;
    }

    return buffer ? crHashtableIsKeyUsed(g->shared->buffersTable, buffer) : GL_FALSE;
}

void crStateBufferObjectInit (CRContext *ctx)
{
    CRStateBits *sb          = GetCurrentBits();
    CRBufferObjectBits *bb = &sb->bufferobject;
    CRBufferObjectState *b = &ctx->bufferobject;

    RESET(bb->dirty, ctx->bitid);
    RESET(bb->arrayBinding, ctx->bitid);
    RESET(bb->elementsBinding, ctx->bitid);
#ifdef CR_ARB_pixel_buffer_object
    RESET(bb->unpackBinding, ctx->bitid);
    RESET(bb->packBinding, ctx->bitid);
#endif

#ifdef IN_GUEST
    b->retainBufferData = GL_TRUE;
#else
    b->retainBufferData = GL_FALSE;
#endif

    b->nullBuffer = AllocBufferObject(0);
    b->arrayBuffer = b->nullBuffer;
    b->elementsBuffer = b->nullBuffer;
    b->nullBuffer->refCount += 2;
#ifdef CR_ARB_pixel_buffer_object
    b->packBuffer = b->nullBuffer;
    b->unpackBuffer = b->nullBuffer;
    b->nullBuffer->refCount += 2;
#endif

    ctx->shared->bVBOResyncNeeded = GL_FALSE;
}

void crStateFreeBufferObject(void *data)
{
    CRBufferObject *pObj = (CRBufferObject *)data;
    if (pObj->data) crFree(pObj->data);

#ifndef IN_GUEST
    if (diff_api.DeleteBuffersARB)
    {
        diff_api.DeleteBuffersARB(1, &pObj->hwid);
    }
#endif

    crFree(pObj);
}

void crStateBufferObjectDestroy (CRContext *ctx)
{
    CRBufferObjectState *b = &ctx->bufferobject;
    crFree(b->nullBuffer);
}

static void crStateCheckBufferHWIDCB(unsigned long key, void *data1, void *data2)
{
    CRBufferObject *pObj = (CRBufferObject *) data1;
    crCheckIDHWID_t *pParms = (crCheckIDHWID_t*) data2;
    (void) key;

    if (pObj->hwid==pParms->hwid)
        pParms->id = pObj->id;
}

DECLEXPORT(GLuint) STATE_APIENTRY crStateBufferHWIDtoID(GLuint hwid)
{
    CRContext *g = GetCurrentContext();
    crCheckIDHWID_t parms;

    parms.id = hwid;
    parms.hwid = hwid;

    crHashtableWalk(g->shared->buffersTable, crStateCheckBufferHWIDCB, &parms);
    return parms.id;
}

DECLEXPORT(GLuint) STATE_APIENTRY crStateGetBufferHWID(GLuint id)
{
    CRContext *g = GetCurrentContext();
    CRBufferObject *pObj = (CRBufferObject *) crHashtableSearch(g->shared->buffersTable, id);

    return pObj ? pObj->hwid : 0;
}

void STATE_APIENTRY
crStateBindBufferARB (GLenum target, GLuint buffer)
{
    CRContext *g = GetCurrentContext();
    CRBufferObjectState *b = &(g->bufferobject);
    CRStateBits *sb = GetCurrentBits();
    CRBufferObjectBits *bb = &(sb->bufferobject);
    CRBufferObject *oldObj, *newObj;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBindBufferARB called in begin/end");
        return;
    }

    FLUSH();

    oldObj = crStateGetBoundBufferObject(target, b);
    if (!oldObj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glBindBufferARB(target)");
        return;
    }

    if (buffer == 0) {
        newObj = b->nullBuffer;
    }
    else {
        newObj = (CRBufferObject *) crHashtableSearch(g->shared->buffersTable, buffer);
        if (!newObj) {
            CRSTATE_CHECKERR(!crHashtableIsKeyUsed(g->shared->buffersTable, buffer), GL_INVALID_OPERATION, "name is not a buffer object");
            newObj = AllocBufferObject(buffer);
            CRSTATE_CHECKERR(!newObj, GL_OUT_OF_MEMORY, "glBindBuffer");
#ifndef IN_GUEST
            diff_api.GenBuffersARB(1, &newObj->hwid);
            if (!newObj->hwid)
            {
                crWarning("GenBuffersARB failed!");
                crFree(newObj);
                return;
            }
#endif
            crHashtableAdd( g->shared->buffersTable, buffer, newObj );
        }

        CR_STATE_SHAREDOBJ_USAGE_SET(newObj, g);
    }

    newObj->refCount++;
    oldObj->refCount--;

    switch (target)
    {
        case GL_ARRAY_BUFFER_ARB:
            b->arrayBuffer = newObj;
            DIRTY(bb->dirty, g->neg_bitid);
            DIRTY(bb->arrayBinding, g->neg_bitid);
            break;
        case GL_ELEMENT_ARRAY_BUFFER_ARB:
            b->elementsBuffer = newObj;
            DIRTY(bb->dirty, g->neg_bitid);
            DIRTY(bb->elementsBinding, g->neg_bitid);
            break;
#ifdef CR_ARB_pixel_buffer_object
        case GL_PIXEL_PACK_BUFFER_ARB:
            b->packBuffer = newObj;
            DIRTY(bb->dirty, g->neg_bitid);
            DIRTY(bb->packBinding, g->neg_bitid);
            break;
        case GL_PIXEL_UNPACK_BUFFER_ARB:
            b->unpackBuffer = newObj;
            DIRTY(bb->dirty, g->neg_bitid);
            DIRTY(bb->unpackBinding, g->neg_bitid);
            break;
#endif
        default: /*can't get here*/
            CRASSERT(false);
            return;
    }

    if (oldObj->refCount <= 0) {
        /*we shouldn't reach this point*/
        CRASSERT(false);
        crHashtableDelete(g->shared->buffersTable, (unsigned long) oldObj->id, crStateFreeBufferObject);
    }

#ifdef IN_GUEST
    if (target == GL_PIXEL_PACK_BUFFER_ARB)
    {
        newObj->bResyncOnRead = GL_TRUE;
    }
#endif
}

static void ctStateBuffersRefsCleanup(CRContext *ctx, CRBufferObject *obj, CRbitvalue *neg_bitid)
{
    CRBufferObjectState *b = &(ctx->bufferobject);
    CRStateBits *sb = GetCurrentBits();
    CRBufferObjectBits *bb = &(sb->bufferobject);
    int j, k;

    if (obj == b->arrayBuffer)
    {
        b->arrayBuffer = b->nullBuffer;
        b->arrayBuffer->refCount++;
        DIRTY(bb->dirty, neg_bitid);
        DIRTY(bb->arrayBinding, neg_bitid);
    }
    if (obj == b->elementsBuffer)
    {
        b->elementsBuffer = b->nullBuffer;
        b->elementsBuffer->refCount++;
        DIRTY(bb->dirty, neg_bitid);
        DIRTY(bb->elementsBinding, neg_bitid);
    }
#ifdef CR_ARB_pixel_buffer_object
    if (obj == b->packBuffer)
    {
        b->packBuffer = b->nullBuffer;
        b->packBuffer->refCount++;
        DIRTY(bb->dirty, neg_bitid);
        DIRTY(bb->packBinding, neg_bitid);
    }
    if (obj == b->unpackBuffer)
    {
        b->unpackBuffer = b->nullBuffer;
        b->unpackBuffer->refCount++;
        DIRTY(bb->dirty, neg_bitid);
        DIRTY(bb->unpackBinding, neg_bitid);
    }
#endif

#ifdef CR_ARB_vertex_buffer_object
    for (j=0; j<CRSTATECLIENT_MAX_VERTEXARRAYS; ++j)
    {
        CRClientPointer *cp = crStateGetClientPointerByIndex(j, &ctx->client.array);
        if (obj == cp->buffer)
        {
            cp->buffer = b->nullBuffer;
            ++b->nullBuffer->refCount;
        }
    }

    for (k=0; k<ctx->client.vertexArrayStackDepth; ++k)
    {
        CRVertexArrays *pArray = &ctx->client.vertexArrayStack[k];
        for (j=0; j<CRSTATECLIENT_MAX_VERTEXARRAYS; ++j)
        {
            CRClientPointer *cp = crStateGetClientPointerByIndex(j, pArray);
            if (obj == cp->buffer)
            {
                cp->buffer = b->nullBuffer;
                ++b->nullBuffer->refCount;
            }
        }
    }
#endif

    CR_STATE_SHAREDOBJ_USAGE_CLEAR(obj, ctx);
}

void STATE_APIENTRY
crStateDeleteBuffersARB(GLsizei n, const GLuint *buffers)
{
    CRContext *g = GetCurrentContext();
    int i;

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glDeleteBuffersARB called in Begin/End");
        return;
    }

    if (n < 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glDeleteBuffersARB(n < 0)");
        return;
    }

    for (i = 0; i < n; i++) {
        if (buffers[i]) {
            CRBufferObject *obj = (CRBufferObject *)
                crHashtableSearch(g->shared->buffersTable, buffers[i]);
            if (obj) {
                int j;

                ctStateBuffersRefsCleanup(g, obj, g->neg_bitid);

                CR_STATE_SHAREDOBJ_USAGE_FOREACH_USED_IDX(obj, j)
                {
                    /* saved state version <= SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS does not have usage bits info,
                     * so on restore, we set mark bits as used.
                     * This is why g_pAvailableContexts[j] could be NULL
                     * also g_pAvailableContexts[0] will hold default context, which we should discard */
                    CRContext *ctx = g_pAvailableContexts[j];
                    if (j && ctx)
                    {
                        ctStateBuffersRefsCleanup(ctx, obj, g->neg_bitid); /* <- yes, use g->neg_bitid, i.e. neg_bitid of the current context to ensure others bits get dirtified,
                                                                            * but not the current context ones*/
                    }
                    else
                        CR_STATE_SHAREDOBJ_USAGE_CLEAR_IDX(obj, j);
                }

                crHashtableDelete(g->shared->buffersTable, buffers[i], crStateFreeBufferObject);
            }
        }
    }
}

void STATE_APIENTRY
crStateBufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage)
{
    CRContext *g = GetCurrentContext();
    CRBufferObjectState *b = &g->bufferobject;
    CRBufferObject *obj;
    CRStateBits *sb = GetCurrentBits();
    CRBufferObjectBits *bb = &sb->bufferobject;

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBufferDataARB called in begin/end");
        return;
    }

    if (size < 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glBufferDataARB(size < 0)");
        return;
    }

    switch (usage) {
        case GL_STREAM_DRAW_ARB:
        case GL_STREAM_READ_ARB:
        case GL_STREAM_COPY_ARB:
        case GL_STATIC_DRAW_ARB:
        case GL_STATIC_READ_ARB:
        case GL_STATIC_COPY_ARB:
        case GL_DYNAMIC_DRAW_ARB:
        case GL_DYNAMIC_READ_ARB:
        case GL_DYNAMIC_COPY_ARB:
            /* OK */
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glBufferDataARB(usage)");
            return;
    }

    obj = crStateGetBoundBufferObject(target, b);
    if (!obj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glBufferDataARB(target)");
        return;
    }

    if (obj->id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glBufferDataARB");
        return;
    }

    if (obj->pointer) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBufferDataARB(buffer is mapped)");
        return;
    }

    obj->usage = usage;
    obj->size = size;

    /* The user of the state tracker should set the retainBufferData field
     * during context initialization, if needed.
     */
    if (b->retainBufferData) {
        if (obj->data) {
            crFree(obj->data);
        }

        obj->data = crAlloc(size);
        if (!obj->data) {
            crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY, "glBufferDataARB");
            return;
        }
        if (data)
            crMemcpy(obj->data, data, size);
    }

    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(obj->dirty, g->neg_bitid);
    obj->dirtyStart = 0;
    obj->dirtyLength = size;
}


void STATE_APIENTRY
crStateBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data)
{
    CRContext *g = GetCurrentContext();
    CRBufferObjectState *b = &g->bufferobject;
    CRBufferObject *obj;
    CRStateBits *sb = GetCurrentBits();
    CRBufferObjectBits *bb = &sb->bufferobject;

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBufferSubDataARB called in begin/end");
        return;
    }

    obj = crStateGetBoundBufferObject(target, b);
    if (!obj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glBufferSubDataARB(target)");
        return;
    }

    if (obj->id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBufferSubDataARB");
        return;
    }

    if (obj->pointer) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBufferSubDataARB(buffer is mapped)");
        return;
    }

    if (size < 0 || offset < 0 || (unsigned int)offset + size > obj->size) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBufferSubDataARB(bad offset and/or size)");
        return;
    }

    if (b->retainBufferData && obj->data) {
        crMemcpy((char *) obj->data + offset, data, size);
    }

    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(obj->dirty, g->neg_bitid);
    /* grow dirty region */
    if (offset + size > obj->dirtyStart + obj->dirtyLength)
        obj->dirtyLength = offset + size;
    if (offset < obj->dirtyStart)
        obj->dirtyStart = offset;
}


void STATE_APIENTRY
crStateGetBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, void * data)
{
    CRContext *g = GetCurrentContext();
    CRBufferObjectState *b = &g->bufferobject;
    CRBufferObject *obj;

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetBufferSubDataARB called in begin/end");
        return;
    }

    obj = crStateGetBoundBufferObject(target, b);
    if (!obj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetBufferSubDataARB(target)");
        return;
    }

    if (obj->id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetBufferSubDataARB");
        return;
    }

    if (obj->pointer) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetBufferSubDataARB(buffer is mapped)");
        return;
    }

    if (size < 0 || offset < 0 || (unsigned int)offset + size > obj->size) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetBufferSubDataARB(bad offset and/or size)");
        return;
    }

    if (b->retainBufferData && obj->data) {
        crMemcpy(data, (char *) obj->data + offset, size);
    }
}


void * STATE_APIENTRY
crStateMapBufferARB(GLenum target, GLenum access)
{
    CRContext *g = GetCurrentContext();
    CRBufferObjectState *b = &g->bufferobject;
    CRBufferObject *obj;

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glMapBufferARB called in begin/end");
        return NULL;
    }

    obj = crStateGetBoundBufferObject(target, b);
    if (!obj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glMapBufferARB(target)");
        return NULL;
    }

    if (obj->id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glMapBufferARB");
        return GL_FALSE;
    }

    switch (access) {
        case GL_READ_ONLY_ARB:
        case GL_WRITE_ONLY_ARB:
        case GL_READ_WRITE_ARB:
            obj->access = access;
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glMapBufferARB(access)");
            return NULL;
    }

    if (b->retainBufferData && obj->data)
        obj->pointer = obj->data;

    return obj->pointer;
}


GLboolean STATE_APIENTRY
crStateUnmapBufferARB(GLenum target)
{
    CRContext *g = GetCurrentContext();
    CRBufferObjectState *b = &g->bufferobject;
    CRBufferObject *obj;
    CRStateBits *sb = GetCurrentBits();
    CRBufferObjectBits *bb = &sb->bufferobject;

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glUnmapBufferARB called in begin/end");
        return GL_FALSE;
    }

    obj = crStateGetBoundBufferObject(target, b);
    if (!obj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glUnmapBufferARB(target)");
        return GL_FALSE;
    }

    if (obj->id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glUnmapBufferARB");
        return GL_FALSE;
    }

    if (!obj->pointer) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glUnmapBufferARB");
        return GL_FALSE;
    }

    obj->pointer = NULL;

    if (obj->access != GL_READ_ONLY_ARB) {
        /* the data was most likely modified */
        DIRTY(bb->dirty, g->neg_bitid);
        DIRTY(obj->dirty, g->neg_bitid);
        obj->dirtyStart = 0;
        obj->dirtyLength = obj->size;
    }

    return GL_TRUE;
}


void STATE_APIENTRY
crStateGetBufferParameterivARB(GLenum target, GLenum pname, GLint *params)
{
    CRContext *g = GetCurrentContext();
    CRBufferObjectState *b = &g->bufferobject;
    CRBufferObject *obj;

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetBufferParameterivARB called in begin/end");
        return;
    }

    obj = crStateGetBoundBufferObject(target, b);
    if (!obj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetBufferParameterivARB(target)");
        return;
    }

    switch (pname) {
        case GL_BUFFER_SIZE_ARB:
            *params = obj->size;
            break;
        case GL_BUFFER_USAGE_ARB:
            *params = obj->usage;
            break;
        case GL_BUFFER_ACCESS_ARB:
            *params = obj->access;
            break;
        case GL_BUFFER_MAPPED_ARB:
            *params = (obj->pointer != NULL);
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glGetBufferParameterivARB(pname)");
            return;
    }
}


void STATE_APIENTRY
crStateGetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params)
{
    CRContext *g = GetCurrentContext();
    CRBufferObjectState *b = &g->bufferobject;
    CRBufferObject *obj;

    FLUSH();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetBufferPointervARB called in begin/end");
        return;
    }

    obj = crStateGetBoundBufferObject(target, b);
    if (!obj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetBufferPointervARB(target)");
        return;
    }

    if (pname != GL_BUFFER_MAP_POINTER_ARB) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetBufferPointervARB(pname)");
        return;
    }

    *params = obj->pointer;
}


/**
 * We need to check if the GL_EXT_vertex/pixel_buffer_object extensions
 * are supported before calling any of the diff_api functions.
 * This flag indicates if the extensions is available (1), not available (0)
 * or needs to be tested for (-1).
 * If we don't do this, we can segfault inside OpenGL.
 * Ideally, the render SPU should no-op unsupported GL functions, but
 * that's a bit complicated.
 */
static GLboolean
HaveBufferObjectExtension(void)
{
    static GLint haveBufferObjectExt = -1;

    if (haveBufferObjectExt == -1) {
        const char *ext;
        /* XXX this check is temporary.  We need to make the tilesort SPU plug
         * GetString into the diff'ing table in order for this to really work.
         */
        if (!diff_api.GetString) {
            haveBufferObjectExt = 0;
            return 0;
        }
        CRASSERT(diff_api.GetString);
        ext = (const char *) diff_api.GetString(GL_EXTENSIONS);
        if (crStrstr(ext, "GL_ARB_vertex_buffer_object") ||
                crStrstr(ext, "GL_ARB_pixel_buffer_object")) {
            haveBufferObjectExt = 1;
        }
        else {
            haveBufferObjectExt = 0;
        }
    }
    return haveBufferObjectExt;
}

static void crStateBufferObjectIntCmp(CRBufferObjectBits *bb, CRbitvalue *bitID,
                                      CRContext *fromCtx, CRContext *toCtx,
                                      GLboolean bSwitch)
{
    CRBufferObjectState *from = &(fromCtx->bufferobject);
    const CRBufferObjectState *to = &(toCtx->bufferobject);

    /* ARRAY_BUFFER */
    if (CHECKDIRTY(bb->arrayBinding, bitID)) 
    {
        if (from->arrayBuffer != to->arrayBuffer)
        {
            GLuint bufferID = to->arrayBuffer ? to->arrayBuffer->hwid : 0;
            diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, bufferID);
            if (bSwitch)
            {
                FILLDIRTY(bb->arrayBinding);
                FILLDIRTY(bb->dirty);
            }
            else
            {
                CLEARDIRTY2(bb->arrayBinding, bitID);
                from->arrayBuffer = to->arrayBuffer;
            }
        }
        if (bSwitch) CLEARDIRTY2(bb->arrayBinding, bitID);
    }

    if (to->arrayBuffer && CHECKDIRTY(to->arrayBuffer->dirty, bitID))
    {
        /* update array buffer data */
        CRBufferObject *bufObj = to->arrayBuffer;
        CRASSERT(bufObj);
        if (bufObj->dirtyStart == 0 && bufObj->dirtyLength == (int) bufObj->size) 
        {
            /* update whole buffer */
            diff_api.BufferDataARB(GL_ARRAY_BUFFER_ARB, bufObj->size,
                                   bufObj->data, bufObj->usage);
        }
        else 
        {
            /* update sub buffer */
            diff_api.BufferSubDataARB(GL_ARRAY_BUFFER_ARB,
                                      bufObj->dirtyStart, bufObj->dirtyLength,
                                      (char *) bufObj->data + bufObj->dirtyStart);
        }
        if (bSwitch) FILLDIRTY(bufObj->dirty);
        CLEARDIRTY2(bufObj->dirty, bitID);
    }

    /* ELEMENTS_BUFFER */
    if (CHECKDIRTY(bb->elementsBinding, bitID)) 
    {
        if (from->elementsBuffer != to->elementsBuffer)
        {
            GLuint bufferID = to->elementsBuffer ? to->elementsBuffer->hwid : 0;
            diff_api.BindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, bufferID);
            if (bSwitch)
            {
                FILLDIRTY(bb->elementsBinding);
                FILLDIRTY(bb->dirty);
            }
            else
            {
                CLEARDIRTY2(bb->elementsBinding, bitID);
                from->elementsBuffer = to->elementsBuffer;
            }
        }
        if (bSwitch) CLEARDIRTY2(bb->elementsBinding, bitID);
    }

    if (to->elementsBuffer && CHECKDIRTY(to->elementsBuffer->dirty, bitID))
    {
        /* update array buffer data */
        CRBufferObject *bufObj = to->elementsBuffer;
        CRASSERT(bufObj);
        if (bufObj->dirtyStart == 0 && bufObj->dirtyLength == (int) bufObj->size) 
        {
            /* update whole buffer */
            diff_api.BufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, bufObj->size,
                                   bufObj->data, bufObj->usage);
        }
        else 
        {
            /* update sub buffer */
            diff_api.BufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
                                      bufObj->dirtyStart, bufObj->dirtyLength,
                                      (char *) bufObj->data + bufObj->dirtyStart);
        }
        if (bSwitch) FILLDIRTY(bufObj->dirty);
        CLEARDIRTY2(bufObj->dirty, bitID);
    }

#ifdef CR_ARB_pixel_buffer_object
    /* PIXEL_PACK_BUFFER */
    if (CHECKDIRTY(bb->packBinding, bitID)) 
    {
        if (from->packBuffer != to->packBuffer)
        {
            GLuint bufferID = to->packBuffer ? to->packBuffer->hwid : 0;
            diff_api.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, bufferID);
            if (bSwitch)
            {
                FILLDIRTY(bb->packBinding);
                FILLDIRTY(bb->dirty);
            }
            else
            {
                CLEARDIRTY2(bb->packBinding, bitID);
                from->packBuffer = to->packBuffer;
            }
        }
        if (bSwitch) CLEARDIRTY2(bb->packBinding, bitID);
    }

    if (to->packBuffer && CHECKDIRTY(to->packBuffer->dirty, bitID))
    {
        /* update array buffer data */
        CRBufferObject *bufObj = to->packBuffer;
        CRASSERT(bufObj);
        if (bufObj->dirtyStart == 0 && bufObj->dirtyLength == (int) bufObj->size) 
        {
            /* update whole buffer */
            diff_api.BufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, bufObj->size,
                                   bufObj->data, bufObj->usage);
        }
        else 
        {
            /* update sub buffer */
            diff_api.BufferSubDataARB(GL_PIXEL_PACK_BUFFER_ARB,
                                      bufObj->dirtyStart, bufObj->dirtyLength,
                                      (char *) bufObj->data + bufObj->dirtyStart);
        }
        if (bSwitch) FILLDIRTY(bufObj->dirty);
        CLEARDIRTY2(bufObj->dirty, bitID);
    }

    /* PIXEL_UNPACK_BUFFER */
    if (CHECKDIRTY(bb->unpackBinding, bitID)) 
    {
        if (from->unpackBuffer != to->unpackBuffer)
        {
            GLuint bufferID = to->unpackBuffer ? to->unpackBuffer->hwid : 0;
            diff_api.BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, bufferID);
            if (bSwitch)
            {
                FILLDIRTY(bb->unpackBinding);
                FILLDIRTY(bb->dirty);
            }
            else
            {
                CLEARDIRTY2(bb->unpackBinding, bitID);
                from->unpackBuffer = to->unpackBuffer;
            }
        }
        if (bSwitch) CLEARDIRTY2(bb->unpackBinding, bitID);
    }

    if (to->unpackBuffer && CHECKDIRTY(to->unpackBuffer->dirty, bitID))
    {
        /* update array buffer data */
        CRBufferObject *bufObj = to->unpackBuffer;
        CRASSERT(bufObj);
        if (bufObj->dirtyStart == 0 && bufObj->dirtyLength == (int) bufObj->size) 
        {
            /* update whole buffer */
            diff_api.BufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bufObj->size,
                                   bufObj->data, bufObj->usage);
        }
        else 
        {
            /* update sub buffer */
            diff_api.BufferSubDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,
                                      bufObj->dirtyStart, bufObj->dirtyLength,
                                      (char *) bufObj->data + bufObj->dirtyStart);
        }
        if (bSwitch) FILLDIRTY(bufObj->dirty);
        CLEARDIRTY2(bufObj->dirty, bitID);
    }
#endif /*ifdef CR_ARB_pixel_buffer_object*/
}

void crStateBufferObjectDiff(CRBufferObjectBits *bb, CRbitvalue *bitID,
                             CRContext *fromCtx, CRContext *toCtx)
{
    /*CRBufferObjectState *from = &(fromCtx->bufferobject); - unused
    const CRBufferObjectState *to = &(toCtx->bufferobject); - unused */

    if (!HaveBufferObjectExtension())
        return;

    crStateBufferObjectIntCmp(bb, bitID, fromCtx, toCtx, GL_FALSE);
}

static void crStateBufferObjectSyncCB(unsigned long key, void *data1, void *data2)
{
    CRBufferObject *pBufferObj = (CRBufferObject *) data1;
    CRBufferObjectState *pState = (CRBufferObjectState *) data2;
    (void)key;

    if (pBufferObj->id && !pBufferObj->hwid)
    {
        diff_api.GenBuffersARB(1, &pBufferObj->hwid);
        CRASSERT(pBufferObj->hwid);
    }

    if (pBufferObj->data)
    {
        /*@todo http://www.opengl.org/registry/specs/ARB/pixel_buffer_object.txt
          "While it is entirely legal to create a buffer object by binding
          it to GL_ARRAY_BUFFER and loading it with data, then using it
          with the GL_PIXEL_UNPACK_BUFFER_ARB or GL_PIXEL_PACK_BUFFER_ARB
          binding, such behavior is liable to confuse the driver and may
          hurt performance." 
         */
        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, pBufferObj->hwid);
        diff_api.BufferDataARB(GL_ARRAY_BUFFER_ARB, pBufferObj->size, pBufferObj->data, pBufferObj->usage);

        if (!pState->retainBufferData)
        {
            crFree(pBufferObj->data);
            pBufferObj->data = NULL;
        }
    }
}

/*
 * XXX this function might need some testing/fixing.
 */
void crStateBufferObjectSwitch(CRBufferObjectBits *bb, CRbitvalue *bitID, 
                               CRContext *fromCtx, CRContext *toCtx)
{
    /*const CRBufferObjectState *from = &(fromCtx->bufferobject); - unused */
    CRBufferObjectState *to = &(toCtx->bufferobject);
    int i;

    if (!HaveBufferObjectExtension())
        return;

    if (toCtx->shared->bVBOResyncNeeded)
    {
        CRClientPointer *cp;
        GLboolean locked = toCtx->client.array.locked;

        crHashtableWalk(toCtx->shared->buffersTable, crStateBufferObjectSyncCB, to);
        toCtx->shared->bVBOResyncNeeded = GL_FALSE;

        /*@todo, move to state_client.c*/
        cp = &toCtx->client.array.v;
        if (cp->buffer && (cp->buffer->id || locked))
        {
            diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
            diff_api.VertexPointer(cp->size, cp->type, cp->stride, cp->p);
        }

        cp = &toCtx->client.array.c;
        if (cp->buffer && (cp->buffer->id || locked))
        {
            diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
            diff_api.ColorPointer(cp->size, cp->type, cp->stride, cp->p);
        }

        cp = &toCtx->client.array.f;
        if (cp->buffer && (cp->buffer->id || locked))
        {
            diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
            diff_api.FogCoordPointerEXT(cp->type, cp->stride, cp->p);
        }

        cp = &toCtx->client.array.s;
        if (cp->buffer && (cp->buffer->id || locked))
        {
            diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
            diff_api.SecondaryColorPointerEXT(cp->size, cp->type, cp->stride, cp->p);
        }

        cp = &toCtx->client.array.e;
        if (cp->buffer && (cp->buffer->id || locked))
        {
            diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
            diff_api.EdgeFlagPointer(cp->stride, cp->p);
        }

        cp = &toCtx->client.array.i;
        if (cp->buffer && (cp->buffer->id || locked))
        {
            diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
            diff_api.IndexPointer(cp->type, cp->stride, cp->p);
        }

        cp = &toCtx->client.array.n;
        if (cp->buffer && (cp->buffer->id || locked))
        {
            diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
            diff_api.NormalPointer(cp->type, cp->stride, cp->p);
        }

        for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++)
        {
            cp = &toCtx->client.array.t[i];
            if (cp->buffer && (cp->buffer->id || locked))
            {
                if (diff_api.ActiveTextureARB)
                    diff_api.ActiveTextureARB(i+GL_TEXTURE0_ARB);
                diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
                diff_api.TexCoordPointer(cp->size, cp->type, cp->stride, cp->p);
            }
        }

        if (diff_api.ActiveTextureARB)
            diff_api.ActiveTextureARB(toCtx->client.curClientTextureUnit+GL_TEXTURE0_ARB);

#ifdef CR_NV_vertex_program
        for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++)
        {
            cp = &toCtx->client.array.a[i];
            if (cp->buffer && (cp->buffer->id || locked))
            {
                diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, cp->buffer->hwid);
                diff_api.VertexAttribPointerARB(i, cp->size, cp->type, cp->normalized, cp->stride, cp->p);
            }
        }
#endif
        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, to->arrayBuffer->hwid);
        diff_api.BindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, to->elementsBuffer->hwid);
#ifdef CR_ARB_pixel_buffer_object
        diff_api.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, to->packBuffer->hwid);
        diff_api.BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, to->unpackBuffer->hwid);
#endif
    }
    else
    {
        crStateBufferObjectIntCmp(bb, bitID, fromCtx, toCtx, GL_TRUE);
    }
}


