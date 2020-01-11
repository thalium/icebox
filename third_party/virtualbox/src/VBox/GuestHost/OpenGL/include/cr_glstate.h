/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_GLSTATE_H
#define CR_GLSTATE_H

/* Forward declaration since some of the state/cr_*.h files need the CRContext type */
struct CRContext;
typedef struct CRContext CRContext;

#include "cr_version.h"

#include "state/cr_buffer.h"
#include "state/cr_bufferobject.h"
#include "state/cr_client.h"
#include "state/cr_current.h"
#include "state/cr_evaluators.h"
#include "state/cr_feedback.h"
#include "state/cr_fog.h"
#include "state/cr_hint.h"
#include "state/cr_lighting.h"
#include "state/cr_limits.h"
#include "state/cr_line.h"
#include "state/cr_lists.h"
#include "state/cr_multisample.h"
#include "state/cr_occlude.h"
#include "state/cr_pixel.h"
#include "state/cr_point.h"
#include "state/cr_polygon.h"
#include "state/cr_program.h"
#include "state/cr_regcombiner.h"
#include "state/cr_stencil.h"
#include "state/cr_texture.h"
#include "state/cr_transform.h"
#include "state/cr_viewport.h"
#include "state/cr_attrib.h"
#include "state/cr_framebuffer.h"
#include "state/cr_glsl.h"

#include "state/cr_statefuncs.h"
#include "state/cr_stateerror.h"

#include "spu_dispatch_table.h"

#include "cr_threads.h"

#include <iprt/cdefs.h>

# include <VBox/vmm/ssm.h>
# include <iprt/asm.h>

# define CR_STATE_SHAREDOBJ_USAGE_INIT(_pObj) (crMemset((_pObj)->ctxUsage, 0, sizeof ((_pObj)->ctxUsage)))
# define CR_STATE_SHAREDOBJ_USAGE_SET(_pObj, _pCtx) (ASMBitSet((_pObj)->ctxUsage, (_pCtx)->id))
# define CR_STATE_SHAREDOBJ_USAGE_IS_SET(_pObj, _pCtx) (ASMBitTest((_pObj)->ctxUsage, (_pCtx)->id))
# define CR_STATE_SHAREDOBJ_USAGE_CLEAR_IDX(_pObj, _i) (ASMBitClear((_pObj)->ctxUsage, (_i)))
# define CR_STATE_SHAREDOBJ_USAGE_CLEAR(_pObj, _pCtx) (CR_STATE_SHAREDOBJ_USAGE_CLEAR_IDX((_pObj), (_pCtx)->id))
# define CR_STATE_SHAREDOBJ_USAGE_IS_USED(_pObj) (ASMBitFirstSet((_pObj)->ctxUsage, sizeof ((_pObj)->ctxUsage)<<3) >= 0)
# define CR_STATE_SHAREDOBJ_USAGE_GET_FIRST_USED_IDX(_pObj) (ASMBitFirstSet((_pObj)->ctxUsage, sizeof ((_pObj)->ctxUsage)<<3))
# define CR_STATE_SHAREDOBJ_USAGE_GET_NEXT_USED_IDX(_pObj, _i) (ASMBitNextSet((_pObj)->ctxUsage, sizeof ((_pObj)->ctxUsage)<<3, (_i)))

# define CR_STATE_SHAREDOBJ_USAGE_FOREACH_USED_IDX(_pObj, _i) for ((_i) = CR_STATE_SHAREDOBJ_USAGE_GET_FIRST_USED_IDX(_pObj); ((int)(_i)) >= 0; (_i) = CR_STATE_SHAREDOBJ_USAGE_GET_NEXT_USED_IDX((_pObj), ((int)(_i))))

#define CR_MAX_EXTENTS 256

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bit vectors describing GL state
 */
