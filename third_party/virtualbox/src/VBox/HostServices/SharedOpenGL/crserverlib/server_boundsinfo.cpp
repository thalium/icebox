/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "server_dispatch.h"
#include "server.h"
#include "cr_error.h"
#include "cr_unpack.h"
#include "cr_mem.h"
#include "state/cr_statetypes.h"



/**
 * Process a crBoundsInfoCR message/function.  This is a bounding box
 * followed by a payload of arbitrary Chromium rendering commands.
 * The tilesort SPU will send this.
 * Note: the bounding box is in mural pixel coordinates (y=0=bottom)
 */
void SERVER_DISPATCH_APIENTRY
crServerDispatchBoundsInfoCR( const CRrecti *bounds, const GLbyte *payload,
															GLint len, GLint num_opcodes )
{
    RT_NOREF(bounds, payload, len, num_opcodes);
}
