/** @file
 * Shared Folders: Mappings support.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef UNITTEST
# include "testcase/tstSharedFolderService.h"
#endif

#include "mappings.h"
#include "vbsfpath.h"
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/path.h>
#include <iprt/string.h>

#ifdef UNITTEST
# include "teststubs.h"
#endif

/* Shared folders order in the saved state and in the FolderMapping can differ.
 * So a translation array of root handle is needed.
 */

static MAPPING FolderMapping[SHFL_MAX_MAPPINGS];
static SHFLROOT aIndexFromRoot[SHFL_MAX_MAPPINGS];

void vbsfMappingInit(void)
{
    unsigned root;

    for (root = 0; root < RT_ELEMENTS(aIndexFromRoot); root++)
    {
        aIndexFromRoot[root] = SHFL_ROOT_NIL;
    }
}

int vbsfMappingLoaded(const PMAPPING pLoadedMapping, SHFLROOT root)
{
    /* Mapping loaded from the saved state with the index. Which means
     * the guest uses the iMapping as root handle for this folder.
     * Check whether there is the same mapping in FolderMapping and
     * update the aIndexFromRoot.
     *
     * Also update the mapping properties, which were lost: cMappings.
     */
    if (root >= SHFL_MAX_MAPPINGS)
    {
        return VERR_INVALID_PARAMETER;
    }

    SHFLROOT i;
    for (i = 0; i < RT_ELEMENTS(FolderMapping); i++)
    {
        MAPPING *pMapping = &FolderMapping[i];

        /* Equal? */
        if (   pLoadedMapping->fValid == pMapping->fValid
            && ShflStringSizeOfBuffer(pLoadedMapping->pMapName) == ShflStringSizeOfBuffer(pMapping->pMapName)
            && memcmp(pLoadedMapping->pMapName, pMapping->pMapName, ShflStringSizeOfBuffer(pMapping->pMapName)) == 0)
        {
            /* Actual index is i. */
            aIndexFromRoot[root] = i;

            /* Update the mapping properties. */
            pMapping->cMappings = pLoadedMapping->cMappings;

            return VINF_SUCCESS;
        }
    }

    /* No corresponding mapping on the host but the guest still uses it.
     * Add a 'placeholder' mapping.
     */
    LogRel2(("SharedFolders: mapping a placeholder for '%ls' -> '%s'\n",
              pLoadedMapping->pMapName->String.ucs2, pLoadedMapping->pszFolderName));
    return vbsfMappingsAdd(pLoadedMapping->pszFolderName, pLoadedMapping->pMapName,
                           pLoadedMapping->fWritable, pLoadedMapping->fAutoMount,
                           pLoadedMapping->fSymlinksCreate, /* fMissing = */ true, /* fPlaceholder = */ true);
}

MAPPING *vbsfMappingGetByRoot(SHFLROOT root)
{
    if (root < RT_ELEMENTS(aIndexFromRoot))
    {
        SHFLROOT iMapping = aIndexFromRoot[root];

        if (   iMapping != SHFL_ROOT_NIL
            && iMapping < RT_ELEMENTS(FolderMapping))
        {
            return &FolderMapping[iMapping];
        }
    }

    return NULL;
}

static SHFLROOT vbsfMappingGetRootFromIndex(SHFLROOT iMapping)
{
    unsigned root;

    for (root = 0; root < RT_ELEMENTS(aIndexFromRoot); root++)
    {
        if (iMapping == aIndexFromRoot[root])
        {
            return root;
        }
    }

    return SHFL_ROOT_NIL;
}

static MAPPING *vbsfMappingGetByName (PRTUTF16 pwszName, SHFLROOT *pRoot)
{
    unsigned i;

    for (i=0; i<SHFL_MAX_MAPPINGS; i++)
    {
        if (FolderMapping[i].fValid == true)
        {
            if (!RTUtf16LocaleICmp(FolderMapping[i].pMapName->String.ucs2, pwszName))
            {
                SHFLROOT root = vbsfMappingGetRootFromIndex(i);

                if (root != SHFL_ROOT_NIL)
                {
                    if (pRoot)
                    {
                        *pRoot = root;
                    }
                    return &FolderMapping[i];
                }
                else
                {
                    AssertFailed();
                }
            }
        }
    }

    return NULL;
}

