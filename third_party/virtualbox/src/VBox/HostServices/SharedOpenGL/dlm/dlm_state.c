/* $Id: dlm_state.c $ */
/** @file
 * Implementation of saving and restoring Display Lists.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "cr_mem.h"
#include "cr_dlm.h"
#include "dlm.h"
#include "dlm_generated.h"

#include "VBox/vmm/ssm.h"
#include "iprt/err.h"


typedef struct {

    PSSMHANDLE pSSM;
    uint32_t   err;

} CRDLMSaveListsCbArg;

static void crDLMSaveListsCb(unsigned long key, void *pData1, void *pData2)
{
    DLMListInfo         *pListInfo      = (DLMListInfo*)pData1;
    CRDLMSaveListsCbArg *pArg           = (CRDLMSaveListsCbArg *)pData2;
    PSSMHANDLE           pSSM           = pArg->pSSM;
    DLMInstanceList     *pInstance      = pListInfo->first;
    uint32_t             cInstanceCheck = 0;
    int32_t              rc;

    crDebug("Saving Display Lists: found ID=%u, numInstances=%d.", key, pListInfo->numInstances);

    /* Store Display List length. */
    rc = SSMR3PutU32(pSSM, pListInfo->numInstances);
    if (RT_SUCCESS(rc))
    {
        /* Store Display List (guest) ID. */
        rc = SSMR3PutU32(pSSM, (uint32_t)key);
        if (RT_SUCCESS(rc))
        {
            /* Store each Display List item one by one. */
            while (pInstance)
            {
                /* Let's count each list item and compare total number with pListInfo->numInstances.
                 * This is simple consistency check. */
                cInstanceCheck++;

                /* Store instance data size. */
                rc = SSMR3PutU32(pSSM, (uint32_t)pInstance->cbInstance);
                if (RT_SUCCESS(rc))
                {
                    rc = SSMR3PutMem(pSSM, pInstance, pInstance->cbInstance);
                    if (RT_SUCCESS(rc))
                    {
                        /* We just stored all we need. Let's move on to the next list element. */
                        pInstance = pInstance->next;
                        continue;
                    }
                }

                crError("Saving Display Lists: can't store data.");

                pArg->err = 1;
                return;
            }

            if (cInstanceCheck == pListInfo->numInstances)
                return;

            crError("Saving Display Lists: list currupted.");
        }
    }

    pArg->err = 1;
}

int32_t DLM_APIENTRY crDLMSaveState(CRDLM *dlm, PSSMHANDLE pSSM)
{
    uint32_t ui32;
    int32_t  rc;

    CRDLMSaveListsCbArg arg;

    arg.pSSM = pSSM;
    arg.err = 0;

    /* Save number of Display Lists assigned to current DLM context. */
    ui32 = (uint32_t)crHashtableNumElements(dlm->displayLists);
    rc = SSMR3PutU32(pSSM, ui32); AssertRCReturn(rc, rc);

    crHashtableWalk(dlm->displayLists, crDLMSaveListsCb, (void *)&arg);

    return arg.err == 0;
}

static VBoxDLMExecuteFn crDLMGetExecuteRoutine(VBoxDLOpCode opcode)
{
    if (opcode < VBOX_DL_OPCODE_MAX)
        return g_VBoxDLMExecuteFns[opcode];

    crError("Restoring Display Lists: Invalid opcode %u.", opcode);

    return NULL;
}

