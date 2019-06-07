/* Copyright (c) 2014, David Anderson
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

/* Testing tsearch etc.

    tsearch [-adds] [-std] -byvalue inputfile ...

    If one or more inputfile is listed then 'standard' tests
    are not done.

    If -std is given then (even with inputfile) standard
    tests are done.

    If -adds is given then extra tdump output is generated
    from one of the test driver functions.

    If -showa is given then extra output is generated identifying
    some some add/delete actions.

    If -byvalue is given then the tests are run using values not pointes.
    Run like this it is impossible to differentiate whether
    dwarf_tsearch() adds a new tree entry or just finds an existing one.
    In the right circumstances this approach is useful in that
    it is a bit faster than the default.  See applybyvalue() and
    applybypointer.

    For timing tests, you probably want to compile with -DFULL_SPEED_RUN

*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "dwarf_tsearch.h"


/*  These defines rename the call targets to Unix standard
    names (or to nothing where there is no standard version).
*/
#ifdef LIBC_TSEARCH
#define _GNU_SOURCE /* for tdestroy */
#define __USE_GNU   /* tdestroy */
#include <search.h>
#define dwarf_tsearch(a,b,c) tsearch(a,b,c)
#define dwarf_tfind(a,b,c) tfind(a,b,c)
#define dwarf_tdelete(a,b,c) tdelete(a,b,c)
#define dwarf_twalk(a,b) twalk(a,b)
#define dwarf_tdestroy(a,b) tdestroy(a,b)
#define dwarf_tdump(a,c,d)
#define dwarf_initialize_search_hash(a,b,c)
#define DW_VISIT VISIT
#define dwarf_preorder  preorder
#define dwarf_postorder postorder
#define dwarf_endorder  endorder
#define dwarf_leaf      leaf
#endif /* LIBC_TSEARCH */

/* The struct is trivially usable to implement a set or
   map (mapping an integer to a string).
   The following struct is the example basis
   because that is the capability I wanted to use.
   tsearch has no idea what data is involved, only the comparison function
   mt_compare_func() and the  free function mt_free_func()
   (passed in to tsearch calls) know what data is involved.
   Making tsearch very flexible indeed.

   Obviously the use of a struct (and this particular
   struct)  is arbitrary, it is just an example.

*/

struct example_tentry {
    unsigned mt_key;
    /* When using this as a set of mt_key the mt_name
    field is set to 0 (NULL). */
    char * mt_name;
};

/*  used to hold test data */
struct myacts {
    char action_;
    unsigned addr_;
};


/*  Another example of tree content is a simple value.
    Since the tree contains a pointer for each object
    we save, we can only directly save a value that fits
    in a pointer.
*/
typedef unsigned VALTYPE;

enum insertorder {
    increasing,
    decreasing,
    balanced
};

static int
applybypointer(struct myacts *m,
    const char *msg,
    int hideactions,
    int printwalk,
    int dumpeverystage);
static int
applybyvalue(struct myacts *m,
    const char *msg,
    int hideactions,
    int printwalk,
    int dumpeverystage);
int(*applyby)(struct myacts *m,
    const char *msg,
    int hideactions,
    int printwalk,
    int dumpeverystage);


static const int increaseorder[] = {1,2,3,4,5,6};
static const int decreaseorder[] = {6,5,4,3,2,1};
/* The following a pseudo-random order. */
static const int balanceorder[] =  {3,6,2,5,4,1};

/* From real code exposing a bug: */
static struct myacts sequence1[] = {
{'a', 0x33c8},
{'a', 0x34d8},
{'a', 0x35c8},
{'a', 0x3640},
{'a', 0x3820},
{'a', 0x38d0},
{'a', 0x3958},
{'a', 0x39e8},
{'a', 0x3a78},
{'a', 0x3b08},
{'a', 0x3b98},
{'a', 0x3c28},
{'a', 0x3cb8},
{'d', 0x3c28},
{'a', 0x3d48},
{'d', 0x3cb8},
{'a', 0x3dd8},
{'d', 0x3d48},
{'a', 0x3e68},
{'d', 0x3dd8},
{'a', 0x3ef8},
{'a', 0x3f88},
{'d', 0x3e68},
{'a', 0x4018},
{'d', 0x3ef8},
{0,0}
};