typedef struct {
    CRAttribBits      attrib;
    CRBufferBits      buffer;
#ifdef CR_ARB_vertex_buffer_object
    CRBufferObjectBits bufferobject;
#endif
    CRClientBits      client;
    CRCurrentBits     current;
    CREvaluatorBits   eval;
    CRFeedbackBits    feedback;
    CRFogBits         fog;
    CRHintBits        hint;
    CRLightingBits    lighting;
    CRLineBits        line;
    CRListsBits       lists;
    CRMultisampleBits multisample;
#if CR_ARB_occlusion_query
    CROcclusionBits   occlusion;
#endif
    CRPixelBits       pixel;
    CRPointBits       point;
    CRPolygonBits     polygon;
    CRProgramBits     program;
    CRRegCombinerBits regcombiner;
    CRSelectionBits   selection;
    CRStencilBits     stencil;
    CRTextureBits     texture;
    CRTransformBits   transform;
    CRViewportBits    viewport;
} CRStateBits;

typedef void (*CRStateFlushFunc)( void *arg );


typedef struct _CRSharedState {
    CRHashTable *textureTable;  /* all texture objects */
    CRHashTable *dlistTable;    /* all display lists */
    CRHashTable *buffersTable;  /* vbo/pbo */
    CRHashTable *fbTable;       /* frame buffers */
    CRHashTable *rbTable;       /* render buffers */

    volatile int32_t refCount;
    GLint id;                   /*unique shared state id, it's not always matching some existing context id!*/
    GLint saveCount;

    /* Indicates that we have to resend data to GPU on first glMakeCurrent call with owning context */
    GLboolean   bTexResyncNeeded;
    GLboolean   bVBOResyncNeeded;
    GLboolean   bFBOResyncNeeded;
} CRSharedState;

/**
 * Chromium version of the state variables in OpenGL
 */
struct CRContext {
    int id;

    /* we keep reference counting of context's makeCurrent for different threads
     * this is primarily needed to avoid having an invalid memory reference in the TLS
     * when the context is assigned to more than one threads and then destroyed from
     * one of those, i.e.
     * 1. Thread1 -> MakeCurrent(ctx1);
     * 2. Thread2 -> MakeCurrent(ctx1);
     * 3. Thread1 -> Destroy(ctx1);
     * => Thread2 still refers to destroyed ctx1
     * */
    VBOXTLSREFDATA

    CRbitvalue bitid[CR_MAX_BITARRAY];
    CRbitvalue neg_bitid[CR_MAX_BITARRAY];

    CRSharedState *shared;

    GLenum     renderMode;

    GLenum     error;

    CRStateFlushFunc flush_func;
    void            *flush_arg;

    CRAttribState      attrib;
    CRBufferState      buffer;
#ifdef CR_ARB_vertex_buffer_object
    CRBufferObjectState bufferobject;
#endif
    CRClientState      client;
    CRCurrentState     current;
    CREvaluatorState   eval;
    CRExtensionState   extensions;
    CRFeedbackState    feedback;
    CRFogState         fog;
    CRHintState        hint;
    CRLightingState    lighting;
    CRLimitsState      limits;
    CRLineState        line;
    CRListsState       lists;
    CRMultisampleState multisample;
#if CR_ARB_occlusion_query
    CROcclusionState   occlusion;
#endif
    CRPixelState       pixel;
    CRPointState       point;
    CRPolygonState     polygon;
    CRProgramState     program;
    CRRegCombinerState regcombiner;
    CRSelectionState   selection;
    CRStencilState     stencil;
    CRTextureState     texture;
    CRTransformState   transform;
    CRViewportState    viewport;

#ifdef CR_EXT_framebuffer_object
    CRFramebufferObjectState    framebufferobject;
#endif

#ifdef CR_OPENGL_VERSION_2_0
    CRGLSLState        glsl;
#endif

    /** The state tracker the context is attached to. */
    PCRStateTracker    pStateTracker;

    /** For buffering vertices for selection/feedback */
    /*@{*/
    GLuint    vCount;
    CRVertex  vBuffer[4];
    GLboolean lineReset;
    GLboolean lineLoop;
    /*@}*/
};


/**
 * CR state tracker.
 */
