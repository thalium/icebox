/* $Id: kAvlBase.h 36 2009-11-09 22:49:02Z bird $ */
/** @file
 * kAvlTmpl - Templated AVL Trees, The Mandatory Base Code.
 */

/*
 * Copyright (c) 2001-2009 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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

/** @page pg_kAvlTmpl   Template Configuration.
 *
 *  This is a templated implementation of AVL trees in C. The template
 *  parameters relates to the kind of key used and how duplicates are
 *  treated.
 *
 *  \#define KAVL_EQUAL_ALLOWED
 *  Define this to tell us that equal keys are allowed.
 *  Then Equal keys will be put in a list pointed to by KAVLNODE::pList.
 *  This is by default not defined.
 *
 *  \#define KAVL_CHECK_FOR_EQUAL_INSERT
 *  Define this to enable insert check for equal nodes.
 *  This is by default not defined.
 *
 *  \#define KAVL_MAX_STACK
 *  Use this to specify the max number of stack entries the stack will use when
 *  inserting and removing nodes from the tree. The size should be something like
 *      log2(<max nodes>) + 3
 *  Must be defined.
 *
 *  \#define KAVL_RANGE
 *  Define this to enable key ranges.
 *
 *  \#define KAVL_OFFSET
 *  Define this to link the tree together using self relative offset
 *  instead of memory pointers, thus making the entire tree relocatable
 *  provided all the nodes - including the root node variable - are moved
 *  the exact same distance.
 *
 *  \#define KAVL_LOOKTHRU
 *  Define this to employ a lookthru cache (direct) to speed up lookup for
 *  some usage patterns. The value should be the number of members of the
 *   array.
 *
 *  \#define KAVL_LOOKTHRU_HASH(Key)
 *  Define this to specify a more efficient translation of the key into
 *  a lookthru array index. The default is key % size.
 *  For some key types this is required as the default will not compile.
 *
 *  \#define KAVL_LOCKED
 *  Define this if you wish for the tree to be locked via the
 *  KAVL_WRITE_LOCK,  KAVL_WRITE_UNLOCK, KAVL_READ_LOCK and
 *  KAVL_READ_UNLOCK macros. If not defined the tree will not be subject
 *  do any kind of locking and the problem of concurrency is left the user.
 *
 *  \#define KAVL_WRITE_LOCK(pRoot)
 *  Lock the tree for writing.
 *
 *  \#define KAVL_WRITE_UNLOCK(pRoot)
 *  Counteracts KAVL_WRITE_LOCK.
 *
 *  \#define KAVL_READ_LOCK(pRoot)
 *  Lock the tree for reading.
 *
 *  \#define KAVL_READ_UNLOCK(pRoot)
 *  Counteracts KAVL_READ_LOCK.
 *
 *  \#define KAVLKEY
 *  Define this to the name of the AVL key type.
 *
 *  \#define KAVL_STD_KEY_COMP
 *  Define this to use the standard key compare macros. If not set all the
 *  compare operations for KAVLKEY have to be defined: KAVL_G, KAVL_E, KAVL_NE,
 *  KAVL_R_IS_IDENTICAL, KAVL_R_IS_INTERSECTING and KAVL_R_IS_IN_RANGE. The
 *  latter three are only required when KAVL_RANGE is defined.
 *
 *  \#define KAVLNODE
 *  Define this to the name (typedef) of the AVL node structure. This
 *  structure must have a mpLeft, mpRight, mKey and mHeight member.
 *  If KAVL_RANGE is defined a mKeyLast is also required.
 *  If KAVL_EQUAL_ALLOWED is defined a mpList member is required.
 *  It's possible to use other member names by redefining the names.
 *
 *  \#define KAVLTREEPTR
 *  Define this to the name (typedef) of the tree pointer type. This is
 *  required when KAVL_OFFSET is defined. When not defined it defaults
 *  to KAVLNODE *.
 *
 *  \#define KAVLROOT
 *  Define this to the name (typedef) of the AVL root structure. This
 *  is optional. However, if specified it must at least have a mpRoot
 *  member of KAVLTREEPTR type. If KAVL_LOOKTHRU is non-zero a
 *  maLookthru[KAVL_LOOKTHRU] member of the KAVLTREEPTR type is also
 *  required.
 *
 *  \#define KAVL_FN
 *  Use this to alter the names of the AVL functions.
 *  Must be defined.
 *
 *  \#define KAVL_TYPE(prefix, name)
 *  Use this to make external type names and unique. The prefix may be empty.
 *  Must be defined.
 *
 *  \#define KAVL_INT(name)
 *  Use this to make internal type names and unique. The prefix may be empty.
 *  Must be defined.
 *
 *  \#define KAVL_DECL(rettype)
 *  Function declaration macro that should be set according to the scope
 *  the instantiated template should have. For instance an inlined scope
 *  (private or public) should K_DECL_INLINE(rettype) here.
 *
 *  This version of the kAVL tree offers the option of inlining the entire
 *  implementation. This depends on the compiler doing a decent job in both
 *  making use of the inlined code and to eliminate const variables.
 */


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
#include <k/kDefs.h>
#include <k/kTypes.h>
#include <k/kHlpAssert.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define KAVL_HEIGHTOF(pNode) ((KU8)((pNode) != NULL ? (pNode)->mHeight : 0))

