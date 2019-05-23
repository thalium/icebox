/* $Id: kRbBase.h 38 2009-11-10 00:01:38Z bird $ */
/** @file
 * kRbTmpl - Templated Red-Black Trees, The Mandatory Base Code.
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
 *  This is a templated implementation of Red-Black trees in C.  The template
 *  parameters relates to the kind of key used and how duplicates are treated.
 *
 *  \#define KRB_EQUAL_ALLOWED
 *  Define this to tell us that equal keys are allowed.
 *  Then Equal keys will be put in a list pointed to by KRBNODE::pList.
 *  This is by default not defined.
 *
 *  \#define KRB_CHECK_FOR_EQUAL_INSERT
 *  Define this to enable insert check for equal nodes.
 *  This is by default not defined.
 *
 *  \#define KRB_MAX_STACK
 *  Use this to specify the max number of stack entries the stack will use when
 *  inserting and removing nodes from the tree. The size should be something like
 *      log2(<max nodes>) + 3
 *  Must be defined.
 *
 *  \#define KRB_RANGE
 *  Define this to enable key ranges.
 *
 *  \#define KRB_OFFSET
 *  Define this to link the tree together using self relative offset
 *  instead of memory pointers, thus making the entire tree relocatable
 *  provided all the nodes - including the root node variable - are moved
 *  the exact same distance.
 *
 *  \#define KRB_CACHE_SIZE
 *  Define this to employ a lookthru cache (direct) to speed up lookup for
 *  some usage patterns. The value should be the number of members of the array.
 *
 *  \#define KRB_CACHE_HASH(Key)
 *  Define this to specify a more efficient translation of the key into
 *  a lookthru array index. The default is key % size.
 *  For some key types this is required as the default will not compile.
 *
 *  \#define KRB_LOCKED
 *  Define this if you wish for the tree to be locked via the
 *  KRB_WRITE_LOCK,  KRB_WRITE_UNLOCK, KRB_READ_LOCK and
 *  KRB_READ_UNLOCK macros. If not defined the tree will not be subject
 *  do any kind of locking and the problem of concurrency is left the user.
 *
 *  \#define KRB_WRITE_LOCK(pRoot)
 *  Lock the tree for writing.
 *
 *  \#define KRB_WRITE_UNLOCK(pRoot)
 *  Counteracts KRB_WRITE_LOCK.
 *
 *  \#define KRB_READ_LOCK(pRoot)
 *  Lock the tree for reading.
 *
 *  \#define KRB_READ_UNLOCK(pRoot)
 *  Counteracts KRB_READ_LOCK.
 *
 *  \#define KRBKEY
 *  Define this to the name of the AVL key type.
 *
 *  \#define KRB_STD_KEY_COMP
 *  Define this to use the standard key compare macros. If not set all the
 *  compare operations for KRBKEY have to be defined: KRB_CMP_G, KRB_CMP_E, KRB_CMP_NE,
 *  KRB_R_IS_IDENTICAL, KRB_R_IS_INTERSECTING and KRB_R_IS_IN_RANGE. The
 *  latter three are only required when KRB_RANGE is defined.
 *
 *  \#define KRBNODE
 *  Define this to the name (typedef) of the AVL node structure. This
 *  structure must have a mpLeft, mpRight, mKey and mHeight member.
 *  If KRB_RANGE is defined a mKeyLast is also required.
 *  If KRB_EQUAL_ALLOWED is defined a mpList member is required.
 *  It's possible to use other member names by redefining the names.
 *
 *  \#define KRBTREEPTR
 *  Define this to the name (typedef) of the tree pointer type. This is
 *  required when KRB_OFFSET is defined. When not defined it defaults
 *  to KRBNODE *.
 *
 *  \#define KRBROOT
 *  Define this to the name (typedef) of the AVL root structure. This
 *  is optional. However, if specified it must at least have a mpRoot
 *  member of KRBTREEPTR type. If KRB_CACHE_SIZE is non-zero a
 *  maLookthru[KRB_CACHE_SIZE] member of the KRBTREEPTR type is also
 *  required.
 *
 *  \#define KRB_FN
 *  Use this to alter the names of the AVL functions.
 *  Must be defined.
 *
 *  \#define KRB_TYPE(prefix, name)
 *  Use this to make external type names and unique. The prefix may be empty.
 *  Must be defined.
 *
 *  \#define KRB_INT(name)
 *  Use this to make internal type names and unique. The prefix may be empty.
 *  Must be defined.
 *
 *  \#define KRB_DECL(rettype)
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
/** @def KRB_GET_POINTER
 * Reads a 'pointer' value.
 *
 * @returns The native pointer.
 * @param   pp      Pointer to the pointer to read.
 * @internal
 */

