/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packspu.h"
#include "cr_packfunctions.h"
#include "cr_glstate.h"
#include "packspu_proto.h"
#include "cr_mem.h"

void PACKSPU_APIENTRY packspu_FogCoordPointerEXT( GLenum type, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
   if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackFogCoordPointerEXT( type, stride, pointer );
    }
#endif
    crStateFogCoordPointerEXT(&pack_spu.StateTracker, type, stride, pointer );
}

void PACKSPU_APIENTRY packspu_ColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackColorPointer( size, type, stride, pointer );
    }
#endif
    crStateColorPointer(&pack_spu.StateTracker, size, type, stride, pointer );
}

void PACKSPU_APIENTRY packspu_SecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackSecondaryColorPointerEXT( size, type, stride, pointer );
    }
#endif
    crStateSecondaryColorPointerEXT(&pack_spu.StateTracker, size, type, stride, pointer );
}

void PACKSPU_APIENTRY packspu_VertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    CRASSERT(ctx->clientState->extensions.ARB_vertex_buffer_object);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackVertexPointer( size, type, stride, pointer );
    }
#endif
    crStateVertexPointer(&pack_spu.StateTracker, size, type, stride, pointer );
}

void PACKSPU_APIENTRY packspu_TexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackTexCoordPointer( size, type, stride, pointer );
    }
#endif
    crStateTexCoordPointer(&pack_spu.StateTracker, size, type, stride, pointer );
}

void PACKSPU_APIENTRY packspu_NormalPointer( GLenum type, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackNormalPointer( type, stride, pointer );
    }
#endif
    crStateNormalPointer(&pack_spu.StateTracker, type, stride, pointer );
}

void PACKSPU_APIENTRY packspu_EdgeFlagPointer( GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackEdgeFlagPointer( stride, pointer );
    }
#endif
    crStateEdgeFlagPointer(&pack_spu.StateTracker, stride, pointer );
}

void PACKSPU_APIENTRY packspu_VertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackVertexAttribPointerARB( index, size, type, normalized, stride, pointer );
    }
#endif
    crStateVertexAttribPointerARB(&pack_spu.StateTracker, index, size, type, normalized, stride, pointer );
}

void PACKSPU_APIENTRY packspu_VertexAttribPointerNV( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackVertexAttribPointerNV( index, size, type, stride, pointer );
    }
#endif
    crStateVertexAttribPointerNV(&pack_spu.StateTracker, index, size, type, stride, pointer );
}

void PACKSPU_APIENTRY packspu_IndexPointer( GLenum type, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackIndexPointer( type, stride, pointer );
    }
#endif
    crStateIndexPointer(&pack_spu.StateTracker, type, stride, pointer);
}

void PACKSPU_APIENTRY packspu_GetPointerv( GLenum pname, GLvoid **params )
{
    crStateGetPointerv(&pack_spu.StateTracker, pname, params );
}

void PACKSPU_APIENTRY packspu_InterleavedArrays( GLenum format, GLsizei stride, const GLvoid *pointer )
{
#if CR_ARB_vertex_buffer_object
    GET_CONTEXT(ctx);
    if (ctx->clientState->extensions.ARB_vertex_buffer_object) {
        crPackInterleavedArrays( format, stride, pointer );
    }
#endif

    /*crDebug("packspu_InterleavedArrays");*/

    crStateInterleavedArrays(&pack_spu.StateTracker, format, stride, pointer );
}

#ifdef DEBUG_misha
/* debugging */
//# define CR_FORCE_ZVA_SERVER_ARRAY
#endif
# define CR_FORCE_ZVA_EXPAND


