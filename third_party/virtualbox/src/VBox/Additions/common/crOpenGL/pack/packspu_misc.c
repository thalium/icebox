/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_packfunctions.h"
#include "packspu.h"
#include "packspu_proto.h"
#include "cr_mem.h"

void PACKSPU_APIENTRY packspu_ChromiumParametervCR(GLenum target, GLenum type, GLsizei count, const GLvoid *values)
{

    CRMessage msg;
    int len;
    GLint ai32ServerValues[2];
    GLboolean fFlush = GL_FALSE;
    GET_THREAD(thread);


    switch(target)
    {
        case GL_GATHER_PACK_CR:
            /* flush the current pack buffer */
            packspuFlush( (void *) thread );

            /* the connection is thread->server.conn */
            msg.header.type = CR_MESSAGE_GATHER;
            msg.gather.offset = 69;
            len = sizeof(CRMessageGather);
            crNetSend(thread->netServer.conn, NULL, &msg, len);
            return;

        case GL_SHARE_LISTS_CR:
        {
            ContextInfo *pCtx[2];
            GLint *ai32Values;
            int i;
            if (count != 2)
            {
                WARN(("GL_SHARE_LISTS_CR invalid cound %d", count));
                return;
            }

            if (type != GL_UNSIGNED_INT && type != GL_INT)
            {
                WARN(("GL_SHARE_LISTS_CR invalid type %d", type));
                return;
            }

            ai32Values = (GLint*)values;

            for (i = 0; i < 2; ++i)
            {
                const int slot = ai32Values[i] - MAGIC_OFFSET;

                if (slot < 0 || slot >= pack_spu.numContexts)
                {
                    WARN(("GL_SHARE_LISTS_CR invalid value[%d] %d", i, ai32Values[i]));
                    return;
                }

                pCtx[i] = &pack_spu.context[slot];
                if (!pCtx[i]->clientState)
                {
                    WARN(("GL_SHARE_LISTS_CR invalid pCtx1 for value[%d] %d", i, ai32Values[i]));
                    return;
                }

                ai32ServerValues[i] = pCtx[i]->serverCtx;
            }

            crStateShareLists(pCtx[0]->clientState, pCtx[1]->clientState);

            values = ai32ServerValues;

            fFlush = GL_TRUE;

            break;
        }

        default:
            break;
    }

    crPackChromiumParametervCR(target, type, count, values);

    if (fFlush)
        packspuFlush( (void *) thread );
}

GLboolean packspuSyncOnFlushes(void)
{
#if 1 /*Seems to still cause issues, always sync for now*/
    return 1;
#else
    GLint buffer;

    crStateGetIntegerv(&pack_spu.StateTracker, GL_DRAW_BUFFER, &buffer);
    /*Usually buffer==GL_BACK, so put this extra check to simplify boolean eval on runtime*/
    return  (buffer != GL_BACK)
            && (buffer == GL_FRONT_LEFT
                || buffer == GL_FRONT_RIGHT
                || buffer == GL_FRONT
                || buffer == GL_FRONT_AND_BACK
                || buffer == GL_LEFT
                || buffer == GL_RIGHT);
#endif
}

void PACKSPU_APIENTRY packspu_DrawBuffer(GLenum mode)
{
    GLboolean hadtoflush;

    hadtoflush = packspuSyncOnFlushes();

    crStateDrawBuffer(&pack_spu.StateTracker, mode);
    crPackDrawBuffer(mode);

    if (hadtoflush && !packspuSyncOnFlushes())
        packspu_Flush();
}

void PACKSPU_APIENTRY packspu_Finish( void )
{
    GET_THREAD(thread);
    GLint writeback = CRPACKSPU_IS_WDDM_CRHGSMI() ? 1 : pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network;

    crPackFinish();
    if (packspuSyncOnFlushes())
    {
        if (writeback)
        {
            crPackWriteback(&writeback);

            packspuFlush( (void *) thread );

            CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
        }
    }
}

