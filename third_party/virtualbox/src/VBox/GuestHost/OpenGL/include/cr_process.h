/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_PROCESS_H
#define CR_PROCESS_H

#ifdef WINDOWS
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

/*
 * Process ID type
 */
#ifdef WINDOWS
typedef HANDLE CRpid;
#else
typedef unsigned long CRpid;
#endif


extern DECLEXPORT(void) crSleep( unsigned int seconds );

extern DECLEXPORT(void) crMsleep( unsigned int msec );

extern DECLEXPORT(CRpid) crSpawn( const char *command, const char *argv[] );

extern DECLEXPORT(void) crKill( CRpid pid );

extern DECLEXPORT(void) crGetProcName( char *name, int maxLen );

extern DECLEXPORT(void) crGetCurrentDir( char *dir, int maxLen );

extern DECLEXPORT(CRpid) crGetPID(void);


#ifdef __cplusplus
}
#endif

#endif /* CR_PROCESS_H */
