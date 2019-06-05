/* Copyright (c) 2013-2014, David Anderson
All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*  The interfaces follow tsearch (See the Single
    Unix Specification) but the implementation is
    written without reference to the source of any
    version of tsearch.

    See http://www.prevanders.net/tsearch.html
    for information and an example of use.

    Implements a red-black tree.

    Based on Sedgewick "Algorithms" 4th Edition.
    Right now showing Kindle 'locations', I do not
    have page numbers.

    On a Kindle, the insert algorithm is found at Location 7930,
        and rotate{left,right} precede that a little.
    Delete as a topic starts at location 7938.
    Delete supporting algos are at about location 8155.

    We insert a ts_entry node as head that has a NULL
    llink and an rlink pointing to the real tree
    root so that
    the use does not see the root changing in flight.


    Kindle location 7808.
    Red-black BSTs are BSTs with red and black links satisfying:
        a)Red links lean left
        b)No node has two red links connected to it.
        c)The tree has 'perfect black balance': every path
            from the root to a null link has the same number
            of black links.
    Sedgewick defines:
        a 3-node is a pair of 2-nodes with a red link
        that leans left.
    So a 2-node is a node which is not marked red and
        whose llink is not marked red.

*/



#include "config.h"
#ifdef HAVE_UNUSED_ATTRIBUTE
#define  UNUSEDARG __attribute__ ((unused))
#else
#define  UNUSEDARG
#endif
#include "stdlib.h" /* for free() */
#include <stdio.h> /* for printf */
#include "dwarf_tsearch.h"

#define TRUE 1
#define RED 1
#define FALSE 0
#define BLACK 0

#ifdef DW_CHECK_CONSISTENCY
struct ts_entry;
void dwarf_check_balance(struct ts_entry *head,int finalprefix);
#endif /* DW_CHECK_CONSISTENCY */


struct ts_entry {
    /*  Keyptr usually points to a a record the user saved, the
        user record contains the user's key itself
        and perhaps more. However, the values actually
        present are controlled by the user.  */
    const void *keyptr;

    /*  Non-zero (RED) red indicates the link
        pointing into this node is red,
        otherwise it is a black link pointing to this node.
        A null llink or rlink (below) means
        the llink or rlink (respectively) is considered black.
    */
    unsigned char color;

    struct ts_entry * llink;
    struct ts_entry * rlink;
};


/* Not needed for this set of functions. */
void *
dwarf_initialize_search_hash( void **treeptr,
    DW_TSHASHTYPE (*hashfunc)(const void *key),
    unsigned long size_estimate)
{
    return *treeptr;
}

static int
isred(const struct ts_entry*n)
{
    if(n && n->color == RED) {
        return TRUE;
    }
    return FALSE;
}

/*  Meaning the node is not part of a 3-node.
    We define NULL as a 2-node.
*/
static int
is_twonode(const struct ts_entry *h)
{
    if(!h) {
        return TRUE;
    }
    if(isred(h)) {
        return FALSE;
    }
    if(isred(h->llink)) {
        return FALSE;
    }
    return TRUE;
}

#if 0 /* DEBUG ONLY */
static const char *
printnode(struct ts_entry*n)
{
    static char b[400];
    if(!n) {
        return "Null node";
    }
    snprintf(b,sizeof(b),"0x%x 2-node %d red %d  l 0x%x r 0x%x",
        (unsigned)n,
        is_twonode(n),
        n->color,
        (unsigned)n->llink,
        (unsigned)n->rlink);
    return b;
}

/* For debugging. Use this to call dumptree_inner
   from inside this file. */
static char *
v_keyprint(const void *l)
{
    unsigned long v = (unsigned long)l;
    static char buf [50];

    snprintf(buf,sizeof(buf),"0x%08lx",v);
    return buf;
}
#endif /* DEBUG ONLY */
/* Prints the level number and indents 1 space
   per level.   That won't work very well for a deep tree, so perhaps
   we should clamp at some number of indent spaces? */
static void printlevel(int level)
{
    int len = 0;
    int targetlen = 4 + level;
    int shownlen = 0;
    char number[10];
    len = snprintf(number,sizeof(number),"<%d>",level);
    printf("%s",number);
    shownlen = len;
    while(shownlen < targetlen) {
        putchar(' ');
        ++shownlen;
    }
}