static bool
crDLMLoadListInstance(PSSMHANDLE pSSM, DLMListInfo *pListInfo, SPUDispatchTable *dispatchTable)
{
    uint32_t         cbInstance = 0;
    DLMInstanceList *pInstance;
    int32_t          rc;

    /* Get Display List item size. */
    rc = SSMR3GetU32(pSSM, &cbInstance);
    if (RT_SUCCESS(rc))
    {
        /* Allocate memory for the item, initialize it and put into the list. */
        pInstance = crCalloc(cbInstance);
        if (pInstance)
        {
            crMemset(pInstance, 0, cbInstance);

            rc = SSMR3GetMem(pSSM, pInstance, cbInstance); AssertRCReturn(rc, rc);
            if (RT_SUCCESS(rc))
            {
                pInstance->execute = crDLMGetExecuteRoutine(pInstance->iVBoxOpCode);
                if (pInstance->execute)
                {
                    pInstance->execute(pInstance, dispatchTable);

                    pInstance->next         = NULL;
                    pInstance->stateNext    = NULL;
                    pInstance->cbInstance   = cbInstance;

                    pListInfo->numInstances++;

                    if (!pListInfo->first)
                        pListInfo->first = pInstance;

                    if (pListInfo->last)
                        pListInfo->last->next = pInstance;

                    pListInfo->last = pInstance;

                    return true;
                }
                else
                    crError("Restoring Display Lists: unknown list item (opcode=%u).", pInstance->iVBoxOpCode);
            }
            else
                crError("Restoring Display Lists: can't read list element size.");
        }
        else
            crError("Restoring Display Lists: not enough memory, aborting.");
    }
    else
        crError("Restoring Display Lists: saved state file might be corrupted.");

    return false;
}

static bool
crDLMLoadList(CRDLM *dlm, PSSMHANDLE pSSM, SPUDispatchTable *dispatchTable)
{
    uint32_t cElements = 0;
    uint32_t idList    = 0;
    uint32_t i;
    int32_t  rc;

    /* Restore Display List length. */
    rc = SSMR3GetU32(pSSM, &cElements);
    if (RT_SUCCESS(rc))
    {
        /* Restore Display List ID. */
        rc = SSMR3GetU32(pSSM, &idList);
        if (RT_SUCCESS(rc))
        {
            /* Initialize new list data and start recording it. */
            DLMListInfo *pListInfo;

            pListInfo = (DLMListInfo *)crCalloc(sizeof(DLMListInfo));
            if (pListInfo)
            {
                GLuint hwid;

                crMemset(pListInfo, 0, sizeof(DLMListInfo));

                hwid = dispatchTable->GenLists(1);
                if (hwid > 0)
                {
                    bool               fSuccess = true;
                    CRDLMContextState *pDLMContextState;

                    pListInfo->numInstances = 0;
                    pListInfo->stateFirst   = pListInfo->stateLast = NULL;
                    pListInfo->hwid         = hwid;

                    dispatchTable->NewList(hwid, GL_COMPILE);

                    /* Fake list state in order to prevent expando SPU from double caching. */
                    pDLMContextState = crDLMGetCurrentState();
                    pDLMContextState->currentListMode = GL_FALSE;

                    crDebug("Restoring Display Lists:\t%u elements to restore.", cElements);

                    /* Iterate over list instances. */
                    for (i = 0; i < cElements; i++)
                    {
                        fSuccess = crDLMLoadListInstance(pSSM, pListInfo, dispatchTable);
                        if (!fSuccess)
                            break;
                    }

                    dispatchTable->EndList();

                    if (fSuccess)
                    {
                        /* Add list to cache. */
                        crHashtableReplace(dlm->displayLists, idList, pListInfo, NULL);
                        return true;
                    }
                    else
                        crError("Restoring Display Lists: some elements could not be restored.");
                }
                else
                    crError("Restoring Display Lists: can't allocate hwid for list %u.", idList);

                crFree(pListInfo);
            }
            else
                crError("Restoring Display Lists: can't allocate memory.");
        }
        else
            crError("Restoring Display Lists: can't get list ID.");
    }
    else
        crError("Restoring Display Lists: can't get number of elements in list.");

    return false;
}


bool DLM_APIENTRY
crDLMLoadState(CRDLM *dlm, PSSMHANDLE pSSM, SPUDispatchTable *dispatchTable)
{
    uint32_t cLists = 0;
    uint32_t i;
    int32_t  rc;
    bool     fSuccess = true;

    /* Get number of Display Lists assigned to current DLM context. */
    rc = SSMR3GetU32(pSSM, &cLists);
    if (RT_SUCCESS(rc))
    {
        crDebug("Restoring Display Lists: %u lists to restore.", cLists);

        for (i = 0; i < cLists; i++)
        {
            fSuccess = crDLMLoadList(dlm, pSSM, dispatchTable);
            if (!fSuccess)
                break;
        }
    }
    else
        crError("Restoring Display Lists: can't get number of lists.");

    return fSuccess;
}
