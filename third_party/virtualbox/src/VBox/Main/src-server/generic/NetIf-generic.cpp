/* $Id: NetIf-generic.cpp $ */
/** @file
 * VirtualBox Main - Generic NetIf implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/process.h>
#include <iprt/env.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>
#include <unistd.h>

#if defined(RT_OS_SOLARIS)
# include <sys/sockio.h>
#endif

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN)
# include <cstdio>
#endif

#include "HostNetworkInterfaceImpl.h"
#include "ProgressImpl.h"
#include "VirtualBoxImpl.h"
#include "netif.h"

#define VBOXNETADPCTL_NAME "VBoxNetAdpCtl"

static int NetIfAdpCtl(const char * pcszIfName, const char *pszAddr, const char *pszOption, const char *pszMask)
{
    const char *args[] = { NULL, pcszIfName, pszAddr, pszOption, pszMask, NULL };

    char szAdpCtl[RTPATH_MAX];
    int rc = RTPathExecDir(szAdpCtl, sizeof(szAdpCtl) - sizeof("/" VBOXNETADPCTL_NAME));
    if (RT_FAILURE(rc))
    {
        LogRel(("NetIfAdpCtl: failed to get program path, rc=%Rrc.\n", rc));
        return rc;
    }
    strcat(szAdpCtl, "/" VBOXNETADPCTL_NAME);
    args[0] = szAdpCtl;
    if (!RTPathExists(szAdpCtl))
    {
        LogRel(("NetIfAdpCtl: path %s does not exist. Failed to run " VBOXNETADPCTL_NAME " helper.\n",
                szAdpCtl));
        return VERR_FILE_NOT_FOUND;
    }

    RTPROCESS pid;
    rc = RTProcCreate(szAdpCtl, args, RTENV_DEFAULT, 0, &pid);
    if (RT_SUCCESS(rc))
    {
        RTPROCSTATUS Status;
        rc = RTProcWait(pid, 0, &Status);
        if (   RT_SUCCESS(rc)
            && Status.iStatus == 0
            && Status.enmReason == RTPROCEXITREASON_NORMAL)
            return VINF_SUCCESS;
    }
    else
        LogRel(("NetIfAdpCtl: failed to create process for %s: %Rrc\n", szAdpCtl, rc));
    return rc;
}

static int NetIfAdpCtl(HostNetworkInterface * pIf, const char *pszAddr, const char *pszOption, const char *pszMask)
{
    Bstr interfaceName;
    pIf->COMGETTER(Name)(interfaceName.asOutParam());
    Utf8Str strName(interfaceName);
    return NetIfAdpCtl(strName.c_str(), pszAddr, pszOption, pszMask);
}

int NetIfAdpCtlOut(const char * pcszName, const char * pcszCmd, char *pszBuffer, size_t cBufSize)
{
    char szAdpCtl[RTPATH_MAX];
    int rc = RTPathExecDir(szAdpCtl, sizeof(szAdpCtl) - sizeof("/" VBOXNETADPCTL_NAME " ") - strlen(pcszCmd));
    if (RT_FAILURE(rc))
    {
        LogRel(("NetIfAdpCtlOut: Failed to get program path, rc=%Rrc\n", rc));
        return VERR_INVALID_PARAMETER;
    }
    strcat(szAdpCtl, "/" VBOXNETADPCTL_NAME " ");
    if (pcszName && strlen(pcszName) <= RTPATH_MAX - strlen(szAdpCtl) - 1 - strlen(pcszCmd))
    {
        strcat(szAdpCtl, pcszName);
        strcat(szAdpCtl, " ");
        strcat(szAdpCtl, pcszCmd);
    }
    else
    {
        LogRel(("NetIfAdpCtlOut: Command line is too long: %s%s %s\n", szAdpCtl, pcszName, pcszCmd));
        return VERR_INVALID_PARAMETER;
    }
    if (strlen(szAdpCtl) < RTPATH_MAX - sizeof(" 2>&1"))
        strcat(szAdpCtl, " 2>&1");
    FILE *fp = popen(szAdpCtl, "r");
    if (fp)
    {
        if (fgets(pszBuffer, (int)cBufSize, fp))
        {
            if (!strncmp(VBOXNETADPCTL_NAME ":", pszBuffer, sizeof(VBOXNETADPCTL_NAME)))
            {
                LogRel(("NetIfAdpCtlOut: %s", pszBuffer));
                rc = VERR_INTERNAL_ERROR;
            }
        }
        else
        {
            LogRel(("NetIfAdpCtlOut: No output from " VBOXNETADPCTL_NAME));
            rc = VERR_INTERNAL_ERROR;
        }
        pclose(fp);
    }
    return rc;
}

int NetIfEnableStaticIpConfig(VirtualBox * /* vBox */, HostNetworkInterface * pIf, ULONG aOldIp, ULONG aNewIp, ULONG aMask)
{
    const char *pszOption, *pszMask;
    char szAddress[16]; /* 4*3 + 3*1 + 1 */
    char szNetMask[16]; /* 4*3 + 3*1 + 1 */
    uint8_t *pu8Addr = (uint8_t *)&aNewIp;
    uint8_t *pu8Mask = (uint8_t *)&aMask;
    if (aNewIp == 0)
    {
        pu8Addr = (uint8_t *)&aOldIp;
        pszOption = "remove";
        pszMask   = NULL;
    }
    else
    {
        pszOption = "netmask";
        pszMask  = szNetMask;
        RTStrPrintf(szNetMask, sizeof(szNetMask), "%d.%d.%d.%d",
                    pu8Mask[0], pu8Mask[1], pu8Mask[2], pu8Mask[3]);
    }
    RTStrPrintf(szAddress, sizeof(szAddress), "%d.%d.%d.%d",
                pu8Addr[0], pu8Addr[1], pu8Addr[2], pu8Addr[3]);
    return NetIfAdpCtl(pIf, szAddress, pszOption, pszMask);
}