static void
dumptree_inner(const struct ts_entry *t,
    char *(* keyprint)(const void *),
    const char *descr, int level)
{
    char *v = "";
    if(!t) {
        return;
    }
    dumptree_inner(t->rlink,keyprint,"right",level+1);
    if(t->keyptr) {
        v = keyprint(t->keyptr);
    }
    printlevel(level);
    printf("0x%08lx <keyptr 0x%08lx> <%s %s> <2-node %d red %u> <l 0x%08lx> <r 0x%08lx> %s\n",
        (unsigned long)t,
        (unsigned long)t->keyptr,
        t->keyptr?"key ":"null",
        v,
        is_twonode(t),
        t->color,
        (unsigned long)t->llink,(unsigned long)t->rlink,
        descr);
    dumptree_inner(t->llink,keyprint,"left ",level+1);
}

/*  Dumping the tree to stdout. */
void
dwarf_tdump(const void*rootin,
    char *(* keyprint)(const void *),
    const char *msg)
{
    const struct ts_entry *head = (const struct ts_entry *)rootin;
    const struct ts_entry *root = 0;
    if(!head) {
        printf("dwarf_tdump null tree ptr : %s\n",msg);
        return;
    }
    root = head->rlink;
    if(!root) {
        printf("dwarf_tdump empty tree : %s\n",msg);
        return;
    }
    printf("dwarf_tdump tree head : 0x%08lx %s\n",(unsigned long)head,msg);
    printf("dwarf_tdump tree root : 0x%08lx %s\n",(unsigned long)root,msg);
    dumptree_inner(root,keyprint,"top",0);
    fflush(stdout);
}
#ifdef DW_CHECK_CONSISTENCY
/*  Checking that a tree (or sub tree) is in balance.
    Only meaningful for balanced trees.
    Returns the count of black links.
        a)Red links lean left
        b)No node has two red links connected to it.
        c)The tree has 'perfect black balance': every path
            from the root to a null link has the same number
            of black links.

*/
struct balance_s {
    int countset_;
    int blackcount_;
    void *firstcount_;
};
static struct balance_s zerobal;

static void
check_or_set(struct ts_entry*t,
    int* errcount,
    struct balance_s *balcount,
    int linkcount,
    const char *prefix)
{
    if (!balcount->countset_) {
        balcount->blackcount_ = linkcount;
        balcount->countset_   = 1;
        balcount->firstcount_ = t;
        return;
    }
    if(balcount->blackcount_ == linkcount) {
        return;
    }
    printf("%s Black link count does not match: node 0x%lx %d vs 0x%lx %d\n",
        prefix,
        (unsigned long)t,
        linkcount,
        (unsigned long)balcount->firstcount_,
        balcount->blackcount_);
    ++(*errcount);
}

int
dwarf_check_balance_inner(struct ts_entry *t,
    int level,
    int maxdepth,
    int blacklinkcount,
    struct balance_s *balcount,
    int *founderror,const char *prefix)
{
    int redcount = 0;
    int leftbcount = blacklinkcount;
    int rightbcount = blacklinkcount;
    if(level > maxdepth) {
        printf("%s Likely internal erroneous link loop, got to depth %d.\n",
            prefix,level);
        exit(1);
    }
    if(!t) {
        return 0;
    }
    redcount = isred(t) + isred(t->llink) + isred(t->rlink);
    if (redcount > 1) {
        printf("%s red count error error at node 0x%lx: %d\n",
            prefix,(unsigned long)t,redcount);
        (*founderror)++;
    }
    if(isred(t->rlink)) {
        printf("%s red right link an error at node 0x%lx\n",
            prefix,(unsigned long)t);
        (*founderror)++;
    }
    if(t->llink) {
        if(!isred(t->llink)) {
            leftbcount++;
        }
    } else {
        check_or_set(t,founderror,balcount,leftbcount,prefix);
    }
    if(t->rlink) {
        if(!isred(t->rlink)) {
            rightbcount++;
        }
    } else {
        check_or_set(t,founderror,balcount,rightbcount,prefix);
    }

    dwarf_check_balance_inner(t->llink,level+1,maxdepth,
        leftbcount,balcount,founderror,prefix);
    dwarf_check_balance_inner(t->rlink,level+1,maxdepth,
        rightbcount,balcount,founderror,prefix);
    return isred(t);
}

void
dwarf_check_balance(struct ts_entry *head,int finalprefix)
{
    const char *prefix = 0;
    int maxdepth = 1000; /*  prevent runaway loop. */
    int errcount = 0;
    int depth = 0;
    int blackcount = 0;
    struct balance_s balancect;

    struct ts_entry*root = 0;
    if(finalprefix) {
        prefix = "BalanceError:";
    } else {
        prefix = "BalanceWarn:";
    }
    balancect = zerobal;

    if(!head) {
        printf("%s check balance null tree ptr\n",prefix);
        return;
    }
    root = head->rlink;
    if(!root) {
        printf("%s check balance null tree ptr\n",prefix);
        return;
    }

    /* Counting in levels, not level number of top level. */
    depth = dwarf_check_balance_inner(root,depth,maxdepth,
        blackcount, &balancect,&errcount,prefix);
    if(errcount) {
        printf("%s error count %d\n",prefix,errcount);
    }
    return;
}
#endif  /* DW_CHECK_CONSISTENCY */