/** @def KAVL_GET_POINTER
 * Reads a 'pointer' value.
 *
 * @returns The native pointer.
 * @param   pp      Pointer to the pointer to read.
 * @internal
 */

/** @def KAVL_GET_POINTER_NULL
 * Reads a 'pointer' value which can be KAVL_NULL.
 *
 * @returns The native pointer.
 * @returns NULL pointer if KAVL_NULL.
 * @param   pp      Pointer to the pointer to read.
 * @internal
 */

/** @def KAVL_SET_POINTER
 * Writes a 'pointer' value.
 * For offset-based schemes offset relative to pp is calculated and assigned to *pp.
 *
 * @returns stored pointer.
 * @param   pp      Pointer to where to store the pointer.
 * @param   p       Native pointer to assign to *pp.
 * @internal
 */

/** @def KAVL_SET_POINTER_NULL
 * Writes a 'pointer' value which can be KAVL_NULL.
 *
 * For offset-based schemes offset relative to pp is calculated and assigned to *pp,
 * if p is not KAVL_NULL of course.
 *
 * @returns stored pointer.
 * @param   pp      Pointer to where to store the pointer.
 * @param   pp2     Pointer to where to pointer to assign to pp. This can be KAVL_NULL
 * @internal
 */

#ifndef KAVLTREEPTR
# define KAVLTREEPTR                        KAVLNODE *
#endif

#ifndef KAVLROOT
# define KAVLROOT                           KAVL_TYPE(,ROOT)
# define KAVL_NEED_KAVLROOT
#endif

#ifdef KAVL_LOOKTHRU
# ifndef KAVL_LOOKTHRU_HASH
#  define KAVL_LOOKTHRU_HASH(Key)           ( (Key) % (KAVL_LOOKTHRU) )
# endif
#elif defined(KAVL_LOOKTHRU_HASH)
# error "KAVL_LOOKTHRU_HASH without KAVL_LOOKTHRU!"
#endif

#ifdef KAVL_LOOKTHRU
# define KAVL_LOOKTHRU_INVALIDATE_NODE(pRoot, pNode, Key) \
    do { \
        KAVLTREEPTR **ppEntry = &pRoot->maLookthru[KAVL_LOOKTHRU_HASH(Key)]; \
        if ((pNode) == KAVL_GET_POINTER_NULL(ppEntry)) \
            *ppEntry = KAVL_NULL; \
    } while (0)
#else
# define KAVL_LOOKTHRU_INVALIDATE_NODE(pRoot, pNode, Key) do { } while (0)
#endif

#ifndef KAVL_LOCKED
# define KAVL_WRITE_LOCK(pRoot)             do { } while (0)
# define KAVL_WRITE_UNLOCK(pRoot)           do { } while (0)
# define KAVL_READ_LOCK(pRoot)              do { } while (0)
# define KAVL_READ_UNLOCK(pRoot)            do { } while (0)
#endif

