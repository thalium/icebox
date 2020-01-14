/*
Copyright (c) 2019, David Anderson
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
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

#include "config.h"
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_MALLOC_H
/* Useful include for some Windows compilers. */
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <string.h>
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include <sys/types.h> /* for open() */
#include <sys/stat.h> /* for open() */
#include <fcntl.h> /* for open() */
#include <errno.h>

#include "dwarf_incl.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"
#include "dwarfstring.h"
#include "dwarf_debuglink.h"

static int errcount = 0;

/* dummy func we do not need real one */
int _dwarf_load_section(Dwarf_Debug dbg,
    struct Dwarf_Section_s *section,
    Dwarf_Error * error)
{
   return DW_DLV_OK;
}

/* A horrible fake version for these tests */
void
_dwarf_error(Dwarf_Debug dbg, Dwarf_Error * error,
    Dwarf_Signed errval)
{
    static struct Dwarf_Error_s stuff;

    stuff.er_errval = errval;
    *error = &stuff;
}

/* literal copy from dwarf_error.c */
Dwarf_Unsigned
dwarf_errno(Dwarf_Error error)
{
    if (!error) {
        return (0);
    }
    return (error->er_errval);
}

/* A literal copy from dwarf_util.c */
int
_dwarf_check_string_valid(Dwarf_Debug dbg,void *areaptr,
    void *strptr, void *areaendptr,
    int suggested_error,
    Dwarf_Error*error)
{
    Dwarf_Small *start = areaptr;
    Dwarf_Small *p = strptr;
    Dwarf_Small *end = areaendptr;

    if (p < start) {
        _dwarf_error(dbg,error,suggested_error);
        return DW_DLV_ERROR;
    }
    if (p >= end) {
        _dwarf_error(dbg,error,suggested_error);
        return DW_DLV_ERROR;
    }
    if (dbg->de_assume_string_in_bounds) {
        /* This NOT the default. But folks can choose
            to live dangerously and just assume strings ok. */
        return DW_DLV_OK;
    }
    while (p < end) {
        if (*p == 0) {
            return DW_DLV_OK;
        }
        ++p;
    }
    _dwarf_error(dbg,error,DW_DLE_STRING_NOT_TERMINATED);
    return DW_DLV_ERROR;
}



static void
check_svalid(int expret,int gotret,int experr,int goterr,int line,
    char *filename_in)
{

    if (expret != gotret) {
        errcount++;
        printf("ERROR expected return %d, got %d line %d %s\n",
            expret,gotret,line,filename_in);
    }
    if (experr != goterr) {
        errcount++;
        printf("ERROR expected errcode %d, got %d line %d %s\n",
            experr,goterr,line,filename_in);
    }
}

static void
test1(Dwarf_Debug dbg)
{
    char testbuffer[1000];
    char *area = testbuffer;
    char * str = testbuffer;
    const char *msg = "This is a simple string for testing.";
    int res = 0;
    char *end = testbuffer +100;
    int errcode = 0;
    Dwarf_Error error = 0;


    testbuffer[0] = 0;
    strcpy(testbuffer,msg);
    /* The error value is arbitrary, not realistic. */
    res = _dwarf_check_string_valid(dbg,
        area,str,
        end,DW_DLE_CORRUPT_GNU_DEBUGID_STRING,
        &error);
    check_svalid(DW_DLV_OK,res,
        0,dwarf_errno(error),
        __LINE__,__FILE__);

    end = testbuffer +10;
    res = _dwarf_check_string_valid(dbg,
        area,str,
        end,DW_DLE_STRING_NOT_TERMINATED,
        &error);
    check_svalid(DW_DLV_ERROR, res,
        DW_DLE_STRING_NOT_TERMINATED, dwarf_errno(error),
        __LINE__,__FILE__);

    end = testbuffer +10;
    area = end +2;
    res = _dwarf_check_string_valid(dbg,area,str,
        end,DW_DLE_CORRUPT_GNU_DEBUGID_STRING,
        &error);
    check_svalid(DW_DLV_ERROR,res,
        DW_DLE_CORRUPT_GNU_DEBUGID_STRING,  dwarf_errno(error),
        __LINE__,__FILE__);

}

