/* $Id: kRbRemove2.h 35 2009-11-08 19:39:03Z bird $ */
/** @file
 * kRbTmpl - Templated Red-Black Trees, Remove A Specific Node.
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
 * @param   pRoot       Pointer to the Red-Back tree's root structure.
 * @param   Key         The Key of which is to be found a best fitting match for..
 *
 * @remark  This implementation isn't the most efficient, but this short and
 *          easier to manage.
 */
KRB_DECL(KRBNODE *) KRB_FN(Remove2)(KRBROOT *pRoot, KRBNODE *pNode)
{
#ifdef KRB_EQUAL_ALLOWED
    /*
     * Find the right node by key and see if it's what we want.
     */
    KRBNODE *pParent;
    KRBNODE *pCurNode = KRB_FN(GetWithParent)(pRoot, pNode->mKey, &pParent);
    if (!pCurNode)
        return NULL;
    KRB_WRITE_LOCK(pRoot); /** @todo the locking here isn't 100% sane. The only way to archive that is by no calling worker functions. */
    if (pCurNode != pNode)
    {
        /*
         * It's not the one we want, but it could be in the duplicate list.
         */
        while (pCurNode->mpList != KRB_NULL)
        {
            KRBNODE *pNext = KRB_GET_POINTER(&pCurNode->mpList);
            if (pNext == pNode)
            {
                KRB_SET_POINTER_NULL(&pCurNode->mpList, KRB_GET_POINTER_NULL(&pNode->mpList));
                pNode->mpList = KRB_NULL;
                KRB_CACHE_INVALIDATE_NODE(pRoot, pNode, pNode->mKey);
                KRB_WRITE_UNLOCK(pRoot);
                return pNode;
            }
            pCurNode = pNext;
        }
        KRB_WRITE_UNLOCK(pRoot);
        return NULL;
    }

    /*
     * Ok, it's the one we want alright.
     *
     * Simply remove it if it's the only one with they Key,
     * if there are duplicates we'll have to unlink it and
     * insert the first duplicate in our place.
     */
    if (pNode->mpList == KRB_NULL)
    {
        KRB_WRITE_UNLOCK(pRoot);
        KRB_FN(Remove)(pRoot, pNode->mKey);
    }
    else
    {
        KRBNODE *pNewUs = KRB_GET_POINTER(&pNode->mpList);

        pNewUs->mHeight = pNode->mHeight;

        if (pNode->mpLeft != KRB_NULL)
            KRB_SET_POINTER(&pNewUs->mpLeft, KRB_GET_POINTER(&pNode->mpLeft))
        else
            pNewUs->mpLeft = KRB_NULL;

        if (pNode->mpRight != KRB_NULL)
            KRB_SET_POINTER(&pNewUs->mpRight, KRB_GET_POINTER(&pNode->mpRight))
        else
            pNewUs->mpRight = KRB_NULL;

        if (pParent)
        {
            if (KRB_GET_POINTER_NULL(&pParent->mpLeft) == pNode)
                KRB_SET_POINTER(&pParent->mpLeft, pNewUs);
            else
                KRB_SET_POINTER(&pParent->mpRight, pNewUs);
        }
        else
            KRB_SET_POINTER(&pRoot->mpRoot, pNewUs);

        KRB_CACHE_INVALIDATE_NODE(pRoot, pNode, pNode->mKey);
        KRB_WRITE_UNLOCK(pRoot);
    }

    return pNode;

#else
    /*
     * Delete it, if we got the wrong one, reinsert it.
     *
     * This ASSUMS that the caller is NOT going to hand us a lot
     * of wrong nodes but just uses this API for his convenience.
     */
    KRBNODE *pRemovedNode = KRB_FN(Remove)(pRoot, pNode->mKey);
    if (pRemovedNode == pNode)
        return pRemovedNode;

    KRB_FN(Insert)(pRoot, pRemovedNode);
    return NULL;
#endif
}

