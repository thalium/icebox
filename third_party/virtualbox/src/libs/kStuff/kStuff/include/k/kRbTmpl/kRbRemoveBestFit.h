/* $Id: kRbRemoveBestFit.h 35 2009-11-08 19:39:03Z bird $ */
/** @file
 * kRbTmpl - Templated Red-Black Trees, Remove Best Fitting Node.
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
 * Finds the best fitting node in the tree for the given Key value and removes the node.
 *
 * @returns Pointer to the removed node.
 * @param   pRoot       Pointer to the Red-Back tree's root structure.
 * @param   Key         The Key of which is to be found a best fitting match for..
 * @param   fAbove      K_TRUE:  Returned node is have the closest key to Key from above.
 *                      K_FALSE: Returned node is have the closest key to Key from below.
 *
 * @remark  This implementation uses GetBestFit and then Remove and might therefore
 *          not be the most optimal kind of implementation, but it reduces the complexity
 *          code size, and the likelyhood for bugs.
 */
KRB_DECL(KRBNODE *) KRB_FN(RemoveBestFit)(KRBROOT *pRoot, KRBKEY Key, KBOOL fAbove)
{
    /*
     * If we find anything we'll have to remove the node and return it.
     * Now, if duplicate keys are allowed we'll remove a duplicate before
     * removing the in-tree node as this is way cheaper.
     */
    KRBNODE *pNode = KRB_FN(GetBestFit)(pRoot, Key, fAbove);
    if (pNode != NULL)
    {
#ifdef KRB_EQUAL_ALLOWED
        KRB_WRITE_LOCK(pRoot); /** @todo the locking isn't quite sane here. :-/ */
        if (pNode->mpList != KRB_NULL)
        {
            KRBNODE *pRet = KRB_GET_POINTER(&pNode->mpList);
            KRB_SET_POINTER_NULL(&pNode->mpList, &pRet->mpList);
            KRB_CACHE_INVALIDATE_NODE(pRoot, pNode, pNode->mKey);
            KRB_WRITE_UNLOCK(pRoot);
            return pRet;
        }
        KRB_WRITE_UNLOCK(pRoot);
#endif
        pNode = KRB_FN(Remove)(pRoot, pNode->mKey);
    }
    return pNode;
}

