/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "cr_mem.h"
#include "cr_error.h"
#include "cr_spu.h"

#include <iprt/asm.h>

/**
 * Allocate a new shared state object.
 * Contains texture objects, display lists, etc.
 */
static CRSharedState *
crStateAllocShared(void)
{
    CRSharedState *s = (CRSharedState *) crCalloc(sizeof(CRSharedState));
    if (s) {
        s->textureTable = crAllocHashtable();
        s->dlistTable = crAllocHashtable();
        s->buffersTable = crAllocHashtable();
        s->fbTable = crAllocHashtable();
        s->rbTable = crAllocHashtable();
        s->refCount = 1; /* refcount is number of contexts using this state */
        s->saveCount = 0;
    }
    return s;
}

/**
 * Callback used for crFreeHashtable().
 */
DECLEXPORT(void)
crStateDeleteTextureCallback(void *texObj, void *pvUser)
{
#ifndef IN_GUEST
    PCRStateTracker pState = (PCRStateTracker)pvUser;

    pState->diff_api.DeleteTextures(1, &((CRTextureObj *)texObj)->hwid);
#else
    RT_NOREF(pvUser);
#endif
    crStateDeleteTextureObject((CRTextureObj *) texObj);
}

typedef struct CR_STATE_RELEASEOBJ
{
    PCRStateTracker pState;
    CRContext       *pCtx;
    CRSharedState   *s;
} CR_STATE_RELEASEOBJ, *PCR_STATE_RELEASEOBJ;

void crStateOnTextureUsageRelease(PCRStateTracker pState, CRSharedState *pS, CRTextureObj *pObj)
{
    if (!pObj->pinned)
        crHashtableDeleteEx(pS->textureTable, pObj->id, crStateDeleteTextureCallback, pState);
    else
        Assert(crHashtableSearch(pS->textureTable, pObj->id));
}

static void crStateReleaseTextureInternal(CRSharedState *pS, CRContext *pCtx, CRTextureObj *pObj)
{
    Assert(CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj) || pObj->pinned);
    CR_STATE_SHAREDOBJ_USAGE_CLEAR(pObj, pCtx);
    if (CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj))
        return;

    crStateOnTextureUsageRelease(pCtx->pStateTracker, pS, pObj);
}

DECLEXPORT(void) crStateReleaseTexture(CRContext *pCtx, CRTextureObj *pObj)
{
    Assert(CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj));
    CR_STATE_SHAREDOBJ_USAGE_CLEAR(pObj, pCtx);
    if (CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj))
        return;

    if (!pCtx->pStateTracker->pSharedState)
    {
        WARN(("no global shared"));
        return;
    }

    crStateOnTextureUsageRelease(pCtx->pStateTracker, pCtx->pStateTracker->pSharedState, pObj);
}

static void crStateReleaseBufferObjectInternal(CRSharedState *pS, CRContext *pCtx, CRBufferObject *pObj)
{
    Assert(CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj));
    CR_STATE_SHAREDOBJ_USAGE_CLEAR(pObj, pCtx);
    if (!CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj))
        crHashtableDeleteEx(pS->buffersTable, pObj->id, crStateFreeBufferObject, pCtx->pStateTracker);
}

static void crStateReleaseFBOInternal(CRSharedState *pS, CRContext *pCtx, CRFramebufferObject *pObj)
{
    Assert(CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj));
    CR_STATE_SHAREDOBJ_USAGE_CLEAR(pObj, pCtx);
    if (!CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj))
        crHashtableDeleteEx(pS->fbTable, pObj->id, crStateFreeFBO, pCtx->pStateTracker);
}

static void crStateReleaseRBOInternal(CRSharedState *pS, CRContext *pCtx, CRRenderbufferObject *pObj)
{
    Assert(CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj));
    CR_STATE_SHAREDOBJ_USAGE_CLEAR(pObj, pCtx);
    if (!CR_STATE_SHAREDOBJ_USAGE_IS_USED(pObj))
        crHashtableDeleteEx(pS->rbTable, pObj->id, crStateFreeRBO, pCtx->pStateTracker);
}

