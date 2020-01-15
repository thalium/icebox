/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packspu.h"
#include "cr_mem.h"
#include "cr_packfunctions.h"
#include "cr_string.h"
#include "packspu_proto.h"

/*
 * Allocate a new ThreadInfo structure, setup a connection to the
 * server, allocate/init a packer context, bind this ThreadInfo to
 * the calling thread with crSetTSD().
 * We'll always call this function at least once even if we're not
 * using threads.
 */
ThreadInfo *packspuNewThread(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                struct VBOXUHGSMI *pHgsmi
#else
                void
#endif
)
{
    ThreadInfo *thread=NULL;
    int i;

    crLockMutex(&_PackMutex);

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    CRASSERT(!CRPACKSPU_IS_WDDM_CRHGSMI() == !pHgsmi);
#endif

    CRASSERT(pack_spu.numThreads < MAX_THREADS);
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
    thread->bInjectThread = GL_FALSE;

    /* connect to the server */
    thread->netServer.name = crStrdup( pack_spu.name );
    thread->netServer.buffer_size = pack_spu.buffer_size;
    packspuConnectToServer( &(thread->netServer)
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
            , pHgsmi
#endif
            );
    CRASSERT(thread->netServer.conn);
    /* packer setup */
    CRASSERT(thread->packer == NULL);
    thread->packer = crPackNewContext();
    CRASSERT(thread->packer);
    crPackInitBuffer( &(thread->buffer), crNetAlloc(thread->netServer.conn),
                thread->netServer.conn->buffer_size, thread->netServer.conn->mtu );
    thread->buffer.canBarf = thread->netServer.conn->Barf ? GL_TRUE : GL_FALSE;
    crPackSetBuffer( thread->packer, &thread->buffer );
    crPackFlushFunc( thread->packer, packspuFlush );
    crPackFlushArg( thread->packer, (void *) thread );
    crPackSendHugeFunc( thread->packer, packspuHuge );

    if (!CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        crPackSetContext( thread->packer );
    }


    if (!CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        crSetTSD(&_PackTSD, thread);
    }

    pack_spu.numThreads++;

    crUnlockMutex(&_PackMutex);
    return thread;
}

GLint PACKSPU_APIENTRY
packspu_VBoxConCreate(struct VBOXUHGSMI *pHgsmi)
{
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    ThreadInfo * thread;
    CRASSERT(CRPACKSPU_IS_WDDM_CRHGSMI());
    CRASSERT(pHgsmi);

    thread = packspuNewThread(pHgsmi);

    if (thread)
    {
        CRASSERT(thread->id);
        CRASSERT(thread->id - THREAD_OFFSET_MAGIC < RT_ELEMENTS(pack_spu.thread)
                && GET_THREAD_VAL_ID(thread->id) == thread);
        return thread->id;
    }
    crError("packspuNewThread failed");
#else
    RT_NOREF(pHgsmi);
#endif
    return 0;
}

void PACKSPU_APIENTRY
packspu_VBoxConFlush(GLint con)
{
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    GET_THREAD_ID(thread, con);
    CRASSERT(con);
    CRASSERT(CRPACKSPU_IS_WDDM_CRHGSMI());
    CRASSERT(thread->packer);
    packspuFlush((void *) thread);
#else
    RT_NOREF(con);
    crError("VBoxConFlush not implemented!");
#endif
}

