/* $Id: expandospu_config.c $ */
/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "expandospu.h"

//#include "cr_mothership.h"
#include "cr_string.h"

#include <stdio.h>

static void __setDefaults( void )
{
}

/* option, type, nr, default, min, max, title, callback
 */
SPUOptions expandoSPUOptions[] = {
	{ NULL, CR_BOOL, 0, NULL, NULL, NULL, NULL, NULL },
};


void expandospuGatherConfiguration( void )
{
	CRConnection *conn;

	__setDefaults();
#if 0
	/* Connect to the mothership and identify ourselves. */
	
	conn = crMothershipConnect( );
	if (!conn)
	{
		/* The mothership isn't running.  Some SPU's can recover gracefully, some 
		 * should issue an error here. */
		crSPUSetDefaultParams( &expando_spu, expandoSPUOptions );
		return;
	}
	crMothershipIdentifySPU( conn, expando_spu.id );

	crSPUGetMothershipParams( conn, &expando_spu, expandoSPUOptions );

	crMothershipDisconnect( conn );
#endif
}
