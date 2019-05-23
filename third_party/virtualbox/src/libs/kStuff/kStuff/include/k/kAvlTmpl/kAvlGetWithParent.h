/* $Id: kAvlGetWithParent.h 34 2009-11-08 19:38:40Z bird $ */
/** @file
 * kAvlTmpl - Templated AVL Trees, Get Node With Parent.
 */

/*
 * Copyright (c) 1999-2009 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Gets a node from the tree and its parent node (if any).
 * The tree remains unchanged.
 *
 * @returns Pointer to the node holding the given key.
 * @param   pRoot       Pointer to the AVL-tree root structure.
 * @param   ppParent    Pointer to a variable which will hold the pointer to the partent node on
 *                      return. When no node is found, this will hold the last searched node.
 * @param   Key         Key value of the node which is to be found.
 */
KAVL_DECL(KAVLNODE *) KAVL_FN(GetWithParent)(KAVLROOT *pRoot, KAVLNODE **ppParent, KAVLKEY Key)
{
    register KAVLNODE *pNode;
    register KAVLNODE *pParent;

    KAVL_READ_LOCK(pRoot);

    pParent = NULL;
    pNode = KAVL_GET_POINTER_NULL(&pRoot->mpRoot);
    while (     pNode != NULL
           &&   KAVL_NE(pNode->mKey, Key))
    {
        pParent = pNode;
        if (KAVL_G(pNode->mKey, Key))
            pNode = KAVL_GET_POINTER_NULL(&pNode->mpLeft);
        else
            pNode = KAVL_GET_POINTER_NULL(&pNode->mpRight);
    }

    KAVL_READ_UNLOCK(pRoot);

    *ppParent = pParent;
    return pNode;
}