static struct myacts sequence2[] = {
{'a', 0x931d2e0},
{'a', 0x931d340},
{'a', 0x931d378},
{'a', 0x931d3b8},
{'a', 0x931d0b0},
{'a', 0x931f7f8},
{'d', 0x931f7f8},
{'d', 0x931d378},
{'a', 0x931d378},
{'a', 0x931f7f8},
{'d', 0x931f7f8},
{'a', 0x931f7f8},
{'d', 0x931f7f8},
{'a', 0x931f9a8},
{'a', 0x931f9f0},
{'a', 0x931fa38},
{'a', 0x93224c0},
{'a', 0x93224f0},
{'a', 0x9322538},
{'a', 0x9322568},
{'a', 0x93225b0},
{'a', 0x93225e0},
{'a', 0x9322628},
{'a', 0x9322658},
{'a', 0x93226a0},
{'a', 0x93226d0},
{'d', 0x93224c0},
{'d', 0x9322538},
{'d', 0x93225b0},
{'d', 0x9322628},
{'d', 0x93226a0},
{'a', 0x931f918},
{'d', 0x931f918},
{'a', 0x931f918},
{0,0}
};

/*  This test is meant to ensure we can add a value with
    key of zero. */
static struct myacts sequence3[] = {
{'a', 0x0},
{'a', 0xffffffff},
{'a', 0xffff},
{0,0}
};

static struct myacts sequential64[] = {
{'a', 1},
{'a', 2},
{'a', 3},
{'a', 4},
{'a', 5},
{'a', 6},
{'a', 7},
{'a', 8},
{'a', 9},
{'a', 10},
{'a', 11},
{'a', 12},
{'a', 13},
{'a', 14},
{'a', 15},
{'a', 16},
{'a', 17},
{'a', 18},
{'a', 19},
{'a', 20},
{'a', 21},
{'a', 22},
{'a', 23},
{'a', 24},
{'a', 25},
{'a', 26},
{'a', 27},
{'a', 28},
{'a', 29},
{'a', 30},
{'a', 31},
{'a', 32},
{'a', 33},
{'a', 34},
{'a', 35},
{'a', 36},
{'a', 37},
{'a', 38},
{'a', 39},
{'a', 40},
{'a', 41},
{'a', 42},
{'a', 43},
{'a', 44},
{'a', 45},
{'a', 46},
{'a', 47},
{'a', 48},
{'a', 49},
{'a', 50},
{'a', 51},
{'a', 52},
{'a', 53},
{'a', 54},
{'a', 55},
{'a', 56},
{'a', 57},
{'a', 58},
{'a', 59},
{'a', 60},
{'a', 61},
{'a', 62},
{'a', 63},
{'a', 64},
{0,0}
};

static int runstandardtests = 1;
static int g_hideactions = 1;
/* showallactions is for debugging tree add/delete
   and should almost always be zero. */
static int g_showallactions = 0;
static char   filetest1name[2000];
static struct myacts *filetest1 = 0;
static char   filetest2name[2000];
static struct myacts *filetest2 = 0;
static char   filetest3name[2000];
static struct myacts *filetest3 = 0;
static char   filetest4name[2000];
static struct myacts *filetest4 = 0;

static int
get_record_id(enum insertorder ord,int indx)
{
    int i = 0;
    switch(ord) {
        case increasing:
            i = increaseorder[indx];
            break;
        case decreasing:
            i = decreaseorder[indx];
            break;
        case balanced:
            i = balanceorder[indx];
            break;
        default:
            printf("FAIL, internal error in test code\n");
            exit(1);
    }
    return i;
}

/* We allow a NULL name so this struct acts sort of like a set
   and sort of like a map.
*/
static struct example_tentry *
make_example_tentry(unsigned k,char *name)
{
    struct example_tentry *mt =
        (struct example_tentry *)calloc(sizeof(struct example_tentry),1);
    if(!mt) {
        printf("calloc fail\n");
        exit(1);
    }
    mt->mt_key = k;
    if(name) {
        mt->mt_name = strdup(name);
    }
    return mt;
}
static void
mt_free_func(void *mt_data)
{
    struct example_tentry *m = mt_data;
    if(!m) {
        return;
    }
    free(m->mt_name);
    free(mt_data);
    return;
}
#ifdef HASHSEARCH
static DW_TSHASHTYPE
mt_hashfunc(const void *keyp)
{
    /* our key here is particularly simple. */
    const struct example_tentry *ml = keyp;
    return ml->mt_key;
}