typedef struct CRStateTracker
{
    bool             fContextTLSInit;
    CRtsd            contextTSD;
    CRStateBits      *pCurrentBits;
    CRContext        *apAvailableContexts[CR_MAX_CONTEXTS];
    uint32_t         cContexts;
    CRSharedState    *pSharedState;
    CRContext        *pDefaultContext;
    GLboolean        fVBoxEnableDiffOnMakeCurrent;
    SPUDispatchTable diff_api;
} CRStateTracker;

DECLEXPORT(void) crStateInit(PCRStateTracker pState);
DECLEXPORT(void) crStateDestroy(PCRStateTracker pState);
DECLEXPORT(void) crStateVBoxDetachThread(PCRStateTracker pState);
DECLEXPORT(void) crStateVBoxAttachThread(PCRStateTracker pState);
DECLEXPORT(CRContext *) crStateCreateContext(PCRStateTracker pState, const CRLimitsState *limits, GLint visBits, CRContext *share);
DECLEXPORT(CRContext *) crStateCreateContextEx(PCRStateTracker pState, const CRLimitsState *limits, GLint visBits, CRContext *share, GLint presetID);
DECLEXPORT(void) crStateMakeCurrent(PCRStateTracker pState, CRContext *ctx);
DECLEXPORT(void) crStateSetCurrent(PCRStateTracker pState, CRContext *ctx);
DECLEXPORT(void) crStateCleanupCurrent(PCRStateTracker pState);
DECLEXPORT(CRContext *) crStateGetCurrent(PCRStateTracker pState);
DECLEXPORT(void) crStateDestroyContext(PCRStateTracker pState, CRContext *ctx);
DECLEXPORT(GLboolean) crStateEnableDiffOnMakeCurrent(PCRStateTracker pState, GLboolean fEnable);

void crStateSwitchPrepare(CRContext *toCtx, CRContext *fromCtx, GLuint idDrawFBO, GLuint idReadFBO);
void crStateSwitchPostprocess(CRContext *toCtx, CRContext *fromCtx, GLuint idDrawFBO, GLuint idReadFBO);

void crStateSyncHWErrorState(CRContext *ctx);
GLenum crStateCleanHWErrorState(PCRStateTracker pState);

#define CR_STATE_CLEAN_HW_ERR_WARN(a_pState, _s) do {\
            GLenum _err = crStateCleanHWErrorState((a_pState)); \
            if (_err != GL_NO_ERROR) { \
                static int _cErrPrints = 0; \
                if (_cErrPrints < 5) { \
                    ++_cErrPrints; \
                    WARN(("%s %#x, ignoring.. (%d out of 5)", _s, _err, _cErrPrints)); \
                } \
            } \
        } while (0)

DECLEXPORT(void) crStateFlushFunc(PCRStateTracker pState, CRStateFlushFunc ff);
DECLEXPORT(void) crStateFlushArg(PCRStateTracker pState, void *arg );
DECLEXPORT(void) crStateDiffAPI(PCRStateTracker pState, SPUDispatchTable *api);
DECLEXPORT(void) crStateUpdateColorBits(PCRStateTracker pState);

DECLEXPORT(void) crStateSetCurrentPointers(CRContext *ctx, CRCurrentStatePointers *current);
DECLEXPORT(void) crStateResetCurrentPointers(CRCurrentStatePointers *current);

DECLEXPORT(void) crStateSetExtensionString(CRContext *ctx, const GLubyte *extensions);

DECLEXPORT(void) crStateDiffContext(CRContext *from, CRContext *to);
DECLEXPORT(void) crStateSwitchContext(CRContext *from, CRContext *to);

DECLEXPORT(unsigned int) crStateHlpComponentsCount(GLenum pname);

typedef struct CRFBDataElement
{
    /* FBO, can be NULL */
    GLint idFBO;
    /* to be used for glDraw/ReadBuffer, i.e. GL_FRONT, GL_BACK, GL_COLOR_ATTACHMENTX */
    GLenum enmBuffer;
    GLint posX;
    GLint posY;
    GLint width;
    GLint height;
    GLenum enmFormat;
    GLenum enmType;
    GLuint cbData;
    GLvoid *pvData;
} CRFBDataElement;