int NetIfEnableStaticIpConfigV6(VirtualBox * /* vBox */, HostNetworkInterface * pIf, IN_BSTR aOldIPV6Address,
                                IN_BSTR aIPV6Address, ULONG aIPV6MaskPrefixLength)
{
    char szAddress[5*8 + 1 + 5 + 1];
    if (Bstr(aIPV6Address).length())
    {
        RTStrPrintf(szAddress, sizeof(szAddress), "%ls/%d",
                    aIPV6Address, aIPV6MaskPrefixLength);
        return NetIfAdpCtl(pIf, szAddress, NULL, NULL);
    }
    else
    {
        RTStrPrintf(szAddress, sizeof(szAddress), "%ls",
                    aOldIPV6Address);
        return NetIfAdpCtl(pIf, szAddress, "remove", NULL);
    }
}

int NetIfEnableDynamicIpConfig(VirtualBox * /* vBox */, HostNetworkInterface * /* pIf */)
{
    return VERR_NOT_IMPLEMENTED;
}


int NetIfCreateHostOnlyNetworkInterface(VirtualBox *pVirtualBox,
                                        IHostNetworkInterface **aHostNetworkInterface,
                                        IProgress **aProgress,
                                        const char *pcszName)
{
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD)
    /* create a progress object */
    ComObjPtr<Progress> progress;
    progress.createObject();

    ComPtr<IHost> host;
    HRESULT hrc = pVirtualBox->COMGETTER(Host)(host.asOutParam());
    if (SUCCEEDED(hrc))
    {
        hrc = progress->init(pVirtualBox, host,
                             Bstr("Creating host only network interface").raw(),
                             FALSE /* aCancelable */);
        if (SUCCEEDED(hrc))
        {
            progress.queryInterfaceTo(aProgress);

            char szAdpCtl[RTPATH_MAX];
            int rc = RTPathExecDir(szAdpCtl, sizeof(szAdpCtl) - sizeof("/" VBOXNETADPCTL_NAME " add"));
            if (RT_FAILURE(rc))
            {
                progress->i_notifyComplete(E_FAIL,
                                           COM_IIDOF(IHostNetworkInterface),
                                           HostNetworkInterface::getStaticComponentName(),
                                           "Failed to get program path, rc=%Rrc\n", rc);
                return rc;
            }
            strcat(szAdpCtl, "/" VBOXNETADPCTL_NAME " ");
            if (pcszName && strlen(pcszName) <= RTPATH_MAX - strlen(szAdpCtl) - sizeof(" add"))
            {
                strcat(szAdpCtl, pcszName);
                strcat(szAdpCtl, " add");
            }
            else
                strcat(szAdpCtl, "add");
            if (strlen(szAdpCtl) < RTPATH_MAX - sizeof(" 2>&1"))
                strcat(szAdpCtl, " 2>&1");
            FILE *fp = popen(szAdpCtl, "r");

            if (fp)
            {
                char szBuf[128]; /* We are not interested in long error messages. */
                if (fgets(szBuf, sizeof(szBuf), fp))
                {
                    /* Remove trailing new line characters. */
                    char *pLast = szBuf + strlen(szBuf) - 1;
                    if (pLast >= szBuf && *pLast == '\n')
                        *pLast = 0;

                    if (!strncmp(VBOXNETADPCTL_NAME ":", szBuf, sizeof(VBOXNETADPCTL_NAME)))
                    {
                        progress->i_notifyComplete(E_FAIL,
                                                   COM_IIDOF(IHostNetworkInterface),
                                                   HostNetworkInterface::getStaticComponentName(),
                                                   "%s", szBuf);
                        pclose(fp);
                        return E_FAIL;
                    }

                    size_t cbNameLen = strlen(szBuf) + 1;
                    PNETIFINFO pInfo = (PNETIFINFO)RTMemAllocZ(RT_OFFSETOF(NETIFINFO, szName[cbNameLen]));
                    if (!pInfo)
                        rc = VERR_NO_MEMORY;
                    else
                    {
                        strcpy(pInfo->szShortName, szBuf);
                        strcpy(pInfo->szName, szBuf);
                        rc = NetIfGetConfigByName(pInfo);
                        if (RT_FAILURE(rc))
                        {
                            progress->i_notifyComplete(E_FAIL,
                                                       COM_IIDOF(IHostNetworkInterface),
                                                       HostNetworkInterface::getStaticComponentName(),
                                                       "Failed to get config info for %s (as reported by '" VBOXNETADPCTL_NAME " add')\n", szBuf);
                        }
                        else
                        {
                            Bstr IfName(szBuf);
                            /* create a new uninitialized host interface object */
                            ComObjPtr<HostNetworkInterface> iface;
                            iface.createObject();
                            iface->init(IfName, HostNetworkInterfaceType_HostOnly, pInfo);
                            iface->i_setVirtualBox(pVirtualBox);
                            iface.queryInterfaceTo(aHostNetworkInterface);
                        }
                        RTMemFree(pInfo);
                    }
                    if ((rc = pclose(fp)) != 0)
                    {
                        progress->i_notifyComplete(E_FAIL,
                                                   COM_IIDOF(IHostNetworkInterface),
                                                   HostNetworkInterface::getStaticComponentName(),
                                                   "Failed to execute '" VBOXNETADPCTL_NAME " add' (exit status: %d)", rc);
                        rc = VERR_INTERNAL_ERROR;
                    }
                }
                else
                {
                    /* Failed to add an interface */
                    rc = VERR_PERMISSION_DENIED;
                    progress->i_notifyComplete(E_FAIL,
                                               COM_IIDOF(IHostNetworkInterface),
                                               HostNetworkInterface::getStaticComponentName(),
                                               "Failed to execute '" VBOXNETADPCTL_NAME " add' (exit status: %d). Check permissions!", rc);
                    pclose(fp);
                }
            }
            if (RT_SUCCESS(rc))
                progress->i_notifyComplete(rc);
            else
                hrc = E_FAIL;
        }
    }

    return hrc;