static GLboolean packspuZvaCreate(ContextInfo *pCtx, const GLfloat *pValue, GLuint cValues)
{
    ZvaBufferInfo *pInfo = &pCtx->zvaBufferInfo;
    GLuint cbValue = 4 * sizeof (*pValue);
    GLuint cbValues = cValues * cbValue;
    GLfloat *pBuffer;
    uint8_t *pu8Buf;
    GLuint i;

    /* quickly sort out if we can use the current value */
    if (pInfo->idBuffer
            && pInfo->cValues >= cValues
            && !crMemcmp(pValue, &pInfo->Value, cbValue))
        return GL_FALSE;

    pBuffer = (GLfloat*)crAlloc(cbValues);
    if (!pBuffer)
    {
        WARN(("crAlloc for pBuffer failed"));
        return GL_FALSE;
    }

    pu8Buf = (uint8_t *)pBuffer;
    for (i = 0; i < cValues; ++i)
    {
        crMemcpy(pu8Buf, pValue, cbValue);
        pu8Buf += cbValue;
    }

    /* */
    if (!pInfo->idBuffer)
    {
        pack_spu.self.GenBuffersARB(1, &pInfo->idBuffer);
        Assert(pInfo->idBuffer);
    }

    pack_spu.self.BindBufferARB(GL_ARRAY_BUFFER_ARB, pInfo->idBuffer);

    if (pInfo->cbBuffer < cbValues)
    {
        pack_spu.self.BufferDataARB(GL_ARRAY_BUFFER_ARB, cbValues, pBuffer, GL_DYNAMIC_DRAW_ARB);
        pInfo->cbBuffer = cbValues;
    }
    else
    {
        pack_spu.self.BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, cbValues, pBuffer);
    }

    pInfo->cValues = cValues;
    crMemcpy(&pInfo->Value, pValue, cbValue);

    crFree(pBuffer);

    return GL_TRUE;
}

typedef struct
{
    ContextInfo *pCtx;
    GLuint idBuffer;
    CRClientPointer cp;
} CR_ZVA_RESTORE_CTX;

static void packspuZvaEnable(ContextInfo *pCtx, const GLfloat *pValue, GLuint cValues, CR_ZVA_RESTORE_CTX *pRestoreCtx)
{
    CRContext *g = pCtx->clientState;

    Assert(0);

#ifdef DEBUG
    {
        CRContext *pCurState = crStateGetCurrent(&pack_spu.StateTracker);

        Assert(g == pCurState);
    }
#endif

    pRestoreCtx->pCtx = pCtx;
    pRestoreCtx->idBuffer = g->bufferobject.arrayBuffer ? g->bufferobject.arrayBuffer->id : 0;
    pRestoreCtx->cp = g->client.array.a[0];

    Assert(!pRestoreCtx->cp.enabled);

    /* buffer ref count mechanism does not work actually atm,
     * still ensure the buffer does not get destroyed if we fix it in the future */
    if (pRestoreCtx->cp.buffer)
        pRestoreCtx->cp.buffer->refCount++;

    packspuZvaCreate(pCtx, pValue, cValues);

    pack_spu.self.BindBufferARB(GL_ARRAY_BUFFER_ARB, pCtx->zvaBufferInfo.idBuffer);

    pack_spu.self.VertexAttribPointerARB(0, 4, GL_FLOAT,
            GL_FALSE, /*normalized*/
            0, /*stride*/
            NULL /*addr*/);

    pack_spu.self.EnableVertexAttribArrayARB(0);
}

