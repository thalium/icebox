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
        crStateError(pState, __LINE__, __FILE__, result, message);  \
        return ret;                                             \
    }

#define CRSTATE_NO_RETURN

#define CRSTATE_CHECKERR(expr, result, message) CRSTATE_CHECKERR_RET(expr, result, message, CRSTATE_NO_RETURN)

typedef struct _crCheckIDHWID {
    GLuint id, hwid;
    PCRStateTracker pState;
} crCheckIDHWID_t;

#define GetCurrentBits(a_pState) (a_pState)->pCurrentBits

#include <cr_threads.h>

#define GetCurrentContext(a_pState) VBoxTlsRefGetCurrent(CRContext, &((a_pState)->contextTSD))

/* NOTE: below SetCurrentContext stuff is supposed to be used only internally!!
 * it is placed here only to simplify things since some code besides state_init.c
 * (i.e. state_glsl.c) is using it */
#define SetCurrentContext(a_pState, _ctx) VBoxTlsRefSetCurrent(CRContext, &((a_pState)->contextTSD), _ctx)

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

void crStateFreeBufferObject(void *data, void *pvUser);
void crStateFreeFBO(void *data, void *pvUser);
void crStateFreeRBO(void *data, void *pvUser);

void crStateGenNames(CRContext *g, CRHashTable *table, GLsizei n, GLuint *names);
void crStateRegNames(CRContext *g, CRHashTable *table, GLsizei n, GLuint *names);
void crStateOnTextureUsageRelease(PCRStateTracker pState, CRSharedState *pS, CRTextureObj *pObj);
#endif