static void
checkjoin(int expret,int gotret,char*expstr,char*gotstr,
    int line,
    const char *filename_in)
{
    if (expret != gotret) {
        errcount++;
        printf("ERROR expected return %d, got %d line %d %s\n",
            expret,gotret,line,filename_in);
    }
    if (strcmp(expstr,gotstr)) {
        errcount++;
        printf("ERROR expected string \"%s\", got \"%s\" line %d %s\n",
            expstr,gotstr,line,filename_in);
    }
}


#if 0
int
_dwarf_pathjoinl(dwarfstring *target,dwarfstring * input);
#endif
static void
test2(Dwarf_Debug dbg)
{
    dwarfstring targ;
    dwarfstring inp;
    int res = 0;

    dwarfstring_constructor(&targ);
    dwarfstring_constructor(&inp);

    dwarfstring_append(&targ,"/a/b");
    dwarfstring_append(&inp,"foo");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"/a/b/foo",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_append(&targ,"gef");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"gef/foo",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&targ,"gef/");
    dwarfstring_append(&inp,"/jkl/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"gef/jkl/",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&targ,"gef/");
    dwarfstring_append(&inp,"jkl/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"gef/jkl/",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&targ,"gef");
    dwarfstring_append(&inp,"jkl/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"gef/jkl/",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&inp,"/jkl/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"/jkl/",dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&inp,"jkl/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"jkl/",dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&targ,"jkl");
    dwarfstring_append(&inp,"pqr/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"jkl/pqr/",dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&targ,"/");
    dwarfstring_append(&inp,"/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"/",dwarfstring_string(&targ),
        __LINE__,__FILE__);


    dwarfstring_destructor(&targ);
    dwarfstring_destructor(&inp);
}


static void
checklinkedto(int expret,int gotret,
    int expcount,int gotcount,int line, char *filename_in)
{
    if (expret != gotret) {
        errcount++;
        printf("ERROR expected return %d, got %d line %d %s\n",
            expret,gotret,line,filename_in);
    }
    if (expcount != gotcount) {
        errcount++;
        printf("ERROR expected return %d, got %d line %d %s\n",
            expcount,gotcount,line,filename_in);
    }
}

static void
printpaths(unsigned count,char **array,dwarfstring *fullpath)
{
    unsigned i = 0;

    printf("linkstring full path: %s\n",
        dwarfstring_string(fullpath));
    printf("\n");

    printf("    Paths:\n");
    for(i = 0 ; i < count ; ++i) {
        char *s = array[i];

        printf("    [%2d] \"%s\"\n",i,s);
    }
    printf("\n");


}

#if 0
int _dwarf_construct_linkedto_path(
   char         **global_prefixes_in,
   unsigned       length_global_prefixes_in,
   char          *pathname_in,
   char          *link_string_in, /* from debug link */
   unsigned char  *crc_in, /* from debug_link, 4 bytes */
   unsigned       builid_length, /* from gnu buildid */
   unsigned char *builid, /* from gnu buildid */
   char        ***paths_out,
   unsigned      *paths_out_length,
   int *errcode);
#endif

static unsigned char buildid[20] = {
    0x11,0x22,0x33, 0x44,
    0x21,0x22,0x23, 0x44,
    0xa1,0xa2,0xa3, 0xa4,
    0xb1,0xb2,0xb3, 0xb4,
    0xc1,0xc2,0xc3, 0xc4 };
/*  Since we don't find the files here this
    is not a good test. However, the program
    is used by rundebuglink.sh */
static void
test3(Dwarf_Debug dbg)
{
    char * executablepath = "a/b";
    char * linkstring = "de";
    dwarfstring result;
    char ** global_prefix = 0;
    unsigned global_prefix_count = 2;
    unsigned global_prefix_len = 0;
    unsigned char crc[4];
    unsigned buildid_length = 20;
    char **paths_returned = 0;
    unsigned paths_returned_count = 0;
    int errcode = 0;
    Dwarf_Error error = 0;
    int res = 0;
    dwarfstring linkstring_fullpath;
    unsigned i = 0;

    crc[0] = 0x12;
    crc[1] = 0x34;
    crc[2] = 0x56;
    crc[3] = 0xab;
    global_prefix_len =  (global_prefix_count+1)
        * sizeof(void *);
    res = dwarf_add_debuglink_global_path(dbg,
        "/usr/lib/debug",&error);
    if (res != DW_DLV_OK){
        ++errcount;
        printf("Adding debuglink global path failed line %d %s\n",
            __LINE__,__FILE__);
        exit(1);
    }
    res = dwarf_add_debuglink_global_path(dbg,
        "/fake/lib/debug",&error);
    if (res != DW_DLV_OK){
        ++errcount;
        printf("Adding debuglink global path failed line %d %s\n",
            __LINE__,__FILE__);
        exit(1);
    }


    dbg->de_path = executablepath;
    dwarfstring_constructor(&result);
    dwarfstring_constructor(&linkstring_fullpath);
    res =_dwarf_construct_linkedto_path(
        (char **)dbg->de_gnu_global_paths,
        dbg->de_gnu_global_path_count,
        executablepath,
        linkstring,
        &linkstring_fullpath,
        crc,
        buildid,
        buildid_length,
        &paths_returned,&paths_returned_count,
        &errcode);
    checklinkedto(DW_DLV_OK,res,6,paths_returned_count,
        __LINE__,__FILE__);
    printpaths(paths_returned_count,paths_returned,&linkstring_fullpath);
    free(paths_returned);
    paths_returned = 0;
    paths_returned_count = 0;
    errcode = 0;

    dwarfstring_reset(&linkstring_fullpath);
    dwarfstring_reset(&result);
    executablepath = "ge";
    linkstring = "h/i";
    res =_dwarf_construct_linkedto_path(
        (char **)dbg->de_gnu_global_paths,
        dbg->de_gnu_global_path_count,
        executablepath,linkstring,
        &linkstring_fullpath,
        crc,
        buildid,
        buildid_length,
        &paths_returned,&paths_returned_count,
        &errcode);
    checklinkedto(DW_DLV_OK,res,6,paths_returned_count,
        __LINE__,__FILE__);
    printpaths(paths_returned_count,paths_returned,&linkstring_fullpath);
    free(paths_returned);
    paths_returned = 0;
    paths_returned_count = 0;
    errcode = 0;

    dwarfstring_reset(&result);
    dwarfstring_reset(&linkstring_fullpath);
    executablepath = "/somewherespecial/a/b/ge";
    linkstring = "h/i";
    res =_dwarf_construct_linkedto_path(
        (char **)dbg->de_gnu_global_paths,
        dbg->de_gnu_global_path_count,
        executablepath,linkstring,
        &linkstring_fullpath,
        crc,
        buildid,
        buildid_length,
        &paths_returned,&paths_returned_count,
        &errcode);
    checklinkedto(DW_DLV_OK,res,6,paths_returned_count,
        __LINE__,__FILE__);
    printpaths(paths_returned_count,paths_returned,&linkstring_fullpath);
    free(paths_returned);
    free(global_prefix);
    paths_returned = 0;
    paths_returned_count = 0;
    for (i = 0; i < dbg->de_gnu_global_path_count; ++i) {
        free((char *) dbg->de_gnu_global_paths[i]);
        dbg->de_gnu_global_paths[i] = 0;
    }
    free(dbg->de_gnu_global_paths);
    dbg->de_gnu_global_paths = 0;
    errcode = 0;
    dwarfstring_destructor(&result);
    dwarfstring_destructor(&linkstring_fullpath);
}

int main()
{
    Dwarf_Debug dbg = 0;
    struct Dwarf_Debug_s db;

    dbg = &db;
    memset(dbg,0,sizeof(db));

#if 0
    dbg->de_copy_word = _dwarf_memcpy_noswap_bytes;
#ifdef WORDS_BIGENDIAN
    dbg->de_big_endian_object = 1;
    if (endianness == DW_OBJECT_LSB ) {
        dbg->de_same_endian = 0;
        dbg->de_big_endian_object = 0;
        dbg->de_copy_word = _dwarf_memcpy_swap_bytes;
    }
#else /* little endian */
    dbg->de_big_endian_object = 0;
    if (endianness == DW_OBJECT_MSB ) {
        dbg->de_same_endian = 0;
        dbg->de_big_endian_object = 1;
        dbg->de_copy_word = _dwarf_memcpy_swap_bytes;
    }
#endif /* !WORDS_BIGENDIAN */
#endif


    test1(dbg);
    test2(dbg);
    test3(dbg);

    if (errcount) {
        return 1;
    }
    return 0;
}