/** @def KRB_GET_POINTER_NULL
 * Reads a 'pointer' value which can be KRB_NULL.
 *
 * @returns The native pointer.
 * @returns NULL pointer if KRB_NULL.
 * @param   pp      Pointer to the pointer to read.
 * @internal
 */

/** @def KRB_SET_POINTER
 * Writes a 'pointer' value.
 * For offset-based schemes offset relative to pp is calculated and assigned to *pp.
 *
 * @returns stored pointer.
 * @param   pp      Pointer to where to store the pointer.
 * @param   p       Native pointer to assign to *pp.
 * @internal
 */

/** @def KRB_SET_POINTER_NULL
 * Writes a 'pointer' value which can be KRB_NULL.
 *
 * For offset-based schemes offset relative to pp is calculated and assigned to *pp,
 * if p is not KRB_NULL of course.
 *
 * @returns stored pointer.
 * @param   pp      Pointer to where to store the pointer.
 * @param   pp2     Pointer to where to pointer to assign to pp. This can be KRB_NULL
 * @internal
 */

#ifndef KRBTREEPTR
# define KRBTREEPTR                         KRBNODE *
#endif

#ifndef KRBROOT
# define KRBROOT                            KRB_TYPE(,ROOT)
# define KRB_NEED_KRBROOT
#endif

#ifdef KRB_CACHE_SIZE
# ifndef KRB_CACHE_HASH
#  define KRB_CACHE_HASH(Key)               ( (Key) % (KRB_CACHE_SIZE) )
# endif
#elif defined(KRB_CACHE_HASH)
# error "KRB_CACHE_HASH without KRB_CACHE_SIZE!"
#endif

#ifdef KRB_CACHE_SIZE
# define KRB_CACHE_INVALIDATE_NODE(pRoot, pNode, Key) \
    do { \
        KRBTREEPTR **ppEntry = &pRoot->maLookthru[KRB_CACHE_HASH(Key)]; \
        if ((pNode) == KRB_GET_POINTER_NULL(ppEntry)) \
            *ppEntry = KRB_NULL; \
    } while (0)
#else
# define KRB_CACHE_INVALIDATE_NODE(pRoot, pNode, Key) do { } while (0)
#endif

#ifndef KRB_LOCKED
# define KRB_WRITE_LOCK(pRoot)              do { } while (0)
# define KRB_WRITE_UNLOCK(pRoot)            do { } while (0)
# define KRB_READ_LOCK(pRoot)               do { } while (0)
# define KRB_READ_UNLOCK(pRoot)             do { } while (0)
#endif

#ifdef KRB_OFFSET
# define KRB_GET_POINTER(pp)                ( (KRBNODE *)((KIPTR)(pp) + *(pp)) )
# define KRB_GET_POINTER_NULL(pp)           ( *(pp) != KRB_NULL ? KRB_GET_POINTER(pp) : NULL )
# define KRB_SET_POINTER(pp, p)             ( (*(pp)) = ((KIPTR)(p) - (KIPTR)(pp)) )
# define KRB_SET_POINTER_NULL(pp, pp2)      ( (*(pp)) = *(pp2) != KRB_NULL ? (KIPTR)KRB_GET_POINTER(pp2) - (KIPTR)(pp) : KRB_NULL )
#else
# define KRB_GET_POINTER(pp)                ( *(pp) )
# define KRB_GET_POINTER_NULL(pp)           ( *(pp) )
# define KRB_SET_POINTER(pp, p)             ( (*(pp)) = (p) )
# define KRB_SET_POINTER_NULL(pp, pp2)      ( (*(pp)) = *(pp2) )
#endif


