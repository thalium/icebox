/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_NETSERVER_H
#define CR_NETSERVER_H

#include "cr_net.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char *name;
	int buffer_size;
	CRConnection *conn;
} CRNetServer;

DECLEXPORT(void) crNetServerConnect( CRNetServer *ns
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , struct VBOXUHGSMI *pHgsmi
#endif
                );
DECLEXPORT(void) crNetNewClient( CRNetServer *ns
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , struct VBOXUHGSMI *pHgsmi
#endif
);

#ifdef __cplusplus
}
#endif

#endif /* CR_NETSERVER_H */