#ifdef KAVL_OFFSET
# define KAVL_GET_POINTER(pp)               ( (KAVLNODE *)((KIPTR)(pp) + *(pp)) )
# define KAVL_GET_POINTER_NULL(pp)          ( *(pp) != KAVL_NULL ? KAVL_GET_POINTER(pp) : NULL )
# define KAVL_SET_POINTER(pp, p)            ( (*(pp)) = ((KIPTR)(p) - (KIPTR)(pp)) )
# define KAVL_SET_POINTER_NULL(pp, pp2)     ( (*(pp)) = *(pp2) != KAVL_NULL ? (KIPTR)KAVL_GET_POINTER(pp2) - (KIPTR)(pp) : KAVL_NULL )
#else
# define KAVL_GET_POINTER(pp)               ( *(pp) )
# define KAVL_GET_POINTER_NULL(pp)          ( *(pp) )
# define KAVL_SET_POINTER(pp, p)            ( (*(pp)) = (p) )
# define KAVL_SET_POINTER_NULL(pp, pp2)     ( (*(pp)) = *(pp2) )
#endif


/** @def KAVL_NULL
 * The NULL 'pointer' equivalent.
 */
#ifdef KAVL_OFFSET
# define KAVL_NULL     0
#else
# define KAVL_NULL     NULL
#endif

#ifdef KAVL_STD_KEY_COMP
# define KAVL_G(key1, key2)                 ( (key1) >  (key2) )
# define KAVL_E(key1, key2)                 ( (key1) == (key2) )
# define KAVL_NE(key1, key2)                ( (key1) != (key2) )
# ifdef KAVL_RANGE
#  define KAVL_R_IS_IDENTICAL(key1B, key2B, key1E, key2E)       ( (key1B) == (key2B) && (key1E) == (key2E) )
#  define KAVL_R_IS_INTERSECTING(key1B, key2B, key1E, key2E)    ( (key1B) <= (key2E) && (key1E) >= (key2B) )
#  define KAVL_R_IS_IN_RANGE(key1B, key1E, key2)                KAVL_R_IS_INTERSECTING(key1B, key2, key1E, key2)
# endif
#endif

#ifndef KAVL_RANGE
# define KAVL_R_IS_INTERSECTING(key1B, key2B, key1E, key2E)     KAVL_E(key1B, key2B)
# define KAVL_R_IS_IDENTICAL(key1B, key2B, key1E, key2E)        KAVL_E(key1B, key2B)
#endif



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Stack used to avoid recursive calls during insert and removal.
 */
typedef struct
{
    unsigned        cEntries;
    KAVLTREEPTR    *aEntries[KAVL_MAX_STACK];
} KAVL_INT(STACK);

/**
 * The callback used by the Destroy and DoWithAll functions.
 */
typedef int (* KAVL_TYPE(PFN,CALLBACK))(KAVLNODE *, void *);

#ifdef KAVL_NEED_KAVLROOT
/**
 * The AVL root structure.
 */
typedef struct
{
    KAVLTREEPTR     mpRoot;
# ifdef KAVL_LOOKTHRU
    KAVLTREEPTR     maLookthru[KAVL_LOOKTHRU];
# endif
} KAVLROOT;
#endif


/**
 * Rewinds a stack of node pointer pointers, rebalancing the tree.
 *
 * @param     pStack  Pointer to stack to rewind.
 * @sketch    LOOP thru all stack entries
 *            BEGIN
 *                Get pointer to pointer to node (and pointer to node) from the stack.
 *                IF 2 higher left subtree than in right subtree THEN
 *                BEGIN
 *                    IF higher (or equal) left-sub-subtree than right-sub-subtree THEN
 *                                *                       n+2|n+3
 *                              /   \                     /     \
 *                            n+2    n       ==>         n+1   n+1|n+2
 *                           /   \                             /     \
 *                         n+1 n|n+1                          n|n+1  n
 *
 *                         Or with keys:
 *
 *                               4                           2
 *                             /   \                       /   \
 *                            2     5        ==>          1     4
 *                           / \                               / \
 *                          1   3                             3   5
 *
 *                    ELSE
 *                                *                         n+2
 *                              /   \                      /   \
 *                            n+2    n                   n+1   n+1
 *                           /   \           ==>        /  \   /  \
 *                          n    n+1                    n  L   R   n
 *                               / \
 *                              L   R
 *
 *                         Or with keys:
 *                               6                           4
 *                             /   \                       /   \
 *                            2     7        ==>          2     6
 *                          /   \                       /  \  /  \
 *                          1    4                      1  3  5  7
 *                              / \
 *                             3   5
 *                END
 *                ELSE IF 2 higher in right subtree than in left subtree THEN
 *                BEGIN
 *                    Same as above but left <==> right. (invert the picture)
 *                ELSE
 *                    IF correct height THEN break
 *                    ELSE correct height.
 *            END
 */