static void vbsfRootHandleAdd(SHFLROOT iMapping)
{
    unsigned root;

    for (root = 0; root < RT_ELEMENTS(aIndexFromRoot); root++)
    {
        if (aIndexFromRoot[root] == SHFL_ROOT_NIL)
        {
            aIndexFromRoot[root] = iMapping;
            return;
        }
    }

    AssertFailed();
}

static void vbsfRootHandleRemove(SHFLROOT iMapping)
{
    unsigned root;

    for (root = 0; root < RT_ELEMENTS(aIndexFromRoot); root++)
    {
        if (aIndexFromRoot[root] == iMapping)
        {
            aIndexFromRoot[root] = SHFL_ROOT_NIL;
            return;
        }
    }

    AssertFailed();
}



#ifdef UNITTEST
/** Unit test the SHFL_FN_ADD_MAPPING API.  Located here as a form of API
 * documentation. */
void testMappingsAdd(RTTEST hTest)
{
    /* If the number or types of parameters are wrong the API should fail. */
    testMappingsAddBadParameters(hTest);
    /* Add tests as required... */
}
#endif
/*
 * We are always executed from one specific HGCM thread. So thread safe.
 */
int vbsfMappingsAdd(const char *pszFolderName, PSHFLSTRING pMapName,
                    bool fWritable, bool fAutoMount, bool fSymlinksCreate, bool fMissing, bool fPlaceholder)
{
    unsigned i;

    Assert(pszFolderName && pMapName);

    Log(("vbsfMappingsAdd %ls\n", pMapName->String.ucs2));

    /* check for duplicates */
    for (i=0; i<SHFL_MAX_MAPPINGS; i++)
    {
        if (FolderMapping[i].fValid == true)
        {
            if (!RTUtf16LocaleICmp(FolderMapping[i].pMapName->String.ucs2, pMapName->String.ucs2))
            {
                AssertMsgFailed(("vbsfMappingsAdd: %ls mapping already exists!!\n", pMapName->String.ucs2));
                return VERR_ALREADY_EXISTS;
            }
        }
    }

    for (i=0; i<SHFL_MAX_MAPPINGS; i++)
    {
        if (FolderMapping[i].fValid == false)
        {
            /* Make sure the folder name is an absolute path, otherwise we're
               likely to get into trouble with buffer sizes in vbsfPathGuestToHost. */
            char szAbsFolderName[RTPATH_MAX];
            int rc = vbsfPathAbs(NULL, pszFolderName, szAbsFolderName, sizeof(szAbsFolderName));
            AssertRCReturn(rc, rc);

            FolderMapping[i].pszFolderName = RTStrDup(szAbsFolderName);
            if (!FolderMapping[i].pszFolderName)
            {
                return VERR_NO_MEMORY;
            }

            FolderMapping[i].pMapName = (PSHFLSTRING)RTMemAlloc(ShflStringSizeOfBuffer(pMapName));
            if (!FolderMapping[i].pMapName)
            {
                RTStrFree(FolderMapping[i].pszFolderName);
                AssertFailed();
                return VERR_NO_MEMORY;
            }

            FolderMapping[i].pMapName->u16Length = pMapName->u16Length;
            FolderMapping[i].pMapName->u16Size   = pMapName->u16Size;
            memcpy(FolderMapping[i].pMapName->String.ucs2, pMapName->String.ucs2, pMapName->u16Size);

            FolderMapping[i].fValid          = true;
            FolderMapping[i].cMappings       = 0;
            FolderMapping[i].fWritable       = fWritable;
            FolderMapping[i].fAutoMount      = fAutoMount;
            FolderMapping[i].fSymlinksCreate = fSymlinksCreate;
            FolderMapping[i].fMissing        = fMissing;
            FolderMapping[i].fPlaceholder    = fPlaceholder;

            /* Check if the host file system is case sensitive */
            RTFSPROPERTIES prop;
            prop.fCaseSensitive = false; /* Shut up MSC. */
            rc = RTFsQueryProperties(FolderMapping[i].pszFolderName, &prop);
            AssertRC(rc);
            FolderMapping[i].fHostCaseSensitive = RT_SUCCESS(rc) ? prop.fCaseSensitive : false;
            vbsfRootHandleAdd(i);
            break;
        }
    }
    if (i == SHFL_MAX_MAPPINGS)
    {
        AssertLogRelMsgFailed(("vbsfMappingsAdd: no more room to add mapping %s to %ls!!\n", pszFolderName, pMapName->String.ucs2));
        return VERR_TOO_MUCH_DATA;
    }

    Log(("vbsfMappingsAdd: added mapping %s to %ls\n", pszFolderName, pMapName->String.ucs2));
    return VINF_SUCCESS;
}