#else
    NOREF(pVirtualBox);
    NOREF(aHostNetworkInterface);
    NOREF(aProgress);
    NOREF(pcszName);
    return VERR_NOT_IMPLEMENTED;
#endif
}

int NetIfRemoveHostOnlyNetworkInterface(VirtualBox *pVirtualBox, IN_GUID aId,
                                        IProgress **aProgress)
{
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD)
    /* create a progress object */
    ComObjPtr<Progress> progress;
    progress.createObject();
    ComPtr<IHost> host;
    int rc = VINF_SUCCESS;
    HRESULT hr = pVirtualBox->COMGETTER(Host)(host.asOutParam());
    if (SUCCEEDED(hr))
    {
        Bstr ifname;
        ComPtr<IHostNetworkInterface> iface;
        if (FAILED(host->FindHostNetworkInterfaceById(Guid(aId).toUtf16().raw(), iface.asOutParam())))
            return VERR_INVALID_PARAMETER;
        iface->COMGETTER(Name)(ifname.asOutParam());
        if (ifname.isEmpty())
            return VERR_INTERNAL_ERROR;

        rc = progress->init(pVirtualBox, host,
                            Bstr("Removing host network interface").raw(),
                            FALSE /* aCancelable */);
        if (SUCCEEDED(rc))
        {
            progress.queryInterfaceTo(aProgress);
            rc = NetIfAdpCtl(Utf8Str(ifname).c_str(), "remove", NULL, NULL);
            if (RT_FAILURE(rc))
                progress->i_notifyComplete(E_FAIL,
                                           COM_IIDOF(IHostNetworkInterface),
                                           HostNetworkInterface::getStaticComponentName(),
                                           "Failed to execute '" VBOXNETADPCTL_NAME "' (exit status: %d)", rc);
            else
                progress->i_notifyComplete(S_OK);
        }
    }
    else
    {
        progress->i_notifyComplete(hr);
        rc = VERR_INTERNAL_ERROR;
    }
    return rc;
#else
    NOREF(pVirtualBox);
    NOREF(aId);
    NOREF(aProgress);
    return VERR_NOT_IMPLEMENTED;
#endif
}

int NetIfGetConfig(HostNetworkInterface * /* pIf */, NETIFINFO *)
{
    return VERR_NOT_IMPLEMENTED;
}

int NetIfDhcpRediscover(VirtualBox * /* pVBox */, HostNetworkInterface * /* pIf */)
{
    return VERR_NOT_IMPLEMENTED;
}

/**
 * Obtain the current state of the interface.
 *
 * @returns VBox status code.
 *
 * @param   pcszIfName  Interface name.
 * @param   penmState   Where to store the retrieved state.
 */
int NetIfGetState(const char *pcszIfName, NETIFSTATUS *penmState)
{
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
        return VERR_OUT_OF_RESOURCES;
    struct ifreq Req;
    RT_ZERO(Req);
    RTStrCopy(Req.ifr_name, sizeof(Req.ifr_name), pcszIfName);
    if (ioctl(sock, SIOCGIFFLAGS, &Req) < 0)
    {
        Log(("NetIfGetState: ioctl(SIOCGIFFLAGS) -> %d\n", errno));
        *penmState = NETIF_S_UNKNOWN;
    }
    else
        *penmState = (Req.ifr_flags & IFF_UP) ? NETIF_S_UP : NETIF_S_DOWN;
    close(sock);
    return VINF_SUCCESS;
}