K_DECL_INLINE(void) KAVL_FN(Rebalance)(KAVL_INT(STACK) *pStack)
{
    while (pStack->cEntries > 0)
    {
        KAVLTREEPTR     *ppNode = pStack->aEntries[--pStack->cEntries];
        KAVLNODE        *pNode = KAVL_GET_POINTER(ppNode);
        KAVLNODE        *pLeftNode = KAVL_GET_POINTER_NULL(&pNode->mpLeft);
        KU8              uLeftHeight = KAVL_HEIGHTOF(pLeftNode);
        KAVLNODE        *pRightNode = KAVL_GET_POINTER_NULL(&pNode->mpRight);
        KU8              uRightHeight = KAVL_HEIGHTOF(pRightNode);

        if (uRightHeight + 1 < uLeftHeight)
        {
            KAVLNODE    *pLeftLeftNode = KAVL_GET_POINTER_NULL(&pLeftNode->mpLeft);
            KAVLNODE    *pLeftRightNode = KAVL_GET_POINTER_NULL(&pLeftNode->mpRight);
            KU8          uLeftRightHeight = KAVL_HEIGHTOF(pLeftRightNode);

            if (KAVL_HEIGHTOF(pLeftLeftNode) >= uLeftRightHeight)
            {
                KAVL_SET_POINTER_NULL(&pNode->mpLeft, &pLeftNode->mpRight);
                KAVL_SET_POINTER(&pLeftNode->mpRight, pNode);
                pLeftNode->mHeight = (KU8)(1 + (pNode->mHeight = (KU8)(1 + uLeftRightHeight)));
                KAVL_SET_POINTER(ppNode, pLeftNode);
            }
            else
            {
                KAVL_SET_POINTER_NULL(&pLeftNode->mpRight, &pLeftRightNode->mpLeft);
                KAVL_SET_POINTER_NULL(&pNode->mpLeft, &pLeftRightNode->mpRight);
                KAVL_SET_POINTER(&pLeftRightNode->mpLeft, pLeftNode);
                KAVL_SET_POINTER(&pLeftRightNode->mpRight, pNode);
                pLeftNode->mHeight = pNode->mHeight = uLeftRightHeight;
                pLeftRightNode->mHeight = uLeftHeight;
                KAVL_SET_POINTER(ppNode, pLeftRightNode);
            }
        }
        else if (uLeftHeight + 1 < uRightHeight)
        {
            KAVLNODE    *pRightLeftNode = KAVL_GET_POINTER_NULL(&pRightNode->mpLeft);
            KU8          uRightLeftHeight = KAVL_HEIGHTOF(pRightLeftNode);
            KAVLNODE    *pRightRightNode = KAVL_GET_POINTER_NULL(&pRightNode->mpRight);

            if (KAVL_HEIGHTOF(pRightRightNode) >= uRightLeftHeight)
            {
                KAVL_SET_POINTER_NULL(&pNode->mpRight, &pRightNode->mpLeft);
                KAVL_SET_POINTER(&pRightNode->mpLeft, pNode);
                pRightNode->mHeight = (KU8)(1 + (pNode->mHeight = (KU8)(1 + uRightLeftHeight)));
                KAVL_SET_POINTER(ppNode, pRightNode);
            }
            else
            {
                KAVL_SET_POINTER_NULL(&pRightNode->mpLeft, &pRightLeftNode->mpRight);
                KAVL_SET_POINTER_NULL(&pNode->mpRight, &pRightLeftNode->mpLeft);
                KAVL_SET_POINTER(&pRightLeftNode->mpRight, pRightNode);
                KAVL_SET_POINTER(&pRightLeftNode->mpLeft, pNode);
                pRightNode->mHeight = pNode->mHeight = uRightLeftHeight;
                pRightLeftNode->mHeight = uRightHeight;
                KAVL_SET_POINTER(ppNode, pRightLeftNode);
            }
        }
        else
        {
            KU8 uHeight = (KU8)(K_MAX(uLeftHeight, uRightHeight) + 1);
            if (uHeight == pNode->mHeight)
                break;
            pNode->mHeight = uHeight;
        }
    }

}