static void packspuZvaDisable(CR_ZVA_RESTORE_CTX *pRestoreCtx)
{
    if (pRestoreCtx->cp.buffer)
        pack_spu.self.BindBufferARB(GL_ARRAY_BUFFER_ARB, pRestoreCtx->cp.buffer->id);
    else
        pack_spu.self.BindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    pack_spu.self.VertexAttribPointerARB(0, pRestoreCtx->cp.size, pRestoreCtx->cp.type,
            pRestoreCtx->cp.normalized, /*normalized*/
            pRestoreCtx->cp.stride, /*stride*/
            pRestoreCtx->cp.p);

    if (pRestoreCtx->cp.enabled)
        pack_spu.self.EnableVertexAttribArrayARB(0);
    else
        pack_spu.self.DisableVertexAttribArrayARB(0);

    if (pRestoreCtx->cp.buffer)
    {
        if (pRestoreCtx->cp.buffer->id != pRestoreCtx->idBuffer)
            pack_spu.self.BindBufferARB(GL_ARRAY_BUFFER_ARB, pRestoreCtx->idBuffer);

        /* we have increased the refcount above, decrease it back */
        pRestoreCtx->cp.buffer->refCount--;
    }
    else
    {
        if (pRestoreCtx->idBuffer)
            pack_spu.self.BindBufferARB(GL_ARRAY_BUFFER_ARB, pRestoreCtx->idBuffer);
    }

#ifdef DEBUG
    {
        CRContext *g = pRestoreCtx->pCtx->clientState;
        CRContext *pCurState = crStateGetCurrent(&pack_spu.StateTracker);

        Assert(g == pCurState);

        Assert(pRestoreCtx->cp.p == g->client.array.a[0].p);
        Assert(pRestoreCtx->cp.size == g->client.array.a[0].size);
        Assert(pRestoreCtx->cp.type == g->client.array.a[0].type);
        Assert(pRestoreCtx->cp.stride == g->client.array.a[0].stride);
        Assert(pRestoreCtx->cp.enabled == g->client.array.a[0].enabled);
        Assert(pRestoreCtx->cp.normalized == g->client.array.a[0].normalized);
        Assert(pRestoreCtx->cp.bytesPerIndex == g->client.array.a[0].bytesPerIndex);
# ifdef CR_ARB_vertex_buffer_object
        Assert(pRestoreCtx->cp.buffer == g->client.array.a[0].buffer);
# endif
# ifdef CR_EXT_compiled_vertex_array
        Assert(pRestoreCtx->cp.locked == g->client.array.a[0].locked);
# endif
        Assert(pRestoreCtx->idBuffer == (g->bufferobject.arrayBuffer ? g->bufferobject.arrayBuffer->id : 0));
    }
#endif
}

void PACKSPU_APIENTRY
packspu_ArrayElement( GLint index )
{
/** @todo cash guest/host pointers calculation and use appropriate path here without crStateUseServerArrays call*/
#if 1
    GLboolean serverArrays = GL_FALSE;
    GLuint cZvaValues = 0;
    GLfloat aAttrib[4];

#if CR_ARB_vertex_buffer_object
    {
        GET_CONTEXT(ctx);
        /*crDebug("packspu_ArrayElement index:%i", index);*/
        if (ctx->clientState->extensions.ARB_vertex_buffer_object)
        {
            serverArrays = crStateUseServerArrays(&pack_spu.StateTracker);
            if (ctx->fCheckZerroVertAttr)
                cZvaValues = crStateNeedDummyZeroVertexArray(thread->currentContext->clientState, &thread->packer->current, aAttrib);
        }
    }
#endif

    if (serverArrays
#ifdef CR_FORCE_ZVA_EXPAND
            && !cZvaValues
#endif
            ) {
        CR_ZVA_RESTORE_CTX RestoreCtx;
        GET_CONTEXT(ctx);
        CRClientState *clientState = &(ctx->clientState->client);

        Assert(cZvaValues < UINT32_MAX/2);

        /* LockArraysEXT can not be executed between glBegin/glEnd pair, it also
         * leads to vertexpointers being adjusted on the host side between glBegin/glEnd calls which
         * produces unpredictable results. Locking is done before the glBegin call instead.
         */
        CRASSERT(!clientState->array.locked || clientState->array.synced);

        if (cZvaValues)
            packspuZvaEnable(ctx, aAttrib, cZvaValues, &RestoreCtx);

        /* Send the DrawArrays command over the wire */
        crPackArrayElement( index );

        if (cZvaValues)
            packspuZvaDisable(&RestoreCtx);
    }
    else {
        /* evaluate locally */
        GET_CONTEXT(ctx);
        CRClientState *clientState = &(ctx->clientState->client);

#ifdef CR_FORCE_ZVA_SERVER_ARRAY
        CR_ZVA_RESTORE_CTX RestoreCtx;

        if (cZvaValues && cZvaValues < UINT32_MAX/2)
            packspuZvaEnable(ctx, aAttrib, cZvaValues, &RestoreCtx);
#endif

        crPackExpandArrayElement( index, clientState, cZvaValues ? aAttrib : NULL );

#ifdef CR_FORCE_ZVA_SERVER_ARRAY
        if (cZvaValues && cZvaValues < UINT32_MAX/2)
            packspuZvaDisable(&RestoreCtx);
#endif
    }
#else
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);
    crPackExpandArrayElement(index, clientState, NULL);