#endif /* HASHSEARCH */
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

static int
mt_compare_func(const void *l, const void *r)
{
    const struct example_tentry *ml = l;
    const struct example_tentry *mr = r;
    if(ml->mt_key < mr->mt_key) {
        return -1;
    }
    if(ml->mt_key > mr->mt_key) {
        return 1;
    }
    return 0;
}
static void
walk_entry(const void *mt_data,DW_VISIT x,int level)
{
    const struct example_tentry *m = *(const struct example_tentry **)mt_data;
    printlevel(level);
    printf("Walk on node %s %u %s  \n",
        x == dwarf_preorder?"preorder":
        x == dwarf_postorder?"postorder":
        x == dwarf_endorder?"endorder":
        x == dwarf_leaf?"leaf":
        "unknown",
        m->mt_key,m->mt_name);
    return;
}
static void
value_only_walk_entry(const void *data,DW_VISIT x,int level)
{
    VALTYPE val =  (VALTYPE)data;
    printlevel(level);
    printf("Walk on node %s 0x%lu\n",
        x == dwarf_preorder?"preorder":
        x == dwarf_postorder?"postorder":
        x == dwarf_endorder?"endorder":
        x == dwarf_leaf?"leaf":
        "unknown",
        (unsigned long)val);
    return;
}

#ifndef LIBC_TSEARCH
static char *
mt_keyprint(const void *v)
{
    static char buf[50];
    const struct example_tentry *mt = (const struct example_tentry *)v;
    buf[0] = 0;
    snprintf(buf,sizeof(buf),"0x%08x",(unsigned)mt->mt_key);
    return buf;
}

static char *
value_keyprint(const void *v)
{
    VALTYPE val = (VALTYPE)v;
    static char buf[50];
    buf[0] = 0;
    snprintf(buf,sizeof(buf),"0x%08lx",(unsigned long)val);
    return buf;
}
#endif /* LIBC_TSEARCH */

static int
insertrecsbypointer(int max, void **tree, const enum insertorder order)
{
    int indx = 0;
    for(indx = 0 ; indx < max ; ++indx) {
        int i = 0;
        int k = 0;
        char kbuf[40];
        char dbuf[60];
        struct example_tentry *mt = 0;
        struct example_tentry *retval = 0;

        i = get_record_id(order,indx);
        snprintf(kbuf,sizeof(kbuf),"%u",i);
        strcpy(dbuf," data for ");
        strcat(dbuf,kbuf);
        printf("insertrec %d\n",i);
        /*  Do it twice so we have test the case where
            tsearch adds and one
            where it finds an existing record. */

        for (k = 0; k < 2 ;++k) {
            mt = make_example_tentry(i,dbuf);
            errno = 0;
            /* tsearch adds an entry if its not present already. */
            retval = dwarf_tsearch(mt,tree, mt_compare_func  );
            if(retval == 0) {
                printf("FAIL ENOMEM in search on  %d, give up insertrecsbypointer\n",i);
                exit(1);
            } else {
                struct example_tentry *re = 0;
                re = *(struct example_tentry **)retval;
                if(re != mt) {
                    if(!k) {
                        printf("FAIL found existing an error %u\n",i);
                        mt_free_func(mt);
                        return 1;
                    } else {
                        printf("found existing ok  %u\n",i);
                    }
                    /*  Prevents data leak: mt was
                        already present. */
                    mt_free_func(mt);
                } else {
                    if(!k) {
                        printf("insert new ok %u\n",i);
                    } else {
                        printf("FAIL new found but expected existing %u\n",i);
                    }
                    /* New entry mt was added. */
                }
            }
        }
    }
    return 0;
}