static void ReleaseTextureCallback(unsigned long key, void *data1, void *data2)
{
    PCR_STATE_RELEASEOBJ pData = (PCR_STATE_RELEASEOBJ)data2;
    CRTextureObj *pObj = (CRTextureObj *)data1;
    (void)key;
    crStateReleaseTextureInternal(pData->s, pData->pCtx, pObj);
}

static void ReleaseBufferObjectCallback(unsigned long key, void *data1, void *data2)
{
    PCR_STATE_RELEASEOBJ pData = (PCR_STATE_RELEASEOBJ)data2;
    CRBufferObject *pObj = (CRBufferObject *)data1;
    (void)key;
    crStateReleaseBufferObjectInternal(pData->s, pData->pCtx, pObj);
}

static void ReleaseFBOCallback(unsigned long key, void *data1, void *data2)
{
    PCR_STATE_RELEASEOBJ pData = (PCR_STATE_RELEASEOBJ)data2;
    CRFramebufferObject *pObj = (CRFramebufferObject *)data1;
    (void)key;
    crStateReleaseFBOInternal(pData->s, pData->pCtx, pObj);
}

static void ReleaseRBOCallback(unsigned long key, void *data1, void *data2)
{
    PCR_STATE_RELEASEOBJ pData = (PCR_STATE_RELEASEOBJ)data2;
    CRRenderbufferObject *pObj = (CRRenderbufferObject *)data1;
    (void)key;
    crStateReleaseRBOInternal(pData->s, pData->pCtx, pObj);
}


/**
 * Decrement shared state's refcount and delete when it hits zero.
 */
#ifndef IN_GUEST
DECLEXPORT(void) crStateFreeShared(PCRStateTracker pState, CRContext *pContext, CRSharedState *s)
#else
static void crStateFreeShared(PCRStateTracker pState, CRContext *pContext, CRSharedState *s)
#endif
{
    int32_t refCount = ASMAtomicDecS32(&s->refCount);

    Assert(refCount >= 0);
    if (refCount <= 0) {
        if (s == pState->pSharedState)
            pState->pSharedState = NULL;
        crFreeHashtableEx(s->textureTable, crStateDeleteTextureCallback, pState);
        crFreeHashtable(s->dlistTable, crFree); /* call crFree for each entry */
        crFreeHashtableEx(s->buffersTable, crStateFreeBufferObject, pState);
        crFreeHashtableEx(s->fbTable, crStateFreeFBO, pState);
        crFreeHashtableEx(s->rbTable, crStateFreeRBO, pState);
        crFree(s);
    }
    else if (pContext)
    {
        /* evaluate usage bits*/
        CR_STATE_RELEASEOBJ CbData;
        CbData.pState = pState;
        CbData.pCtx   = pContext;
        CbData.s      = s;
        crHashtableWalk(s->textureTable, ReleaseTextureCallback, &CbData);
        crHashtableWalk(s->buffersTable, ReleaseBufferObjectCallback , &CbData);
        crHashtableWalk(s->fbTable, ReleaseFBOCallback, &CbData);
        crHashtableWalk(s->rbTable, ReleaseRBOCallback, &CbData);
    }
}

#ifndef IN_GUEST

DECLEXPORT(CRSharedState *) crStateGlobalSharedAcquire(PCRStateTracker pState)
{
    if (!pState->pSharedState)
    {
        crWarning("No Global Shared State!");
        return NULL;
    }
    ASMAtomicIncS32(&pState->pSharedState->refCount);
    return pState->pSharedState;
}

DECLEXPORT(void) crStateGlobalSharedRelease(PCRStateTracker pState)
{
    crStateFreeShared(pState, NULL, pState->pSharedState);
}

#endif /* IN_GUEST */

