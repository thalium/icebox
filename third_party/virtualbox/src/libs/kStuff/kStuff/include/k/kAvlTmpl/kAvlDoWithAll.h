/* $Id: kAvlDoWithAll.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kAvlTmpl - Templated AVL Trees, The Callback Iterator.
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

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Stack used by DoWithAll to avoid recusive function calls.
 */
typedef struct
{
    unsigned        cEntries;
    KAVLNODE       *aEntries[KAVL_MAX_STACK];
    char            achFlags[KAVL_MAX_STACK];
    KAVLROOT        pRoot;
} KAVL_INT(STACK2);


/**
 * Iterates thru all nodes in the given tree.
 *
 * @returns   0 on success. Return from callback on failure.
 * @param     pRoot        Pointer to the AVL-tree root structure.
 * @param     fFromLeft    K_TRUE:  Left to right.
 *                         K_FALSE: Right to left.
 * @param     pfnCallBack  Pointer to callback function.
 * @param     pvUser       User parameter passed on to the callback function.
 */
KAVL_DECL(int) KAVL_FN(DoWithAll)(KAVLROOT *pRoot, KBOOL fFromLeft, KAVL_TYPE(PFN,CALLBACK) pfnCallBack, void *pvUser)
{
    KAVL_INT(STACK2)    AVLStack;
    KAVLNODE           *pNode;
#ifdef KAVL_EQUAL_ALLOWED
    KAVLNODE           *pEqual;
#endif
    int                 rc;

    KAVL_READ_LOCK(pRoot);
    if (pRoot->mpRoot == KAVL_NULL)
    {
        KAVL_READ_UNLOCK(pRoot);
        return 0;
    }

    AVLStack.cEntries = 1;
    AVLStack.achFlags[0] = 0;
    AVLStack.aEntries[0] = KAVL_GET_POINTER(&pRoot->mpRoot);

    if (fFromLeft)
    {   /* from left */
        while (AVLStack.cEntries > 0)
        {
            pNode = AVLStack.aEntries[AVLStack.cEntries - 1];

            /* left */
            if (!AVLStack.achFlags[AVLStack.cEntries - 1]++)
            {
                if (pNode->mpLeft != KAVL_NULL)
                {
                    AVLStack.achFlags[AVLStack.cEntries] = 0; /* 0 first, 1 last */
                    AVLStack.aEntries[AVLStack.cEntries++] = KAVL_GET_POINTER(&pNode->mpLeft);
                    continue;
                }
            }

            /* center */
            rc = pfnCallBack(pNode, pvUser);
            if (rc)
                return rc;
#ifdef KAVL_EQUAL_ALLOWED
            if (pNode->mpList != KAVL_NULL)
                for (pEqual = KAVL_GET_POINTER(&pNode->mpList); pEqual; pEqual = KAVL_GET_POINTER_NULL(&pEqual->mpList))
                {
                    rc = pfnCallBack(pEqual, pvUser);
                    if (rc)
                    {
                        KAVL_READ_UNLOCK(pRoot);
                        return rc;
                    }
                }
#endif

            /* right */
            AVLStack.cEntries--;
            if (pNode->mpRight != KAVL_NULL)
            {
                AVLStack.achFlags[AVLStack.cEntries] = 0;
                AVLStack.aEntries[AVLStack.cEntries++] = KAVL_GET_POINTER(&pNode->mpRight);
            }
        } /* while */
    }
    else
    {   /* from right */
        while (AVLStack.cEntries > 0)
        {
            pNode = AVLStack.aEntries[AVLStack.cEntries - 1];

            /* right */
            if (!AVLStack.achFlags[AVLStack.cEntries - 1]++)
            {
                if (pNode->mpRight != KAVL_NULL)
                {
                    AVLStack.achFlags[AVLStack.cEntries] = 0;  /* 0 first, 1 last */
                    AVLStack.aEntries[AVLStack.cEntries++] = KAVL_GET_POINTER(&pNode->mpRight);
                    continue;
                }
            }

            /* center */
            rc = pfnCallBack(pNode, pvUser);
            if (rc)
                return rc;
#ifdef KAVL_EQUAL_ALLOWED
            if (pNode->mpList != KAVL_NULL)
                for (pEqual = KAVL_GET_POINTER(&pNode->mpList); pEqual; pEqual = KAVL_GET_POINTER_NULL(&pEqual->pList))
                {
                    rc = pfnCallBack(pEqual, pvUser);
                    if (rc)
                    {
                        KAVL_READ_UNLOCK(pRoot);
                        return rc;
                    }
                }
#endif

            /* left */
            AVLStack.cEntries--;
            if (pNode->mpLeft != KAVL_NULL)
            {
                AVLStack.achFlags[AVLStack.cEntries] = 0;
                AVLStack.aEntries[AVLStack.cEntries++] = KAVL_GET_POINTER(&pNode->mpLeft);
            }
        } /* while */
    }

    KAVL_READ_UNLOCK(pRoot);
    return 0;
}