static int
findrecsbypointer(int max,int findexpected,
    const void **tree,
    const enum insertorder order)
{
    int indx = 0;
    for(indx = 0 ; indx < max ; ++indx) {
        char kbuf[40];
        char dbuf[60];
        dbuf[0] = 0;
        struct example_tentry *mt = 0;
        struct example_tentry *retval = 0;
        int i = 0;

        i = get_record_id(order,indx);
        snprintf(kbuf,sizeof(kbuf),"%u",i);
        mt = make_example_tentry(i,dbuf);
        printf("findrec %d\n",i);
        retval = dwarf_tfind(mt,(void *const*)tree,mt_compare_func);
        if(!retval) {
            if(indx < findexpected) {
                mt_free_func(mt);
                printf("FAIL tfind on %s is FAILURE\n",kbuf);
                return 1;
            } else {
                printf("Not found with tfind on %s is ok\n",kbuf);
            }
        } else {
            printf("found ok %u\n",i);
            if(indx >= findexpected) {
                mt_free_func(mt);
                printf("FAIL: found with tfind on %s is FAILURE\n",kbuf);
                return 1;
            } else {
                printf("Found with tfind on %s is ok\n",kbuf);
            }
        }
        mt_free_func(mt);
    }
    return 0;
}
/* The dodump flag is so we can distinguish binarysearch actions
   from the binarysearch with eppinger mods easily in the test output.
   The difference is slight and not significant,
   but the difference is what we look for when we look.
*/
static int
delrecsbypointer(int max,int findexpected, void **tree,const enum insertorder order,int dodump)
{
    int indx = 0;
    for (indx = 0; indx < max; indx++) {
        struct example_tentry *mt = 0;
        struct example_tentry *re3 = 0;
        void *r = 0;

        int i = 0;

        i = get_record_id(order,indx);
        printf("delrec %d\n",i);
        mt = make_example_tentry(i,0);
        r = dwarf_tfind(mt,(void *const*)tree,mt_compare_func);
        if (r) {
            /*  This is what tdelete will delete.
                tdelete just removes the reference from
                the tree, it does not actually delete
                the memory for the entry itself.
                In fact there is no way to know for sure
                what was done just given the return
                from tdelete.  You just just assume the delete
                worked and use the tfind result to delete
                your contents if you want to.*/
            re3 = *(struct example_tentry **)r;
            if(indx < findexpected) {
                ;
            } else {
                mt_free_func(mt);
                printf("FAIL delrecsbypointer should not have found record to delete for %d\n",i);
                return 1;
            }
            r = dwarf_tdelete(mt,tree,mt_compare_func);
            if (! *tree) {
                printf("tree itself now empty\n");
            }
            /* We don't want the 'test' node left around. */
            if(r) {
                /*  If the node deleted was root, r
                    is really the new root, not the parent.
                    Or r is non-null but bogus.
                    (so don't print).
                    */
                printf("tdelete returned parent or something.\n");
            } else {
                printf("tdelete returned NULL, tree now empty.\n");
#ifdef HASHSEARCH
                printf("Only really means some hash chain is now empty.\n");
#endif /* HASHSEARCH */
            }
            mt_free_func(mt);
            mt_free_func(re3);
        } else {
            if(indx >= findexpected) {
                ;
            } else {
                mt_free_func(mt);
                printf("FAIL delrecsbypointer should have found record to delete for %d\n",i);
                return 1;
            }
            /* There is no node like this to delete. */
            /* We don't want the 'test' node left around. */
            mt_free_func(mt);
        }
        if (dodump) {
            dwarf_tdump( *tree,mt_keyprint,"In Delrecs");
        }
        dwarf_twalk( *tree,walk_entry);
    }
    return 0;
}

/*  mt must point to data in static storage for this to make any sense.
    Malloc()ed data or unique static data for this instance mt points at.
    For example, if there was an immobile array and make_example_tentry() somehow
    selected a unique entry.
*/
static int
insertonebypointer(void **tree, unsigned long addr,int ct)
{
    struct example_tentry *mt = 0;
    void *retval = 0;
    mt = make_example_tentry(addr,0);
    /* tsearch adds an entry if its not present already. */
    retval = dwarf_tsearch(mt,tree, mt_compare_func  );
    if(retval == 0) {
        printf("FAIL ENOMEM in search on rec %d adr  0x%lu,"
            " error in insertonebypointer\n",
            ct,(unsigned long)addr);
        exit(1);
    } else {
        struct example_tentry *re = 0;
        re = *(struct example_tentry **)retval;
        if(re != mt) {
            /* Found existing, error. */
            printf("insertonebypointer rec %d addr %lu 0x%lx found record"
                " preexisting, error\n",
                ct,
                (unsigned long)addr,
                (unsigned long)addr);
            mt_free_func(mt);
            return 1;
        } else {
            /* inserted new entry, make sure present. */
#ifndef FULL_SPEED_RUN
            struct example_tentry *mt2 = make_example_tentry(addr,0);
            retval = dwarf_tfind(mt2,tree,mt_compare_func);
            mt_free_func(mt2);
            if(!retval) {
                printf("insertonebypointer record %d addr 0x%lu "
                    "failed to add as desired,"
                    " error\n",
                    ct,(unsigned long)addr);
                return 1;
            }
#endif /* FULL_SPEED_RUN */
        }
    }
    return 0;
}

