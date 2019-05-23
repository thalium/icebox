/* $Id: dlm_lists.c $ */
/** @file
 * Implementation of all the Display Lists related routines:
 *
 *   glGenLists, glDeleteLists, glNewList, glEndList, glCallList, glCallLists,
 *   glListBase and glIsList.
 *
 * Provide OpenGL IDs mapping between host and guest.
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

#include <float.h>
#include "cr_dlm.h"
#include "cr_mem.h"
#include "dlm.h"


/**
 * Destroy each list entry.
 */
static void crdlmFreeDisplayListElements(DLMInstanceList *instance)
{
    while (instance)
    {
        DLMInstanceList *nextInstance = instance->next;
        crFree(instance);
        instance = nextInstance;
    }
}


/**
 * A callback routine used when iterating over all
 * available lists in order to remove them.
 *
 * NOTE: @param pParam2 might be NULL.
 */
void crdlmFreeDisplayListResourcesCb(void *pParm1, void *pParam2)
{
    DLMListInfo      *pListInfo     = (DLMListInfo *)pParm1;
    SPUDispatchTable *dispatchTable = (SPUDispatchTable *)pParam2;

    if (pListInfo)
    {
        crdlmFreeDisplayListElements(pListInfo->first);
        pListInfo->first = pListInfo->last = NULL;

        /* Free host OpenGL resources. */
        if (dispatchTable)
            dispatchTable->DeleteLists(pListInfo->hwid, 1);

        crFree(pListInfo);
    }
}


/**
 * Generate host and guest IDs, setup IDs mapping between host and guest.
 */
GLuint DLM_APIENTRY crDLMGenLists(GLsizei range, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();
    GLuint             idHostRangeStart = 0;
    GLuint             idGuestRangeStart = 0;

    crDebug("DLM: GenLists(%d) (DLM=%p).", range, listState ? listState->dlm : 0);

    if (listState)
    {
        idHostRangeStart = dispatchTable->GenLists(range);
        if (idHostRangeStart > 0)
        {
            idGuestRangeStart = crHashtableAllocKeys(listState->dlm->displayLists, range);
            if (idGuestRangeStart > 0)
            {
                GLuint i;
                bool fSuccess = true;

                /* Now have successfully generated IDs range for host and guest. Let's make IDs association. */
                for (i = 0; i < (GLuint)range; i++)
                {
                    DLMListInfo *pListInfo;

                    pListInfo = (DLMListInfo *)crCalloc(sizeof(DLMListInfo));
                    if (pListInfo)
                    {
                        crMemset(pListInfo, 0, sizeof(DLMListInfo));
                        pListInfo->hwid = idHostRangeStart + i;

                        /* Insert pre-initialized list data which contains IDs mapping into the hash. */
                        crHashtableReplace(listState->dlm->displayLists, idGuestRangeStart + i, pListInfo, NULL);
                    }
                    else
                    {
                        fSuccess = false;
                        break;
                    }
                }

                /* All structures allocated and initialized successfully. */
                if (fSuccess)
                    return idGuestRangeStart;

                /* Rollback some data was not allocated. */
                crDLMDeleteLists(idGuestRangeStart, range, NULL /* we do DeleteLists() later in this routine */ );
            }
            else
                crDebug("DLM: Can't allocate Display List IDs range for the guest.");

            dispatchTable->DeleteLists(idHostRangeStart, range);
        }
        else
            crDebug("DLM: Can't allocate Display List IDs range on the host side.");
    }
    else
        crDebug("DLM: GenLists(%u) called with no current state.", range);

    /* Can't reserve IDs range.  */
    return 0;
}


/**
 * Release host and guest IDs, free memory resources.
 */
void DLM_APIENTRY crDLMDeleteLists(GLuint list, GLsizei range, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();

    crDebug("DLM: DeleteLists(%u, %d) (DLM=%p).", list, range, listState ? listState->dlm : 0);

    if (listState)
    {
        if (range >= 0)
        {
            int i;

            /* Free resources: host memory, host IDs and guest IDs. */
            DLM_LOCK(listState->dlm)
            for (i = 0; i < range; i++)
                crHashtableDeleteEx(listState->dlm->displayLists, list + i, crdlmFreeDisplayListResourcesCb, dispatchTable);
            DLM_UNLOCK(listState->dlm)
        }
        else
            crDebug("DLM: DeleteLists(%u, %d) not allowed.", list, range);
    }
    else
        crDebug("DLM: DeleteLists(%u, %d) called with no current state.", list, range);
}


/**
 * Start recording a list.
 */