typedef struct CRFBData
{
    /* override default draw and read buffers to be used for offscreen rendering */
    GLint idOverrrideFBO;
    uint32_t u32Version;
    uint32_t cElements;
    CRFBDataElement aElements[1];
} CRFBData;

DECLEXPORT(void) crStateApplyFBImage(CRContext *to, CRFBData *data);
DECLEXPORT(int) crStateAcquireFBImage(CRContext *to, CRFBData *data);
DECLEXPORT(void) crStateFreeFBImageLegacy(CRContext *to);

DECLEXPORT(void) crStateGetTextureObjectAndImage(CRContext *g, GLenum texTarget, GLint level,
                                                 CRTextureObj **obj, CRTextureLevel **img);


DECLEXPORT(void) crStateReleaseTexture(CRContext *pCtx, CRTextureObj *pObj);

#ifndef IN_GUEST
DECLEXPORT(int32_t) crStateSaveContext(CRContext *pContext, PSSMHANDLE pSSM);
typedef DECLCALLBACK(CRContext*) FNCRSTATE_CONTEXT_GET(void*);
typedef FNCRSTATE_CONTEXT_GET *PFNCRSTATE_CONTEXT_GET;
DECLEXPORT(int32_t) crStateLoadContext(CRContext *pContext, CRHashTable * pCtxTable, PFNCRSTATE_CONTEXT_GET pfnCtxGet, PSSMHANDLE pSSM, uint32_t u32Version);
DECLEXPORT(void) crStateFreeShared(PCRStateTracker pState, CRContext *pContext, CRSharedState *s);

DECLEXPORT(int32_t) crStateLoadGlobals(PCRStateTracker pState, PSSMHANDLE pSSM, uint32_t u32Version);
DECLEXPORT(int32_t) crStateSaveGlobals(PCRStateTracker pState, PSSMHANDLE pSSM);

DECLEXPORT(CRSharedState *) crStateGlobalSharedAcquire(PCRStateTracker pState);
DECLEXPORT(void) crStateGlobalSharedRelease(PCRStateTracker pState);
#endif

DECLEXPORT(void) crStateSetTextureUsed(PCRStateTracker pState, GLuint texture, GLboolean used);
DECLEXPORT(void) crStatePinTexture(PCRStateTracker pState, GLuint texture, GLboolean pin);
DECLEXPORT(void) crStateDeleteTextureCallback(void *texObj, void *pvUser);

   /* XXX move these! */

DECLEXPORT(void) STATE_APIENTRY crStateChromiumParameteriCR(PCRStateTracker pState, GLenum target, GLint value );
DECLEXPORT(void) STATE_APIENTRY crStateChromiumParameterfCR(PCRStateTracker pState, GLenum target, GLfloat value );
DECLEXPORT(void) STATE_APIENTRY crStateChromiumParametervCR(PCRStateTracker pState, GLenum target, GLenum type, GLsizei count, const GLvoid *values );
DECLEXPORT(void) STATE_APIENTRY crStateGetChromiumParametervCR(PCRStateTracker pState, GLenum target, GLuint index, GLenum type,
                                                               GLsizei count, GLvoid *values );
DECLEXPORT(void) STATE_APIENTRY crStateReadPixels(PCRStateTracker pState, GLint x, GLint y, GLsizei width, GLsizei height,
                                                  GLenum format, GLenum type, GLvoid *pixels );
DECLEXPORT(void) STATE_APIENTRY crStateShareContext(PCRStateTracker pState, GLboolean value);
DECLEXPORT(void) STATE_APIENTRY crStateShareLists(CRContext *pContext1, CRContext *pContext2);
DECLEXPORT(void) STATE_APIENTRY crStateSetSharedContext(CRContext *pCtx);
DECLEXPORT(GLboolean) STATE_APIENTRY crStateContextIsShared(CRContext *pCtx);
DECLEXPORT(void) STATE_APIENTRY crStateQueryHWState(PCRStateTracker pState, GLuint fbFbo, GLuint bbFbo);
#ifdef __cplusplus
}
#endif

#endif /* CR_GLSTATE_H */
