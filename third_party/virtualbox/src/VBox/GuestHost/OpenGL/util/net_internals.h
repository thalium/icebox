#ifndef NET_INTERNALS_H
#define NET_INTERNALS_H

#include "cr_bufpool.h"
#include "cr_threads.h"

#ifndef WINDOWS
#include <sys/time.h>
#endif

/*
 * VirtualBox HGCM
 */
#ifdef VBOX_WITH_HGCM
extern void crVBoxHGCMInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl);
extern void crVBoxHGCMConnection( CRConnection *conn
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        , struct VBOXUHGSMI *pHgsmi
#endif
        );
extern int crVBoxHGCMRecv(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        CRConnection *conn
#else
        void
#endif
        );
#ifdef IN_GUEST
extern uint32_t crVBoxHGCMHostCapsGet(void);
#endif
extern CRConnection** crVBoxHGCMDump( int *num );
extern void crVBoxHGCMTearDown(void);
#endif

extern CRConnection** crNetDump( int *num );

#endif /* NET_INTERNALS_H */
