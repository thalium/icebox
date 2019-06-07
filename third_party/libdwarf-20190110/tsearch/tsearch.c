/*
   Copyright David Anderson 2010-2014.
   This is free software.
   Permission hereby granted for anyone to copy or
   use this code for any purpose without restriction.
   Attribution may be given or may not, it is your choice.

   September 8, 2011: The tdelete example code was wrong in that
   it did not clean up entirely.  So was the tsearch example code.
   Both are now fixed (it's surprisingly difficult to do this
   all correctly, but then the available documentation is at best
   a hint).

   The GNU/Linux tsearch man page (in the man-page 3.54 edition)
   suggests that tdestroy() takes a pointer to a variable which
   points to the root.  In reality tdestroy() takes a pointer
   to the root.  As revealed by trying tdestroy() both ways.
*/

/* tsearch()  tfind()  tdelete() twalk() tdestroy() example.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* __USE_GNU exposes the GNU tdestroy() function, a
   function that is not mentioned by the Single Unix
   Specification. */
#define __USE_GNU 1
#include <search.h>

/* The struct is trivially usable to implement a set or
   map (mapping an integer to a string).
   The following struct is the example basis
   because that is the capability I wanted to use.
   tsearch has no idea what data is involved, only the comparison function
   mt_compare_func() and the  free function mt_free_func()
   (passed in to tsearch calls) know what data is involved.
   Making tsearch very flexible indeed.

   Obviously the use of a struct is arbitrary, it is just
   an example.
*/

struct my_tentry {
    unsigned mt_key;
    /* When using this as a set of mt_key the mt_name
    field is set to 0 (NULL). */
    char * mt_name;
};

/* We allow a NULL name so this struct acts sort of like a set
   and sort of like a map.
*/
struct my_tentry *
make_my_tentry(unsigned k,char *name)
{
    struct my_tentry *mt =
        (struct my_tentry *)calloc(sizeof(struct my_tentry),1);
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
void
mt_free_func(void *mt_data)
{
    struct my_tentry *m = mt_data;
    if(!m) {
        return;
    }
    free(m->mt_name);
    free(mt_data);
    return;
}

int mt_compare_func(const void *l, const void *r)
{
    const struct my_tentry *ml = l;
    const struct my_tentry *mr = r;
    if(ml->mt_key < mr->mt_key) {
        return -1;
    }
    if(ml->mt_key > mr->mt_key) {
        return 1;
    }
    return 0;
}
void
walk_entry(const void *mt_data,VISIT x,int level)
{
    struct my_tentry *m = *(struct my_tentry **)mt_data;
    printf("<%d>Walk on node %s %u %s  \n",
        level,
        x == preorder?"preorder":
        x == postorder?"postorder":
        x == endorder?"endorder":
        x == leaf?"leaf":
        "unknown",
        m->mt_key,m->mt_name);
    return;
}


int main()
{
    unsigned i;
    void *tree1 = 0;
#define RECMAX 3

    for(i = 0 ; i < RECMAX ; ++i) {
        int k = 0;
        char kbuf[40];
        char dbuf[60];
        struct my_tentry *mt = 0;
        struct my_tentry *retval = 0;
        snprintf(kbuf,sizeof(kbuf),"%u",i);
        strcpy(dbuf," data for ");
        strcat(dbuf,kbuf);

        /*  Do it twice so we have test the case where
            tsearch adds and one
            where it finds an existing record. */
        for (k = 0; k < 2 ;++k) {
            mt = make_my_tentry(i,dbuf);
            errno = 0;
            /* tsearch adds an entry if its not present already. */
            retval = tsearch(mt,&tree1, mt_compare_func  );
            if(retval == 0) {
                printf("Fail ENOMEM\n");
                exit(1);
            } else {
                struct my_tentry *re = 0;
                re = *(struct my_tentry **)retval;
                if(re != mt) {
                    printf("found existing ok %u\n",i);
                    /*  Prevents data leak: mt was
                        already present. */
                    mt_free_func(mt);
                } else {
                    printf("insert ok %u\n",i);
                    /* New entry mt was added. */
                }
            }
        }
    }
    for(i = 0 ; i < 5 ; ++i) {
        char kbuf[40];
        char dbuf[60];
        dbuf[0] = 0;
        struct my_tentry *mt = 0;
        struct my_tentry *retval = 0;

        snprintf(kbuf,sizeof(kbuf),"%u",i);
        mt = make_my_tentry(i,dbuf);
        retval = tfind(mt,&tree1,mt_compare_func);
        if(!retval) {
            if(i < RECMAX) {
                printf("Fail TSRCH on %s is FAILURE\n",kbuf);
                exit(1);
            } else {
                printf("Fail TSRCH on %s is ok\n",kbuf);
            }
        } else {
            printf("found ok %u\n",i);
        }
        mt_free_func(mt);
    }
    twalk(tree1,walk_entry);
    {
        struct my_tentry *mt = 0;
        struct my_tentry *re3 = 0;
        void *r = 0;
        mt = make_my_tentry(1,0);
        r = tfind(mt,&tree1,mt_compare_func);
        if (r) {
            /*  This is what tdelete will delete.
                tdelete just removes the reference from
                the tree, it does not actually delete
                the memory for the entry itself.  */
            re3 = *(struct my_tentry **)r;
            r = tdelete(mt,&tree1,mt_compare_func);
            /* We don't want the 'test' node left around. */
            mt_free_func(mt);
            if(r) {
                struct my_tentry *re2 = 0;
                re2 = *(struct my_tentry **)r;
                printf("tdelete returned parent: %u %s\n",
                    re2->mt_key, re2->mt_name);
            } else {
                printf("tdelete returned NULL, tree now empty.\n");
            }
            /* Delete the content of the node that tdelete removed. */
            mt_free_func(re3);
        } else {
            /* There is no node like this to delete. */
            /* We don't want the 'test' node left around. */
            mt_free_func(mt);
        }
    }
    twalk(tree1,walk_entry);

    tdestroy(tree1,mt_free_func);
    printf("PASS tsearch test.\n");
    exit(0);
}
