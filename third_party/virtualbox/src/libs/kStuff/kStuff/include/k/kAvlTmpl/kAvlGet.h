/* $Id: kAvlGet.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kAvlTmpl - Templated AVL Trees, Get a Node.
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
 * Gets a node from the tree (does not remove it!)
 *
 * @returns Pointer to the node holding the given key.
 * @param   pRoot       Pointer to the AVL-tree root structure.
 * @param   Key         Key value of the node which is to be found.
 */
KAVL_DECL(KAVLNODE *) KAVL_FN(Get)(KAVLROOT *pRoot, KAVLKEY Key)
{
    KAVLNODE *pNode;
#ifdef KAVL_LOOKTHRU_CACHE
    KAVLTREEPTR *ppEntry;
#endif

    KAVL_READ_LOCK(pRoot);
    if (pRoot->mpRoot == KAVL_NULL)
    {
        KAVL_READ_UNLOCK(pRoot);
        return NULL;
    }

#ifdef KAVL_LOOKTHRU_CACHE
    ppEntry = &pRoot->maLookthru[KAVL_LOOKTHRU_HASH(Key)];
    pNode = KAVL_GET_POINTER_NULL(ppEntry);
    if (!pNode || KAVL_NE(pNode->mKey, Key))
#endif
    {
        pNode = KAVL_GET_POINTER(&pRoot->mpRoot);
        while (KAVL_NE(pNode->mKey, Key))
        {
            if (KAVL_G(pNode->mKey, Key))
            {
                if (pNode->mpLeft == KAVL_NULL)
                {
                    KAVL_READ_UNLOCK(pRoot);
                    return NULL;
                }
                pNode = KAVL_GET_POINTER(&pNode->mpLeft);
            }
            else
            {
                if (pNode->mpRight == KAVL_NULL)
                {
                    KAVL_READ_UNLOCK(pRoot);
                    return NULL;
                }
                pNode = KAVL_GET_POINTER(&pNode->mpRight);
            }
        }

#ifdef KAVL_LOOKTHRU_CACHE
        KAVL_SET_POINTER(ppEntry, pNode);
#endif
    }

    KAVL_READ_UNLOCK(pRoot);
    return pNode;
}

