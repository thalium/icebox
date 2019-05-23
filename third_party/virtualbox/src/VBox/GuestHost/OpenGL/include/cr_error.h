/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_ERROR_H
#define CR_ERROR_H

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WARN
# ifndef IN_RING0
#  define LOG(_m) do { crDebug _m ; } while (0)
#  define LOGREL(_m) do { crDebug _m ; } while (0)
#  define WARN(_m) do { crWarning _m ; AssertMsgFailed(_m); } while (0)
# else
#  define LOG(_m) do { } while (0)
#  define LOGREL(_m) do { } while (0)
#  define WARN(_m) do { AssertMsgFailed(_m); } while (0)
# endif
#endif

DECLEXPORT(void) crEnableWarnings(int onOff);

DECLEXPORT(void) crDebug(const char *format, ... ) RT_IPRT_FORMAT_ATTR(1, 2);
DECLEXPORT(void) crDbgCmdPrint(const char *description1, const char *description2, const char *cmd, ...);
DECLEXPORT(void) crDbgCmdSymLoadPrint(const char *modName, const void*pvAddress);
#if defined(DEBUG_misha) && defined(RT_OS_WINDOWS)
typedef void FNCRDEBUG(const char *format, ... ) RT_IPRT_FORMAT_ATTR(1, 2);
typedef FNCRDEBUG *PFNCRDEBUG;
DECLINLINE(PFNCRDEBUG) crGetDebug() {return crDebug;}
# define crWarning (RT_BREAKPOINT(), crDebug)
#else
DECLEXPORT(void) crWarning(const char *format, ... ) RT_IPRT_FORMAT_ATTR(1, 2);
#endif
DECLEXPORT(void) crInfo(const char *format, ... ) RT_IPRT_FORMAT_ATTR(1, 2);

DECLEXPORT(void) crError(const char *format, ... ) RT_IPRT_FORMAT_ATTR(1, 2);

/* Throw more info while opengl is not stable */
#if defined(DEBUG) || 1
# ifdef DEBUG_misha
#  include <iprt/assert.h>
#  define CRASSERT Assert
/*extern int g_VBoxFbgFBreakDdi;*/
#  define CR_DDI_PROLOGUE() do { /*if (g_VBoxFbgFBreakDdi) {Assert(0);}*/ } while (0)
# else
#  define CRASSERT( PRED ) ((PRED)?(void)0:crWarning( "Assertion failed: %s=%d, file %s, line %d", #PRED, (int)(intptr_t)(PRED), __FILE__, __LINE__))
#  define CR_DDI_PROLOGUE() do {} while (0)
# endif
# define THREADASSERT( PRED ) ((PRED)?(void)0:crError( "Are you trying to run a threaded app ?\nBuild with 'make threadsafe'\nAssertion failed: %s, file %s, line %d", #PRED, __FILE__, __LINE__))
#else
# define CRASSERT( PRED ) ((void)0)
# define THREADASSERT( PRED ) ((void)0)
# define CR_DDI_PROLOGUE() do {} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* CR_ERROR_H */
