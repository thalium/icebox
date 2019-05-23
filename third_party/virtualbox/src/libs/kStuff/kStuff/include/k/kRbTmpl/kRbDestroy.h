/* $Id: kRbDestroy.h 35 2009-11-08 19:39:03Z bird $ */
/** @file
 * kRbTmpl - Templated Red-Black Trees, Destroy the tree.
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
 * Destroys the specified tree, starting with the root node and working our way down.
 *
 * @returns 0 on success.
 * @returns Return value from callback on failure. On failure, the tree will be in
 *          an unbalanced condition and only further calls to the Destroy should be
 *          made on it. Note that the node we fail on will be considered dead and
 *          no action is taken to link it back into the tree.
 * @param   pRoot           Pointer to the Red-Back tree's root structure.
 * @param   pfnCallBack     Pointer to callback function.
 * @param   pvUser          User parameter passed on to the callback function.
 */
KRB_DECL(int) KRB_FN(Destroy)(KRBROOT *pRoot, KRB_TYPE(PFN,CALLBACK) pfnCallBack, void *pvUser)
{
#ifdef KRB_CACHE_SIZE
    unsigned    i;
#endif
    unsigned    cEntries;
    KRBNODE    *apEntries[KRB_MAX_STACK];
    int         rc;

    KRB_WRITE_LOCK(pRoot);
    if (pRoot->mpRoot == KRB_NULL)
    {
        KRB_WRITE_UNLOCK(pRoot);
        return 0;
    }

#ifdef KRB_CACHE_SIZE
    /*
     * Kill the lookthru cache.
     */
    for (i = 0; i < (KRB_CACHE_SIZE); i++)
        pRoot->maLookthru[i] = KRB_NULL;
#endif

    cEntries = 1;
    apEntries[0] = KRB_GET_POINTER(&pRoot->mpRoot);
    while (cEntries > 0)
    {
        /*
         * Process the subtrees first.
         */
        KRBNODE *pNode = apEntries[cEntries - 1];
        if (pNode->mpLeft != KRB_NULL)
            apEntries[cEntries++] = KRB_GET_POINTER(&pNode->mpLeft);
        else if (pNode->mpRight != KRB_NULL)
            apEntries[cEntries++] = KRB_GET_POINTER(&pNode->mpRight);
        else
        {
#ifdef KRB_EQUAL_ALLOWED
            /*
             * Process nodes with the same key.
             */
            while (pNode->pList != KRB_NULL)
            {
                KRBNODE *pEqual = KRB_GET_POINTER(&pNode->pList);
                KRB_SET_POINTER(&pNode->pList, KRB_GET_POINTER_NULL(&pEqual->pList));
                pEqual->pList = KRB_NULL;

                rc = pfnCallBack(pEqual, pvUser);
                if (rc)
                {
                    KRB_WRITE_UNLOCK(pRoot);
                    return rc;
                }
            }
#endif

            /*
             * Unlink the node.
             */
            if (--cEntries > 0)
            {
                KRBNODE *pParent = apEntries[cEntries - 1];
                if (KRB_GET_POINTER(&pParent->mpLeft) == pNode)
                    pParent->mpLeft = KRB_NULL;
                else
                    pParent->mpRight = KRB_NULL;
            }
            else
                pRoot->mpRoot = KRB_NULL;

            kHlpAssert(pNode->mpLeft == KRB_NULL);
            kHlpAssert(pNode->mpRight == KRB_NULL);
            rc = pfnCallBack(pNode, pvUser);
            if (rc)
            {
                KRB_WRITE_UNLOCK(pRoot);
                return rc;
            }
        }
    } /* while */
    kHlpAssert(pRoot->mpRoot == KRB_NULL);

    KRB_WRITE_UNLOCK(pRoot);
    return 0;
}

