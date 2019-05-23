/* $Id: kRbDoWithAll.h 35 2009-11-08 19:39:03Z bird $ */
/** @file
 * kRbTmpl - Templated Red-Black Trees, The Callback Iterator.
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
    KRBNODE        *aEntries[KRB_MAX_STACK];
    char            achFlags[KRB_MAX_STACK];
    KRBROOT        pRoot;
} KRB_INT(STACK2);


/**
 * Iterates thru all nodes in the given tree.
 *
 * @returns   0 on success. Return from callback on failure.
 * @param     pRoot        Pointer to the Red-Back tree's root structure.
 * @param     fFromLeft    K_TRUE:  Left to right.
 *                         K_FALSE: Right to left.
 * @param     pfnCallBack  Pointer to callback function.
 * @param     pvUser       User parameter passed on to the callback function.
 */
KRB_DECL(int) KRB_FN(DoWithAll)(KRBROOT *pRoot, KBOOL fFromLeft, KRB_TYPE(PFN,CALLBACK) pfnCallBack, void *pvUser)
{
    KRB_INT(STACK2)     Stack;
    KRBNODE            *pNode;
#ifdef KRB_EQUAL_ALLOWED
    KRBNODE            *pEqual;
#endif
    int                 rc;

    KRB_READ_LOCK(pRoot);
    if (pRoot->mpRoot == KRB_NULL)
    {
        KRB_READ_UNLOCK(pRoot);
        return 0;
    }

    Stack.cEntries = 1;
    Stack.achFlags[0] = 0;
    Stack.aEntries[0] = KRB_GET_POINTER(&pRoot->mpRoot);

    if (fFromLeft)
    {   /* from left */
        while (Stack.cEntries > 0)
        {
            pNode = Stack.aEntries[Stack.cEntries - 1];

            /* left */
            if (!Stack.achFlags[Stack.cEntries - 1]++)
            {
                if (pNode->mpLeft != KRB_NULL)
                {
                    Stack.achFlags[Stack.cEntries] = 0; /* 0 first, 1 last */
                    Stack.aEntries[Stack.cEntries++] = KRB_GET_POINTER(&pNode->mpLeft);
                    continue;
                }
            }

            /* center */
            rc = pfnCallBack(pNode, pvUser);
            if (rc)
                return rc;
#ifdef KRB_EQUAL_ALLOWED
            if (pNode->mpList != KRB_NULL)
                for (pEqual = KRB_GET_POINTER(&pNode->mpList); pEqual; pEqual = KRB_GET_POINTER_NULL(&pEqual->mpList))
                {
                    rc = pfnCallBack(pEqual, pvUser);
                    if (rc)
                    {
                        KRB_READ_UNLOCK(pRoot);
                        return rc;
                    }
                }
#endif

            /* right */
            Stack.cEntries--;
            if (pNode->mpRight != KRB_NULL)
            {
                Stack.achFlags[Stack.cEntries] = 0;
                Stack.aEntries[Stack.cEntries++] = KRB_GET_POINTER(&pNode->mpRight);
            }
        } /* while */
    }
    else
    {   /* from right */
        while (Stack.cEntries > 0)
        {
            pNode = Stack.aEntries[Stack.cEntries - 1];

            /* right */
            if (!Stack.achFlags[Stack.cEntries - 1]++)
            {
                if (pNode->mpRight != KRB_NULL)
                {
                    Stack.achFlags[Stack.cEntries] = 0;  /* 0 first, 1 last */
                    Stack.aEntries[Stack.cEntries++] = KRB_GET_POINTER(&pNode->mpRight);
                    continue;
                }
            }

            /* center */
            rc = pfnCallBack(pNode, pvUser);
            if (rc)
                return rc;
#ifdef KRB_EQUAL_ALLOWED
            if (pNode->mpList != KRB_NULL)
                for (pEqual = KRB_GET_POINTER(&pNode->mpList); pEqual; pEqual = KRB_GET_POINTER_NULL(&pEqual->pList))
                {
                    rc = pfnCallBack(pEqual, pvUser);
                    if (rc)
                    {
                        KRB_READ_UNLOCK(pRoot);
                        return rc;
                    }
                }
#endif

            /* left */
            Stack.cEntries--;
            if (pNode->mpLeft != KRB_NULL)
            {
                Stack.achFlags[Stack.cEntries] = 0;
                Stack.aEntries[Stack.cEntries++] = KRB_GET_POINTER(&pNode->mpLeft);
            }
        } /* while */
    }

    KRB_READ_UNLOCK(pRoot);
    return 0;
}

