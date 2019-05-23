/** @file
 *
 * VBox HDD container test utility - scripting engine, internal stack implementation.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef _VDScriptStack_h__
#define _VDScriptStack_h__

#include <iprt/list.h>
#include <iprt/string.h>

#include "VDScript.h"

/**
 * Stack structure.
 */
typedef struct VDSCRIPTSTACK
{
    /** Size of one stack element. */
    size_t          cbStackEntry;
    /** Stack memory. */
    void           *pvStack;
    /** Number of elements on the stack. */
    unsigned        cOnStack;
    /** Maximum number of elements the stack can hold. */
    unsigned        cOnStackMax;
} VDSCRIPTSTACK;
/** Pointer to a stack. */
typedef VDSCRIPTSTACK *PVDSCRIPTSTACK;

/**
 * Init the stack structure.
 *
 * @returns nothing.
 * @param   pStack          The stack to initialize.
 * @param   cbStackEntry    The size of one stack entry.
 */
DECLINLINE(void) vdScriptStackInit(PVDSCRIPTSTACK pStack, size_t cbStackEntry)
{
    pStack->cbStackEntry = cbStackEntry;
    pStack->pvStack      = NULL;
    pStack->cOnStack     = 0;
    pStack->cOnStackMax  = 0;
}

/**
 * Destroys the given stack freeing all memory.
 *
 * @returns nothing.
 * @param   pStack         The stack to destroy.
 */
DECLINLINE(void) vdScriptStackDestroy(PVDSCRIPTSTACK pStack)
{
    if (pStack->pvStack)
        RTMemFree(pStack->pvStack);
    pStack->cbStackEntry = 0;
    pStack->pvStack      = NULL;
    pStack->cOnStack     = 0;
    pStack->cOnStackMax  = 0;
}

/**
 * Gets the topmost unused stack entry.
 *
 * @returns Pointer to the first unused entry.
 *          NULL if there is no room left and increasing the stack failed.
 * @param   pStack          The stack.
 */
DECLINLINE(void *)vdScriptStackGetUnused(PVDSCRIPTSTACK pStack)
{
    void *pvElem = NULL;

    if (pStack->cOnStack >= pStack->cOnStackMax)
    {
        unsigned cOnStackMaxNew = pStack->cOnStackMax + 10;
        void *pvStackNew = NULL;

        /* Try to increase stack space. */
        pvStackNew = RTMemRealloc(pStack->pvStack, cOnStackMaxNew * pStack->cbStackEntry);
        if (pvStackNew)
        {
            pStack->pvStack     = pvStackNew;
            pStack->cOnStackMax = cOnStackMaxNew;
        }

    }

    if (pStack->cOnStack < pStack->cOnStackMax)
        pvElem = (char *)pStack->pvStack + pStack->cOnStack * pStack->cbStackEntry;

    return pvElem;
}

/**
 * Gets the topmost used entry on the stack.
 *
 * @returns Pointer to the first used entry
 *          or NULL if the stack is empty.
 * @param   pStack          The stack.
 */
DECLINLINE(void *)vdScriptStackGetUsed(PVDSCRIPTSTACK pStack)
{
    if (!pStack->cOnStack)
        return NULL;
    else
        return (char *)pStack->pvStack + (pStack->cOnStack - 1) * pStack->cbStackEntry;
}

/**
 * Increases the used element count for the given stack.
 *
 * @returns nothing.
 * @param   pStack          The stack.
 */
DECLINLINE(void) vdScriptStackPush(PVDSCRIPTSTACK pStack)
{
    pStack->cOnStack++;
}

/**
 * Decreases the used element count for the given stack.
 *
 * @returns nothing.
 * @param   pStack          The stack.
 */
DECLINLINE(void) vdScriptStackPop(PVDSCRIPTSTACK pStack)
{
    pStack->cOnStack--;
}

#endif /* _VDScriptStack_h__ */