/**
 * Initializes the root of the AVL-tree.
 *
 * @param     pTree     Pointer to the root structure.
 */
KAVL_DECL(void) KAVL_FN(Init)(KAVLROOT *pRoot)
{
#ifdef KAVL_LOOKTHRU
    unsigned i;
#endif

    pRoot->mpRoot = KAVL_NULL;
#ifdef KAVL_LOOKTHRU
    for (i = 0; i < (KAVL_LOOKTHRU); i++)
        pRoot->maLookthru[i] = KAVL_NULL;
#endif
}


/**
 * Inserts a node into the AVL-tree.
 * @returns   K_TRUE if inserted.
 *            K_FALSE if node exists in tree.
 * @param     pRoot     Pointer to the AVL-tree root structure.
 * @param     pNode     Pointer to the node which is to be added.
 * @sketch    Find the location of the node (using binary tree algorithm.):
 *            LOOP until NULL leaf pointer
 *            BEGIN
 *                Add node pointer pointer to the AVL-stack.
 *                IF new-node-key < node key THEN
 *                    left
 *                ELSE
 *                    right
 *            END
 *            Fill in leaf node and insert it.
 *            Rebalance the tree.
 */
KAVL_DECL(KBOOL) KAVL_FN(Insert)(KAVLROOT *pRoot, KAVLNODE *pNode)
{
    KAVL_INT(STACK)     AVLStack;
    KAVLTREEPTR        *ppCurNode = &pRoot->mpRoot;
    register KAVLKEY    Key = pNode->mKey;
#ifdef KAVL_RANGE
    register KAVLKEY    KeyLast = pNode->mKeyLast;
#endif

#ifdef KAVL_RANGE
    if (Key > KeyLast)
        return K_FALSE;
#endif

    KAVL_WRITE_LOCK(pRoot);

    AVLStack.cEntries = 0;
    while (*ppCurNode != KAVL_NULL)
    {
        register KAVLNODE *pCurNode = KAVL_GET_POINTER(ppCurNode);

        kHlpAssert(AVLStack.cEntries < KAVL_MAX_STACK);
        AVLStack.aEntries[AVLStack.cEntries++] = ppCurNode;
#ifdef KAVL_EQUAL_ALLOWED
        if (KAVL_R_IS_IDENTICAL(pCurNode->mKey, Key, pCurNode->mKeyLast, KeyLast))
        {
            /*
             * If equal then we'll use a list of equal nodes.
             */
            pNode->mpLeft = pNode->mpRight = KAVL_NULL;
            pNode->mHeight = 0;
            KAVL_SET_POINTER_NULL(&pNode->mpList, &pCurNode->mpList);
            KAVL_SET_POINTER(&pCurNode->mpList, pNode);
            KAVL_WRITE_UNLOCK(pRoot);
            return K_TRUE;
        }
#endif
#ifdef KAVL_CHECK_FOR_EQUAL_INSERT
        if (KAVL_R_IS_INTERSECTING(pCurNode->mKey, Key, pCurNode->mKeyLast, KeyLast))
        {
            KAVL_WRITE_UNLOCK(pRoot);
            return K_FALSE;
        }
#endif
        if (KAVL_G(pCurNode->mKey, Key))
            ppCurNode = &pCurNode->mpLeft;
        else
            ppCurNode = &pCurNode->mpRight;
    }

    pNode->mpLeft = pNode->mpRight = KAVL_NULL;
#ifdef KAVL_EQUAL_ALLOWED
    pNode->mpList = KAVL_NULL;
#endif
    pNode->mHeight = 1;
    KAVL_SET_POINTER(ppCurNode, pNode);

    KAVL_FN(Rebalance)(&AVLStack);

    KAVL_WRITE_UNLOCK(pRoot);
    return K_TRUE;
}


