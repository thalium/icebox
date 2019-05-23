/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef ARRAY_SPU_H
#define ARRAY_SPU_H

#ifdef WINDOWS
#define ARRAYSPU_APIENTRY __stdcall
#else
#define ARRAYSPU_APIENTRY
#endif

#include "cr_spu.h"
#include "cr_glstate.h"

void arrayspuSetVBoxConfiguration( void );

typedef struct context_info_t ContextInfo;

struct context_info_t {
    CRContext *clientState;  /* used to store client-side GL state */
    GLint clientCtx;         /* client context ID */
};

typedef struct {
    int id;
    int has_child;
    CRContext *defaultctx;
    SPUDispatchTable self, child, super;
    int numContexts;
    ContextInfo context[CR_MAX_CONTEXTS];
} ArraySPU;

extern ArraySPU array_spu;

#ifdef CHROMIUM_THREADSAFE
extern CRmutex _ArrayMutex;
#endif

#endif /* ARRAY_SPU_H */