void DLM_APIENTRY
crDLMNewList(GLuint list, GLenum mode, SPUDispatchTable *dispatchTable)
{
    DLMListInfo       *listInfo;
    CRDLMContextState *listState = CURRENT_STATE();

    crDebug("DLM: NewList(%u, %u) (DLM=%p).", list, mode, listState ? listState->dlm : 0);

    if (listState)
    {
        /* Valid list ID should be > 0. */
        if (list > 0)
        {
            if (listState->currentListInfo == NULL)
            {
                listInfo = (DLMListInfo *)crHashtableSearch(listState->dlm->displayLists, list);
                if (listInfo)
                {
                    listInfo->first = listInfo->last = NULL;
                    listInfo->stateFirst = listInfo->stateLast = NULL;

                    listInfo->numInstances = 0;

                    listState->currentListInfo = listInfo;
                    listState->currentListIdentifier = list;
                    listState->currentListMode = mode;

                    dispatchTable->NewList(listInfo->hwid, mode);

                    crDebug("DLM: create new list with [guest, host] ID pair [%u, %u].", list, listInfo->hwid);

                    return;
                }
                else
                    crDebug("DLM: Requested Display List %u was not previously reserved with glGenLists().", list);
            }
            else
                crDebug("DLM: NewList called with display list %u while display list %u was already open.", list, listState->currentListIdentifier);
        }
        else
            crDebug("DLM: NewList called with a list identifier of 0.");
    }
    else
        crDebug("DLM: NewList(%u, %u) called with no current state.\n", list, mode);
}


/**
 * Stop recording a list.
 */
void DLM_APIENTRY crDLMEndList(SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();

    crDebug("DLM: EndList() (DLM=%p).", listState ? listState->dlm : 0);

    if (listState)
    {
        /* Check if list was ever started. */
        if (listState->currentListInfo)
        {
            /* reset the current state to show the list had been ended */
            listState->currentListIdentifier = 0;
            listState->currentListInfo = NULL;
            listState->currentListMode = GL_FALSE;

            dispatchTable->EndList();
        }
        else
            crDebug("DLM: glEndList() is assuming glNewList() was issued previously.");
    }
    else
        crDebug("DLM: EndList called with no current state.");
}


/**
 * Execute list on hardware and cach ethis call if we currently recording a list.
 */
void DLM_APIENTRY crDLMCallList(GLuint list, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();

    //crDebug("DLM: CallList(%u).", list);

    if (listState)
    {
        DLMListInfo *listInfo;

        /* Add to calls cache if we recording a list. */
        if (listState->currentListInfo)
            crDLMCompileCallList(list);

        /* Find hwid for list.
         * We need to take into account listBase:
         * - displayLists hash table contains absolute IDs, so we need to add offset in order to resolve guest ID;
         * - we also need to substract from hwid in order to execute correct list. */
        listInfo = (DLMListInfo *)crHashtableSearch(listState->dlm->displayLists, list + listState->listBase);
        if (listInfo)
            dispatchTable->CallList(listInfo->hwid - listState->listBase);
        else
            crDebug("DLM: CallList(%u) issued for non-existent list.", list);
    }
    else
        crDebug("DLM: CallList(%u) called with no current state.", list);
}


/* This routine translates guest Display List IDs in given format to host IDs
 * and return resulting IDs as an array of elements of type GL_UNSIGNED_INT.
 * It is based on TranslateListIDs() function from crserverlib/server_lists.c. */