void PACKSPU_APIENTRY
packspu_VBoxConDestroy(GLint con)
{
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    GET_THREAD_ID(thread, con);
    CRASSERT(con);
    CRASSERT(CRPACKSPU_IS_WDDM_CRHGSMI());
    CRASSERT(pack_spu.numThreads>0);
    CRASSERT(thread->packer);
    packspuFlush((void *) thread);

    crLockMutex(&_PackMutex);

    crPackDeleteContext(thread->packer);

    if (thread->buffer.pack)
    {
        crNetFree(thread->netServer.conn, thread->buffer.pack);
        thread->buffer.pack = NULL;
    }

    crNetFreeConnection(thread->netServer.conn);

    if (thread->netServer.name)
        crFree(thread->netServer.name);

    pack_spu.numThreads--;
    /*note can't shift the array here, because other threads have TLS references to array elements*/
    crMemZero(thread, sizeof(ThreadInfo));

#if 0
    if (&pack_spu.thread[pack_spu.idxThreadInUse]==thread)
    {
        int i;
        crError("Should not be here since idxThreadInUse should be always 0 for the dummy connection created in packSPUInit!");
        for (i=0; i<MAX_THREADS; ++i)
        {
            if (pack_spu.thread[i].inUse)
            {
                pack_spu.idxThreadInUse=i;
                break;
            }
        }
    }
#endif
    crUnlockMutex(&_PackMutex);
#else
    RT_NOREF(con);
#endif
}

GLvoid PACKSPU_APIENTRY
packspu_VBoxConChromiumParameteriCR(GLint con, GLenum param, GLint value)
{
    GET_THREAD(thread);
    CRPackContext * curPacker = crPackGetContext();
    ThreadInfo *curThread = thread;

    CRASSERT(!curThread == !curPacker);
    CRASSERT(!curThread || !curPacker || curThread->packer == curPacker);
    crLockMutex(&_PackMutex);

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    CRASSERT(!con == !CRPACKSPU_IS_WDDM_CRHGSMI());
#endif

    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        if (!con)
        {
            crError("connection should be specified!");
            return;
        }
        thread = GET_THREAD_VAL_ID(con);
    }
    else
    {
        CRASSERT(!con);
        if (!thread)
        {
            thread = packspuNewThread(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                NULL
#endif
                );
        }
    }
    CRASSERT(thread);
    CRASSERT(thread->packer);

    crPackSetContext( thread->packer );

    packspu_ChromiumParameteriCR(param, value);

    crUnlockMutex(&_PackMutex);
    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        /* restore the packer context to the tls */
        crPackSetContext(curPacker);
    }
}

GLvoid PACKSPU_APIENTRY
packspu_VBoxConChromiumParametervCR(GLint con, GLenum target, GLenum type, GLsizei count, const GLvoid *values)
{
    GET_THREAD(thread);
    CRPackContext * curPacker = crPackGetContext();
    ThreadInfo *curThread = thread;

    CRASSERT(!curThread == !curPacker);
    CRASSERT(!curThread || !curPacker || curThread->packer == curPacker);
    crLockMutex(&_PackMutex);

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    CRASSERT(!con == !CRPACKSPU_IS_WDDM_CRHGSMI());
#endif

    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        if (!con)
        {
            crError("connection should be specified!");
            return;
        }
        thread = GET_THREAD_VAL_ID(con);
    }
    else
    {
        CRASSERT(!con);
        if (!thread)
        {
            thread = packspuNewThread(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                NULL
#endif
                );
        }
    }
    CRASSERT(thread);
    CRASSERT(thread->packer);

    crPackSetContext( thread->packer );

    packspu_ChromiumParametervCR(target, type, count, values);

    crUnlockMutex(&_PackMutex);
    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        /* restore the packer context to the tls */
        crPackSetContext(curPacker);
    }
}