static struct ts_entry*
getlink(struct ts_entry*t,int a)
{
    if(a < 0) {
        return(t->llink);
    }
    return(t->rlink);
}

static struct ts_entry *
allocate_ts_entry(const void *key)
{
    struct ts_entry *e = (struct ts_entry *)
        malloc(sizeof(struct ts_entry));
    if(!e) {
        return NULL;
    }
    e->keyptr = key;
    e->color = BLACK; /* That is, set black. */
    e->llink = 0;
    e->rlink = 0;
    return e;
}

static void
flipcolors(struct ts_entry*h)
{
    /*  Sedgewick does not verify llink rlink non-null?  */
    h->color = RED;
    if(h->llink) h->llink->color = BLACK;
    if(h->rlink) h->rlink->color = BLACK;
}


/* Kindle loc 7840.  */
static struct ts_entry*
rotateleft(struct ts_entry *h)
{
    struct ts_entry *x = h->rlink;
    h->rlink = x->llink;
    x->llink = h;
    x->color = h->color;
    h->color = RED;
    return x;
}


/* Kindle loc 7848.  */
static struct ts_entry*
rotateright(struct ts_entry *h)
{
    struct ts_entry *x = h->llink;
    h->llink = x->rlink;
    x->rlink = h;
    x->color = h->color;
    h->color = RED;
    return x;
}
static struct ts_entry*
moveredright(struct ts_entry *h)
{
    flipcolors(h);
    /*  In 4th Ed. book had ! before isred,
        corrected in errata, Oct 2012. */
    if(isred(h->llink->llink)) {
        h = rotateright(h);
    }
    return h;
}

/* Kindle loc 8155. */
static struct ts_entry*
moveredleft(struct ts_entry *h)
{
    flipcolors(h);
    /* Added test for h->rlink.. davea. */
    if(h->rlink && isred(h->rlink->llink)) {
        h->rlink = rotateright(h->rlink);
        h = rotateleft(h);
    }
    return h;
}

static struct ts_entry *
tsearch_insert( const void *key,
    struct ts_entry* h,
    int (*compar)(const void *, const void *),
    int*inserted,
    struct ts_entry **insertednode)
{
    int kc = 1;
    if(!h) {
        h = allocate_ts_entry(key);
        if (!h) {
            return h;
        }
        h->color = RED;
        *inserted = TRUE;
        *insertednode = h;
        return h;
    }
    kc = compar(key,h->keyptr);
    if(kc < 0) {
        struct ts_entry *t = tsearch_insert(key,h->llink,compar,inserted, insertednode);
        if(!t) {
            /* out of memory */
            return t;
        }
        h->llink = t;
    } else if (kc > 0) {
        struct ts_entry *t = tsearch_insert(key,h->rlink,compar,inserted, insertednode);
        if(!t) {
            /* out of memory */
            return t;
        }
        h->rlink = t;
    } else {
        /* Found existing. Return it. */
        return h;
    }
    /* Now fix up links so left is red and right is black. */
    if(isred(h->rlink) && !isred(h->llink)) {
        /* Maintaining red on left. */
        /* insert is between. */
        h = rotateleft(h);
    }
    if(isred(h->llink) && isred(h->llink->llink)) {
        /* Avoiding sequencial red links, turning into
            paired reds fixed just below. */
        h = rotateright(h);
    }
    if(isred(h->llink) && isred(h->rlink)) {
        /* Pair reds below h,flip to black. */
        flipcolors(h);
    }
    return h;
}