DECLEXPORT(void) STATE_APIENTRY crStateShareContext(PCRStateTracker pState, GLboolean value)
{
    CRContext *pCtx = GetCurrentContext(pState);
    CRASSERT(pCtx && pCtx->shared);

    if (value)
    {
        if (pCtx->shared == pState->pSharedState)
        {
            return;
        }

        crDebug("Context(%i) shared", pCtx->id);

        if (!pState->pSharedState)
        {
            pState->pSharedState = pCtx->shared;
        }
        else
        {
            crStateFreeShared(pState, pCtx, pCtx->shared);
            pCtx->shared = pState->pSharedState;
            ASMAtomicIncS32(&pState->pSharedState->refCount);
        }
    }
    else
    {
        if (pCtx->shared != pState->pSharedState)
        {
            return;
        }

        crDebug("Context(%i) unshared", pCtx->id);

        if (pState->pSharedState->refCount==1)
        {
            pState->pSharedState = NULL;
        }
        else
        {
            pCtx->shared = crStateAllocShared();
            pCtx->shared->id = pCtx->id;
            crStateFreeShared(pState, pCtx, pState->pSharedState);
        }
    }
}

DECLEXPORT(void) STATE_APIENTRY crStateShareLists(CRContext *pContext1, CRContext *pContext2)
{
    CRASSERT(pContext1->shared);
    CRASSERT(pContext2->shared);

    if (pContext2->shared == pContext1->shared)
    {
        return;
    }

    crStateFreeShared(pContext1->pStateTracker, pContext1, pContext1->shared);
    pContext1->shared = pContext2->shared;
    ASMAtomicIncS32(&pContext2->shared->refCount);
}

DECLEXPORT(GLboolean) STATE_APIENTRY crStateContextIsShared(CRContext *pCtx)
{
    return pCtx->shared == pCtx->pStateTracker->pSharedState;
}

DECLEXPORT(void) STATE_APIENTRY crStateSetSharedContext(CRContext *pCtx)
{
    PCRStateTracker pState = pCtx->pStateTracker;

    if (pState->pSharedState)
    {
        crWarning("crStateSetSharedContext: shared is being changed from %p to %p", pState->pSharedState, pCtx->shared);
    }

    pState->pSharedState = pCtx->shared;
}

static void crStateFreeContext(CRContext *ctx);

static DECLCALLBACK(void) crStateContextDtor(void *pvCtx)
{
    crStateFreeContext((CRContext*)pvCtx);
}

/*
 * Helper for crStateCreateContext, below.
 */