/**
 * Removes a node from the AVL-tree.
 * @returns   Pointer to the node.
 * @param     pRoot     Pointer to the AVL-tree root structure.
 * @param     Key       Key value of the node which is to be removed.
 * @sketch    Find the node which is to be removed:
 *            LOOP until not found
 *            BEGIN
 *                Add node pointer pointer to the AVL-stack.
 *                IF the keys matches THEN break!
 *                IF remove key < node key THEN
 *                    left
 *                ELSE
 *                    right
 *            END
 *            IF found THEN
 *            BEGIN
 *                IF left node not empty THEN
 *                BEGIN
 *                    Find the right most node in the left tree while adding the pointer to the pointer to it's parent to the stack:
 *                    Start at left node.
 *                    LOOP until right node is empty
 *                    BEGIN
 *                        Add to stack.
 *                        go right.
 *                    END
 *                    Link out the found node.
 *                    Replace the node which is to be removed with the found node.
 *                    Correct the stack entry for the pointer to the left tree.
 *                END
 *                ELSE
 *                BEGIN
 *                    Move up right node.
 *                    Remove last stack entry.
 *                END
 *                Balance tree using stack.
 *            END
 *            return pointer to the removed node (if found).
 */
KAVL_DECL(KAVLNODE *) KAVL_FN(Remove)(KAVLROOT *pRoot, KAVLKEY Key)
{
    KAVL_INT(STACK)     AVLStack;
    KAVLTREEPTR        *ppDeleteNode = &pRoot->mpRoot;
    register KAVLNODE  *pDeleteNode;

    KAVL_WRITE_LOCK(pRoot);

    AVLStack.cEntries = 0;
    for (;;)
    {
        if (*ppDeleteNode == KAVL_NULL)
        {
            KAVL_WRITE_UNLOCK(pRoot);
            return NULL;
        }
        pDeleteNode = KAVL_GET_POINTER(ppDeleteNode);

        kHlpAssert(AVLStack.cEntries < KAVL_MAX_STACK);
        AVLStack.aEntries[AVLStack.cEntries++] = ppDeleteNode;
        if (KAVL_E(pDeleteNode->mKey, Key))
            break;

        if (KAVL_G(pDeleteNode->mKey, Key))
            ppDeleteNode = &pDeleteNode->mpLeft;
        else
            ppDeleteNode = &pDeleteNode->mpRight;
    }

    if (pDeleteNode->mpLeft != KAVL_NULL)
    {
        /* find the rightmost node in the left tree. */
        const unsigned      iStackEntry = AVLStack.cEntries;
        KAVLTREEPTR        *ppLeftLeast = &pDeleteNode->mpLeft;
        register KAVLNODE  *pLeftLeast = KAVL_GET_POINTER(ppLeftLeast);

        while (pLeftLeast->mpRight != KAVL_NULL)
        {
            kHlpAssert(AVLStack.cEntries < KAVL_MAX_STACK);
            AVLStack.aEntries[AVLStack.cEntries++] = ppLeftLeast;
            ppLeftLeast = &pLeftLeast->mpRight;
            pLeftLeast  = KAVL_GET_POINTER(ppLeftLeast);
        }

        /* link out pLeftLeast */
        KAVL_SET_POINTER_NULL(ppLeftLeast, &pLeftLeast->mpLeft);

        /* link it in place of the delete node. */
        KAVL_SET_POINTER_NULL(&pLeftLeast->mpLeft, &pDeleteNode->mpLeft);
        KAVL_SET_POINTER_NULL(&pLeftLeast->mpRight, &pDeleteNode->mpRight);
        pLeftLeast->mHeight = pDeleteNode->mHeight;
        KAVL_SET_POINTER(ppDeleteNode, pLeftLeast);
        AVLStack.aEntries[iStackEntry] = &pLeftLeast->mpLeft;
    }
    else
    {
        KAVL_SET_POINTER_NULL(ppDeleteNode, &pDeleteNode->mpRight);
        AVLStack.cEntries--;
    }

    KAVL_FN(Rebalance)(&AVLStack);

    KAVL_LOOKTHRU_INVALIDATE_NODE(pRoot, pDeleteNode, Key);

    KAVL_WRITE_UNLOCK(pRoot);
    return pDeleteNode;
}