GLint PACKSPU_APIENTRY
packspu_VBoxCreateContext( GLint con, const char *dpyName, GLint visual, GLint shareCtx )
{
    GET_THREAD(thread);
    CRPackContext * curPacker = crPackGetContext();
    ThreadInfo *curThread = thread;
    int writeback = 1;
    GLint serverCtx = (GLint) -1;
    int slot;

    CRASSERT(!curThread == !curPacker);
    CRASSERT(!curThread || !curPacker || curThread->packer == curPacker);
    crLockMutex(&_PackMutex);

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    CRASSERT(!con == !CRPACKSPU_IS_WDDM_CRHGSMI());
#endif

    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        if (!con)
        {
            crError("connection should be specified!");
            return -1;
        }
        thread = GET_THREAD_VAL_ID(con);
    }
    else
    {
        CRASSERT(!con);
        if (!thread)
        {
            thread = packspuNewThread(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                NULL
#endif
                );
        }
    }
    CRASSERT(thread);
    CRASSERT(thread->packer);

    if (shareCtx > 0) {
        /* translate to server ctx id */
        shareCtx -= MAGIC_OFFSET;
        if (shareCtx >= 0 && shareCtx < pack_spu.numContexts) {
            shareCtx = pack_spu.context[shareCtx].serverCtx;
        }
    }

    crPackSetContext( thread->packer );

    /* Pack the command */
    crPackCreateContext( dpyName, visual, shareCtx, &serverCtx, &writeback );

    /* Flush buffer and get return value */
    packspuFlush(thread);
    if (!(thread->netServer.conn->actual_network))
    {
        /* HUMUNGOUS HACK TO MATCH SERVER NUMBERING
         *
         * The hack exists solely to make file networking work for now.  This
         * is totally gross, but since the server expects the numbers to start
         * from 5000, we need to write them out this way.  This would be
         * marginally less gross if the numbers (500 and 5000) were maybe
         * some sort of #define'd constants somewhere so the client and the
         * server could be aware of how each other were numbering things in
         * cases like file networking where they actually
         * care.
         *
         *  -Humper
         *
         */
        serverCtx = 5000;
    }
    else {
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);

        if (serverCtx < 0) {
            crUnlockMutex(&_PackMutex);
            crWarning("Failure in packspu_CreateContext");

            if (CRPACKSPU_IS_WDDM_CRHGSMI())
            {
                /* restore the packer context to the tls */
                crPackSetContext(curPacker);
            }
            return -1;  /* failed */
        }
    }

    /* find an empty context slot */
    for (slot = 0; slot < pack_spu.numContexts; slot++) {
        if (!pack_spu.context[slot].clientState) {
            /* found empty slot */
            break;
        }
    }
    if (slot == pack_spu.numContexts) {
        pack_spu.numContexts++;
    }

    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        thread->currentContext = &pack_spu.context[slot];
        pack_spu.context[slot].currentThread = thread;
    }

    /* Fill in the new context info */
    /* XXX fix-up sharedCtx param here */
    pack_spu.context[slot].clientState = crStateCreateContext(&pack_spu.StateTracker, NULL, visual, NULL);
    pack_spu.context[slot].clientState->bufferobject.retainBufferData = GL_TRUE;
    pack_spu.context[slot].serverCtx = serverCtx;

    crUnlockMutex(&_PackMutex);
    if (CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        /* restore the packer context to the tls */
        crPackSetContext(curPacker);
    }

    return MAGIC_OFFSET + slot;
}

GLint PACKSPU_APIENTRY
packspu_CreateContext( const char *dpyName, GLint visual, GLint shareCtx )
{
    return packspu_VBoxCreateContext( 0, dpyName, visual, shareCtx );
}


void PACKSPU_APIENTRY packspu_DestroyContext( GLint ctx )
{
    GET_THREAD(thread);
    ThreadInfo *curThread = thread;
    const int slot = ctx - MAGIC_OFFSET;
    ContextInfo *context, *curContext;
    CRPackContext * curPacker = crPackGetContext();

    CRASSERT(slot >= 0);
    CRASSERT(slot < pack_spu.numContexts);

    context = (slot >= 0 && slot < pack_spu.numContexts) ? &(pack_spu.context[slot]) : NULL;
    curContext = curThread ? curThread->currentContext : NULL;

    if (context)
    {
        if (CRPACKSPU_IS_WDDM_CRHGSMI())
        {
            thread = context->currentThread;
            if (thread)
            {
                crPackSetContext(thread->packer);
                CRASSERT(!(thread->packer == curPacker) == !(thread == curThread));
            }
        }

        crPackDestroyContext( context->serverCtx );
        crStateDestroyContext(&pack_spu.StateTracker, context->clientState );

        context->clientState = NULL;
        context->serverCtx = 0;
        context->currentThread = NULL;

        crMemset (&context->zvaBufferInfo, 0, sizeof (context->zvaBufferInfo));
    }

    if (curContext == context)
    {
        if (!CRPACKSPU_IS_WDDM_CRHGSMI())
        {
            curThread->currentContext = NULL;
        }
        else
        {
            CRASSERT(thread == curThread);
            crSetTSD(&_PackTSD, NULL);
            crPackSetContext(NULL);
        }
        crStateMakeCurrent(&pack_spu.StateTracker, NULL);
    }
    else
    {
        if (CRPACKSPU_IS_WDDM_CRHGSMI())
        {
            crPackSetContext(curPacker);
        }
    }
}