/* Search and, if missing, insert. */
void *
dwarf_tsearch(const void *key, void **headpin,
    int (*compar)(const void *, const void *))
{
    struct ts_entry **headp = (struct ts_entry **)headpin;
    struct ts_entry *head = *headp;
    struct ts_entry *root = 0;
    struct ts_entry *r = 0;
    struct ts_entry *insertednode = 0;
    int  inserted = 0;

    if (!head) {
        struct ts_entry *rhead = 0;
        struct ts_entry *r2 = 0;
        rhead = allocate_ts_entry(0);
        if(!rhead) {
            return NULL;
        }
        r2 = allocate_ts_entry(key);
        if(!r2) {
            free(rhead);
            return NULL;
        }
        *headp = rhead;
        rhead->rlink = r2;
        r2->color = BLACK;
        return (void *)&(r2->keyptr);
    }
    root = head->rlink;
    r = tsearch_insert(key,root,compar,&inserted,&insertednode);
    if (!r) {
        return NULL;
    }
#ifdef DW_CHECK_CONSISTENCY
    dwarf_check_balance(head,1);
#endif   /* DW_CHECK_CONSISTENCY */
    if (inserted) {
        /*  Discards const.  Required by the interface definition. */
        /*  root might change, but never the head pointer, so
            no need to update *headp.
            Do need to update head.rlink as balancing might
            have changed root node. */
        head->rlink = r;
        return (void *)&(insertednode->keyptr);
    }
    /*  Discards const.  Required by the interface definition. */
    return (void *)&(r->keyptr);
}

/* Search. */
void *
dwarf_tfind(const void *key, void *const*rootp,
    int (*compar)(const void *, const void *))
{
    struct ts_entry **phead = (struct ts_entry **)rootp;
    struct ts_entry *head = *phead;
    struct ts_entry *p = 0;

    if (!head) {
        return NULL;
    }
    p = head->rlink;
    while( p) {
        int kc = compar(key,p->keyptr);
        if (!kc) {
            return (void *)&(p->keyptr);
        }
        p  = getlink(p,kc);
    }
    return NULL;
}

static void
complementcolors( struct ts_entry *h)
{
    h->color = !h->color;
    if(h->llink) { h->llink->color = !h->llink->color; }
    if(h->rlink) { h->rlink->color = !h->rlink->color; }
}

/* Kindle loc 8155. */
static struct ts_entry *
balance( struct ts_entry *h)
{
    if(!h) { return NULL; }
    if(isred(h->rlink)) { h = rotateleft(h); }

    /* The following from the insert code. loc 7930. plus complementcolors() */
    if(isred(h->rlink) && !isred(h->llink)) { h = rotateleft(h); }
    if(isred(h->llink) && isred(h->llink->llink)) { h = rotateright(h); }
    if(isred(h->llink) && isred(h->rlink)) { complementcolors(h); }
    return h;
}

/* Kindle Location 8154. */
/* This finds the min record that findmin will find and
   delete. Meaning the record with llink NULL, descending
   left links. */
static struct ts_entry *
findmin(struct ts_entry *h)
{
    if(!h) {
        return NULL;
    }
    while (h->llink) {
        h = h->llink;
    }
    return h;
}


/*  At Loc 8154 in Kindle.
    We copied out relevant data, so now delete the
    lowest key record (possibly while reorganizing the
    tree).
    Invariant: current node is not a 2-node.
*/
static struct ts_entry *
deletemin(struct ts_entry *h)
{
    if(!h->llink) {
        /*  Found minimum with key > key to delete.
            Sedgewick does not do this, so something
            is wrong somewhere.
            Mistake in Sedgewick?   */
        return h->rlink;
    }
    if(!isred(h->llink) && !isred(h->llink->llink)) {
        h = moveredleft(h);
    }
    h->llink = deletemin(h->llink);
    h =  balance(h);
    return h;
}

enum delete_result_e {
    dr_unknown,
    dr_notfound,
    dr_deleted,
    dr_noteparent
};

/* Kindle location 8168 for  the algorithm. */

static struct ts_entry *
tdelete_inner(const void *key,
    struct ts_entry *h,
    int (*compar)(const void *, const void *),
    struct ts_entry **parent,
    enum delete_result_e *dr)
{
    int kc = 0;

    if(!h) {
        *dr = dr_notfound;
        return NULL;
    }
    kc = compar(key,h->keyptr);
    if(kc < 0) {
        if(!isred(h->llink) && (h->llink && !isred(h->llink->llink))) {
            h = moveredleft(h);
        }
        h->llink = tdelete_inner(key,h->llink, compar,
            parent,dr);
        if( *dr == dr_noteparent) {
            *dr = dr_deleted;
            *parent = h;
        }
    } else {
        if (isred(h->llink)) {
            h = rotateright(h);
        }
        kc = compar(key,h->keyptr);
        if (!kc && !h->rlink) {
            struct ts_entry *l = h->llink;
            /*  This is a case where h is deleted (it matches our key value)
                and no rlink means it is end of chain at right,
                so it is replaced with left (corrected
                by davea). */
            free(h);
            *dr = dr_noteparent;
            return l;
        }
        /*  Fixed test. Mistake in Sedgewick.
            Unless both links off h are non-null we crash. */
        if(h->rlink && h->llink &&
            !isred(h->rlink) &&
            !isred(h->rlink->llink)) {
            h = moveredright(h);
        }
        kc = compar(key,h->keyptr);
        if(!kc) {
            struct ts_entry *r = 0;
            /* ASSERT: We have non-null rlink. */
            r = findmin(h->rlink);
            h->keyptr = r->keyptr;
            h->rlink = deletemin(h->rlink);
            /*  r is the node we are to delete.
                r value moved to h so we keep  the
                changed h. */
            free(r);
            *dr = dr_noteparent;
        } else   {
            h->rlink = tdelete_inner(key,h->rlink,compar,parent,dr);
            if( *dr == dr_noteparent) {
                *parent = h;
                *dr = dr_deleted;
            }
        }
    }
    h = balance(h);
    return h;
}