static CRContext *
crStateCreateContextId(PCRStateTracker pState, int i, const CRLimitsState *limits, GLint visBits, CRContext *shareCtx)
{
    CRContext *ctx;
    int j;
    int node32 = i >> 5;
    int node = i & 0x1f;
    (void)limits;

    if (pState->apAvailableContexts[i] != NULL)
    {
        crWarning("trying to create context with used id");
        return NULL;
    }

    ctx = (CRContext *) crCalloc( sizeof( *ctx ) );
    if (!ctx)
    {
        crWarning("failed to allocate context");
        return NULL;
    }
    pState->apAvailableContexts[i] = ctx;
    pState->cContexts++;
    CRASSERT(pState->cContexts < RT_ELEMENTS(pState->apAvailableContexts));
    ctx->id = i;
    ctx->pStateTracker = pState;
    VBoxTlsRefInit(ctx, crStateContextDtor);
    ctx->flush_func = NULL;
    for (j=0;j<CR_MAX_BITARRAY;j++){
        if (j == node32) {
            ctx->bitid[j] = (1 << node);
        } else {
            ctx->bitid[j] = 0;
        }
        ctx->neg_bitid[j] = ~(ctx->bitid[j]);
    }

    if (shareCtx) {
        CRASSERT(shareCtx->shared);
        ctx->shared = shareCtx->shared;
        ASMAtomicIncS32(&ctx->shared->refCount);
    }
    else {
        ctx->shared = crStateAllocShared();
        ctx->shared->id = ctx->id;
    }

    /* use Chromium's OpenGL defaults */
    crStateLimitsInit( &(ctx->limits) );
    crStateExtensionsInit( &(ctx->limits), &(ctx->extensions) );

    crStateBufferObjectInit( ctx ); /* must precede client state init! */
    crStateClientInit( ctx );

    crStateBufferInit( ctx );
    crStateCurrentInit( ctx );
    crStateEvaluatorInit( ctx );
    crStateFogInit( ctx );
    crStateHintInit( ctx );
    crStateLightingInit( ctx );
    crStateLineInit( ctx );
    crStateListsInit( ctx );
    crStateMultisampleInit( ctx );
    crStateOcclusionInit( ctx );
    crStatePixelInit( ctx );
    crStatePolygonInit( ctx );
    crStatePointInit( ctx );
    crStateProgramInit( ctx );
    crStateRegCombinerInit( ctx );
    crStateStencilInit( ctx );
    crStateTextureInit( ctx );
    crStateTransformInit( ctx );
    crStateViewportInit ( ctx );
    crStateFramebufferObjectInit(ctx);
    crStateGLSLInit(ctx);

    /* This has to come last. */
    crStateAttribInit( &(ctx->attrib) );

    ctx->renderMode = GL_RENDER;

    /* Initialize values that depend on the visual mode */
    if (visBits & CR_DOUBLE_BIT) {
        ctx->limits.doubleBuffer = GL_TRUE;
    }
    if (visBits & CR_RGB_BIT) {
        ctx->limits.redBits = 8;
        ctx->limits.greenBits = 8;
        ctx->limits.blueBits = 8;
        if (visBits & CR_ALPHA_BIT) {
            ctx->limits.alphaBits = 8;
        }
    }
    else {
        ctx->limits.indexBits = 8;
    }
    if (visBits & CR_DEPTH_BIT) {
        ctx->limits.depthBits = 24;
    }
    if (visBits & CR_STENCIL_BIT) {
        ctx->limits.stencilBits = 8;
    }
    if (visBits & CR_ACCUM_BIT) {
        ctx->limits.accumRedBits = 16;
        ctx->limits.accumGreenBits = 16;
        ctx->limits.accumBlueBits = 16;
        if (visBits & CR_ALPHA_BIT) {
            ctx->limits.accumAlphaBits = 16;
        }
    }
    if (visBits & CR_STEREO_BIT) {
        ctx->limits.stereo = GL_TRUE;
    }
    if (visBits & CR_MULTISAMPLE_BIT) {
        ctx->limits.sampleBuffers = 1;
        ctx->limits.samples = 4;
        ctx->multisample.enabled = GL_TRUE;
    }

    if (visBits & CR_OVERLAY_BIT) {
        ctx->limits.level = 1;
    }

    return ctx;
}

/** @todo crStateAttribDestroy*/
static void
crStateFreeContext(CRContext *ctx)
{
    PCRStateTracker pState = ctx->pStateTracker;

#ifndef DEBUG_misha
    CRASSERT(pState->apAvailableContexts[ctx->id] == ctx);
#endif
    if (pState->apAvailableContexts[ctx->id] == ctx)
    {
        pState->apAvailableContexts[ctx->id] = NULL;
        pState->cContexts--;
        CRASSERT(pState->cContexts < RT_ELEMENTS(pState->apAvailableContexts));
    }
    else
    {
#ifndef DEBUG_misha
        crWarning("freeing context %p, id(%d) not being in the context list", ctx, ctx->id);
#endif
    }

    crStateClientDestroy( ctx );
    crStateLimitsDestroy( &(ctx->limits) );
    crStateBufferObjectDestroy( ctx );
    crStateEvaluatorDestroy( ctx );
    crStateListsDestroy( ctx );
    crStateLightingDestroy( ctx );
    crStateOcclusionDestroy( ctx );
    crStateProgramDestroy( ctx );
    crStateTextureDestroy( ctx );
    crStateTransformDestroy( ctx );
    crStateFreeShared(pState, ctx, ctx->shared);
    crStateFramebufferObjectDestroy(ctx);
    crStateGLSLDestroy(ctx);
    if (ctx->buffer.pFrontImg) crFree(ctx->buffer.pFrontImg);
    if (ctx->buffer.pBackImg) crFree(ctx->buffer.pBackImg);
    crFree( ctx );
}

#ifndef RT_OS_WINDOWS
static void crStateThreadTlsDtor(void *pvValue)
{
    CRContext *pCtx = (CRContext*)pvValue;
    VBoxTlsRefRelease(pCtx);
}
#endif

/*
 * Allocate the state (dirty) bits data structures.
 * This should be called before we create any contexts.
 * We'll also create the default/NULL context at this time and make
 * it the current context by default.  This means that if someone
 * tries to set GL state before calling MakeCurrent() they'll be
 * modifying the default state object, and not segfaulting on a NULL
 * pointer somewhere.
 */
