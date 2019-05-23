/* $Id: expandospu_init.c $ */
/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "cr_spu.h"
#include "cr_dlm.h"
#include "cr_hash.h"
#include "cr_mem.h"
#include "expandospu.h"

/* This magic number is used for SSM data consistency check. */
#define VBOX_EXPANDOSPU_SSM_MAGIC           0x3d3d3d3d
/* Modify VBox Expando SPU SSM version if SSM data structure changed. */
#define VBOX_EXPANDOSPU_SSM_VERSION_ONE     1
#define VBOX_EXPANDOSPU_SSM_VERSION         VBOX_EXPANDOSPU_SSM_VERSION_ONE

ExpandoSPU expando_spu;

static SPUFunctions expando_functions = {
    NULL,               /* CHILD COPY */
    NULL,               /* DATA */
    _cr_expando_table   /* THE ACTUAL FUNCTIONS */
};

/*
 * Structure of SSM data:
 *
 * <VBOX_EXPANDOSPU_SSM_MAGIC>
 * <VBOX_EXPANDOSPU_SSM_VERSION>
 * <Number of Expando SPU contexts>
 *
 *     <Context ID>
 *     <CRDLMContextState structure>
 *         <DLM module data>
 *
 *     <Next context...>
 *
 * <VBOX_EXPANDOSPU_SSM_MAGIC>
 */

static void
expandoSPUSaveContextCb(unsigned long id, void *pData1, void *pData2)
{
    uint32_t   ui32 = (uint32_t)id;
    PSSMHANDLE pSSM = (PSSMHANDLE)pData2;
    int32_t    rc;

    ExpandoContextState *pExpandoContextState = (ExpandoContextState *)pData1;
    CRDLMContextState    dlmContextState;

    /* Save context ID. */
    rc = SSMR3PutU32(pSSM, ui32); AssertRCReturnVoid(rc);

    /* Save DLM context state. Clean fields which will not be valid on restore (->dlm and ->currentListInfo).
     * We interested only in fields: currentListIdentifier, currentListMode and listBase. */
    crMemcpy(&dlmContextState, pExpandoContextState->dlmContext, sizeof(CRDLMContextState));
    dlmContextState.dlm             = NULL;
    dlmContextState.currentListInfo = NULL;
    rc = SSMR3PutMem(pSSM, &dlmContextState, sizeof(CRDLMContextState)); AssertRCReturnVoid(rc);

    /* Delegate the rest of work to DLM module. */
    crDLMSaveState(pExpandoContextState->dlmContext->dlm, pSSM);
}

static int
expandoSPUSaveState(void *pData)
{
    uint32_t    magic   = VBOX_EXPANDOSPU_SSM_MAGIC;
    uint32_t    version = VBOX_EXPANDOSPU_SSM_VERSION;
    PSSMHANDLE  pSSM    = (PSSMHANDLE)pData;
    int32_t     rc;
    uint32_t    cStates;

    crDebug("Saving state of Expando SPU.");

    AssertReturn(pSSM, 1);

    /* Magic & version first. */
    rc = SSMR3PutU32(pSSM, magic);      AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, version);    AssertRCReturn(rc, rc);

    /* Store number of Expando SPU contexts. */
    cStates = (uint32_t)crHashtableNumElements(expando_spu.contextTable);
    rc = SSMR3PutU32(pSSM, cStates);  AssertRCReturn(rc, rc);

    /* Walk over context table and store required data. */
    crHashtableWalk(expando_spu.contextTable, expandoSPUSaveContextCb, pSSM);

    /* Expando SPU and DLM data should end with magic (consistency check). */
    rc = SSMR3PutU32(pSSM, magic);      AssertRCReturn(rc, rc);

    return 0;
}