#endif
}

/*#define CR_USE_LOCKARRAYS*/
#ifdef CR_USE_LOCKARRAYS
# error "check Zero Vertex Attrib hack is supported properly!"
#endif

void PACKSPU_APIENTRY
packspu_DrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
    GLboolean serverArrays = GL_FALSE;
    GLuint cZvaValues = 0;
    GLfloat aAttrib[4];

#if CR_ARB_vertex_buffer_object
    GLboolean lockedArrays = GL_FALSE;
    CRBufferObject *elementsBuffer;
    {
        GET_CONTEXT(ctx);
        elementsBuffer = crStateGetCurrent(&pack_spu.StateTracker)->bufferobject.elementsBuffer;
        /*crDebug("DrawElements count=%d, indices=%p", count, indices);*/
        if (ctx->clientState->extensions.ARB_vertex_buffer_object)
        {
            serverArrays = crStateUseServerArrays(&pack_spu.StateTracker);
            if (ctx->fCheckZerroVertAttr)
                cZvaValues = crStateNeedDummyZeroVertexArray(thread->currentContext->clientState, &thread->packer->current, aAttrib);
        }
    }

# ifdef CR_USE_LOCKARRAYS
    if (!serverArrays && !ctx->clientState->client.array.locked && (count>3)
        && (!elementsBuffer || !elementsBuffer->id))
    {
        GLuint min, max;
        GLsizei i;

        switch (type)
        {
            case GL_UNSIGNED_BYTE:
            {
                GLubyte *pIdx = (GLubyte *)indices;
                min = max = pIdx[0];
                for (i=0; i<count; ++i)
                {
                    if (pIdx[i]<min) min = pIdx[i];
                    else if (pIdx[i]>max) max = pIdx[i];
                }
                break;
            }
            case GL_UNSIGNED_SHORT:
            {
                GLushort *pIdx = (GLushort *)indices;
                min = max = pIdx[0];
                for (i=0; i<count; ++i)
                {
                    if (pIdx[i]<min) min = pIdx[i];
                    else if (pIdx[i]>max) max = pIdx[i];
                }
                break;
            }
            case GL_UNSIGNED_INT:
            {
                GLuint *pIdx = (GLuint *)indices;
                min = max = pIdx[0];
                for (i=0; i<count; ++i)
                {
                    if (pIdx[i]<min) min = pIdx[i];
                    else if (pIdx[i]>max) max = pIdx[i];
                }
                break;
            }
            default: crError("Unknown type 0x%x", type);
        }

        if ((max-min)<(GLuint)(2*count))
        {
            crStateLockArraysEXT(&pack_spu.StateTracker, min, max-min+1);

            serverArrays = crStateUseServerArrays();
            if (serverArrays)
            {
                lockedArrays = GL_TRUE;
            }
            else
            {
                crStateUnlockArraysEXT(&pack_spu.StateTracker);
            }
        }
    }