void PACKSPU_APIENTRY packspu_Flush( void )
{
    GET_THREAD(thread);
    int writeback=1;
    int found=0;

    if (!thread->bInjectThread)
    {
        crPackFlush();
        if (packspuSyncOnFlushes())
        {
            crPackWriteback(&writeback);
            packspuFlush( (void *) thread );
            CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
        }
    }
    else
    {
        int i;

        crLockMutex(&_PackMutex);

        /*Make sure we process commands in order they should appear, so flush other threads first*/
        for (i=0; i<MAX_THREADS; ++i)
        {
            if (pack_spu.thread[i].inUse
                && (thread != &pack_spu.thread[i]) && pack_spu.thread[i].netServer.conn
                && pack_spu.thread[i].packer && pack_spu.thread[i].packer->currentBuffer)
            {
                packspuFlush((void *) &pack_spu.thread[i]);

                if (pack_spu.thread[i].netServer.conn->u32ClientID == thread->netServer.conn->u32InjectClientID)
                {
                    found=1;
                }

            }
        }

        if (!found)
        {
            /*Thread we're supposed to inject commands for has been detached,
              so there's nothing to sync with and we should just pass commands through our own connection.
             */
            thread->netServer.conn->u32InjectClientID=0;
        }

        packspuFlush((void *) thread);

        crUnlockMutex(&_PackMutex);
    }
}

void PACKSPU_APIENTRY packspu_NewList(GLuint list, GLenum mode)
{
    crStateNewList(&pack_spu.StateTracker, list, mode);
    crPackNewList(list, mode);
}

void PACKSPU_APIENTRY packspu_EndList()
{
    crStateEndList(&pack_spu.StateTracker);
    crPackEndList();
}

void PACKSPU_APIENTRY packspu_VBoxWindowDestroy( GLint con, GLint window )
{
    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        GET_THREAD(thread);
        if (con)
        {
            CRPackContext * curPacker = crPackGetContext();
            CRASSERT(!thread || !thread->bInjectThread);
            thread = GET_THREAD_VAL_ID(con);
            crPackSetContext(thread->packer);
            crPackWindowDestroy(window);
            if (curPacker != thread->packer)
                crPackSetContext(curPacker);
            return;
        }
        CRASSERT(thread);
        CRASSERT(thread->bInjectThread);
    }
    crPackWindowDestroy(window);
}

GLint PACKSPU_APIENTRY packspu_VBoxWindowCreate( GLint con, const char *dpyName, GLint visBits )
{
    GET_THREAD(thread);
    static int num_calls = 0;
    int writeback = CRPACKSPU_IS_WDDM_CRHGSMI() ? 1 : pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network;
    GLint return_val = (GLint) 0;
    ThreadInfo *curThread = thread;
    GLint retVal;

    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        if (!con)
        {
            crError("connection expected!");
            return 0;
        }
        thread = GET_THREAD_VAL_ID(con);
    }
    else
    {
        CRASSERT(!con);
        if (!thread) {
            thread = packspuNewThread(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                NULL
#endif
                );
        }
    }
    CRASSERT(thread);
    CRASSERT(thread->packer);
    CRASSERT(crPackGetContext() == (curThread ? curThread->packer : NULL));

    crPackSetContext(thread->packer);
    crPackWindowCreate( dpyName, visBits, &return_val, &writeback );
    packspuFlush(thread);
    if (!(thread->netServer.conn->actual_network))
    {
        retVal = num_calls++;
    }
    else
    {
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
        retVal = return_val;
    }

    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        if (thread != curThread)
        {
            if (curThread)
                crPackSetContext(curThread->packer);
            else
                crPackSetContext(NULL);
        }
    }

    return retVal;
}

GLint PACKSPU_APIENTRY packspu_WindowCreate( const char *dpyName, GLint visBits )
{
    return packspu_VBoxWindowCreate( 0, dpyName, visBits );
}

GLboolean PACKSPU_APIENTRY
packspu_AreTexturesResident( GLsizei n, const GLuint * textures,
                                                         GLboolean * residences )
{
    GET_THREAD(thread);
    int writeback = 1;
    GLboolean return_val = GL_TRUE;
    GLsizei i;

    if (!CRPACKSPU_IS_WDDM_CRHGSMI() && !(pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network))
    {
        crError( "packspu_AreTexturesResident doesn't work when there's no actual network involved!\nTry using the simplequery SPU in your chain!" );
    }

    crPackAreTexturesResident( n, textures, residences, &return_val, &writeback );
    packspuFlush( (void *) thread );

    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);

    /* Since the Chromium packer/unpacker can't return both 'residences'
     * and the function's return value, compute the return value here.
     */
    for (i = 0; i < n; i++) {
        if (!residences[i]) {
            return_val = GL_FALSE;
            break;
        }
    }

    return return_val;
}


