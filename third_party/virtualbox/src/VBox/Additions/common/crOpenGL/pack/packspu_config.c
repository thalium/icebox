/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packspu.h"
#include "cr_string.h"
#include "cr_error.h"
#include "cr_spu.h"
#include "cr_mem.h"

#include <stdio.h>

static void __setDefaults( void )
{
    crMemZero(pack_spu.context, CR_MAX_CONTEXTS * sizeof(ContextInfo));
    pack_spu.numContexts = 0;

    crMemZero(pack_spu.thread, MAX_THREADS * sizeof(ThreadInfo));
    pack_spu.numThreads = 0;
}


void packspuSetVBoxConfiguration( const SPU *child_spu )
{
    RT_NOREF(child_spu);
    __setDefaults();
    pack_spu.emit_GATHER_POST_SWAPBUFFERS = 0;
    pack_spu.swapbuffer_sync = 0;
    pack_spu.name = crStrdup("vboxhgcm://llp:7000");
    pack_spu.buffer_size = 5 * 1024 * 1024;
}