/*  For tfind and tdelete one can use static data and take its address
    for mt instead of using malloc/free.
*/
static int
deleteonebypointer(void **tree, unsigned addr,int ct)
{
    struct example_tentry *mt = 0;
    struct example_tentry *re3 = 0;
    void *r = 0;
    int err=0;

    mt = make_example_tentry(addr,0);
    r = dwarf_tfind(mt,(void *const*)tree,mt_compare_func);
    if (r) {
        re3 = *(struct example_tentry **)r;
        dwarf_tdelete(mt,tree,mt_compare_func);
        mt_free_func(mt);
        mt_free_func(re3);
    } else {
        printf("deleteonebypointer could not find rec %d ! error! addr"
            " 0x%lx\n",
            ct,(unsigned long)addr);
        mt_free_func(mt);
        err = 1;
    }
    return err;
}

#ifdef HASHSEARCH
/* Only needed for hash based search in a tsearch style. */
#define INITTREE(x,y) x = dwarf_initialize_search_hash(&(x),(y),0)
#else
#define INITTREE(x,y)
#endif /* HASHSEARCH */

static const char *
describe_action(char a)
{
    static const char* ad = "add    ";
    static const char* de = "delete ";
    static const char* un = "unknown";
    switch(a) {
    case 'a': return ad;
    case 'd': return de;
    }
    return un;
}

static int
applybypointer(struct myacts *m,
    const char *msg,
    int hideactions,
    int printwalk,
    int dumpeverystage)
{

    unsigned ct = 1;
    void *treesq1 = 0;
    int errcount = 0;

    INITTREE(treesq1,mt_hashfunc);
    printf("special sequence %s\n",msg);
    for(; m->action_ != 0; m++,ct++) {
        if(!hideactions) {
            printf("Action %2u: %s 0x%x val 0x%x\n",ct,
                describe_action(m->action_),
                m->action_,m->addr_);
        }
        if(m->action_ == 'a') {
            errcount += insertonebypointer(&treesq1,m->addr_,ct);
            continue;
        }
        if(m->action_ == 'd') {
            errcount += deleteonebypointer(&treesq1,m->addr_,ct);
            continue;
        }
        printf("Fail applybypointer, bad action %s entry %d.\n",msg,ct);
        return 1;
    }
    dwarf_tdestroy(treesq1,mt_free_func);
    return errcount;

}


#ifdef HASHSEARCH
static DW_TSHASHTYPE
value_hashfunc(const void *keyp)
{
    VALTYPE up = (VALTYPE )keyp;
    return up;
}
#endif /* HASHFUNC */
static int
value_compare_func(const void *l, const void *r)
{
    VALTYPE lp = (VALTYPE)l;
    VALTYPE rp = (VALTYPE)r;
    if(lp < rp) {
        return -1;
    }
    if(lp > rp) {
        return 1;
    }
    return 0;
}

/* Nothing to free for the 'value' example. */
static void
value_node_free(void *valp)
{

}

static int
insertbyvalue(void **tree, VALTYPE addr,int ct)
{
    void *retval = 0;
    /*  tsearch adds an entry if its not present already. */
    /*  Since in this test we do not malloc anything there is no free needed either.
        Instead we just let tsearch store the value in the pointer in the tree.   */
    VALTYPE  newval = addr;
    retval = dwarf_tsearch((void *)newval,tree, value_compare_func  );
    if(retval == 0) {
        printf("FAIL ENOMEM in search  on item %d, value %lu, "
            "error in insertbyvalue\n",
            ct, (unsigned long)newval);
        exit(1);
    } else {
        /*  Since we insert a value there is no  possible distinction
            to be made between newly-inserted and found-in-tree. */
#ifndef FULL_SPEED_RUN
        {
            /* For debugging. */
            VALTYPE mt2 = addr;
            retval = dwarf_tfind((void *)mt2,tree,value_compare_func);
            if(!retval) {
                printf("insertone record %d value 0x%lu failed to add"
                    " as desired, error\n",
                    ct,(unsigned long)mt2);
                return 1;
            }
        }
#endif /*FULL_SPEED_RUN */
    }
    return 0;

}