#ifdef UNITTEST
/** Unit test the SHFL_FN_REMOVE_MAPPING API.  Located here as a form of API
 * documentation. */
void testMappingsRemove(RTTEST hTest)
{
    /* If the number or types of parameters are wrong the API should fail. */
    testMappingsRemoveBadParameters(hTest);
    /* Add tests as required... */
}
#endif
int vbsfMappingsRemove(PSHFLSTRING pMapName)
{
    unsigned i;

    Assert(pMapName);

    Log(("vbsfMappingsRemove %ls\n", pMapName->String.ucs2));
    for (i=0; i<SHFL_MAX_MAPPINGS; i++)
    {
        if (FolderMapping[i].fValid == true)
        {
            if (!RTUtf16LocaleICmp(FolderMapping[i].pMapName->String.ucs2, pMapName->String.ucs2))
            {
                if (FolderMapping[i].cMappings != 0)
                {
                    LogRel2(("SharedFolders: removing '%ls' -> '%s', which is still used by the guest\n",
                             pMapName->String.ucs2, FolderMapping[i].pszFolderName));
                    FolderMapping[i].fMissing = true;
                    FolderMapping[i].fPlaceholder = true;
                    return VINF_PERMISSION_DENIED;
                }

                /* pMapName can be the same as FolderMapping[i].pMapName,
                 * log it before deallocating the memory.
                 */
                Log(("vbsfMappingsRemove: mapping %ls removed\n", pMapName->String.ucs2));

                RTStrFree(FolderMapping[i].pszFolderName);
                RTMemFree(FolderMapping[i].pMapName);
                FolderMapping[i].pszFolderName = NULL;
                FolderMapping[i].pMapName      = NULL;
                FolderMapping[i].fValid        = false;
                vbsfRootHandleRemove(i);
                return VINF_SUCCESS;
            }
        }
    }

    AssertMsgFailed(("vbsfMappingsRemove: mapping %ls not found!!!!\n", pMapName->String.ucs2));
    return VERR_FILE_NOT_FOUND;
}

const char* vbsfMappingsQueryHostRoot(SHFLROOT root)
{
    MAPPING *pFolderMapping = vbsfMappingGetByRoot(root);
    AssertReturn(pFolderMapping, NULL);
    if (pFolderMapping->fMissing)
        return NULL;
    return pFolderMapping->pszFolderName;
}

int vbsfMappingsQueryHostRootEx(SHFLROOT hRoot, const char **ppszRoot, uint32_t *pcbRootLen)
{
    MAPPING *pFolderMapping = vbsfMappingGetByRoot(hRoot);
    AssertReturn(pFolderMapping, VERR_INVALID_PARAMETER);
    if (pFolderMapping->fMissing)
        return VERR_NOT_FOUND;
    if (   pFolderMapping->pszFolderName == NULL
        || pFolderMapping->pszFolderName[0] == 0)
        return VERR_NOT_FOUND;
    *ppszRoot = pFolderMapping->pszFolderName;
    *pcbRootLen = (uint32_t)strlen(pFolderMapping->pszFolderName);
    return VINF_SUCCESS;
}

bool vbsfIsGuestMappingCaseSensitive(SHFLROOT root)
{
    MAPPING *pFolderMapping = vbsfMappingGetByRoot(root);
    AssertReturn(pFolderMapping, false);
    return pFolderMapping->fGuestCaseSensitive;
}

