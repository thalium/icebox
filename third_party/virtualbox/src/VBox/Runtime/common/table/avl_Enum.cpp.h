/* $Id: avl_Enum.cpp.h $ */
/** @file
 * Enumeration routines for AVL trees.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef _kAVLEnum_h_
#define _kAVLEnum_h_


/**
 * Gets the root node.
 *
 * @returns Pointer to the root node.
 * @returns NULL if the tree is empty.
 *
 * @param   ppTree      Pointer to pointer to the tree root node.
 */
KAVL_DECL(PKAVLNODECORE) KAVL_FN(GetRoot)(PPKAVLNODECORE ppTree)
{
    return KAVL_GET_POINTER_NULL(ppTree);
}


/**
 * Gets the right node.
 *
 * @returns Pointer to the right node.
 * @returns NULL if no right node.
 *
 * @param   pNode       The current node.
 */
KAVL_DECL(PKAVLNODECORE)    KAVL_FN(GetRight)(PKAVLNODECORE pNode)
{
    if (pNode)
        return KAVL_GET_POINTER_NULL(&pNode->pRight);
    return NULL;
}


/**
 * Gets the left node.
 *
 * @returns Pointer to the left node.
 * @returns NULL if no left node.
 *
 * @param   pNode       The current node.
 */
KAVL_DECL(PKAVLNODECORE) KAVL_FN(GetLeft)(PKAVLNODECORE pNode)
{
    if (pNode)
        return KAVL_GET_POINTER_NULL(&pNode->pLeft);
    return NULL;
}


# ifdef KAVL_EQUAL_ALLOWED
/**
 * Gets the next node with an equal (start) key.
 *
 * @returns Pointer to the next equal node.
 * @returns NULL if the current node was the last one with this key.
 *
 * @param   pNode       The current node.
 */
KAVL_DECL(PKAVLNODECORE) KAVL_FN(GetNextEqual)(PKAVLNODECORE pNode)
{
    if (pNode)
        return KAVL_GET_POINTER_NULL(&pNode->pList);
    return NULL;
}
# endif /* KAVL_EQUAL_ALLOWED */

#endif