static int
deletebyvalue(void **tree, unsigned  addr,int ct)
{
    void *r = 0;
    int err=0;

    VALTYPE newval = addr;
    /* We are not mallocing, so nothing to free he tree holds simple values for us. */
    r = dwarf_tfind((void *)newval,(void *const*)tree,value_compare_func);
    if (r) {
        void *r2 = dwarf_tdelete((void *)newval,tree,value_compare_func);
        if(r2) {
            /* tdelete returned parent */
        } else {
            /* tdelete returned NULL, tree now empty */
        }
    } else {
        printf("deletebyvalue action %d could not find rec! error!"
            " addr 0x%x\n",
            addr,ct);
        err = 1;
    }
    return err;
}



/*  This demonstrates using a simple integer as the
    value saved, as itself, not a pointer, per-se.

    The various flags are not important except in that
    they can help in case bugs still exist.

*/
static int
applybyvalue(struct myacts *m,
    const char *msg,
    int hideactions,
    int printwalk,
    int dumpeverystage)
{
    unsigned ct = 1;
    void *treesq1 = 0;
    int errcount = 0;

    INITTREE(treesq1,value_hashfunc);
    printf("special sequence %s\n",msg);
    for(; m->action_ != 0; m++,ct++) {
        if(!hideactions) {
            printf("Action %2u: %s 0x%x val 0x%x\n",ct,
                describe_action(m->action_),
                m->action_,m->addr_);
        }
        if(m->action_ == 'a') {
            errcount += insertbyvalue(&treesq1,m->addr_,ct);
            if(ct == 0) {
                printf("Add    done. action# %2d value 0x%x\n",
                    ct,m->addr_);
                dwarf_tdump(treesq1,value_keyprint,"first sequence2 added");
            } else if(dumpeverystage) {
                dwarf_tdump(treesq1,value_keyprint,"after add");
            }
            continue;
        }
        if(m->action_ == 'd') {
            errcount += deletebyvalue(&treesq1,m->addr_,ct);
            if(dumpeverystage) {
                printf("Delete done. action# %2d value 0x%x\n",
                    ct,m->addr_);
                dwarf_tdump(treesq1,value_keyprint,"after delete");
            }
            continue;
        }
        printf("Fail applybyvalue, bad action %s entry %d.\n",msg,ct);
        return 1;
    }
    if(printwalk) {
        printf("Twalk start, simple value \n");
        dwarf_twalk(treesq1,value_only_walk_entry);
        printf("Twalk end, simple value\n");
        dwarf_tdump(treesq1,value_keyprint,"tdump simple value from applybyvalue");
    }
    dwarf_tdestroy(treesq1,value_node_free);
    return errcount;

}