void crStateInit(PCRStateTracker pState)
{
    unsigned int i;

    /* Purely initialize the context bits */
    if (!pState->pCurrentBits) {
        pState->fVBoxEnableDiffOnMakeCurrent = GL_TRUE;
        pState->pCurrentBits = (CRStateBits *) crCalloc( sizeof(CRStateBits) );
        crStateClientInitBits( &(pState->pCurrentBits->client) );
        crStateLightingInitBits( &(pState->pCurrentBits->lighting) );
    } else
    {
#ifndef DEBUG_misha
        crWarning("State tracker is being re-initialized..\n");
#endif
    }

    for (i=0;i<CR_MAX_CONTEXTS;i++)
        pState->apAvailableContexts[i] = NULL;
    pState->cContexts = 0;

    if (!pState->fContextTLSInit)
    {
# ifndef RT_OS_WINDOWS
        /* tls destructor is implemented for all platforms except windows*/
        crInitTSDF(&pState->contextTSD, crStateThreadTlsDtor);
# else
        /* windows should do cleanup via DllMain THREAD_DETACH notification */
        crInitTSD(&pState->contextTSD);
# endif
        pState->fContextTLSInit = true;
    }

    if (pState->pDefaultContext) {
        /* Free the default/NULL context.
         * Ensures context bits are reset */
        SetCurrentContext(pState, NULL);
        VBoxTlsRefRelease(pState->pDefaultContext);
    }

    /* Reset diff_api */
    crMemZero(&pState->diff_api, sizeof(SPUDispatchTable));

    Assert(!pState->pSharedState);
    pState->pSharedState = NULL;

    /* Allocate the default/NULL context */
    CRASSERT(pState->apAvailableContexts[0] == NULL);
    pState->pDefaultContext = crStateCreateContextId(pState, 0, NULL, CR_RGB_BIT, NULL);
    CRASSERT(pState->apAvailableContexts[0] == pState->pDefaultContext);
    CRASSERT(pState->cContexts == 1);
    SetCurrentContext(pState, pState->pDefaultContext);
}

void crStateDestroy(PCRStateTracker pState)
{
    int i;
    if (pState->pCurrentBits)
    {
        crStateClientDestroyBits(&(pState->pCurrentBits->client));
        crStateLightingDestroyBits(&(pState->pCurrentBits->lighting));
        crFree(pState->pCurrentBits);
        pState->pCurrentBits = NULL;
    }

    SetCurrentContext(pState, NULL);

    for (i = CR_MAX_CONTEXTS-1; i >= 0; i--)
    {
        if (pState->apAvailableContexts[i])
        {
            if (VBoxTlsRefIsFunctional(pState->apAvailableContexts[i]))
                VBoxTlsRefRelease(pState->apAvailableContexts[i]);
        }
    }

    /* default context was stored in g_pAvailableContexts[0], so it was destroyed already */
    pState->pDefaultContext = NULL;


    crFreeTSD(&pState->contextTSD);
    pState->fContextTLSInit = false;
}

/*
 * Notes on context switching and the "default context".
 *
 * See the paper "Tracking Graphics State for Networked Rendering"
 * by Ian Buck, Greg Humphries and Pat Hanrahan for background
 * information about how the state tracker and context switching
 * works.
 *
 * When we make a new context current, we call crStateSwitchContext()
 * in order to transform the 'from' context into the 'to' context
 * (i.e. the old context to the new context).  The transformation
 * is accomplished by calling GL functions through the 'diff_api'
 * so that the downstream GL machine (represented by the __currentContext
 * structure) is updated to reflect the new context state.  Finally,
 * we point __currentContext to the new context.
 *
 * A subtle problem we have to deal with is context destruction.
 * This issue arose while testing with Glean.  We found that when
 * the currently bound context was getting destroyed that state
 * tracking was incorrect when a subsequent new context was activated.
 * In DestroyContext, the __hwcontext was being set to NULL and effectively
 * going away.  Later in MakeCurrent we had no idea what the state of the
 * downstream GL machine was (since __hwcontext was gone).  This meant
 * we had nothing to 'diff' against and the downstream GL machine was
 * in an unknown state.
 *
 * The solution to this problem is the "default/NULL" context.  The
 * default context is created the first time CreateContext is called
 * and is never freed.  Whenever we get a crStateMakeCurrent(NULL) call
 * or destroy the currently bound context in crStateDestroyContext()
 * we call crStateSwitchContext() to switch to the default context and
 * then set the __currentContext pointer to point to the default context.
 * This ensures that the dirty bits are updated and the diff_api functions
 * are called to keep the downstream GL machine in a known state.
 * Finally, the __hwcontext variable is no longer needed now.
 *
 * Yeah, this is kind of a mind-bender, but it really solves the problem
 * pretty cleanly.
 *
 * -Brian
 */