/** @def KRB_NULL
 * The NULL 'pointer' equivalent.
 */
#ifdef KRB_OFFSET
# define KRB_NULL     0
#else
# define KRB_NULL     NULL
#endif

#ifdef KRB_STD_KEY_COMP
# define KRB_CMP_G(key1, key2)              ( (key1) >  (key2) )
# define KRB_CMP_E(key1, key2)              ( (key1) == (key2) )
# define KRB_CMP_NE(key1, key2)             ( (key1) != (key2) )
# ifdef KRB_RANGE
#  define KRB_R_IS_IDENTICAL(key1B, key2B, key1E, key2E)       ( (key1B) == (key2B) && (key1E) == (key2E) )
#  define KRB_R_IS_INTERSECTING(key1B, key2B, key1E, key2E)    ( (key1B) <= (key2E) && (key1E) >= (key2B) )
#  define KRB_R_IS_IN_RANGE(key1B, key1E, key2)                KRB_R_IS_INTERSECTING(key1B, key2, key1E, key2)
# endif
#endif

#ifndef KRB_RANGE
# define KRB_R_IS_INTERSECTING(key1B, key2B, key1E, key2E)     KRB_CMP_E(key1B, key2B)
# define KRB_R_IS_IDENTICAL(key1B, key2B, key1E, key2E)        KRB_CMP_E(key1B, key2B)
#endif


/** Is the node red or black?
 * @returns true / false
 * @param   pNode       Pointer to the node in question.
 * @remarks All NULL pointers are considered black leaf nodes.
 */
#define KRB_IS_RED(pNode)                   ( (pNode) != NULL && (pNode)->mfIsRed )


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Stack used to avoid recursive calls during insert and removal.
 */
typedef struct
{
    unsigned        cEntries;
    KRBTREEPTR    *aEntries[KRB_MAX_STACK];
} KRB_INT(STACK);

/**
 * The callback used by the Destroy and DoWithAll functions.
 */
typedef int (* KRB_TYPE(PFN,CALLBACK))(KRBNODE *, void *);

#ifdef KRB_NEED_KRBROOT
/**
 * The Red-Black tree root structure.
 */
typedef struct
{
    KRBTREEPTR     mpRoot;
# ifdef KRB_CACHE_SIZE
    KRBTREEPTR     maLookthru[KRB_CACHE_SIZE];
# endif
} KRBROOT;
#endif



/**
 * Initializes the root of the Red-Black tree.
 *
 * @param     pTree     Pointer to the root structure.
 */
KRB_DECL(void) KRB_FN(Init)(KRBROOT *pRoot)
{
#ifdef KRB_CACHE_SIZE
    unsigned i;
#endif

    pRoot->mpRoot = KRB_NULL;
#ifdef KRB_CACHE_SIZE
    for (i = 0; i < (KRB_CACHE_SIZE); i++)
        pRoot->maLookthru[i] = KRB_NULL;
#endif
}


/**
 * Rotates the tree to the left (shift direction) and recolors the nodes.
 *
 * @pre
 *
 *       2                 4
 *      / \               / \
 *     1   4     ==>     2   5
 *        / \           / \
 *       3   5         1   3
 *
 * @endpre
 *
 * @returns The new root node.
 * @param   pRoot           The root node.
 *
 * @remarks This will not update any pointer <tt>to</tt> the root node!
 */
K_DECL_INLINE(KRBNODE *) KAVL_FN(RotateLeft)(KRBNODE *pRoot)
{
    KRBNODE *pNewRoot = pRoot->mRight;
    pRoot->mRight     = pNewRoot->mLeft;
    pNewRoot->mLeft   = pRoot;

    pRoot->mfIsRed    = 1;
    pNewRoot->mfIsRed = 0;
    return pNewRoot;
}