# endif
#endif

    if (serverArrays
#ifdef CR_FORCE_ZVA_EXPAND
            && !cZvaValues
#endif
            ) {
        CR_ZVA_RESTORE_CTX RestoreCtx;
        GET_CONTEXT(ctx);
        CRClientState *clientState = &(ctx->clientState->client);

        Assert(cZvaValues < UINT32_MAX/2);

        if (cZvaValues)
            packspuZvaEnable(ctx, aAttrib, cZvaValues, &RestoreCtx);

        /*Note the comment in packspu_LockArraysEXT*/
        if (clientState->array.locked && !clientState->array.synced)
        {
            crPackLockArraysEXT(clientState->array.lockFirst, clientState->array.lockCount);
            clientState->array.synced = GL_TRUE;
        }

        /* Send the DrawArrays command over the wire */
        crPackDrawElements( mode, count, type, indices );

        if (cZvaValues)
            packspuZvaDisable(&RestoreCtx);
    }
    else {
        /* evaluate locally */
        GET_CONTEXT(ctx);
        CRClientState *clientState = &(ctx->clientState->client);

#ifdef CR_FORCE_ZVA_SERVER_ARRAY
        CR_ZVA_RESTORE_CTX RestoreCtx;

        if (cZvaValues && cZvaValues < UINT32_MAX/2)
            packspuZvaEnable(ctx, aAttrib, cZvaValues, &RestoreCtx);
#endif

        //packspu_Begin(mode);
        crPackExpandDrawElements( mode, count, type, indices, clientState, cZvaValues ? aAttrib : NULL );
        //packspu_End();

#ifdef CR_FORCE_ZVA_SERVER_ARRAY
        if (cZvaValues && cZvaValues < UINT32_MAX/2)
            packspuZvaDisable(&RestoreCtx);
#endif
    }

#if CR_ARB_vertex_buffer_object
    if (lockedArrays)
    {
        packspu_UnlockArraysEXT();
    }
#endif
}


void PACKSPU_APIENTRY
packspu_DrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices )
{
    GLboolean serverArrays = GL_FALSE;
    GLuint cZvaValues = 0;
    GLfloat aAttrib[4];

#if CR_ARB_vertex_buffer_object
    {
        GET_CONTEXT(ctx);
        /*crDebug("DrawRangeElements count=%d", count);*/
        if (ctx->clientState->extensions.ARB_vertex_buffer_object)
        {
             serverArrays = crStateUseServerArrays(&pack_spu.StateTracker);
             if (ctx->fCheckZerroVertAttr)
                 cZvaValues = crStateNeedDummyZeroVertexArray(thread->currentContext->clientState, &thread->packer->current, aAttrib);
        }
    }
#endif

    if (serverArrays
#ifdef CR_FORCE_ZVA_EXPAND
            && !cZvaValues
#endif
            ) {
        CR_ZVA_RESTORE_CTX RestoreCtx;
        GET_CONTEXT(ctx);
        CRClientState *clientState = &(ctx->clientState->client);

        Assert(cZvaValues < UINT32_MAX/2);

        if (cZvaValues)
            packspuZvaEnable(ctx, aAttrib, cZvaValues, &RestoreCtx);

        /*Note the comment in packspu_LockArraysEXT*/
        if (clientState->array.locked && !clientState->array.synced)
        {
            crPackLockArraysEXT(clientState->array.lockFirst, clientState->array.lockCount);
            clientState->array.synced = GL_TRUE;
        }

        /* Send the DrawRangeElements command over the wire */
        crPackDrawRangeElements( mode, start, end, count, type, indices );

        if (cZvaValues)
            packspuZvaDisable(&RestoreCtx);
    }
    else {
        /* evaluate locally */
        GET_CONTEXT(ctx);
        CRClientState *clientState = &(ctx->clientState->client);
#ifdef CR_FORCE_ZVA_SERVER_ARRAY
        CR_ZVA_RESTORE_CTX RestoreCtx;

        if (cZvaValues && cZvaValues < UINT32_MAX/2)
            packspuZvaEnable(ctx, aAttrib, cZvaValues, &RestoreCtx);
#endif

        crPackExpandDrawRangeElements( mode, start, end, count, type, indices, clientState, cZvaValues ? aAttrib : NULL );

#ifdef CR_FORCE_ZVA_SERVER_ARRAY
        if (cZvaValues && cZvaValues < UINT32_MAX/2)
            packspuZvaDisable(&RestoreCtx);
#endif
    }
}