void *
dwarf_tdelete(const void *key, void **rootp,
    int (*compar)(const void *, const void *))
{
    struct ts_entry **proot = (struct ts_entry **)rootp;
    struct ts_entry *head = *proot;
    struct ts_entry *p= 0;
    struct ts_entry *root= 0;
    struct ts_entry *parent= 0;
    enum delete_result_e dr = dr_unknown;

    if (!head) {
        return NULL;
    }
    root = p = head->rlink;
    if (!p) {
        return NULL;
    }
    if (!isred(p->llink) && !isred(p->rlink)) {
        p->color = RED;
    }
    p = tdelete_inner(key,root,compar,&parent,&dr);
    if( dr == dr_unknown || dr == dr_notfound ) {
        return NULL;
    }
    if (dr == dr_noteparent) {
        parent = p;
        dr = dr_deleted;
    }
    /* INVARIANT: dr == dr_deleted. */
    if(p) {
        /* ASSERT: parent non-null */
        root = p;
        root->color = BLACK;
        /* We have a root, might be unchanged (or changed). */
        head->rlink = root;
#ifdef DW_CHECK_CONSISTENCY
        dwarf_check_balance(head,1);
#endif /* DW_CHECK_CONSISTENCY */
        /*  ASSERT:  any rebalancing would leave
            parent set to the same parent node, just
            links might have changed  and it
            might not literally be parent due
            to rebalancing. */
        return(void *)(&(parent->keyptr));
    }
    /* The tree is empty. Remove it. */
    free(head);
    *rootp = NULL;
    return NULL;
}

static void
dwarf_twalk_inner(const struct ts_entry *p,
    void (*action)(const void *nodep, const DW_VISIT which, const int depth),
    unsigned level)
{
    if (!p->llink && !p->rlink) {
        action((const void *)(&(p->keyptr)),dwarf_leaf,level);
        return;
    }
    action((const void *)(&(p->keyptr)),dwarf_preorder,level);
    if(p->llink) {
        dwarf_twalk_inner(p->llink,action,level+1);
    }
    action((const void *)(&(p->keyptr)),dwarf_postorder,level);
    if(p->rlink) {
        dwarf_twalk_inner(p->rlink,action,level+1);
    }
    action((const void *)(&(p->keyptr)),dwarf_endorder,level);
}


void
dwarf_twalk(const void *rootp,
    void (*action)(const void *nodep, const DW_VISIT which, const int depth))
{
    const struct ts_entry *head = (const struct ts_entry *)rootp;
    const struct ts_entry *root = 0;
    if(!head) {
        return;
    }
    root = head->rlink;
    if(!root) {
        return;
    }
    dwarf_twalk_inner(root,action,0);
}

static void
dwarf_tdestroy_inner(struct ts_entry*p,
    void (*free_node)(void *nodep),
    int depth)
{
    if(p->llink) {
        dwarf_tdestroy_inner(p->llink,free_node,depth+1);
        p->llink = 0;
    }
    if(p->rlink) {
        dwarf_tdestroy_inner(p->rlink,free_node,depth+1);
        p->rlink = 0;
    }
    /* Discards const.  Required by the interface definition. */
    free_node((void *)p->keyptr);
    free(p);
}

/*  Walk the tree, freeing all space in the tree
    and calling the user's callback function on each node. */
void
dwarf_tdestroy(void *rootp, void (*free_node)(void *nodep))
{
    struct ts_entry *head = (struct ts_entry *)rootp;
    struct ts_entry *root = 0;
    if(!head) {
        return;
    }
    root = head->rlink;
    if(!root) {
        free(head);
        return;
    }
    dwarf_tdestroy_inner(root,free_node,0);
    free(head);
}