/**
 * Rotates the tree to the right (shift direction) and recolors the nodes.
 *
 * @pre
 *
 *         4             2
 *        / \           / \
 *       2   5   ==>   1   4
 *      / \               / \
 *     1   3             3   5
 *
 * @endpre
 *
 * @returns The new root node.
 * @param   pRoot           The root node.
 *
 * @remarks This will not update any pointer <tt>to</tt> the root node!
 */
K_DECL_INLINE(KRBNODE *) KAVL_FN(RotateRight)(KRBNODE *pRoot)
{
    KRBNODE *pNewRoot = pRoot->mLeft;
    pRoot->mLeft      = pNewRoot->mRight;
    pNewRoot->mRight  = pRoot;

    pRoot->mfIsRed    = 1;
    pNewRoot->mfIsRed = 0;
    return pNewRoot;
}


/**
 * Performs a double left rotation with recoloring.
 *
 * @pre
 *
 *       2               2                    4
 *      / \             / \                 /   \
 *     1   6     ==>   1   4       ==>     2     6
 *        / \             / \             / \   / \
 *       4   7           3   6           1   3 5   7
 *      / \                 / \
 *     3   5               5   7
 * @endpre
 *
 * @returns The new root node.
 * @param   pRoot           The root node.
 *
 * @remarks This will not update any pointer <tt>to</tt> the root node!
 */
K_DECL_INLINE(KRBNODE *) KAVL_FN(DoubleRotateLeft)(KRBNODE *pRoot)
{
    pRoot->mRight = KAVL_FN(RotateRight)(pRoot->mRight);
    return KAVL_FN(RotateLeft)(pRoot);
}


/**
 * Performs a double right rotation with recoloring.
 *
 * @pre
 *         6                 6                4
 *        / \               / \             /   \
 *       2   7             4   7           2     6
 *      / \      ==>      / \      ==>    / \   / \
 *     1   4             2   5           1   3 5   7
 *        / \           / \
 *       3   5         1   3
 *
 * @endpre
 *
 * @returns The new root node.
 * @param   pRoot           The root node.
 *
 * @remarks This will not update any pointer <tt>to</tt> the root node!
 */
K_DECL_INLINE(KRBNODE *) KAVL_FN(DoubleRotateRight)(KRBNODE *pRoot)
{
    pRoot->mLeft = KAVL_FN(RotateLeft)(pRoot->mLeft);
    return KAVL_FN(RotateRight)(pRoot);
}


/**
 * Inserts a node into the Red-Black tree.
 * @returns   K_TRUE if inserted.
 *            K_FALSE if node exists in tree.
 * @param     pRoot     Pointer to the Red-Back tree's root structure.
 * @param     pNode     Pointer to the node which is to be added.
 */
KRB_DECL(KBOOL) KRB_FN(Insert)(KRBROOT *pRoot, KRBNODE *pNode)
{
    KRBTREEPTR        *ppCurNode = &pRoot->mpRoot;
    register KRBKEY    Key       = pNode->mKey;
#ifdef KRB_RANGE
    register KRBKEY    KeyLast   = pNode->mKeyLast;
#endif

#ifdef KRB_RANGE
    if (Key > KeyLast)
        return K_FALSE;
#endif

    KRB_WRITE_LOCK(pRoot);

    Stack.cEntries = 0;
    while (*ppCurNode != KRB_NULL)
    {
        register KRBNODE *pCurNode = KRB_GET_POINTER(ppCurNode);

        kHlpAssert(Stack.cEntries < KRB_MAX_STACK);
        Stack.aEntries[Stack.cEntries++] = ppCurNode;
#ifdef KRB_EQUAL_ALLOWED
        if (KRB_R_IS_IDENTICAL(pCurNode->mKey, Key, pCurNode->mKeyLast, KeyLast))
        {
            /*
             * If equal then we'll use a list of equal nodes.
             */
            pNode->mpLeft = pNode->mpRight = KRB_NULL;
            pNode->mHeight = 0;
            KRB_SET_POINTER_NULL(&pNode->mpList, &pCurNode->mpList);
            KRB_SET_POINTER(&pCurNode->mpList, pNode);
            KRB_WRITE_UNLOCK(pRoot);
            return K_TRUE;
        }
#endif
#ifdef KRB_CHECK_FOR_EQUAL_INSERT
        if (KRB_R_IS_INTERSECTING(pCurNode->mKey, Key, pCurNode->mKeyLast, KeyLast))
        {
            KRB_WRITE_UNLOCK(pRoot);
            return K_FALSE;
        }
#endif
        if (KRB_CMP_G(pCurNode->mKey, Key))
            ppCurNode = &pCurNode->mpLeft;
        else
            ppCurNode = &pCurNode->mpRight;
    }

    pNode->mpLeft = pNode->mpRight = KRB_NULL;
#ifdef KRB_EQUAL_ALLOWED
    pNode->mpList = KRB_NULL;
#endif
    pNode->mHeight = 1;
    KRB_SET_POINTER(ppCurNode, pNode);

    KRB_FN(Rebalance)(&Stack);

    KRB_WRITE_UNLOCK(pRoot);
    return K_TRUE;
}


