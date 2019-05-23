/*
 * Copyright (C) 1995-2009 Contributors
 * More detailed copyright information can be found in the individual source code files.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
 *    If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/** Taken from:
 *  http://nsis.sourceforge.net/Examples/Plugin/exdll.h
 */

#ifndef _EXDLL_H_
#define _EXDLL_H_

#include <iprt/win/windows.h>

#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

// only include this file from one place in your DLL.
// (it is all static, if you use it in two places it will fail)

#define EXDLL_INIT()           {  \
        g_stringsize=string_size; \
        g_stacktop=stacktop;      \
        g_variables=variables; }

typedef struct _stack_t {
  struct _stack_t *next;
  char text[1]; // this should be the length of string_size
} stack_t;


static unsigned int g_stringsize;
static stack_t **g_stacktop;
static char *g_variables;

static int __stdcall popstring(char *str) UNUSED; // 0 on success, 1 on empty stack
static void __stdcall pushstring(const char *str) UNUSED;
static char * __stdcall getuservariable(const int varnum) UNUSED;
static void __stdcall setuservariable(const int varnum, const char *var) UNUSED;

enum
{
INST_0,         // $0
INST_1,         // $1
INST_2,         // $2
INST_3,         // $3
INST_4,         // $4
INST_5,         // $5
INST_6,         // $6
INST_7,         // $7
INST_8,         // $8
INST_9,         // $9
INST_R0,        // $R0
INST_R1,        // $R1
INST_R2,        // $R2
INST_R3,        // $R3
INST_R4,        // $R4
INST_R5,        // $R5
INST_R6,        // $R6
INST_R7,        // $R7
INST_R8,        // $R8
INST_R9,        // $R9
INST_CMDLINE,   // $CMDLINE
INST_INSTDIR,   // $INSTDIR
INST_OUTDIR,    // $OUTDIR
INST_EXEDIR,    // $EXEDIR
INST_LANG,      // $LANGUAGE
__INST_LAST
};

// utility functions (not required but often useful)
static int __stdcall popstring(char *str)
{
  stack_t *th;
  if (!g_stacktop || !*g_stacktop)
      return 1;
  th=(*g_stacktop);
  lstrcpyA(str,th->text);
  *g_stacktop = th->next;
  GlobalFree((HGLOBAL)th);
  return 0;
}

static void __stdcall pushstring(const char *str)
{
  stack_t *th;
  if (!g_stacktop)
      return;
  th=(stack_t*)GlobalAlloc(GPTR,sizeof(stack_t)+g_stringsize);
  lstrcpynA(th->text,str,g_stringsize);
  th->next=*g_stacktop;
  *g_stacktop=th;
}

static char * __stdcall getuservariable(const int varnum)
{
  if (varnum < 0 || varnum >= __INST_LAST)
      return NULL;
  return g_variables+varnum*g_stringsize;
}

static void __stdcall setuservariable(const int varnum, const char *var)
{
    if (var != NULL && varnum >= 0 && varnum < __INST_LAST)
        lstrcpyA(g_variables + varnum*g_stringsize, var);
}
#endif//_EXDLL_H_