void PACKSPU_APIENTRY packspu_MakeCurrent( GLint window, GLint nativeWindow, GLint ctx )
{
    ThreadInfo *thread = NULL;
    GLint serverCtx;
    ContextInfo *newCtx = NULL;

    if (!CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        thread = GET_THREAD_VAL();
        if (!thread) {
            thread = packspuNewThread(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                    NULL
#endif
                    );
        }
        CRASSERT(thread);
        CRASSERT(thread->packer);
    }

    if (ctx) {
        const int slot = ctx - MAGIC_OFFSET;

        CRASSERT(slot >= 0);
        CRASSERT(slot < pack_spu.numContexts);

        newCtx = &pack_spu.context[slot];
        CRASSERT(newCtx);
        CRASSERT(newCtx->clientState);  /* verify valid */

        if (CRPACKSPU_IS_WDDM_CRHGSMI())
        {
            thread = newCtx->currentThread;
            CRASSERT(thread);
            crSetTSD(&_PackTSD, thread);
            crPackSetContext( thread->packer );
        }
        else
        {
            CRASSERT(thread);
            if (newCtx->fAutoFlush)
            {
                if (newCtx->currentThread && newCtx->currentThread != thread)
                {
                    crLockMutex(&_PackMutex);
                    /* do a flush for the previously assigned thread
                     * to ensure all commands issued there are submitted */
                    if (newCtx->currentThread
                        && newCtx->currentThread->inUse
                        && newCtx->currentThread->netServer.conn
                        && newCtx->currentThread->packer && newCtx->currentThread->packer->currentBuffer)
                    {
                        packspuFlush((void *) newCtx->currentThread);
                    }
                    crUnlockMutex(&_PackMutex);
                }
                newCtx->currentThread = thread;
            }

            if (thread->currentContext && newCtx != thread->currentContext && thread->currentContext->fCheckZerroVertAttr)
                crStateCurrentRecoverNew(thread->currentContext->clientState, &thread->packer->current);

            thread->currentContext = newCtx;
            crPackSetContext( thread->packer );
        }

        crStateMakeCurrent(&pack_spu.StateTracker, newCtx->clientState );
        //crStateSetCurrentPointers(newCtx->clientState, &thread->packer->current);
        serverCtx = pack_spu.context[slot].serverCtx;
    }
    else {
        crStateMakeCurrent(&pack_spu.StateTracker, NULL );
        if (CRPACKSPU_IS_WDDM_CRHGSMI())
        {
            thread = GET_THREAD_VAL();
            if (!thread)
            {
                CRASSERT(crPackGetContext() == NULL);
                return;
            }
            CRASSERT(thread->currentContext);
            CRASSERT(thread->packer == crPackGetContext());
        }
        else
        {
            thread->currentContext = NULL;
        }
        newCtx = NULL;
        serverCtx = 0;
    }

    crPackMakeCurrent( window, nativeWindow, serverCtx );
    if (serverCtx)
    {
        packspuInitStrings();
    }

    {
        GET_THREAD(t);
        (void) t;
        CRASSERT(t);
    }
}