bool vbsfIsHostMappingCaseSensitive(SHFLROOT root)
{
    MAPPING *pFolderMapping = vbsfMappingGetByRoot(root);
    AssertReturn(pFolderMapping, false);
    return pFolderMapping->fHostCaseSensitive;
}

#ifdef UNITTEST
/** Unit test the SHFL_FN_QUERY_MAPPINGS API.  Located here as a form of API
 * documentation (or should it better be inline in include/VBox/shflsvc.h?) */
void testMappingsQuery(RTTEST hTest)
{
    /* The API should return all mappings if we provide enough buffers. */
    testMappingsQuerySimple(hTest);
    /* If we provide too few buffers that should be signalled correctly. */
    testMappingsQueryTooFewBuffers(hTest);
    /* The SHFL_MF_AUTOMOUNT flag means return only auto-mounted mappings. */
    testMappingsQueryAutoMount(hTest);
    /* The mappings return array must have numberOfMappings entries. */
    testMappingsQueryArrayWrongSize(hTest);
}
#endif
/**
 * Note: If pMappings / *pcMappings is smaller than the actual amount of mappings
 *       that *could* have been returned *pcMappings contains the required buffer size
 *       so that the caller can retry the operation if wanted.
 */
int vbsfMappingsQuery(PSHFLCLIENTDATA pClient, PSHFLMAPPING pMappings, uint32_t *pcMappings)
{
    int rc = VINF_SUCCESS;

    uint32_t cMappings = 0; /* Will contain actual valid mappings. */
    uint32_t idx = 0;       /* Current index in mappings buffer. */

    LogFlow(("vbsfMappingsQuery: pClient = %p, pMappings = %p, pcMappings = %p, *pcMappings = %d\n",
             pClient, pMappings, pcMappings, *pcMappings));

    for (uint32_t i = 0; i < SHFL_MAX_MAPPINGS; i++)
    {
        MAPPING *pFolderMapping = vbsfMappingGetByRoot(i);
        if (   pFolderMapping != NULL
            && pFolderMapping->fValid == true)
        {
            if (idx < *pcMappings)
            {
                /* Skip mappings which are not marked for auto-mounting if
                 * the SHFL_MF_AUTOMOUNT flag ist set. */
                if (   (pClient->fu32Flags & SHFL_MF_AUTOMOUNT)
                    && !pFolderMapping->fAutoMount)
                    continue;

                pMappings[idx].u32Status = SHFL_MS_NEW;
                pMappings[idx].root = i;
                idx++;
            }
            cMappings++;
        }
    }

    /* Return actual number of mappings, regardless whether the handed in
     * mapping buffer was big enough. */
    *pcMappings = cMappings;

    LogFlow(("vbsfMappingsQuery: return rc = %Rrc\n", rc));
    return rc;
}

#ifdef UNITTEST
/** Unit test the SHFL_FN_QUERY_MAP_NAME API.  Located here as a form of API
 * documentation. */
void testMappingsQueryName(RTTEST hTest)
{
    /* If we query an valid mapping it should be returned. */
    testMappingsQueryNameValid(hTest);
    /* If we query an invalid mapping that should be signalled. */
    testMappingsQueryNameInvalid(hTest);
    /* If we pass in a bad string buffer that should be detected. */
    testMappingsQueryNameBadBuffer(hTest);
}
#endif
int vbsfMappingsQueryName(PSHFLCLIENTDATA pClient, SHFLROOT root, SHFLSTRING *pString)
{
    int rc = VINF_SUCCESS;

    LogFlow(("vbsfMappingsQuery: pClient = %p, root = %d, *pString = %p\n",
             pClient, root, pString));

    MAPPING *pFolderMapping = vbsfMappingGetByRoot(root);
    if (pFolderMapping == NULL)
    {
        return VERR_INVALID_PARAMETER;
    }

    if (BIT_FLAG(pClient->fu32Flags, SHFL_CF_UTF8))
    {
        /* Not implemented. */
        AssertFailed();
        return VERR_INVALID_PARAMETER;
    }

    if (pFolderMapping->fValid == true)
    {
        if (pString->u16Size < pFolderMapping->pMapName->u16Size)
        {
            Log(("vbsfMappingsQuery: passed string too short (%d < %d bytes)!\n",
                pString->u16Size,  pFolderMapping->pMapName->u16Size));
            rc = VERR_INVALID_PARAMETER;
        }
        else
        {
            pString->u16Length = pFolderMapping->pMapName->u16Length;
            memcpy(pString->String.ucs2, pFolderMapping->pMapName->String.ucs2,
                   pFolderMapping->pMapName->u16Size);
        }
    }
    else
        rc = VERR_FILE_NOT_FOUND;

    LogFlow(("vbsfMappingsQuery:Name return rc = %Rrc\n", rc));

    return rc;
}

