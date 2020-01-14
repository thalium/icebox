/*
Copyright (c) 2019-2019, David Anderson
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

#include <stdio.h>  /* for printf */
#include <stdlib.h>
#include <string.h>
#include "dwarfstring.h"
#ifndef TRUE
#define TRUE 1
#endif /* TRUE */
#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

static int errcount;

static void
check_string(const char *msg,char *exp,
    char *actual,int line)
{
    if(!strcmp(exp,actual)) {
        return;
    }
    printf("FAIL %s expected \"%s\" got \"%s\" test line %d\n",
        msg,exp,actual,line);
    ++errcount;
}
static void
check_value(const char *msg,unsigned long exp,
    unsigned long actual,int line)
{
    if(exp == actual) {
        return;
    }
    printf("FAIL %s expected %lu got %lu test line %d\n",
        msg,exp,actual,line);
    ++errcount;
}

static int
test1(int tnum)
{
    struct dwarfstring_s g;
    char *d = 0;
    const char *expstr = "";
    int res = 0;
    unsigned long biglen = 0;
    char *bigstr = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
        "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
        "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
        "ccccccbbbbbyyyybbbbbbbbbbbbccc";

    dwarfstring_constructor(&g);
    d = dwarfstring_string(&g);
    check_string("expected empty string",(char *)expstr,d,__LINE__);

    res = dwarfstring_append(&g,"abc");
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwarfstring_string(&g);
    check_string("expected abc ",(char *)"abc",d,__LINE__);

    res = dwarfstring_append(&g,"xy");
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwarfstring_string(&g);
    check_string("expected abcxy ",(char *)"abcxy",d,__LINE__);

    dwarfstring_destructor(&g);

    dwarfstring_constructor(&g);
    res = dwarfstring_append(&g,bigstr);
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwarfstring_string(&g);
    check_string("expected bigstring ",bigstr,d,__LINE__);
    biglen = dwarfstring_strlen(&g);
    check_value("expected 120  ",strlen(bigstr),biglen,__LINE__);

    dwarfstring_append_length(&g,"xxyyzz",3);

    biglen = dwarfstring_strlen(&g);
    check_value("expected 123  ",strlen(bigstr)+3,biglen,__LINE__);
    dwarfstring_destructor(&g);
    return 0;
}

static int
test2(int tnum)
{
    struct dwarfstring_s g;
    char *d = 0;
    const char *expstr = "";
    int res = 0;
    unsigned long biglen = 0;
    char *bigstr = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
        "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
        "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
        "ccccccbbbbbyyyybbbbbbbbbbbbccc";

    dwarfstring_constructor_fixed(&g,10);

    d = dwarfstring_string(&g);
    check_string("expected empty string",(char *)expstr,d,__LINE__);

    res = dwarfstring_append(&g,"abc");
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwarfstring_string(&g);
    check_string("expected abc ",(char *)"abc",d,__LINE__);

    res = dwarfstring_append(&g,"xy");
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwarfstring_string(&g);
    check_string("expected abcxy ",(char *)"abcxy",d,__LINE__);

    dwarfstring_destructor(&g);

    dwarfstring_constructor_fixed(&g,3);
    res = dwarfstring_append(&g,bigstr);
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwarfstring_string(&g);
    check_string("expected bigstring ",bigstr,d,__LINE__);
    biglen = dwarfstring_strlen(&g);
    check_value("expected 120  ",strlen(bigstr),biglen,__LINE__);
    dwarfstring_destructor(&g);
    return 0;
}

static int
test3(int tnum)
{
    struct dwarfstring_s g;
    char *d = 0;
    const char *expstr = "";
    int res = 0;
    unsigned long biglen = 0;
    char *bigstr = "a012345";
    char *targetbigstr = "a012345xy";

    dwarfstring_constructor_fixed(&g,10);

    d = dwarfstring_string(&g);
    check_string("expected empty string",(char *)expstr,d,__LINE__);

    res = dwarfstring_append(&g,bigstr);
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwarfstring_string(&g);
    check_string("expected a012345 ",(char *)bigstr,d,__LINE__);

    res = dwarfstring_append_length(&g,"xyzzz",2);
    check_value("expected TRUE  ",TRUE,res,__LINE__);

    check_value("expected 9  ", 9,(unsigned)dwarfstring_strlen(&g),
        __LINE__);

    d = dwarfstring_string(&g);
    check_string("expected a012345xy ",
        (char *)targetbigstr,d,__LINE__);
    dwarfstring_destructor(&g);
    return 0;
}