void PACKSPU_APIENTRY
packspu_DrawArrays( GLenum mode, GLint first, GLsizei count )
{
    GLboolean serverArrays = GL_FALSE;
    GLuint cZvaValues = 0;
    GLfloat aAttrib[4];

#if CR_ARB_vertex_buffer_object
    GLboolean lockedArrays = GL_FALSE;
    {
        GET_CONTEXT(ctx);
        /*crDebug("DrawArrays count=%d", count);*/
        if (ctx->clientState->extensions.ARB_vertex_buffer_object)
        {
             serverArrays = crStateUseServerArrays(&pack_spu.StateTracker);
             if (ctx->fCheckZerroVertAttr)
                 cZvaValues = crStateNeedDummyZeroVertexArray(thread->currentContext->clientState, &thread->packer->current, aAttrib);
        }
    }

# ifdef CR_USE_LOCKARRAYS
    if (!serverArrays && !ctx->clientState->client.array.locked && (count>3))
    {
        crStateLockArraysEXT(&pack_spu.StateTracker, first, count);
        serverArrays = crStateUseServerArrays(&pack_spu.StateTracker);
        if (serverArrays)
        {
            lockedArrays = GL_TRUE;
        }
        else
        {
            crStateUnlockArraysEXT(&pack_spu.StateTracker);
        }
    }
# endif
#endif

    if (serverArrays
#ifdef CR_FORCE_ZVA_EXPAND
            && !cZvaValues
#endif
            )
    {
        CR_ZVA_RESTORE_CTX RestoreCtx;
        GET_CONTEXT(ctx);
        CRClientState *clientState = &(ctx->clientState->client);

        Assert(cZvaValues < UINT32_MAX/2);

        if (cZvaValues)
            packspuZvaEnable(ctx, aAttrib, cZvaValues, &RestoreCtx);

        /*Note the comment in packspu_LockArraysEXT*/
        if (clientState->array.locked && !clientState->array.synced)
        {
            crPackLockArraysEXT(clientState->array.lockFirst, clientState->array.lockCount);
            clientState->array.synced = GL_TRUE;
        }

        /* Send the DrawArrays command over the wire */
        crPackDrawArrays( mode, first, count );

        if (cZvaValues)
            packspuZvaDisable(&RestoreCtx);
    }
    else
    {
        /* evaluate locally */
        GET_CONTEXT(ctx);
        CRClientState *clientState = &(ctx->clientState->client);
#ifdef CR_FORCE_ZVA_SERVER_ARRAY
        CR_ZVA_RESTORE_CTX RestoreCtx;

        if (cZvaValues && cZvaValues < UINT32_MAX/2)
            packspuZvaEnable(ctx, aAttrib, cZvaValues, &RestoreCtx);
#endif

        crPackExpandDrawArrays( mode, first, count, clientState, cZvaValues ? aAttrib : NULL );

#ifdef CR_FORCE_ZVA_SERVER_ARRAY
        if (cZvaValues && cZvaValues < UINT32_MAX/2)
            packspuZvaDisable(&RestoreCtx);
#endif

    }

#if CR_ARB_vertex_buffer_object
    if (lockedArrays)
    {
        packspu_UnlockArraysEXT();
    }
#endif
}


#ifdef CR_EXT_multi_draw_arrays
void PACKSPU_APIENTRY packspu_MultiDrawArraysEXT( GLenum mode, GLint *first, GLsizei *count, GLsizei primcount )
{
    GLint i;
    for (i = 0; i < primcount; i++) {
        if (count[i] > 0) {
            packspu_DrawArrays(mode, first[i], count[i]);
        }
    }
}

void PACKSPU_APIENTRY packspu_MultiDrawElementsEXT( GLenum mode, const GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount )
{
    GLint i;
    for (i = 0; i < primcount; i++) {
        if (count[i] > 0) {
            packspu_DrawElements(mode, count[i], type, indices[i]);
        }
    }
}
#endif


void PACKSPU_APIENTRY packspu_EnableClientState( GLenum array )
{
    crStateEnableClientState(&pack_spu.StateTracker, array);
    crPackEnableClientState(array);
}

