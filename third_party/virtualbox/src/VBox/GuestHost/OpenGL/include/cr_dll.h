/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_DLL_H
#define CR_DLL_H

#if defined(WINDOWS)
#define WIN32_LEAN_AND_MEAN
# ifdef VBOX
#  include <iprt/win/windows.h>
# else
#include <windows.h>
# endif
#endif

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char *name;
#if defined(WINDOWS)
	HINSTANCE hinstLib;
#elif defined(IRIX) || defined(IRIX64) || defined(Linux) || defined(FreeBSD) || defined(AIX) || defined(SunOS) || defined(OSF1)
	void *hinstLib;
#elif defined(DARWIN)
	void *hinstLib;		/* void to avoid including the headers */
	int type;		/* to avoid calling crStrstr all the time */
#else
#error ARCHITECTURE DLL NOT IMPLEMENTED
#endif
} CRDLL;

typedef void (*CRDLLFunc)(void);
DECLEXPORT(CRDLL *)   crDLLOpen( const char *dllname, int resolveGlobal );
DECLEXPORT(CRDLLFunc) crDLLGetNoError( CRDLL *dll, const char *symname );
DECLEXPORT(CRDLLFunc) crDLLGet( CRDLL *dll, const char *symname );
DECLEXPORT(void)      crDLLClose( CRDLL *dll );

#ifdef __cplusplus
}
#endif

#endif /* CR_DLL_H */