GLboolean PACKSPU_APIENTRY
packspu_AreProgramsResidentNV( GLsizei n, const GLuint * ids,
                                                             GLboolean * residences )
{
    GET_THREAD(thread);
    int writeback = 1;
    GLboolean return_val = GL_TRUE;
    GLsizei i;

    if (!CRPACKSPU_IS_WDDM_CRHGSMI() && !(pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network))
    {
        crError( "packspu_AreProgramsResidentNV doesn't work when there's no actual network involved!\nTry using the simplequery SPU in your chain!" );
    }

    crPackAreProgramsResidentNV( n, ids, residences, &return_val, &writeback );
    packspuFlush( (void *) thread );

    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);

    /* Since the Chromium packer/unpacker can't return both 'residences'
     * and the function's return value, compute the return value here.
     */
    for (i = 0; i < n; i++) {
        if (!residences[i]) {
            return_val = GL_FALSE;
            break;
        }
    }

    return return_val;
}

void PACKSPU_APIENTRY packspu_GetPolygonStipple( GLubyte * mask )
{
    GET_THREAD(thread);
    int writeback = 1;

    crPackGetPolygonStipple( mask, &writeback );
#ifdef CR_ARB_pixel_buffer_object
    if (!crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
#endif
    {
        packspuFlush( (void *) thread );
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    }
}

void PACKSPU_APIENTRY packspu_GetPixelMapfv( GLenum map, GLfloat * values )
{
    GET_THREAD(thread);
    int writeback = 1;

    crPackGetPixelMapfv( map, values, &writeback );
#ifdef CR_ARB_pixel_buffer_object
    if (!crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
#endif
    {
        packspuFlush( (void *) thread );
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    }
}

void PACKSPU_APIENTRY packspu_GetPixelMapuiv( GLenum map, GLuint * values )
{
    GET_THREAD(thread);
    int writeback = 1;

    crPackGetPixelMapuiv( map, values, &writeback );

#ifdef CR_ARB_pixel_buffer_object
    if (!crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
#endif
    {
        packspuFlush( (void *) thread );
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    }
}

void PACKSPU_APIENTRY packspu_GetPixelMapusv( GLenum map, GLushort * values )
{
    GET_THREAD(thread);
    int writeback = 1;

    crPackGetPixelMapusv( map, values, &writeback );
#ifdef CR_ARB_pixel_buffer_object
    if (!crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
#endif
    {
        packspuFlush( (void *) thread );
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    }
}

static void packspuFluchOnThreadSwitch(GLboolean fEnable)
{
    GET_THREAD(thread);
    if (thread->currentContext->fAutoFlush == fEnable)
        return;

    thread->currentContext->fAutoFlush = fEnable;
    thread->currentContext->currentThread = fEnable ? thread : NULL;
}

static void packspuCheckZerroVertAttr(GLboolean fEnable)
{
    GET_THREAD(thread);

    thread->currentContext->fCheckZerroVertAttr = fEnable;
}

void PACKSPU_APIENTRY packspu_ChromiumParameteriCR(GLenum target, GLint value)
{
    switch (target)
    {
        case GL_FLUSH_ON_THREAD_SWITCH_CR:
            /* this is a pure packspu state, don't propagate it any further */
            packspuFluchOnThreadSwitch(value);
            return;
        case GL_CHECK_ZERO_VERT_ARRT:
            packspuCheckZerroVertAttr(value);
            return;
        case GL_SHARE_CONTEXT_RESOURCES_CR:
            crStateShareContext(&pack_spu.StateTracker, value);
            break;
        case GL_RCUSAGE_TEXTURE_SET_CR:
        {
            Assert(value);
            crStateSetTextureUsed(&pack_spu.StateTracker, value, GL_TRUE);
            break;
        }
        case GL_RCUSAGE_TEXTURE_CLEAR_CR:
        {
            Assert(value);
#ifdef DEBUG
            {
                CRContext *pCurState = crStateGetCurrent(&pack_spu.StateTracker);
                CRTextureObj *tobj = (CRTextureObj*)crHashtableSearch(pCurState->shared->textureTable, value);
                Assert(tobj);
            }
#endif
            crStateSetTextureUsed(&pack_spu.StateTracker, value, GL_FALSE);
            break;
        }
        default:
            break;
    }
    crPackChromiumParameteriCR(target, value);
}

GLenum PACKSPU_APIENTRY packspu_GetError( void )
{
    GET_THREAD(thread);
    int writeback = 1;
    GLenum return_val = (GLenum) 0;
    CRContext *pCurState = crStateGetCurrent(&pack_spu.StateTracker);
    NOREF(pCurState); /* it's unused, but I don't know about side effects.. */

    if (!CRPACKSPU_IS_WDDM_CRHGSMI() && !(pack_spu.thread[pack_spu.idxThreadInUse].netServer.conn->actual_network))
    {
        crError( "packspu_GetError doesn't work when there's no actual network involved!\nTry using the simplequery SPU in your chain!" );
    }

    crPackGetError( &return_val, &writeback );
    packspuFlush( (void *) thread );
    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    return return_val;
}

GLint PACKSPU_APIENTRY packspu_VBoxPackSetInjectThread(struct VBOXUHGSMI *pHgsmi)
{
    GLint con = 0;
    int i;
    GET_THREAD(thread);
    CRASSERT(!thread);
    RT_NOREF(pHgsmi);
    crLockMutex(&_PackMutex);
    {
        CRASSERT(CRPACKSPU_IS_WDDM_CRHGSMI() || (pack_spu.numThreads>0));
        CRASSERT(pack_spu.numThreads<MAX_THREADS);
        for (i=0; i<MAX_THREADS; ++i)
        {
            if (!pack_spu.thread[i].inUse)
            {
                thread = &pack_spu.thread[i];
                break;
            }
        }
        CRASSERT(thread);

        thread->inUse = GL_TRUE;
        if (!CRPACKSPU_IS_WDDM_CRHGSMI())
            thread->id = crThreadID();
        else
            thread->id = THREAD_OFFSET_MAGIC + i;
        thread->currentContext = NULL;
        thread->bInjectThread = GL_TRUE;

        thread->netServer.name = crStrdup(pack_spu.name);
        thread->netServer.buffer_size = 64 * 1024;

        packspuConnectToServer(&(thread->netServer)
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , pHgsmi
#endif
        );
        CRASSERT(thread->netServer.conn);

        CRASSERT(thread->packer == NULL);
        thread->packer = crPackNewContext();
        CRASSERT(thread->packer);
        crPackInitBuffer(&(thread->buffer), crNetAlloc(thread->netServer.conn),
                         thread->netServer.conn->buffer_size, thread->netServer.conn->mtu);
        thread->buffer.canBarf = thread->netServer.conn->Barf ? GL_TRUE : GL_FALSE;

        crPackSetBuffer( thread->packer, &thread->buffer );
        crPackFlushFunc( thread->packer, packspuFlush );
        crPackFlushArg( thread->packer, (void *) thread );
        crPackSendHugeFunc( thread->packer, packspuHuge );
        crPackSetContext( thread->packer );

        crSetTSD(&_PackTSD, thread);

        pack_spu.numThreads++;
    }
    crUnlockMutex(&_PackMutex);

    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        CRASSERT(thread->id - THREAD_OFFSET_MAGIC < RT_ELEMENTS(pack_spu.thread)
                && GET_THREAD_VAL_ID(thread->id) == thread);
        con = thread->id;
    }
    return con;
}

GLuint PACKSPU_APIENTRY packspu_VBoxPackGetInjectID(GLint con)
{
    GLuint ret;

    crLockMutex(&_PackMutex);
    {
        ThreadInfo *thread = NULL;
        if (CRPACKSPU_IS_WDDM_CRHGSMI())
        {
            if (!con)
            {
                crError("connection expected!");
                return 0;
            }
            thread = GET_THREAD_VAL_ID(con);
        }
        else
        {
            CRASSERT(!con);
            thread = GET_THREAD_VAL();
        }
        CRASSERT(thread && thread->netServer.conn && thread->netServer.conn->type==CR_VBOXHGCM);
        ret = thread->netServer.conn->u32ClientID;
    }
    crUnlockMutex(&_PackMutex);

    return ret;
}

void PACKSPU_APIENTRY packspu_VBoxPackSetInjectID(GLuint id)
{
    crLockMutex(&_PackMutex);
    {
        GET_THREAD(thread);

        CRASSERT(thread && thread->netServer.conn && thread->netServer.conn->type==CR_VBOXHGCM && thread->bInjectThread);
        thread->netServer.conn->u32InjectClientID = id;
    }
    crUnlockMutex(&_PackMutex);
}

void PACKSPU_APIENTRY packspu_VBoxAttachThread()
{
#if 0
    int i;
    GET_THREAD(thread);

    for (i=0; i<MAX_THREADS; ++i)
    {
        if (pack_spu.thread[i].inUse && thread==&pack_spu.thread[i] && thread->id==crThreadID())
        {
            crError("2nd attach to same thread");
        }
    }
#endif

    crSetTSD(&_PackTSD, NULL);

    crStateVBoxAttachThread(&pack_spu.StateTracker);
}

void PACKSPU_APIENTRY packspu_VBoxDetachThread()
{
    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        crPackSetContext(NULL);
        crSetTSD(&_PackTSD, NULL);
    }
    else
    {
        int i;
        GET_THREAD(thread);
        if (thread)
        {
            crLockMutex(&_PackMutex);

            for (i=0; i<MAX_THREADS; ++i)
            {
                if (pack_spu.thread[i].inUse && thread==&pack_spu.thread[i]
                    && thread->id==crThreadID() && thread->netServer.conn)
                {
                    CRASSERT(pack_spu.numThreads>0);

                    packspuFlush((void *) thread);

                    if (pack_spu.thread[i].packer)
                    {
                        CR_LOCK_PACKER_CONTEXT(thread->packer);
                        crPackSetContext(NULL);
                        CR_UNLOCK_PACKER_CONTEXT(thread->packer);
                        crPackDeleteContext(pack_spu.thread[i].packer);

                        if (pack_spu.thread[i].buffer.pack)
                        {
                            crNetFree(pack_spu.thread[i].netServer.conn, pack_spu.thread[i].buffer.pack);
                            pack_spu.thread[i].buffer.pack = NULL;
                        }
                    }
                    crNetFreeConnection(pack_spu.thread[i].netServer.conn);

                    if (pack_spu.thread[i].netServer.name)
                        crFree(pack_spu.thread[i].netServer.name);

                    pack_spu.numThreads--;
                    /*note can't shift the array here, because other threads have TLS references to array elements*/
                    crMemZero(&pack_spu.thread[i], sizeof(ThreadInfo));

                    crSetTSD(&_PackTSD, NULL);

                    if (i==pack_spu.idxThreadInUse)
                    {
                        for (i=0; i<MAX_THREADS; ++i)
                        {
                            if (pack_spu.thread[i].inUse)
                            {
                                pack_spu.idxThreadInUse=i;
                                break;
                            }
                        }
                    }

                    break;
                }
            }

            for (i=0; i<CR_MAX_CONTEXTS; ++i)
            {
                ContextInfo *ctx = &pack_spu.context[i];
                if (ctx->currentThread == thread)
                {
                    CRASSERT(ctx->fAutoFlush);
                    ctx->currentThread = NULL;
                }
            }

            crUnlockMutex(&_PackMutex);
        }
    }

    crStateVBoxDetachThread(&pack_spu.StateTracker);
}

void PACKSPU_APIENTRY packspu_VBoxPresentComposition(GLint win, const struct VBOXVR_SCR_COMPOSITOR * pCompositor,
                                                     const struct VBOXVR_SCR_COMPOSITOR_ENTRY *pChangedEntry)
{
    RT_NOREF(win, pCompositor, pChangedEntry);
}

void PACKSPU_APIENTRY packspu_StringMarkerGREMEDY(GLsizei len, const GLvoid *string)
{
    RT_NOREF(len, string);
}

