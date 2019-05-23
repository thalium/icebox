/* $Id: expandospu.c $ */
/** @file
 * Implementation of routines which Expando SPU explicitly overrides.
 */

/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */


/*
 * Copyright (C) 2015-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <stdio.h>
#include "cr_spu.h"
#include "cr_dlm.h"
#include "cr_mem.h"
#include "expandospu.h"

extern GLint EXPANDOSPU_APIENTRY
expandoCreateContext(const char *displayName, GLint visBits, GLint shareCtx)
{
    ExpandoContextState *contextState;

    /* Allocate our own per-context record */
    contextState = crCalloc(sizeof(ExpandoContextState));
    if (contextState)
    {
        GLint contextId;

        /* Get an official context ID from our super */
        contextId = expando_spu.super.CreateContext(displayName, visBits, shareCtx);

        /* Supplement that with our DLM.  In a more correct situation, we should
         * see if we've been called through glXCreateContext, which has a parameter
         * for sharing DLMs.  We don't currently get that information, so for now
         * give each context its own DLM.
         */
        contextState->dlm = crDLMNewDLM(0, NULL);
        if (contextState->dlm)
        {
            contextState->dlmContext = crDLMNewContext(contextState->dlm);
            if (contextState->dlmContext)
            {
                /* The DLM needs us to use the state tracker to track client
                 * state, so we can compile client-state-using functions correctly.
                 */
                contextState->State = crStateCreateContext(NULL, visBits, NULL);

                /* Associate the Expando context with the user context. */
                crHashtableAdd(expando_spu.contextTable, contextId, (void *)contextState);

                crDebug("Expando SPU: created context %d (contextState=%p, contextState->dlm=%p, "
                        "contextState->dlmContext=%p, contextState->State=%p).",
                        contextId, contextState, contextState->dlm, contextState->dlmContext, contextState->State);

                return contextId;
            }
            else
                crError("Expando SPU: can't allocate new DLM context.");

            crDLMFreeDLM(contextState->dlm, &expando_spu.super);
        }
        else
            crError("Expando SPU: can't allocate new DLM.");

        crFree(contextState);
    }
    else
        crError("Expando SPU: couldn't allocate per-context state");

    return 0;
}

void expando_free_context_state(void *data)
{
    ExpandoContextState *contextState = (ExpandoContextState *)data;

    crDebug("Expando SPU: destroying context internals: "
            "contextState=%p, contextState->dlm=%p, contextState->dlmContext=%p, contextState->State=%p",
            contextState, contextState->dlm, contextState->dlmContext, contextState->State);

    crDLMFreeContext(contextState->dlmContext, &expando_spu.super);
    crDLMFreeDLM(contextState->dlm, &expando_spu.super);
    crStateDestroyContext(contextState->State);
    crFree(contextState);
}

void EXPANDOSPU_APIENTRY
expandoDestroyContext(GLint contextId)
{
    crDebug("Expando SPU: destroy context %d.", contextId);

    /* Destroy our context information */
    crHashtableDelete(expando_spu.contextTable, contextId, expando_free_context_state);

    /* Pass along the destruction to our super. */
    expando_spu.super.DestroyContext(contextId);
}

void EXPANDOSPU_APIENTRY
expandoMakeCurrent(GLint crWindow, GLint nativeWindow, GLint contextId)
{
    ExpandoContextState *expandoContextState;

    expando_spu.super.MakeCurrent(crWindow, nativeWindow, contextId);

    expandoContextState = crHashtableSearch(expando_spu.contextTable, contextId);
    if (expandoContextState)
    {
        crDebug("Expando SPU: switch to context %d.", contextId);

        crDLMSetCurrentState(expandoContextState->dlmContext);
        crStateMakeCurrent(expandoContextState->State);
    }
    else
    {
        crDebug("Expando SPU: can't switch to context %d: not found.", contextId);

        crDLMSetCurrentState(NULL);
        crStateMakeCurrent(NULL);
    }
}

extern void EXPANDOSPU_APIENTRY
expandoNewList(GLuint list, GLenum mode)
{
    crDLMNewList(list, mode, &expando_spu.super);
}

extern void EXPANDOSPU_APIENTRY
expandoEndList(void)
{
    crDLMEndList(&expando_spu.super);
}

extern void EXPANDOSPU_APIENTRY
expandoDeleteLists(GLuint first, GLsizei range)
{
    crDLMDeleteLists(first, range, &expando_spu.super);
}

extern GLuint EXPANDOSPU_APIENTRY
expandoGenLists(GLsizei range)
{
    return crDLMGenLists(range, &expando_spu.super);
}

void EXPANDOSPU_APIENTRY
expandoListBase(GLuint base)
{
    crDLMListBase(base, &expando_spu.super);
}

extern GLboolean EXPANDOSPU_APIENTRY
expandoIsList(GLuint list)
{
    return crDLMIsList(list, &expando_spu.super);
}

extern void EXPANDOSPU_APIENTRY
expandoCallList(GLuint list)
{
    crDLMCallList(list, &expando_spu.super);
}

extern void EXPANDOSPU_APIENTRY
expandoCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    crDLMCallLists(n, type, lists, &expando_spu.super);
}