/**
 * Removes a node from the Red-Black tree.
 * @returns   Pointer to the node.
 * @param     pRoot     Pointer to the Red-Back tree's root structure.
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
KRB_DECL(KRBNODE *) KRB_FN(Remove)(KRBROOT *pRoot, KRBKEY Key)
{
    KRB_INT(STACK)     Stack;
    KRBTREEPTR        *ppDeleteNode = &pRoot->mpRoot;
    register KRBNODE  *pDeleteNode;

    KRB_WRITE_LOCK(pRoot);

    Stack.cEntries = 0;
    for (;;)
    {
        if (*ppDeleteNode == KRB_NULL)
        {
            KRB_WRITE_UNLOCK(pRoot);
            return NULL;
        }
        pDeleteNode = KRB_GET_POINTER(ppDeleteNode);

        kHlpAssert(Stack.cEntries < KRB_MAX_STACK);
        Stack.aEntries[Stack.cEntries++] = ppDeleteNode;
        if (KRB_CMP_E(pDeleteNode->mKey, Key))
            break;

        if (KRB_CMP_G(pDeleteNode->mKey, Key))
            ppDeleteNode = &pDeleteNode->mpLeft;
        else
            ppDeleteNode = &pDeleteNode->mpRight;
    }

    if (pDeleteNode->mpLeft != KRB_NULL)
    {
        /* find the rightmost node in the left tree. */
        const unsigned      iStackEntry = Stack.cEntries;
        KRBTREEPTR        *ppLeftLeast = &pDeleteNode->mpLeft;
        register KRBNODE  *pLeftLeast = KRB_GET_POINTER(ppLeftLeast);

        while (pLeftLeast->mpRight != KRB_NULL)
        {
            kHlpAssert(Stack.cEntries < KRB_MAX_STACK);
            Stack.aEntries[Stack.cEntries++] = ppLeftLeast;
            ppLeftLeast = &pLeftLeast->mpRight;
            pLeftLeast  = KRB_GET_POINTER(ppLeftLeast);
        }

        /* link out pLeftLeast */
        KRB_SET_POINTER_NULL(ppLeftLeast, &pLeftLeast->mpLeft);

        /* link it in place of the delete node. */
        KRB_SET_POINTER_NULL(&pLeftLeast->mpLeft, &pDeleteNode->mpLeft);
        KRB_SET_POINTER_NULL(&pLeftLeast->mpRight, &pDeleteNode->mpRight);
        pLeftLeast->mHeight = pDeleteNode->mHeight;
        KRB_SET_POINTER(ppDeleteNode, pLeftLeast);
        Stack.aEntries[iStackEntry] = &pLeftLeast->mpLeft;
    }
    else
    {
        KRB_SET_POINTER_NULL(ppDeleteNode, &pDeleteNode->mpRight);
        Stack.cEntries--;
    }

    KRB_FN(Rebalance)(&Stack);

    KRB_CACHE_INVALIDATE_NODE(pRoot, pDeleteNode, Key);

    KRB_WRITE_UNLOCK(pRoot);
    return pDeleteNode;
}

