/* $Id: VBoxSharedFolders.cpp $ */
/** @file
 * VBoxSharedFolders - Handling for shared folders
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VBoxSharedFolders.h"
#include "VBoxTray.h"
#include "VBoxHelpers.h"

#include <iprt/mem.h>
#include <VBox/VBoxGuestLib.h>

#ifdef DEBUG
# define LOG_ENABLED
# define LOG_GROUP LOG_GROUP_DEFAULT
#endif
#include <VBox/log.h>



int VBoxSharedFoldersAutoMount(void)
{
    uint32_t u32ClientId;
    int rc = VbglR3SharedFolderConnect(&u32ClientId);
    if (RT_SUCCESS(rc))
    {
        uint32_t cMappings;
        VBGLR3SHAREDFOLDERMAPPING *paMappings;

        rc = VbglR3SharedFolderGetMappings(u32ClientId, true /* Only process auto-mounted folders */,
                                           &paMappings, &cMappings);
        if (RT_SUCCESS(rc))
        {
#if 0
            /* Check for a fixed/virtual auto-mount share. */
            if (VbglR3SharedFolderExists(u32ClientId, "vbsfAutoMount"))
            {
                LogFlowFunc(("Hosts supports auto-mount root\n"));
            }
            else
            {
#endif
                LogFlowFunc(("Got %u shared folder mappings\n", cMappings));
                for (uint32_t i = 0; i < cMappings && RT_SUCCESS(rc); i++)
                {
                    char *pszName = NULL;
                    rc = VbglR3SharedFolderGetName(u32ClientId, paMappings[i].u32Root, &pszName);
                    if (   RT_SUCCESS(rc)
                        && *pszName)
                    {
                        LogFlowFunc(("Connecting share %u (%s) ...\n", i+1, pszName));

                        char *pszShareName;
                        if (RTStrAPrintf(&pszShareName, "\\\\vboxsrv\\%s", pszName) >= 0)
                        {
                            char chDrive = 'D'; /* Start probing whether drive D: is free to use. */
                            do
                            {
                                char szCurDrive[3];
                                RTStrPrintf(szCurDrive, sizeof(szCurDrive), "%c:", chDrive++);

                                NETRESOURCE resource;
                                RT_ZERO(resource);
                                resource.dwType = RESOURCETYPE_ANY;
                                resource.lpLocalName = TEXT(szCurDrive);
                                resource.lpRemoteName = TEXT(pszShareName);
                                /* Go straight to our network provider in order to get maximum lookup speed. */
                                resource.lpProvider = TEXT("VirtualBox Shared Folders");

                                /** @todo Figure out how to map the drives in a block (F,G,H, ...).
                                          Save the mapping for later use. */
                                DWORD dwErr = WNetAddConnection2A(&resource, NULL, NULL, 0);
                                if (dwErr == NO_ERROR)
                                {
                                    LogRel(("Shared folder \"%s\" was mounted to drive \"%s\"\n", pszName, szCurDrive));
                                    break;
                                }
                                else
                                {
                                    LogRel(("Mounting \"%s\" to \"%s\" resulted in dwErr = %ld\n", pszName, szCurDrive, dwErr));

                                    switch (dwErr)
                                    {
                                        /*
                                         * The local device specified by the lpLocalName member is already
                                         * connected to a network resource.  Try next drive ...
                                         */
                                        case ERROR_ALREADY_ASSIGNED:
                                            break;

                                        default:
                                            LogRel(("Error while mounting shared folder \"%s\" to \"%s\", error = %ld\n",
                                                    pszName, szCurDrive, dwErr));
                                            break;
                                    }
                                }
                            } while (chDrive <= 'Z');

                            if (chDrive > 'Z')
                            {
                                LogRel(("No free driver letter found to assign shared folder \"%s\", aborting\n", pszName));
                                break;
                            }

                            RTStrFree(pszShareName);
                        }
                        else
                            rc = VERR_NO_STR_MEMORY;
                        RTStrFree(pszName);
                    }
                    else
                        LogFlowFunc(("Error while getting the shared folder name for root node = %u, rc = %Rrc\n",
                             paMappings[i].u32Root, rc));
                }
#if 0
            }
#endif
            VbglR3SharedFolderFreeMappings(paMappings);
        }
        else
            LogFlowFunc(("Error while getting the shared folder mappings, rc = %Rrc\n", rc));
        VbglR3SharedFolderDisconnect(u32ClientId);
    }
    else
    {
        LogFlowFunc(("Failed to connect to the shared folder service, error %Rrc\n", rc));
        /* return success, otherwise VBoxTray will not start! */
        rc = VINF_SUCCESS;
    }
    return rc;
}

int VBoxSharedFoldersAutoUnmount(void)
{
    uint32_t u32ClientId;
    int rc = VbglR3SharedFolderConnect(&u32ClientId);
    if (!RT_SUCCESS(rc))
        LogFlowFunc(("Failed to connect to the shared folder service, error %Rrc\n", rc));
    else
    {
        uint32_t cMappings;
        VBGLR3SHAREDFOLDERMAPPING *paMappings;

        rc = VbglR3SharedFolderGetMappings(u32ClientId, true /* Only process auto-mounted folders */,
                                           &paMappings, &cMappings);
        if (RT_SUCCESS(rc))
        {
            for (uint32_t i = 0; i < cMappings && RT_SUCCESS(rc); i++)
            {
                char *pszName = NULL;
                rc = VbglR3SharedFolderGetName(u32ClientId, paMappings[i].u32Root, &pszName);
                if (   RT_SUCCESS(rc)
                    && *pszName)
                {
                    LogFlowFunc(("Disconnecting share %u (%s) ...\n", i+1, pszName));

                    char *pszShareName;
                    if (RTStrAPrintf(&pszShareName, "\\\\vboxsrv\\%s", pszName) >= 0)
                    {
                        DWORD dwErr = WNetCancelConnection2(pszShareName, 0, FALSE /* Force disconnect */);
                        if (dwErr == NO_ERROR)
                        {
                            LogRel(("Share \"%s\" was disconnected\n", pszShareName));
                            RTStrFree(pszShareName);
                            RTStrFree(pszName);
                            break;
                        }

                        LogRel(("Disconnecting \"%s\" failed, dwErr = %ld\n", pszShareName, dwErr));

                        switch (dwErr)
                        {
                            case ERROR_NOT_CONNECTED:
                                break;

                            default:
                                LogRel(("Error while disconnecting shared folder \"%s\", error = %ld\n",
                                        pszShareName, dwErr));
                                break;
                        }

                        RTStrFree(pszShareName);
                    }
                    else
                        rc = VERR_NO_MEMORY;
                    RTStrFree(pszName);
                }
                else
                    LogFlowFunc(("Error while getting the shared folder name for root node = %u, rc = %Rrc\n",
                         paMappings[i].u32Root, rc));
            }
            VbglR3SharedFolderFreeMappings(paMappings);
        }
        else
            LogFlowFunc(("Error while getting the shared folder mappings, rc = %Rrc\n", rc));
        VbglR3SharedFolderDisconnect(u32ClientId);
    }
    return rc;
}

