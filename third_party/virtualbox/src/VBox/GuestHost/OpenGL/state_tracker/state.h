/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef STATE_H
#define STATE_H

#include "cr_glstate.h"

#define CRSTATE_CHECKERR_RET(expr, result, message, ret)         \
    if (expr) {                                             \
        crStateError(__LINE__, __FILE__, result, message);  \
        return ret;                                             \
    }

#define CRSTATE_NO_RETURN

#define CRSTATE_CHECKERR(expr, result, message) CRSTATE_CHECKERR_RET(expr, result, message, CRSTATE_NO_RETURN)

typedef struct _crCheckIDHWID {
    GLuint id, hwid;
} crCheckIDHWID_t;

extern SPUDispatchTable diff_api;
extern CRStateBits *__currentBits;

#define GetCurrentBits() __currentBits

#ifdef CHROMIUM_THREADSAFE
#include <cr_threads.h>

extern CRtsd __contextTSD;
#define GetCurrentContext() VBoxTlsRefGetCurrent(CRContext, &__contextTSD)

/* NOTE: below SetCurrentContext stuff is supposed to be used only internally!!
 * it is placed here only to simplify things since some code besides state_init.c
 * (i.e. state_glsl.c) is using it */
#define SetCurrentContext(_ctx) VBoxTlsRefSetCurrent(CRContext, &__contextTSD, _ctx)
#else
extern CRContext *__currentContext;
#define GetCurrentContext() __currentContext
#endif

extern GLboolean g_bVBoxEnableDiffOnMakeCurrent;

extern CRContext *g_pAvailableContexts[CR_MAX_CONTEXTS];
extern uint32_t g_cContexts;

extern void crStateTextureInitTextureObj (CRContext *ctx, CRTextureObj *tobj, GLuint name, GLenum target);
extern void crStateTextureInitTextureFormat( CRTextureLevel *tl, GLenum internalFormat );

/* Normally these functions would have been in cr_bufferobject.h but
 * that led to a number of issues.
 */
void crStateBufferObjectInit(CRContext *ctx);

void crStateBufferObjectDestroy (CRContext *ctx);

void crStateBufferObjectDiff(CRBufferObjectBits *bb, CRbitvalue *bitID,
                             CRContext *fromCtx, CRContext *toCtx);

void crStateBufferObjectSwitch(CRBufferObjectBits *bb, CRbitvalue *bitID, 
                               CRContext *fromCtx, CRContext *toCtx);

/* These would normally be in cr_client.h */

void crStateClientDiff(CRClientBits *cb, CRbitvalue *bitID, CRContext *from, CRContext *to);
                                             
void crStateClientSwitch(CRClientBits *cb, CRbitvalue *bitID,   CRContext *from, CRContext *to);

void crStateFreeBufferObject(void *data);
void crStateFreeFBO(void *data);
void crStateFreeRBO(void *data);

void crStateGenNames(CRContext *g, CRHashTable *table, GLsizei n, GLuint *names);
void crStateRegNames(CRContext *g, CRHashTable *table, GLsizei n, GLuint *names);
void crStateOnTextureUsageRelease(CRSharedState *pS, CRTextureObj *pObj);
#endif