static bool
crDLMConvertListIDs(CRDLMContextState *pListState, GLsizei n, GLenum type, const GLvoid *aGuest, GLuint *aHost)
{
#define CRDLM_HANDLE_CONVERSION_CASE(_type, _item) \
    { \
        const _type *src = (const _type *)aGuest; \
        for (i = 0; i < n; i++) \
        { \
            GLuint idGuest = (GLuint)(_item) + pListState->listBase; \
            pListInfo = (DLMListInfo *)crHashtableSearch(pListState->dlm->displayLists, idGuest); \
            if (pListInfo) \
            { \
                aHost[i] = pListInfo->hwid - pListState->listBase; \
            } \
            else \
            { \
                crDebug("DLM: CallLists() cannot resolve host list ID for guest ID %u.", idGuest); \
                fSuccess = false; \
                break; \
            } \
        } \
    }

    GLsizei      i;
    DLMListInfo *pListInfo;
    bool         fSuccess = true;

    switch (type)
    {
        case GL_UNSIGNED_BYTE:  CRDLM_HANDLE_CONVERSION_CASE(GLubyte,  src[i]); break;
        case GL_BYTE:           CRDLM_HANDLE_CONVERSION_CASE(GLbyte,   src[i]); break;
        case GL_UNSIGNED_SHORT: CRDLM_HANDLE_CONVERSION_CASE(GLushort, src[i]); break;
        case GL_SHORT:          CRDLM_HANDLE_CONVERSION_CASE(GLshort,  src[i]); break;
        case GL_UNSIGNED_INT:   CRDLM_HANDLE_CONVERSION_CASE(GLuint,   src[i]); break;
        case GL_INT:            CRDLM_HANDLE_CONVERSION_CASE(GLint,    src[i]); break;
        case GL_FLOAT:          CRDLM_HANDLE_CONVERSION_CASE(GLfloat,  src[i]); break;

        case GL_2_BYTES:
        {
            CRDLM_HANDLE_CONVERSION_CASE(GLubyte, src[i * 2 + 0] * 256 +
                                                  src[i * 2 + 1]);
            break;
        }

        case GL_3_BYTES:
        {
            CRDLM_HANDLE_CONVERSION_CASE(GLubyte, src[i * 3 + 0] * 256 * 256 +
                                                  src[i * 3 + 1] * 256 +
                                                  src[i * 3 + 2]);
            break;
        }

        case GL_4_BYTES:
        {
            CRDLM_HANDLE_CONVERSION_CASE(GLubyte, src[i * 4 + 0] * 256 * 256 * 256 +
                                                  src[i * 4 + 1] * 256 * 256 +
                                                  src[i * 4 + 2] * 256 +
                                                  src[i * 4 + 3]);
            break;
        }

        default:
            crWarning("DLM: attempt to pass to crDLMCallLists() an unknown type: 0x%x.", type);
    }

    return fSuccess;
#undef CRDLM_HANDLE_CONVERSION_CASE
}


/**
 * Execute lists on hardware and cache this call if we currently recording a list.
 */
void DLM_APIENTRY crDLMCallLists(GLsizei n, GLenum type, const GLvoid *lists, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *pListState = CURRENT_STATE();

    crDebug("DLM: CallLists(%d, %u, %p).", n, type, lists);

    if (pListState)
    {
        GLsizei i;
        GLuint  *aHostIDs;

        /* Add to calls cache if we recording a list. */
        if (pListState->currentListInfo)
            crDLMCompileCallLists(n, type, lists);

        aHostIDs = (GLuint *)crAlloc(n * sizeof(GLuint));
        if (aHostIDs)
        {
            /* Convert IDs. Resulting array contains elements of type of GL_UNSIGNED_INT. */
            if (crDLMConvertListIDs(pListState, n, type, lists, aHostIDs))
                dispatchTable->CallLists(n, GL_UNSIGNED_INT, aHostIDs);
            else
                crDebug("DLM: CallLists() failed.");

            crFree(aHostIDs);
        }
        else
            crDebug("DLM: no memory on CallLists().");
    }
    else
        crDebug("DLM: CallLists(%d, %u, %p) called with no current state.", n, type, lists);
}


/**
 * Set list base, remember its value and add call to the cache.
 */
void DLM_APIENTRY crDLMListBase(GLuint base, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *pListState = CURRENT_STATE();

    crDebug("DLM: ListBase(%u).", base);

    if (pListState)
    {
        pListState->listBase = base;

        /* Only add to cache if we are currently recording a list. */
        /** @todo Do we really need to chache it? */
        if (pListState->currentListInfo)
            crDLMCompileListBase(base);

        dispatchTable->ListBase(base);
    }
    else
        crDebug("DLM: ListBase(%u) called with no current state.", base);
}


/**
 * Check if specified list ID belongs to valid Display List.
 * Positive result is only returned in case both conditions below are satisfied:
 *
 *   - given list found in DLM hash table (i.e., it was previously allocated
 *     with crDLMGenLists and still not released with crDLMDeleteLists);
 *
 *   - list is valid on the host side.
 */
GLboolean DLM_APIENTRY crDLMIsList(GLuint list, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();

    crDebug("DLM: IsList(%u).", list);

    if (listState)
    {
        if (list > 0)
        {
            DLMListInfo *listInfo = (DLMListInfo *)crHashtableSearch(listState->dlm->displayLists, list);
            if (listInfo)
            {
                if (dispatchTable->IsList(listInfo->hwid))
                    return true;
                else
                    crDebug("DLM: list [%u, %u] not found on the host side.", list, listInfo->hwid);
            }
            else
                crDebug("DLM: list %u not found in guest cache.", list);
        }
        else
            crDebug("DLM: IsList(%u) is not allowed.", list);
    }
    else
        crDebug("DLM: IsList(%u) called with no current state.", list);

    return false;
}
