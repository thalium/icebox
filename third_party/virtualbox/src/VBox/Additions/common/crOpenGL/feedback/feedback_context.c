/* $Id: feedback_context.c $ */
/** @file
 * VBox feedback spu, context tracking.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "cr_spu.h"
#include "cr_error.h"
#include "feedbackspu.h"

/** @todo r=bird: None of the code here is referenced externally, so I've
 *        just prototyped the function here at the top of the file to make
 *        the compiler happy. */
GLint FEEDBACKSPU_APIENTRY  feedbackspu_VBoxCreateContext( GLint con, const char *dpyName, GLint visual, GLint shareCtx );
GLint FEEDBACKSPU_APIENTRY  feedbackspu_CreateContext( const char *dpyName, GLint visual, GLint shareCtx );
void FEEDBACKSPU_APIENTRY   feedbackspu_MakeCurrent( GLint window, GLint nativeWindow, GLint ctx );
void FEEDBACKSPU_APIENTRY   feedbackspu_DestroyContext( GLint ctx );


/** @todo Multithreading case. (See feedback_spu.self.RenderMode)*/

GLint FEEDBACKSPU_APIENTRY
feedbackspu_VBoxCreateContext( GLint con, const char *dpyName, GLint visual, GLint shareCtx )
{
    GLint ctx, slot;

    crLockMutex(&feedback_spu.mutex);
    ctx = feedback_spu.child.VBoxCreateContext(con, dpyName, visual, shareCtx);

    /* find an empty context slot */
    for (slot = 0; slot < feedback_spu.numContexts; slot++) {
        if (!feedback_spu.context[slot].clientState) {
            /* found empty slot */
            break;
        }
    }
    if (slot == feedback_spu.numContexts) {
        feedback_spu.numContexts++;
    }

    feedback_spu.context[slot].clientState = crStateCreateContext(&feedback_spu.StateTracker, NULL, visual, NULL);
    feedback_spu.context[slot].clientCtx = ctx;

    crUnlockMutex(&feedback_spu.mutex);
    return ctx;
}

GLint FEEDBACKSPU_APIENTRY
feedbackspu_CreateContext( const char *dpyName, GLint visual, GLint shareCtx )
{
    return feedbackspu_VBoxCreateContext( 0, dpyName, visual, shareCtx );
}

void FEEDBACKSPU_APIENTRY
feedbackspu_MakeCurrent( GLint window, GLint nativeWindow, GLint ctx )
{
    crLockMutex(&feedback_spu.mutex);
    feedback_spu.child.MakeCurrent(window, nativeWindow, ctx);

    if (ctx) {
        int slot;
        GLint oldmode;

        for (slot=0; slot<feedback_spu.numContexts; ++slot)
            if (feedback_spu.context[slot].clientCtx == ctx) break;
        CRASSERT(slot < feedback_spu.numContexts);

        crStateMakeCurrent(&feedback_spu.StateTracker, feedback_spu.context[slot].clientState);

        crStateGetIntegerv(&feedback_spu.StateTracker, GL_RENDER_MODE, &oldmode);

        if (oldmode!=feedback_spu.render_mode)
        {
            feedback_spu.self.RenderMode(oldmode);
        }
    }
    else
    {
        crStateMakeCurrent(&feedback_spu.StateTracker, NULL);
    }

    crUnlockMutex(&feedback_spu.mutex);
}

void FEEDBACKSPU_APIENTRY
feedbackspu_DestroyContext( GLint ctx )
{
    crLockMutex(&feedback_spu.mutex);
    feedback_spu.child.DestroyContext(ctx);

    if (ctx) {
        int slot;

        for (slot=0; slot<feedback_spu.numContexts; ++slot)
            if (feedback_spu.context[slot].clientCtx == ctx) break;
        CRASSERT(slot < feedback_spu.numContexts);

        crStateDestroyContext(&feedback_spu.StateTracker, feedback_spu.context[slot].clientState);

        feedback_spu.context[slot].clientState = NULL;
        feedback_spu.context[slot].clientCtx = 0;
    }

    crUnlockMutex(&feedback_spu.mutex);
}