void PACKSPU_APIENTRY packspu_DisableClientState( GLenum array )
{
    crStateDisableClientState(&pack_spu.StateTracker, array);
    crPackDisableClientState(array);
}

void PACKSPU_APIENTRY packspu_ClientActiveTextureARB( GLenum texUnit )
{
    crStateClientActiveTextureARB(&pack_spu.StateTracker, texUnit);
    crPackClientActiveTextureARB(texUnit);
}

void PACKSPU_APIENTRY packspu_EnableVertexAttribArrayARB(GLuint index)
{
    crStateEnableVertexAttribArrayARB(&pack_spu.StateTracker, index);
    crPackEnableVertexAttribArrayARB(index);
}


void PACKSPU_APIENTRY packspu_DisableVertexAttribArrayARB(GLuint index)
{
    crStateDisableVertexAttribArrayARB(&pack_spu.StateTracker, index);
    crPackDisableVertexAttribArrayARB(index);
}

void PACKSPU_APIENTRY packspu_Enable( GLenum cap )
{
    if (cap!=GL_LIGHT_MODEL_TWO_SIDE)
    {
        crStateEnable(&pack_spu.StateTracker, cap);
        crPackEnable(cap);
    }
    else
    {
        static int g_glmts1_warn=0;
        if (!g_glmts1_warn)
        {
            crWarning("glEnable(GL_LIGHT_MODEL_TWO_SIDE) converted to valid glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,1)");
            g_glmts1_warn=1;
        }
        crStateLightModeli(&pack_spu.StateTracker, GL_LIGHT_MODEL_TWO_SIDE, 1);
        crPackLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    }
}


void PACKSPU_APIENTRY packspu_Disable( GLenum cap )
{
    if (cap!=GL_LIGHT_MODEL_TWO_SIDE)
    {
        crStateDisable(&pack_spu.StateTracker, cap);
        crPackDisable(cap);
    }
    else
    {
        static int g_glmts0_warn=0;
        if (!g_glmts0_warn)
        {
            crWarning("glDisable(GL_LIGHT_MODEL_TWO_SIDE) converted to valid glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0)");
            g_glmts0_warn=1;
        }
        crStateLightModeli(&pack_spu.StateTracker, GL_LIGHT_MODEL_TWO_SIDE, 0);
        crPackLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
    }
}

GLboolean PACKSPU_APIENTRY packspu_IsEnabled(GLenum cap)
{
    GLboolean res = crStateIsEnabled(&pack_spu.StateTracker, cap);
#ifdef DEBUG
    {
    	GET_THREAD(thread);
	    int writeback = 1;
	    GLboolean return_val = (GLboolean) 0;
        crPackIsEnabled(cap, &return_val, &writeback);
	    packspuFlush( (void *) thread );
	    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
        CRASSERT(return_val==res);
    }
#endif

    return res;
}

void PACKSPU_APIENTRY packspu_PushClientAttrib( GLbitfield mask )
{
    crStatePushClientAttrib(&pack_spu.StateTracker, mask);
    crPackPushClientAttrib(mask);
}

void PACKSPU_APIENTRY packspu_PopClientAttrib( void )
{
    crStatePopClientAttrib(&pack_spu.StateTracker);
    crPackPopClientAttrib();
}

void PACKSPU_APIENTRY packspu_LockArraysEXT(GLint first, GLint count)
{
    if (first>=0 && count>0)
    {
        crStateLockArraysEXT(&pack_spu.StateTracker, first, count);
        /*Note: this is a workaround for quake3 based apps.
          It's modifying vertex data between glLockArraysEXT and glDrawElements calls,
          so we'd pass data to host right before the glDrawSomething or glBegin call.
        */
        /*crPackLockArraysEXT(first, count);*/
    }
    else crDebug("Ignoring packspu_LockArraysEXT: first:%i, count:%i", first, count);
}

void PACKSPU_APIENTRY packspu_UnlockArraysEXT()
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (clientState->array.locked && clientState->array.synced)
    {
        crPackUnlockArraysEXT();
    }

    crStateUnlockArraysEXT(&pack_spu.StateTracker);
}
