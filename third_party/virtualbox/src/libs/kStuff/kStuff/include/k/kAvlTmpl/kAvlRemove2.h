/* $Id: kAvlRemove2.h 34 2009-11-08 19:38:40Z bird $ */
/** @file
 * kAvlTmpl - Templated AVL Trees, Remove A Specific Node.
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
 * Removes the specified node from the tree.
 *
 * @returns Pointer to the removed node (NULL if not in the tree)
 * @param   pRoot       Pointer to the AVL-tree root structure.
 * @param   Key         The Key of which is to be found a best fitting match for..
 *
 * @remark  This implementation isn't the most efficient, but this short and
 *          easier to manage.
 */
KAVL_DECL(KAVLNODE *) KAVL_FN(Remove2)(KAVLROOT *pRoot, KAVLNODE *pNode)
{
#ifdef KAVL_EQUAL_ALLOWED
    /*
     * Find the right node by key and see if it's what we want.
     */
    KAVLNODE *pParent;
    KAVLNODE *pCurNode = KAVL_FN(GetWithParent)(pRoot, pNode->mKey, &pParent);
    if (!pCurNode)
        return NULL;
    KAVL_WRITE_LOCK(pRoot); /** @todo the locking here isn't 100% sane. The only way to archive that is by no calling worker functions. */
    if (pCurNode != pNode)
    {
        /*
         * It's not the one we want, but it could be in the duplicate list.
         */
        while (pCurNode->mpList != KAVL_NULL)
        {
            KAVLNODE *pNext = KAVL_GET_POINTER(&pCurNode->mpList);
            if (pNext == pNode)
            {
                KAVL_SET_POINTER_NULL(&pCurNode->mpList, KAVL_GET_POINTER_NULL(&pNode->mpList));
                pNode->mpList = KAVL_NULL;
                KAVL_LOOKTHRU_INVALIDATE_NODE(pRoot, pNode, pNode->mKey);
                KAVL_WRITE_UNLOCK(pRoot);
                return pNode;
            }
            pCurNode = pNext;
        }
        KAVL_WRITE_UNLOCK(pRoot);
        return NULL;
    }

    /*
     * Ok, it's the one we want alright.
     *
     * Simply remove it if it's the only one with they Key,
     * if there are duplicates we'll have to unlink it and
     * insert the first duplicate in our place.
     */
    if (pNode->mpList == KAVL_NULL)
    {
        KAVL_WRITE_UNLOCK(pRoot);
        KAVL_FN(Remove)(pRoot, pNode->mKey);
    }
    else
    {
        KAVLNODE *pNewUs = KAVL_GET_POINTER(&pNode->mpList);

        pNewUs->mHeight = pNode->mHeight;

        if (pNode->mpLeft != KAVL_NULL)
            KAVL_SET_POINTER(&pNewUs->mpLeft, KAVL_GET_POINTER(&pNode->mpLeft))
        else
            pNewUs->mpLeft = KAVL_NULL;

        if (pNode->mpRight != KAVL_NULL)
            KAVL_SET_POINTER(&pNewUs->mpRight, KAVL_GET_POINTER(&pNode->mpRight))
        else
            pNewUs->mpRight = KAVL_NULL;

        if (pParent)
        {
            if (KAVL_GET_POINTER_NULL(&pParent->mpLeft) == pNode)
                KAVL_SET_POINTER(&pParent->mpLeft, pNewUs);
            else
                KAVL_SET_POINTER(&pParent->mpRight, pNewUs);
        }
        else
            KAVL_SET_POINTER(&pRoot->mpRoot, pNewUs);

        KAVL_LOOKTHRU_INVALIDATE_NODE(pRoot, pNode, pNode->mKey);
        KAVL_WRITE_UNLOCK(pRoot);
    }

    return pNode;

#else
    /*
     * Delete it, if we got the wrong one, reinsert it.
     *
     * This ASSUMS that the caller is NOT going to hand us a lot
     * of wrong nodes but just uses this API for his convenience.
     */
    KAVLNODE *pRemovedNode = KAVL_FN(Remove)(pRoot, pNode->mKey);
    if (pRemovedNode == pNode)
        return pRemovedNode;

    KAVL_FN(Insert)(pRoot, pRemovedNode);
    return NULL;
#endif
}

