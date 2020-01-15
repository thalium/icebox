/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_spu.h"
#include "packspu.h"
#include "cr_packfunctions.h"
#include <stdio.h>

extern SPUNamedFunctionTable _cr_pack_table[];

SPUFunctions pack_functions = {
    NULL, /* CHILD COPY */
    NULL, /* DATA */
    _cr_pack_table /* THE ACTUAL FUNCTIONS */
};

PackSPU pack_spu;
DECLHIDDEN(PCRStateTracker) g_pStateTracker;

CRtsd _PackTSD;
CRmutex _PackMutex;

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
# include <VBoxCrHgsmi.h>
# include <VBoxUhgsmi.h>
#endif

#if defined(RT_OS_WINDOWS) && defined(VBOX_WITH_WDDM)
static bool isVBoxWDDMCrHgsmi(void)
{
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    PVBOXUHGSMI pHgsmi = VBoxCrHgsmiCreate();
    if (pHgsmi)
    {
        VBoxCrHgsmiDestroy(pHgsmi);
        return true;
    }
#endif
    return false;
}
#endif /* RT_OS_WINDOWS && VBOX_WITH_WDDM */

static SPUFunctions *
packSPUInit( int id, SPU *child, SPU *self,
                         unsigned int context_id,
                         unsigned int num_contexts )
{
    ThreadInfo *thread;

    (void) context_id;
    (void) num_contexts;
    (void) child;
    (void) self;

    crInitMutex(&_PackMutex);

    crInitTSD(&_PackerTSD);
    crInitTSD(&_PackTSD);

    pack_spu.id = id;

    packspuSetVBoxConfiguration( child );

#if defined(WINDOWS) && defined(VBOX_WITH_WDDM)
    pack_spu.bIsWDDMCrHgsmi = isVBoxWDDMCrHgsmi();
#endif

#ifdef VBOX_WITH_CRPACKSPU_DUMPER
    memset(&pack_spu.Dumper, 0, sizeof (pack_spu.Dumper));
#endif

    if (!CRPACKSPU_IS_WDDM_CRHGSMI())
    {
        /* This connects to the server, sets up the packer, etc. */
        thread = packspuNewThread(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
            NULL
#endif
                 );

        if (!thread) {
            return NULL;
        }
        CRASSERT( thread == &(pack_spu.thread[0]) );
        pack_spu.idxThreadInUse = 0;
    }

    packspuCreateFunctions();
    crStateInit(&pack_spu.StateTracker);
    g_pStateTracker = &pack_spu.StateTracker;

    return &pack_functions;
}

static void
packSPUSelfDispatch(SPUDispatchTable *self)
{
    crSPUInitDispatchTable( &(pack_spu.self) );
    crSPUCopyDispatchTable( &(pack_spu.self), self );
}

static int
packSPUCleanup(void)
{
    int i;
    crLockMutex(&_PackMutex);
    for (i=0; i<MAX_THREADS; ++i)
    {
        if (pack_spu.thread[i].inUse && pack_spu.thread[i].packer)
        {
            crPackDeleteContext(pack_spu.thread[i].packer);
        }
    }

    crFreeTSD(&_PackerTSD);
    crFreeTSD(&_PackTSD);
    crUnlockMutex(&_PackMutex);
    crFreeMutex(&_PackMutex);
    crNetTearDown(); /** @todo Why here? */
    return 1;
}

DECLHIDDEN(const SPUREG) g_PackSpuReg =
 {
    /** pszName. */
    "pack",
    /** pszSuperName. */
    NULL,
    /** fFlags. */
    SPU_HAS_PACKER | SPU_IS_TERMINAL | SPU_MAX_SERVERS_ONE,
    /** pfnInit. */
    packSPUInit,
    /** pfnDispatch. */
    packSPUSelfDispatch,
    /** pfnCleanup. */
    packSPUCleanup
};
