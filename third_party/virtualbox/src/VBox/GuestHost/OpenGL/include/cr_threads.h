/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */
	
#ifndef CR_THREADS_H
#define CR_THREADS_H

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "chromium.h"
#include "cr_bits.h"

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
# ifdef VBOX
#  include <iprt/win/windows.h>
# else
#include <windows.h>
# endif
#else
#include <pthread.h>
#include <semaphore.h>
#endif

#include "cr_error.h"

#include <iprt/asm.h>
/*
 * Handle for Thread-Specific Data
 */
typedef struct {
#ifdef WINDOWS
	DWORD key;
#else
	pthread_key_t key;
#endif
	int initMagic;
} CRtsd;


extern DECLEXPORT(void) crInitTSD(CRtsd *tsd);
extern DECLEXPORT(void) crInitTSDF(CRtsd *tsd, void (*destructor)(void *));
extern DECLEXPORT(void) crFreeTSD(CRtsd *tsd);
extern DECLEXPORT(void) crSetTSD(CRtsd *tsd, void *ptr);
extern DECLEXPORT(void *) crGetTSD(CRtsd *tsd);
extern DECLEXPORT(unsigned long) crThreadID(void);


/* Mutex datatype */
#ifdef WINDOWS
typedef CRITICAL_SECTION CRmutex;
#else
typedef pthread_mutex_t CRmutex;
#endif

extern DECLEXPORT(void) crInitMutex(CRmutex *mutex);
extern DECLEXPORT(void) crFreeMutex(CRmutex *mutex);
extern DECLEXPORT(void) crLockMutex(CRmutex *mutex);
extern DECLEXPORT(void) crUnlockMutex(CRmutex *mutex);


/* Condition variable datatype */
#ifdef WINDOWS
typedef int CRcondition;
#else
typedef pthread_cond_t CRcondition;
#endif

extern DECLEXPORT(void) crInitCondition(CRcondition *cond);
extern DECLEXPORT(void) crFreeCondition(CRcondition *cond);
extern DECLEXPORT(void) crWaitCondition(CRcondition *cond, CRmutex *mutex);
extern DECLEXPORT(void) crSignalCondition(CRcondition *cond);


/* Barrier datatype */
typedef struct {
	unsigned int count;
#ifdef WINDOWS
	HANDLE hEvents[CR_MAX_CONTEXTS];
#else
	unsigned int waiting;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
#endif
} CRbarrier;

extern DECLEXPORT(void) crInitBarrier(CRbarrier *b, unsigned int count);
extern DECLEXPORT(void) crFreeBarrier(CRbarrier *b);
extern DECLEXPORT(void) crWaitBarrier(CRbarrier *b);


/* Semaphores */
#ifdef WINDOWS
	typedef int CRsemaphore;
#else
	typedef sem_t CRsemaphore;
#endif

extern DECLEXPORT(void) crInitSemaphore(CRsemaphore *s, unsigned int count);
extern DECLEXPORT(void) crWaitSemaphore(CRsemaphore *s);
extern DECLEXPORT(void) crSignalSemaphore(CRsemaphore *s);

#define VBoxTlsRefGetImpl(_tls) (crGetTSD((CRtsd*)(_tls)))
#define VBoxTlsRefSetImpl(_tls, _val) (crSetTSD((CRtsd*)(_tls), (_val)))
#define VBoxTlsRefAssertImpl CRASSERT
#include <VBox/Graphics/VBoxVideo3D.h>

#ifdef __cplusplus
}
#endif

#endif /* CR_THREADS_H */