static int
standard_tests(void)
{
    void *tree1 = 0;
    int errcount = 0;

    if(applyby == applybypointer) {
        printf("Test with increasing input\n");
        INITTREE(tree1,mt_hashfunc);

        errcount += insertrecsbypointer(3,&tree1,increasing);
        errcount += findrecsbypointer(6,3,(const void **)&tree1,increasing);
        dwarf_twalk(tree1,walk_entry);
        dwarf_tdump(tree1,mt_keyprint,"Dump Tree from increasing input");
        errcount += delrecsbypointer(6,3,&tree1,increasing,0);
#ifdef HASHSEARCH
        dwarf_tdestroy(tree1,mt_free_func);
        tree1 = 0;
#endif
        if(tree1) {
            printf("FAIL: delrecsbypointer of increasing did not empty the tree.\n");
            exit(1);
        }

        printf("Test twalk with empty tree\n");
        dwarf_twalk(tree1,walk_entry);

        INITTREE(tree1,mt_hashfunc);
        printf("Insert decreasing, try tdestroy\n");
        errcount += insertrecsbypointer(6,&tree1,decreasing);
        dwarf_twalk(tree1,walk_entry);
        dwarf_tdestroy(tree1,mt_free_func);
        tree1 = 0;

        INITTREE(tree1,mt_hashfunc);
        printf("Now test with decreasing input and test twalk and tdelete\n");
        errcount += insertrecsbypointer(5,&tree1,decreasing);
        errcount += findrecsbypointer(6,5,(const void **)&tree1,decreasing);
        dwarf_twalk(tree1,walk_entry);
        dwarf_tdump(tree1,mt_keyprint,"Dump Tree from decreasing input");
        errcount += delrecsbypointer(6,5,&tree1,decreasing,0);
#ifdef HASHSEARCH
        dwarf_tdestroy(tree1,mt_free_func);
        tree1 = 0;
#endif
        if(tree1) {
            printf("FAIL: delrecsbypointer of decreasing did not empty the tree.\n");
            exit(1);
        }

        INITTREE(tree1,mt_hashfunc);
        printf("Now test with balanced input and test twalk and tdelete\n");
        errcount += insertrecsbypointer(4,&tree1,balanced);
        errcount += findrecsbypointer(6,4,(const void **)&tree1,balanced);
        dwarf_twalk(tree1,walk_entry);
        dwarf_tdump(tree1,mt_keyprint,"Dump Tree from balanced input");
        errcount += delrecsbypointer(6,4,&tree1,balanced,1);
#ifdef HASHSEARCH
        dwarf_tdestroy(tree1,mt_free_func);
        tree1 = 0;
#endif
        if(tree1) {
            printf("FAIL: delrecsbypointer of balanced did not empty the tree.\n");
            exit(1);
        }

        dwarf_twalk(tree1,walk_entry);
        if (errcount > 0) {
            printf("FAIL tsearch test.\n");
            exit(1);
        }

        errcount += applyby(&sequence1[0],"Sequence 1",
            g_hideactions,0,0);
        errcount += applyby(&sequence2[0],"Sequence 2, a",
            g_hideactions,0,0);
    } else {
        errcount += applyby(&sequence2[0],"Sequence 2, b",
            g_hideactions,0,0);
        errcount += applyby(&sequence3[0],"Sequence 3",
            g_hideactions,0,0);
        errcount += applyby(&sequential64[0],"Sequential 64",
            g_hideactions,1,0);
    }
    return errcount;
}

static int
getaddr(const char *in, unsigned long *addrout)
{
    unsigned long int res = 0;

    errno = 0;
    res = strtoul(in,0,0);
    if(errno) {
        return 1;
    }
    *addrout = res;
    return 0;
}

/*  Valid input lines start with
        # (the rest of the line ignored)
    or
        a 12345
    or
        d 0x12345
    meaning add a tree record or delete one, respectively.
    Where the value is the key.
    Leading spaces on a line are not allowed.
    Only a single space after the 'a' or 'd' and before
    the value is allowed.
*/
static int
build_filetest(struct myacts **tout, char *pathout,
   const char *filename,FILE *f)
{
    char buffer[500];
    char * buf = &buffer[0];
    size_t bufsize = sizeof(buffer);
    size_t filelen = 0;
    size_t ct = 0;
    int done = 0;
    size_t ixout = 0;
    struct myacts *recordacts = 0;

    while(!done) {
        ssize_t charsread = 0;

        bufsize = sizeof(buffer);
        charsread = getline(&buf,&bufsize,f);
        if(charsread < 0) {
            done = 1;
            break;
        }
        ++filelen;
    }
    rewind(f);
    /* Leave zeroed entry (at least one) at the end. */
    recordacts = calloc(sizeof(struct myacts),filelen+2);
    *tout = recordacts;
    strcpy(pathout,filename);
    done = 0;
    for(ct = 0; !done && ( ct < filelen); ++ct) {
        ssize_t charsread = 0;

        charsread = getline(&buf,&bufsize,f);
        if(charsread < 0) {
            done = 1;
            break;
        }
        if(buf[0] == '#') {
            continue;
        }
        if(buf[0] == 'a' && buf[1] == ' ') {
            int readaddrfail = 0;
            unsigned long addr = 0;
            recordacts[ixout].action_ = 'a';
            readaddrfail = getaddr(&buf[2],&addr);
            if(readaddrfail) {
                fprintf(stderr,"Improper value input, line %lu of file %s\n"
                    "%s\n",
                    (unsigned long)ct,filename,buf);
                return 1;
            }
            recordacts[ixout].addr_ = addr;
        } else if(buf[0] == 'd' && buf[1] == ' ') {
            int readaddrfail = 0;
            unsigned long addr = 0;
            recordacts[ixout].action_ = 'd';
            readaddrfail = getaddr(&buf[2],&addr);
            if(readaddrfail) {
                fprintf(stderr,"Improper value input, line %lu of file %s\n"
                    "%s\n",
                    (unsigned long)ct,filename,buf);
                return 1;
            }
            recordacts[ixout].addr_ = addr;
        } else {
            fprintf(stderr,"Improper input, line %lu of file %s\n"
                "%s\n",
                (unsigned long)ct,filename,buf);
            return 1;
        }
        ixout++;
    }
    return  0;
}