static int
expandoSPULoadState(void *pData)
{
    uint32_t    magic   = 0;
    uint32_t    version = 0;
    PSSMHANDLE  pSSM    = (PSSMHANDLE)pData;
    int32_t     rc;

    crDebug("Loading state of Expando SPU.");

    AssertReturn(pSSM, 1);

    /* Check magic and version. */
    rc = SSMR3GetU32(pSSM, &magic);
    AssertRCReturn(rc, rc);

    if (magic == VBOX_EXPANDOSPU_SSM_MAGIC)
    {
        rc = SSMR3GetU32(pSSM, &version);
        AssertRCReturn(rc, rc);

        if (version >= VBOX_EXPANDOSPU_SSM_VERSION_ONE)
        {
            uint32_t cStates = 0;
            uint32_t i;
            bool     fSuccess = false;

            CRDLMContextState *pCurrentDLMState;
            CRContext         *pCurrentCRState;

            /* Remember current state. */
            pCurrentDLMState = crDLMGetCurrentState();
            pCurrentCRState  = crStateGetCurrent();

            /* Restore number of Expando SPU contexts. */
            rc = SSMR3GetU32(pSSM, &cStates);
            AssertRCReturn(rc, rc);

            /* Restore and update Expando SPU contexts one by one. */
            for (i = 0; i < cStates; i++)
            {
                uint32_t             idContext = 0;
                ExpandoContextState *pExpandoContextState;

                rc = SSMR3GetU32(pSSM, &idContext);
                AssertRCReturn(rc, rc);

                /* Find context which was previously created by CR Server. */
                pExpandoContextState = crHashtableSearch(expando_spu.contextTable, idContext);
                if (pExpandoContextState)
                {
                    CRDLMContextState dlmContextState;

                    /* Restore and update DLM context state. */
                    rc = SSMR3GetMem(pSSM, &dlmContextState, sizeof(CRDLMContextState));
                    if (RT_SUCCESS(rc))
                    {
                        pExpandoContextState->dlmContext->currentListIdentifier = dlmContextState.currentListIdentifier;
                        pExpandoContextState->dlmContext->currentListMode       = dlmContextState.currentListMode;
                        pExpandoContextState->dlmContext->listBase              = dlmContextState.listBase;

                        crDLMSetCurrentState(pExpandoContextState->dlmContext);
                        crStateMakeCurrent(pExpandoContextState->State);

                        /* Delegate the rest of work to DLM module. */
                        fSuccess = crDLMLoadState(pExpandoContextState->dlmContext->dlm, pSSM, &expando_spu.server->dispatch);
                        if (fSuccess)
                        {
                            continue;
                        }
                        else
                        {
                            crError("Expando SPU: stop restoring Display Lists.");
                            break;
                        }
                    }
                    else
                    {
                        crError("Expando SPU: unable to load state: state file structure error (1).");
                        break;
                    }
                }
                else
                {
                    crError("Expando SPU: unable to load state: no context ID %u found.", idContext);
                    break;
                }
            }

            /* Restore original state. */
            crDLMSetCurrentState(pCurrentDLMState);
            crStateMakeCurrent(pCurrentCRState);

            if (fSuccess)
            {
                /* Expando SPU and DLM data should end with magic (consistency check). */
                magic = 0;
                rc = SSMR3GetU32(pSSM, &magic);
                if (RT_SUCCESS(rc))
                {
                    if (magic == VBOX_EXPANDOSPU_SSM_MAGIC)
                    {
                        crInfo("Expando SPU state loaded.");
                        return 0;
                    }
                    else
                        crError("Expando SPU: unable to load state: SSM data corrupted.");
                }
                else
                    crError("Expando SPU: unable to load state: state file structure error (2): no magic.");
            }
            else
                crError("Expando SPU: unable to load state: some list(s) could not be restored.");
        }
        else
            crError("Expando SPU: unable to load state: unexpected SSM version (0x%x).", version);
    }
    else
        crError("Expando SPU: unable to load state: SSM data possibly corrupted.");

    return VERR_SSM_UNEXPECTED_DATA;
}

static SPUFunctions *
expandoSPUInit(int id, SPU *child, SPU *self, unsigned int context_id, unsigned int num_contexts)
{

    (void)self;
    (void)context_id;
    (void)num_contexts;

    expando_spu.id = id;
    expando_spu.has_child = 0;
    expando_spu.server = NULL;

    if (child)
    {
        crSPUInitDispatchTable(&(expando_spu.child));
        crSPUCopyDispatchTable(&(expando_spu.child), &(child->dispatch_table));
        expando_spu.has_child = 1;
    }

    crSPUInitDispatchTable(&(expando_spu.super));
    crSPUCopyDispatchTable(&(expando_spu.super), &(self->superSPU->dispatch_table));
    expandospuGatherConfiguration();

    /* Expando-specific initialization */
    expando_spu.contextTable = crAllocHashtable();

    /* We'll be using the state tracker for each context */
    crStateInit();

    /* Export optional interfaces for SPU save/restore. */
    self->dispatch_table.spu_save_state = expandoSPUSaveState;
    self->dispatch_table.spu_load_state = expandoSPULoadState;

    return &expando_functions;
}

static void
expandoSPUSelfDispatch(SPUDispatchTable *self)
{
    crSPUInitDispatchTable(&(expando_spu.self));
    crSPUCopyDispatchTable(&(expando_spu.self), self);

    expando_spu.server = (CRServer *)(self->server);
}


static int
expandoSPUCleanup(void)
{
    crFreeHashtable(expando_spu.contextTable, expando_free_context_state);
    crStateDestroy();
    return 1;
}

int
SPULoad(char **name, char **super, SPUInitFuncPtr *init, SPUSelfDispatchFuncPtr *self,
    SPUCleanupFuncPtr *cleanup, SPUOptionsPtr *options, int *flags)
{
    *name = "expando";
    *super = "render";
    *init = expandoSPUInit;
    *self = expandoSPUSelfDispatch;
    *cleanup = expandoSPUCleanup;
    *options = expandoSPUOptions;
    *flags = (SPU_NO_PACKER|SPU_NOT_TERMINAL|SPU_MAX_SERVERS_ZERO);

    return 1;
}