/** Queries fWritable flag for the given root. Returns error if the root is not accessible.
 */
int vbsfMappingsQueryWritable(PSHFLCLIENTDATA pClient, SHFLROOT root, bool *fWritable)
{
    RT_NOREF1(pClient);
    int rc = VINF_SUCCESS;

    LogFlow(("vbsfMappingsQueryWritable: pClient = %p, root = %d\n", pClient, root));

    MAPPING *pFolderMapping = vbsfMappingGetByRoot(root);
    AssertReturn(pFolderMapping, VERR_INVALID_PARAMETER);

    if (   pFolderMapping->fValid
        && !pFolderMapping->fMissing)
        *fWritable = pFolderMapping->fWritable;
    else
        rc = VERR_FILE_NOT_FOUND;

    LogFlow(("vbsfMappingsQuery:Writable return rc = %Rrc\n", rc));

    return rc;
}

int vbsfMappingsQueryAutoMount(PSHFLCLIENTDATA pClient, SHFLROOT root, bool *fAutoMount)
{
    RT_NOREF1(pClient);
    int rc = VINF_SUCCESS;

    LogFlow(("vbsfMappingsQueryAutoMount: pClient = %p, root = %d\n", pClient, root));

    MAPPING *pFolderMapping = vbsfMappingGetByRoot(root);
    AssertReturn(pFolderMapping, VERR_INVALID_PARAMETER);

    if (pFolderMapping->fValid == true)
        *fAutoMount = pFolderMapping->fAutoMount;
    else
        rc = VERR_FILE_NOT_FOUND;

    LogFlow(("vbsfMappingsQueryAutoMount:Writable return rc = %Rrc\n", rc));

    return rc;
}

int vbsfMappingsQuerySymlinksCreate(PSHFLCLIENTDATA pClient, SHFLROOT root, bool *fSymlinksCreate)
{
    RT_NOREF1(pClient);
    int rc = VINF_SUCCESS;

    LogFlow(("vbsfMappingsQueryAutoMount: pClient = %p, root = %d\n", pClient, root));

    MAPPING *pFolderMapping = vbsfMappingGetByRoot(root);
    AssertReturn(pFolderMapping, VERR_INVALID_PARAMETER);

    if (pFolderMapping->fValid == true)
        *fSymlinksCreate = pFolderMapping->fSymlinksCreate;
    else
        rc = VERR_FILE_NOT_FOUND;

    LogFlow(("vbsfMappingsQueryAutoMount:SymlinksCreate return rc = %Rrc\n", rc));

    return rc;
}

#ifdef UNITTEST
/** Unit test the SHFL_FN_MAP_FOLDER API.  Located here as a form of API
 * documentation. */