static int
fill_in_filetest(const char *filename)
{
    FILE *f = 0;
    int errcount = 0;

    f = fopen(filename,"r");
    if (!f) {
        fprintf(stderr,"Open of %s failed",filename);
        return 1;
    }
    if(!filetest1) {
        errcount += build_filetest(&filetest1,filetest1name,filename,f);
    } else if(!filetest2) {
        errcount += build_filetest(&filetest2,filetest2name,filename,f);
    } else if(!filetest3) {
        errcount += build_filetest(&filetest3,filetest3name,filename,f);
    } else if(!filetest4) {
        errcount += build_filetest(&filetest4,filetest4name,filename,f);
    } else {
        printf("Exceeded limit on input files. %s ignored\n",filename);
        errcount = 1;
    }

    fclose(f);
    return errcount;
}

static void
print_usage(const char *a, const char *b,const char *app)
{
    fprintf(stderr,"%s : %s\n",a,b);
    fprintf(stderr,"run as\n");
    fprintf(stderr,"  %s [-std] [samplefile]...\n",app);
    fprintf(stderr,"By default runs standard tests\n");
    fprintf(stderr,"with pathnames, standard tests are not run\n");
    fprintf(stderr,"unless -std passed in as first arg.\n");
    exit(1);
}

static void
readargs(int argc, char **argv)
{
    int ix = 0;
    int notedstd = 0;
    int defaultstd = 1;
    if(argc < 2) {
        /* No arguments, take defaults. */
        return;
    }
    for(ix = 1; ix <argc; ++ix) {
        int resfail = 0;
        const char *a=argv[ix];
        if(strcmp(a,"-showa") == 0) {
            g_hideactions = 0;
            continue;
        }
        if(strcmp(a,"-adds") == 0) {
            g_showallactions = 1;
            continue;
        }
        if(strcmp(a,"-std") == 0) {
            notedstd = 1;
            continue;
        }
        if(strcmp(a,"-byvalue") == 0) {
            applyby = applybyvalue;
            continue;
        }
        resfail = fill_in_filetest(a);
        defaultstd = 0;
        if(resfail) {
            print_usage("Failed in attempting to read in file ",a,argv[0]);
        }
    }
    runstandardtests = defaultstd;
    if(notedstd) {
        runstandardtests = 1;
    }
    return;
}

int
main(int argc, char **argv)
{
    int errcount = 0;
    applyby = applybypointer;
    readargs(argc,argv);

    if( runstandardtests) {
        errcount += standard_tests();
    }
    {
        if(filetest1) {
            errcount += applyby(filetest1,filetest1name,
                g_hideactions,0,g_showallactions);
        }
        if(filetest2) {
            errcount += applyby(filetest2,filetest2name,
                g_hideactions,0,g_showallactions);
        }
        if(filetest3) {
            errcount += applyby(filetest3,filetest3name,
                g_hideactions,0,g_showallactions);
        }
        if(filetest4) {
            errcount += applyby(filetest4,filetest4name,
                g_hideactions,0,g_showallactions);
        }
    }
    free(filetest1);
    free(filetest2);
    free(filetest3);
    free(filetest4);

    if(errcount) {
        printf("FAIL tsearch test.\n");
        exit(1);
    }
    printf("PASS tsearch test.\n");
    exit(0);
}
