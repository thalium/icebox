/* $Id: dlm.h $ */

#ifndef _DLM_H
#define _DLM_H

#include "cr_dlm.h"
#include "cr_spu.h"

#ifdef CHROMIUM_THREADSAFE
#define DLM_LOCK(dlm) crLockMutex(&(dlm->dlMutex));
#define DLM_UNLOCK(dlm) crUnlockMutex(&(dlm->dlMutex));
extern CRtsd CRDLMTSDKey;
#define SET_CURRENT_STATE(state) crSetTSD(&CRDLMTSDKey, (void *)state);
#define CURRENT_STATE() ((CRDLMContextState *)crGetTSD(&CRDLMTSDKey))
#else
#define DLM_LOCK(dlm)
#define DLM_UNLOCK(dlm)
extern CRDLMContextState *CRDLMCurrentState;
#define SET_CURRENT_STATE(state) CRDLMCurrentState = (state);
#define CURRENT_STATE() (CRDLMCurrentState)
#endif

/* These routines are intended to be used within the DLM library, across
 * the modules therein, but not as an API into the DLM library from
 * outside.
 */
extern void crdlmWarning( int line, char *file, GLenum error, char *format, ... );
extern void crdlmFreeDisplayListResourcesCb(void *pParm1, void *pParam2);
extern void crdlm_error(int line, const char *file, GLenum error, const char *info);

#endif