static int
test4(int tnum)
{
    struct dwarfstring_s g;
    char *d = 0;
    const char *expstr = "";
    int res = 0;
    unsigned long biglen = 0;
    char *mystr = "a01234";
    char *targetmystr = "a01234xyz";
    char fixedarea[7];

    dwarfstring_constructor_static(&g,fixedarea,sizeof(fixedarea));

    d = dwarfstring_string(&g);
    check_string("expected empty string",(char *)expstr,d,__LINE__);
    res = dwarfstring_append(&g,mystr);
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwarfstring_string(&g);
    check_string("expected a01234 ",(char *)mystr,d,__LINE__);

    if (d != fixedarea) {
        printf(" FAIL  reallocated but should not line %d ",
            __LINE__);
        ++errcount;
    }
    res = dwarfstring_append(&g,"xyz");
    d = dwarfstring_string(&g);
    check_string("expected a01234xyz ",(char *)targetmystr,
        d,__LINE__);
    check_value("expected 9",9,dwarfstring_strlen(&g),__LINE__);

    if (d == fixedarea) {
        printf(" FAIL  not reallocated but should  be! line %d ",
            __LINE__);
        ++errcount;
    }
    dwarfstring_destructor(&g);
    return 0;
}

static int
test5(int tnum)
{
    struct dwarfstring_s g;
    char *d = 0;
    const char *expstr = "";
    int res = 0;
    unsigned long biglen = 0x654a;
    signed long lminus= -55;
    char *mystr = "a01234";
    char *targetmystr = "a01234xyz";

    dwarfstring_constructor(&g);
    dwarfstring_append_printf_s(&g,"initialstring-%s-finalstring",
        mystr);
    d = dwarfstring_string(&g);
    expstr = "initialstring-a01234-finalstring";
    check_string("from pct-s",(char *)expstr,d,__LINE__);
    dwarfstring_destructor(&g);

    dwarfstring_constructor(&g);
    dwarfstring_append_printf_u(&g,"initialstring--0x%x-finalstring",
        biglen);
    d = dwarfstring_string(&g);
    expstr = "initialstring--0x654a-finalstring";
    check_string("from pct-s",(char *)expstr,d,__LINE__);
    dwarfstring_destructor(&g);

    dwarfstring_constructor(&g);
    dwarfstring_append_printf_i(&g,"initialstring: %d (",
        lminus);
    dwarfstring_append_printf_u(&g,"0x%08x) -finalstring",
        lminus);
    d = dwarfstring_string(&g);
    expstr = "initialstring: -55 (0xffffffffffffffc9) -finalstring";
    check_string("from pct-s",(char *)expstr,d,__LINE__);
    dwarfstring_destructor(&g);

    dwarfstring_constructor(&g);
    dwarfstring_append_printf_u(&g,"smallhex (0x%08x) -finalstring",
        biglen);
    d = dwarfstring_string(&g);
    expstr = "smallhex (0x0000654a) -finalstring";
    check_string("from pct-s",(char *)expstr,d,__LINE__);
    dwarfstring_destructor(&g);

    dwarfstring_constructor(&g);
    dwarfstring_append_printf_u(&g,"longlead (%08u) -end",
        20);
    d = dwarfstring_string(&g);
    expstr = "longlead (00000020) -end";
    check_string("from pct-s",(char *)expstr,d,__LINE__);
    dwarfstring_destructor(&g);
    return 0;
    return 0;
}   


int main()
{
    test1(1);
    test2(2);
    test3(3);
    test4(3);
    test5(3);
    if (errcount) {
        exit(1);
    }
    exit(0);
}