CRContext *
crStateCreateContext(PCRStateTracker pState, const CRLimitsState *limits, GLint visBits, CRContext *share)
{
    return crStateCreateContextEx(pState, limits, visBits, share, -1);
}

CRContext *
crStateCreateContextEx(PCRStateTracker pState, const CRLimitsState *limits, GLint visBits, CRContext *share, GLint presetID)
{
    /* Must have created the default context via crStateInit() first */
    CRASSERT(pState->pDefaultContext);

    if (presetID>0)
    {
        if(pState->apAvailableContexts[presetID])
        {
            crWarning("requesting to create context with already allocated id");
            return NULL;
        }
    }
    else
    {
        int i;

        for (i = 1 ; i < CR_MAX_CONTEXTS ; i++)
        {
            if (!pState->apAvailableContexts[i])
            {
                presetID = i;
                break;
            }
        }

        if (presetID<=0)
        {
            crError( "Out of available contexts in crStateCreateContexts (max %d)",
                             CR_MAX_CONTEXTS );
            /* never get here */
            return NULL;
        }
    }

    return crStateCreateContextId(pState, presetID, limits, visBits, share);
}

void crStateDestroyContext(PCRStateTracker pState, CRContext *ctx )
{
    CRContext *current = GetCurrentContext(pState);

    if (current == ctx) {
        /* destroying the current context - have to be careful here */
        CRASSERT(pState->pDefaultContext);
        /* Check to see if the differencer exists first,
           we may not have one, aka the packspu */
        if (pState->diff_api.AlphaFunc)
            crStateSwitchContext(current, pState->pDefaultContext);

        SetCurrentContext(pState, pState->pDefaultContext);
        /* ensure matrix state is also current */
        crStateMatrixMode(pState, pState->pDefaultContext->transform.matrixMode);
    }

    VBoxTlsRefMarkDestroy(ctx);
# ifdef IN_GUEST
    if (VBoxTlsRefCountGet(ctx) > 1 && ctx->shared == pState->pSharedState)
    {
        /* we always need to free the global shared state to prevent the situation when guest thinks the shared objects are still valid, while host destroys them */
        crStateFreeShared(pState, ctx, ctx->shared);
        ctx->shared = crStateAllocShared();
    }
# endif
    VBoxTlsRefRelease(ctx);
}

GLboolean crStateEnableDiffOnMakeCurrent(PCRStateTracker pState, GLboolean fEnable)
{
    GLboolean bOld = pState->fVBoxEnableDiffOnMakeCurrent;
    pState->fVBoxEnableDiffOnMakeCurrent = fEnable;
    return bOld;
}

void crStateMakeCurrent(PCRStateTracker pState, CRContext *ctx )
{
    CRContext *current = GetCurrentContext(pState);
    CRContext *pLocalCtx = ctx;

    if (pLocalCtx == NULL)
        pLocalCtx = pState->pDefaultContext;

    if (current == pLocalCtx)
        return; /* no-op */

    CRASSERT(pLocalCtx);

    if (pState->fVBoxEnableDiffOnMakeCurrent && current) {
        /* Check to see if the differencer exists first,
           we may not have one, aka the packspu */
        if (pState->diff_api.AlphaFunc)
            crStateSwitchContext(current, pLocalCtx );
    }

    SetCurrentContext(pState, pLocalCtx);

    /* ensure matrix state is also current */
    crStateMatrixMode(pState, pLocalCtx->transform.matrixMode);
}


