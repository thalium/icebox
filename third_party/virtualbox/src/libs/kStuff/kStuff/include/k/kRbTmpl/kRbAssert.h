/* $Id: kRbAssert.h 38 2009-11-10 00:01:38Z bird $ */
/** @file
 * kRbTmpl - Templated Red-Black Trees, Assert Valid Tree.
 */

/*
 * Copyright (c) 2009 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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
 * Internal helper for KRB_FN(Assert)
 *
 * @returns The number of black nodes. -1 is return if the tree is invalid.
 * @param   pRoot       The root of the (sub-)tree to assert.
 */
K_DECL_INLINE(int) KRB_FN(AssertRecurse)(KRBNODE *pRoot)
{
    int cLeft;
    int cRight;

    if (!pRoot)
        /* leafs are black. */
        return 1;

#ifdef KRB_EQUAL_ALLOWED
    /* equal nodes are equal :) */
    if (pNode->mpList != KRB_NULL)
    {
        KRBROOT *pEqual;
        for (pEqual = KRB_GET_POINTER(&pNode->mpList); pEqual; pEqual = KRB_GET_POINTER_NULL(&pEqual->mpList))
            kHlpAssertReturn(K_CMP_E(pEqual->mKey, pNode->mKey), -1);
    }
#endif

    /* binary tree. */
    kHlpAssertReturn(pRoot->mpLeft  != KRB_NULL && KRB_CMP_G(KRB_GET_POINTER(&pRoot->mpLeft)->mpKey, pRoot->mKey), -1);
    kHlpAssertReturn(pRoot->mpRight != KRB_NULL && KRB_CMP_G(pRoot->mKey, KRB_GET_POINTER(&pRoot->mpRigth)->mpKey), -1);

    /* both children of red nodes are black. */
    kHlpAssertReturn(!KRB_IS_RED(pRoot) || (!KRB_IS_RED(pRoot->mpLeft) && !KRB_IS_RED(pRoot->mpRight)), -1);

    /* all paths to leafs contains the same number of black nodes. */
    cLeft  = KRB_FN(AssertRecurse)(KRB_GET_POINTER_NULL(&pRoot->mpLeft));
    cRight = KRB_FN(AssertRecurse)(KRB_GET_POINTER_NULL(&pRoot->mpRight));
    kHlpAssertMsgReturn(cLeft == cRight || cLeft == -1 || cRight == -1, ("%d vs. %d\n", cLeft, cRight), -1);

    return cLeft + !KRB_IS_RED(pRoot);
}


/**
 * Asserts the validity of the Red-Black tree.
 *
 * This method is using recursion and may therefore consume quite a bit of stack
 * on a large tree.
 *
 * @returns K_TRUE if valid.
 * @returns K_FALSE if invalid, assertion raised on each violation.
 * @param   pRoot           Pointer to the Red-Back tree's root structure.
 */
KRB_DECL(KBOOL) KRB_FN(Assert)(KRBROOT *pRoot)
{
    KBOOL       fRc = K_TRUE;
#ifdef KRB_CACHE_SIZE
    unsigned    i;
#endif
    KRBNODE    *pNode;

    KRB_READ_LOCK(pRoot);
    if (pRoot->mpRoot == KRB_NULL)
    {
        KRB_READ_UNLOCK(pRoot);
        return 0;
    }

#ifdef KRB_CACHE_SIZE
    /*
     * Validate the cache.
     */
    for (i = 0; i < (KRB_CACHE_SIZE); i++)
        if (pRoot->maLookthru[i] != KRB_NULL)
        {
            KRBNODE pCache = KRB_GET_POINTER(&pRoot->maLookthru[i]);

            /** @todo ranges */
            kHlpAssertMsgStmt(i == KRB_CACHE_HASH(pCache->Key), ("Invalid cache entry %u, hashed to %u\n", i, KRB_CACHE_HASH(pCache->Key)), fRc = K_FALSE);

            pNode = KRB_GET_POINTER(&pRoot->mpRoot);
            while (pNode)
            {
                if (KRB_CMP_E(pCache->mKey, pNode->mKey))
                {
                    kHlpAssertMsgStmt(pNode == pCache, ("Invalid cache entry %u=%p, found %p\n", i, pCache, pNode), fRc = K_FALSE);
                    break;
                }
                if (KRB_CMP_G(pCache->mKey, pNode->mKey))
                    pNode = KRB_GET_POINTER_NULL(&pNode->mRight);
                else
                    pNode = KRB_GET_POINTER_NULL(&pNode->mLeft);
            }
            kHlpAssertMsgStmt(pNode, ("Invalid cache entry %u/%p - not found\n", i, pCache), fRc = K_FALSE);
        }
#endif

    /*
     * Recurse thru the tree.
     */
    if (KRB_FN(AssertRecurse)(KRB_GET_POINTER(&pRoot->mpRoot)) == -1)
        fRc = K_FALSE;

    KRB_READ_UNLOCK(pRoot);
    return fRc;
}

