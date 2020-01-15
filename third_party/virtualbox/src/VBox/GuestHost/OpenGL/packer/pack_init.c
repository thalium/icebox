/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_error.h"
#include "cr_mem.h"
#include "packer.h"
#include <stdio.h>

CRtsd _PackerTSD;
int cr_packer_globals;  /* dummy - for the sake of packer.def */
uint32_t cr_packer_cmd_blocks_enabled = 0;

CRPackContext *crPackNewContext(void)
{
    CRPackContext *pc = crCalloc(sizeof(CRPackContext));
    if (!pc)
        return NULL;
    crInitMutex(&pc->mutex);
    pc->u32CmdBlockState = 0;
    pc->Flush = NULL;
    pc->SendHuge = NULL;
    pc->updateBBOX = 0;
    return pc;
}

void crPackDeleteContext(CRPackContext *pc)
{
    crFreeMutex(&pc->mutex);
    crFree(pc);
}

/* Set packing context for the calling thread */
void crPackSetContext( CRPackContext *pc )
{
    crSetTSD( &_PackerTSD, pc );
}


/* Return packing context for the calling thread */
CRPackContext *crPackGetContext( void )
{
    return (CRPackContext *) crGetTSD( &_PackerTSD );
}

void crPackCapsSet(uint32_t u32Caps)
{
    cr_packer_cmd_blocks_enabled = (u32Caps & (CR_VBOX_CAP_CMDBLOCKS_FLUSH | CR_VBOX_CAP_CMDBLOCKS));
}