/*
 * As above, but don't call crStateSwitchContext().
 */
static void crStateSetCurrentEx(PCRStateTracker pState, CRContext *ctx, GLboolean fCleanupDefault )
{
    CRContext *current = GetCurrentContext(pState);
    CRContext *pLocalCtx = ctx;

    if (pLocalCtx == NULL && !fCleanupDefault)
        pLocalCtx = pState->pDefaultContext;

    if (current == pLocalCtx)
        return; /* no-op */

    SetCurrentContext(pState, pLocalCtx);
    if (pLocalCtx)
    {
        /* ensure matrix state is also current */
        crStateMatrixMode(pState, pLocalCtx->transform.matrixMode);
    }
}

void crStateSetCurrent(PCRStateTracker pState, CRContext *ctx )
{
    crStateSetCurrentEx(pState, ctx, GL_FALSE );
}

void crStateCleanupCurrent(PCRStateTracker pState)
{
    crStateSetCurrentEx(pState, NULL, GL_TRUE );
}


CRContext *crStateGetCurrent(PCRStateTracker pState)
{
    return GetCurrentContext(pState);
}


void crStateUpdateColorBits(PCRStateTracker pState)
{
    /* This is a hack to force updating the 'current' attribs */
    CRStateBits *sb = GetCurrentBits(pState);
    FILLDIRTY(sb->current.dirty);
    FILLDIRTY(sb->current.vertexAttrib[VERT_ATTRIB_COLOR0]);
}


void STATE_APIENTRY
crStateChromiumParameteriCR(PCRStateTracker pState, GLenum target, GLint value )
{
    /* This no-op function helps smooth code-gen */
    RT_NOREF(pState);
    (void)target; (void)value;
}

void STATE_APIENTRY
crStateChromiumParameterfCR(PCRStateTracker pState, GLenum target, GLfloat value )
{
    /* This no-op function helps smooth code-gen */
    RT_NOREF(pState);
    (void)target; (void)value;
}

void STATE_APIENTRY
crStateChromiumParametervCR(PCRStateTracker pState, GLenum target, GLenum type, GLsizei count, const GLvoid *values )
{
    /* This no-op function helps smooth code-gen */
    RT_NOREF(pState);
    (void)target; (void)type; (void)count; (void)values;
}

void STATE_APIENTRY
crStateGetChromiumParametervCR(PCRStateTracker pState, GLenum target, GLuint index, GLenum type, GLsizei count, GLvoid *values )
{
    /* This no-op function helps smooth code-gen */
    RT_NOREF(pState);
    (void)target; (void)index; (void)type; (void)count; (void)values;
}

void STATE_APIENTRY
crStateReadPixels(PCRStateTracker pState, GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels )
{
    /* This no-op function helps smooth code-gen */
    RT_NOREF(pState);
    (void)x; (void)y; (void)width; (void)height; (void)format; (void)type; (void)pixels;
}

void crStateVBoxDetachThread(PCRStateTracker pState)
{
    /* release the context ref so that it can be freed */
    SetCurrentContext(pState, NULL);
}


void crStateVBoxAttachThread(PCRStateTracker pState)
{
    RT_NOREF(pState);
}

#if 0 /* who's refering to these? */

GLint crStateVBoxCreateContext( GLint con, const char * dpyName, GLint visual, GLint shareCtx )
{
    (void)con; (void)dpyName; (void)visual; (void)shareCtx;
    return 0;
}

GLint crStateVBoxWindowCreate( GLint con, const char *dpyName, GLint visBits  )
{
    (void)con; (void)dpyName; (void)visBits;
    return 0;
}

void crStateVBoxWindowDestroy( GLint con, GLint window )
{
    (void)con; (void)window;
}

GLint crStateVBoxConCreate(struct VBOXUHGSMI *pHgsmi)
{
    (void)pHgsmi;
    return 0;
}

void crStateVBoxConDestroy(GLint con)
{
    (void)con;
}

void crStateVBoxConFlush(GLint con)
{
    (void)con;
}

#endif /* unused? */