void testMapFolder(RTTEST hTest)
{
    /* If we try to map a valid name we should get the root. */
    testMapFolderValid(hTest);
    /* If we try to map a valid name we should get VERR_FILE_NOT_FOUND. */
    testMapFolderInvalid(hTest);
    /* If we map a folder twice we can unmap it twice.
     * Currently unmapping too often is only asserted but not signalled. */
    testMapFolderTwice(hTest);
    /* The delimiter should be converted in e.g. file delete operations. */
    testMapFolderDelimiter(hTest);
    /* Test case sensitive mapping by opening a file with the wrong case. */
    testMapFolderCaseSensitive(hTest);
    /* Test case insensitive mapping by opening a file with the wrong case. */
    testMapFolderCaseInsensitive(hTest);
    /* If the number or types of parameters are wrong the API should fail. */
    testMapFolderBadParameters(hTest);
}
#endif
int vbsfMapFolder(PSHFLCLIENTDATA pClient, PSHFLSTRING pszMapName,
                  RTUTF16 wcDelimiter, bool fCaseSensitive, SHFLROOT *pRoot)
{
    MAPPING *pFolderMapping = NULL;

    if (BIT_FLAG(pClient->fu32Flags, SHFL_CF_UTF8))
    {
        Log(("vbsfMapFolder %s\n", pszMapName->String.utf8));
    }
    else
    {
        Log(("vbsfMapFolder %ls\n", pszMapName->String.ucs2));
    }

    AssertMsgReturn(wcDelimiter == '/' || wcDelimiter == '\\',
                    ("Invalid path delimiter: %#x\n", wcDelimiter),
                    VERR_INVALID_PARAMETER);
    if (pClient->PathDelimiter == 0)
    {
        pClient->PathDelimiter = wcDelimiter;
    }
    else
    {
        AssertMsgReturn(wcDelimiter == pClient->PathDelimiter,
                        ("wcDelimiter=%#x PathDelimiter=%#x", wcDelimiter, pClient->PathDelimiter),
                        VERR_INVALID_PARAMETER);
    }

    if (BIT_FLAG(pClient->fu32Flags, SHFL_CF_UTF8))
    {
        int rc;
        PRTUTF16 utf16Name;

        rc = RTStrToUtf16 ((const char *) pszMapName->String.utf8, &utf16Name);
        if (RT_FAILURE (rc))
            return rc;

        pFolderMapping = vbsfMappingGetByName(utf16Name, pRoot);
        RTUtf16Free (utf16Name);
    }
    else
    {
        pFolderMapping = vbsfMappingGetByName(pszMapName->String.ucs2, pRoot);
    }

    if (!pFolderMapping)
    {
        return VERR_FILE_NOT_FOUND;
    }

    pFolderMapping->cMappings++;
    Assert(pFolderMapping->cMappings == 1 || pFolderMapping->fGuestCaseSensitive == fCaseSensitive);
    pFolderMapping->fGuestCaseSensitive = fCaseSensitive;
    return VINF_SUCCESS;
}

#ifdef UNITTEST
/** Unit test the SHFL_FN_UNMAP_FOLDER API.  Located here as a form of API
 * documentation. */
void testUnmapFolder(RTTEST hTest)
{
    /* Unmapping a mapped folder should succeed.
     * If the folder is not mapped this is only asserted, not signalled. */
    testUnmapFolderValid(hTest);
    /* Unmapping a non-existant root should fail. */
    testUnmapFolderInvalid(hTest);
    /* If the number or types of parameters are wrong the API should fail. */
    testUnmapFolderBadParameters(hTest);
}
#endif
int vbsfUnmapFolder(PSHFLCLIENTDATA pClient, SHFLROOT root)
{
    RT_NOREF1(pClient);
    int rc = VINF_SUCCESS;

    MAPPING *pFolderMapping = vbsfMappingGetByRoot(root);
    if (pFolderMapping == NULL)
    {
        AssertFailed();
        return VERR_FILE_NOT_FOUND;
    }

    Assert(pFolderMapping->fValid == true && pFolderMapping->cMappings > 0);
    if (pFolderMapping->cMappings > 0)
        pFolderMapping->cMappings--;

    if (   pFolderMapping->cMappings == 0
        && pFolderMapping->fPlaceholder)
    {
        /* Automatically remove, it is not used by the guest anymore. */
        Assert(pFolderMapping->fMissing);
        LogRel2(("SharedFolders: unmapping placeholder '%ls' -> '%s'\n",
                pFolderMapping->pMapName->String.ucs2, pFolderMapping->pszFolderName));
        vbsfMappingsRemove(pFolderMapping->pMapName);
    }

    Log(("vbsfUnmapFolder\n"));
    return rc;
}
