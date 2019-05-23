/* $Id: kAvlDestroy.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kAvlTmpl - Templated AVL Trees, Destroy the tree.
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
 * @param   pRoot           Pointer to the AVL-tree root structure.
 * @param   pfnCallBack     Pointer to callback function.
 * @param   pvUser          User parameter passed on to the callback function.
 */
KAVL_DECL(int) KAVL_FN(Destroy)(KAVLROOT *pRoot, KAVL_TYPE(PFN,CALLBACK) pfnCallBack, void *pvUser)
{
#ifdef KAVL_LOOKTHRU
    unsigned    i;
#endif
    unsigned    cEntries;
    KAVLNODE   *apEntries[KAVL_MAX_STACK];
    int         rc;

    KAVL_WRITE_LOCK(pRoot);
    if (pRoot->mpRoot == KAVL_NULL)
    {
        KAVL_WRITE_UNLOCK(pRoot);
        return 0;
    }

#ifdef KAVL_LOOKTHRU
    /*
     * Kill the lookthru cache.
     */
    for (i = 0; i < (KAVL_LOOKTHRU); i++)
        pRoot->maLookthru[i] = KAVL_NULL;
#endif

    cEntries = 1;
    apEntries[0] = KAVL_GET_POINTER(&pRoot->mpRoot);
    while (cEntries > 0)
    {
        /*
         * Process the subtrees first.
         */
        KAVLNODE *pNode = apEntries[cEntries - 1];
        if (pNode->mpLeft != KAVL_NULL)
            apEntries[cEntries++] = KAVL_GET_POINTER(&pNode->mpLeft);
        else if (pNode->mpRight != KAVL_NULL)
            apEntries[cEntries++] = KAVL_GET_POINTER(&pNode->mpRight);
        else
        {
#ifdef KAVL_EQUAL_ALLOWED
            /*
             * Process nodes with the same key.
             */
            while (pNode->pList != KAVL_NULL)
            {
                KAVLNODE *pEqual = KAVL_GET_POINTER(&pNode->pList);
                KAVL_SET_POINTER(&pNode->pList, KAVL_GET_POINTER_NULL(&pEqual->pList));
                pEqual->pList = KAVL_NULL;

                rc = pfnCallBack(pEqual, pvUser);
                if (rc)
                {
                    KAVL_WRITE_UNLOCK(pRoot);
                    return rc;
                }
            }
#endif

            /*
             * Unlink the node.
             */
            if (--cEntries > 0)
            {
                KAVLNODE *pParent = apEntries[cEntries - 1];
                if (KAVL_GET_POINTER(&pParent->mpLeft) == pNode)
                    pParent->mpLeft = KAVL_NULL;
                else
                    pParent->mpRight = KAVL_NULL;
            }
            else
                pRoot->mpRoot = KAVL_NULL;

            kHlpAssert(pNode->mpLeft == KAVL_NULL);
            kHlpAssert(pNode->mpRight == KAVL_NULL);
            rc = pfnCallBack(pNode, pvUser);
            if (rc)
            {
                KAVL_WRITE_UNLOCK(pRoot);
                return rc;
            }
        }
    } /* while */
    kHlpAssert(pRoot->mpRoot == KAVL_NULL);

    KAVL_WRITE_UNLOCK(pRoot);
    return 0;
}

