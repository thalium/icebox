/* $Id: VBoxUsbLib-win.cpp $ */
/** @file
 * VBox USB ring-3 Driver Interface library, Windows.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DRV_USBPROXY
#include <iprt/win/windows.h>

#include <VBox/sup.h>
#include <VBox/types.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <iprt/path.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <VBox/log.h>
#include <VBox/usblib.h>
#include <VBox/usblib-win.h>
#include <VBox/usb.h>
#include <VBox/VBoxDrvCfg-win.h>
#include <stdio.h>
#pragma warning (disable:4200) /* shuts up the empty array member warnings */
#include <iprt/win/setupapi.h>
#include <usbdi.h>
#include <hidsdi.h>

#define VBOX_USB_USE_DEVICE_NOTIFICATION

#ifdef VBOX_USB_USE_DEVICE_NOTIFICATION
# include <Dbt.h>
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct _USB_INTERFACE_DESCRIPTOR2
{
    UCHAR  bLength;
    UCHAR  bDescriptorType;
    UCHAR  bInterfaceNumber;
    UCHAR  bAlternateSetting;
    UCHAR  bNumEndpoints;
    UCHAR  bInterfaceClass;
    UCHAR  bInterfaceSubClass;
    UCHAR  bInterfaceProtocol;
    UCHAR  iInterface;
    USHORT wNumClasses;
} USB_INTERFACE_DESCRIPTOR2, *PUSB_INTERFACE_DESCRIPTOR2;

typedef struct VBOXUSBGLOBALSTATE
{
    HANDLE hMonitor;
    HANDLE hNotifyEvent;
    HANDLE hInterruptEvent;
#ifdef VBOX_USB_USE_DEVICE_NOTIFICATION
    HANDLE hThread;
    HWND   hWnd;
    HANDLE hTimerQueue;
    HANDLE hTimer;
#endif
} VBOXUSBGLOBALSTATE, *PVBOXUSBGLOBALSTATE;

typedef struct VBOXUSB_STRING_DR_ENTRY
{
    struct VBOXUSB_STRING_DR_ENTRY *pNext;
    UCHAR iDr;
    USHORT idLang;
    USB_STRING_DESCRIPTOR StrDr;
} VBOXUSB_STRING_DR_ENTRY, *PVBOXUSB_STRING_DR_ENTRY;

/**
 * This represents VBoxUsb device instance
 */
typedef struct VBOXUSB_DEV
{
    struct VBOXUSB_DEV *pNext;
    char                szName[512];
    char                szDriverRegName[512];
} VBOXUSB_DEV, *PVBOXUSB_DEV;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static VBOXUSBGLOBALSTATE g_VBoxUsbGlobal;


int usbLibVuDeviceValidate(PVBOXUSB_DEV pVuDev)
{
    HANDLE hOut = INVALID_HANDLE_VALUE;

    hOut = CreateFile(pVuDev->szName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);

    if (hOut == INVALID_HANDLE_VALUE)
    {
        DWORD dwErr = GetLastError(); NOREF(dwErr);
        AssertMsgFailed(("CreateFile FAILED to open %s, dwErr (%d)\n", pVuDev->szName, dwErr));
        return VERR_GENERAL_FAILURE;
    }

    USBSUP_VERSION version = {0};
    DWORD cbReturned = 0;
    int rc = VERR_VERSION_MISMATCH;

    do
    {
        if (!DeviceIoControl(hOut, SUPUSB_IOCTL_GET_VERSION, NULL, 0,&version, sizeof(version),  &cbReturned, NULL))
        {
            AssertMsgFailed(("DeviceIoControl SUPUSB_IOCTL_GET_VERSION failed with LastError=%Rwa\n", GetLastError()));
            break;
        }

        if (   version.u32Major != USBDRV_MAJOR_VERSION
#if USBDRV_MINOR_VERSION != 0
            || version.u32Minor <  USBDRV_MINOR_VERSION
#endif
           )
        {
            AssertMsgFailed(("Invalid version %d:%d vs %d:%d\n", version.u32Major, version.u32Minor, USBDRV_MAJOR_VERSION, USBDRV_MINOR_VERSION));
            break;
        }

        if (!DeviceIoControl(hOut, SUPUSB_IOCTL_IS_OPERATIONAL, NULL, 0, NULL, NULL, &cbReturned, NULL))
        {
            AssertMsgFailed(("DeviceIoControl SUPUSB_IOCTL_IS_OPERATIONAL failed with LastError=%Rwa\n", GetLastError()));
            break;
        }

        rc = VINF_SUCCESS;
    } while (0);

    CloseHandle(hOut);
    return rc;
}

static int usbLibVuDevicePopulate(PVBOXUSB_DEV pVuDev, HDEVINFO hDevInfo, PSP_DEVICE_INTERFACE_DATA pIfData)
{
    DWORD cbIfDetailData;
    int rc = VINF_SUCCESS;

    SetupDiGetDeviceInterfaceDetail(hDevInfo, pIfData,
                NULL, /* OUT PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData */
                0, /* IN DWORD DeviceInterfaceDetailDataSize */
                &cbIfDetailData,
                NULL
                );
    Assert(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    PSP_DEVICE_INTERFACE_DETAIL_DATA pIfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)RTMemAllocZ(cbIfDetailData);
    if (!pIfDetailData)
    {
        AssertMsgFailed(("RTMemAllocZ failed\n"));
        return VERR_OUT_OF_RESOURCES;
    }

    DWORD cbDbgRequired;
    SP_DEVINFO_DATA DevInfoData;
    DevInfoData.cbSize = sizeof (DevInfoData);
    /* the cbSize should contain the sizeof a fixed-size part according to the docs */
    pIfDetailData->cbSize = sizeof (*pIfDetailData);
    do
    {
        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, pIfData,
                                pIfDetailData,
                                cbIfDetailData,
                                &cbDbgRequired,
                                &DevInfoData))
        {
            DWORD dwErr = GetLastError(); NOREF(dwErr);
            AssertMsgFailed(("SetupDiGetDeviceInterfaceDetail, cbRequired (%d), was (%d), dwErr (%d)\n", cbDbgRequired, cbIfDetailData, dwErr));
            rc = VERR_GENERAL_FAILURE;
            break;
        }

        strncpy(pVuDev->szName, pIfDetailData->DevicePath, sizeof (pVuDev->szName));

        if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DevInfoData, SPDRP_DRIVER,
            NULL, /* OUT PDWORD PropertyRegDataType */
            (PBYTE)pVuDev->szDriverRegName,
            sizeof (pVuDev->szDriverRegName),
            &cbDbgRequired))
        {
            DWORD dwErr = GetLastError(); NOREF(dwErr);
            AssertMsgFailed(("SetupDiGetDeviceRegistryPropertyA, cbRequired (%d), was (%d), dwErr (%d)\n", cbDbgRequired, sizeof (pVuDev->szDriverRegName), dwErr));
            rc = VERR_GENERAL_FAILURE;
            break;
        }

        rc = usbLibVuDeviceValidate(pVuDev);
        AssertRC(rc);
    } while (0);

    RTMemFree(pIfDetailData);
    return rc;
}

static void usbLibVuFreeDevices(PVBOXUSB_DEV pDevInfos)
{
    while (pDevInfos)
    {
        PVBOXUSB_DEV pNext = pDevInfos->pNext;
        RTMemFree(pDevInfos);
        pDevInfos = pNext;
    }
}

static int usbLibVuGetDevices(PVBOXUSB_DEV *ppVuDevs, uint32_t *pcVuDevs)
{
    *ppVuDevs = NULL;
    *pcVuDevs = 0;

    HDEVINFO hDevInfo =  SetupDiGetClassDevs(&GUID_CLASS_VBOXUSB,
            NULL, /* IN PCTSTR Enumerator */
            NULL, /* IN HWND hwndParent */
            (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE) /* IN DWORD Flags */
            );
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        DWORD dwErr = GetLastError(); NOREF(dwErr);
        AssertMsgFailed(("SetupDiGetClassDevs, dwErr (%d)\n", dwErr));
        return VERR_GENERAL_FAILURE;
    }

    for (int i = 0; ; ++i)
    {
        SP_DEVICE_INTERFACE_DATA IfData;
        IfData.cbSize = sizeof (IfData);
        if (!SetupDiEnumDeviceInterfaces(hDevInfo,
                            NULL, /* IN PSP_DEVINFO_DATA DeviceInfoData */
                            &GUID_CLASS_VBOXUSB, /* IN LPGUID InterfaceClassGuid */
                            i,
                            &IfData))
        {
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_NO_MORE_ITEMS)
                break;

            AssertMsgFailed(("SetupDiEnumDeviceInterfaces, dwErr (%d), resuming\n", dwErr));
            continue;
        }

        /* we've now got the IfData */
        PVBOXUSB_DEV pVuDev = (PVBOXUSB_DEV)RTMemAllocZ(sizeof (*pVuDev));
        if (!pVuDev)
        {
            AssertMsgFailed(("RTMemAllocZ failed, resuming\n"));
            continue;
        }

        int rc = usbLibVuDevicePopulate(pVuDev, hDevInfo, &IfData);
        if (!RT_SUCCESS(rc))
        {
            AssertMsgFailed(("usbLibVuDevicePopulate failed, rc (%d), resuming\n", rc));
            continue;
        }

        pVuDev->pNext = *ppVuDevs;
        *ppVuDevs = pVuDev;
        ++*pcVuDevs;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return VINF_SUCCESS;
}

static int usbLibDevPopulate(PUSBDEVICE pDev, PUSB_NODE_CONNECTION_INFORMATION_EX pConInfo, ULONG iPort, LPCSTR lpszDrvKeyName, LPCSTR lpszHubName, PVBOXUSB_STRING_DR_ENTRY pDrList)
{
    pDev->bcdUSB = pConInfo->DeviceDescriptor.bcdUSB;
    pDev->bDeviceClass = pConInfo->DeviceDescriptor.bDeviceClass;
    pDev->bDeviceSubClass = pConInfo->DeviceDescriptor.bDeviceSubClass;
    pDev->bDeviceProtocol = pConInfo->DeviceDescriptor.bDeviceProtocol;
    pDev->idVendor = pConInfo->DeviceDescriptor.idVendor;
    pDev->idProduct = pConInfo->DeviceDescriptor.idProduct;
    pDev->bcdDevice = pConInfo->DeviceDescriptor.bcdDevice;
    pDev->bBus = 0; /** @todo figure out bBus on windows... */
    pDev->bPort = iPort;
    /** @todo check which devices are used for primary input (keyboard & mouse) */
    if (!lpszDrvKeyName || *lpszDrvKeyName == 0)
        pDev->enmState = USBDEVICESTATE_UNUSED;
    else
        pDev->enmState = USBDEVICESTATE_USED_BY_HOST_CAPTURABLE;
    pDev->enmSpeed = USBDEVICESPEED_UNKNOWN;
    int rc = RTStrAPrintf((char **)&pDev->pszAddress, "%s", lpszDrvKeyName);
    if (rc < 0)
        return VERR_NO_MEMORY;
    pDev->pszBackend = RTStrDup("host");
    if (!pDev->pszBackend)
    {
        RTStrFree((char *)pDev->pszAddress);
        return VERR_NO_STR_MEMORY;
    }
    pDev->pszHubName = RTStrDup(lpszHubName);
    pDev->bNumConfigurations = 0;
    pDev->u64SerialHash = 0;

    for (; pDrList; pDrList = pDrList->pNext)
    {
        char **ppszString = NULL;
        if (   pConInfo->DeviceDescriptor.iManufacturer
            && pDrList->iDr == pConInfo->DeviceDescriptor.iManufacturer)
            ppszString = (char **)&pDev->pszManufacturer;
        else if (   pConInfo->DeviceDescriptor.iProduct
                 && pDrList->iDr == pConInfo->DeviceDescriptor.iProduct)
            ppszString = (char **)&pDev->pszProduct;
        else if (   pConInfo->DeviceDescriptor.iSerialNumber
                 && pDrList->iDr == pConInfo->DeviceDescriptor.iSerialNumber)
            ppszString = (char **)&pDev->pszSerialNumber;
        if (ppszString)
        {
            rc = RTUtf16ToUtf8((PCRTUTF16)pDrList->StrDr.bString, ppszString);
            if (RT_SUCCESS(rc))
            {
                Assert(*ppszString);
                USBLibPurgeEncoding(*ppszString);

                if (pDrList->iDr == pConInfo->DeviceDescriptor.iSerialNumber)
                    pDev->u64SerialHash = USBLibHashSerial(*ppszString);
            }
            else
            {
                AssertMsgFailed(("RTUtf16ToUtf8 failed, rc (%d), resuming\n", rc));
                *ppszString = NULL;
            }
        }
    }

    return VINF_SUCCESS;
}

static void usbLibDevStrFree(LPSTR lpszName)
{
    RTStrFree(lpszName);
}

static int usbLibDevStrDriverKeyGet(HANDLE hHub, ULONG iPort, LPSTR* plpszName)
{
    USB_NODE_CONNECTION_DRIVERKEY_NAME Name;
    DWORD cbReturned = 0;
    Name.ConnectionIndex = iPort;
    *plpszName = NULL;
    if (!DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, &Name, sizeof (Name), &Name, sizeof (Name), &cbReturned, NULL))
    {
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
        DWORD dwErr = GetLastError();
        AssertMsgFailed(("DeviceIoControl 1 fail dwErr (%d)\n", dwErr));
#endif
        return VERR_GENERAL_FAILURE;
    }

    if (Name.ActualLength < sizeof (Name))
    {
        AssertFailed();
        return VERR_OUT_OF_RESOURCES;
    }

    PUSB_NODE_CONNECTION_DRIVERKEY_NAME pName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)RTMemAllocZ(Name.ActualLength);
    if (!pName)
    {
        AssertFailed();
        return VERR_OUT_OF_RESOURCES;
    }

    int rc = VINF_SUCCESS;
    pName->ConnectionIndex = iPort;
    if (DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, pName, Name.ActualLength, pName, Name.ActualLength, &cbReturned, NULL))
    {
        rc = RTUtf16ToUtf8Ex((PCRTUTF16)pName->DriverKeyName, pName->ActualLength / sizeof (WCHAR), plpszName, 0, NULL);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            rc = VINF_SUCCESS;
    }
    else
    {
        DWORD dwErr = GetLastError(); NOREF(dwErr);
        AssertMsgFailed(("DeviceIoControl 2 fail dwErr (%d)\n", dwErr));
        rc = VERR_GENERAL_FAILURE;
    }
    RTMemFree(pName);
    return rc;
}

static int usbLibDevStrHubNameGet(HANDLE hHub, ULONG iPort, LPSTR* plpszName)
{
    USB_NODE_CONNECTION_NAME Name;
    DWORD cbReturned = 0;
    Name.ConnectionIndex = iPort;
    *plpszName = NULL;
    if (!DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_NAME, &Name, sizeof (Name), &Name, sizeof (Name), &cbReturned, NULL))
    {
        AssertFailed();
        return VERR_GENERAL_FAILURE;
    }

    if (Name.ActualLength < sizeof (Name))
    {
        AssertFailed();
        return VERR_OUT_OF_RESOURCES;
    }

    PUSB_NODE_CONNECTION_NAME pName = (PUSB_NODE_CONNECTION_NAME)RTMemAllocZ(Name.ActualLength);
    if (!pName)
    {
        AssertFailed();
        return VERR_OUT_OF_RESOURCES;
    }

    int rc = VINF_SUCCESS;
    pName->ConnectionIndex = iPort;
    if (DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_NAME, pName, Name.ActualLength, pName, Name.ActualLength, &cbReturned, NULL))
    {
        rc = RTUtf16ToUtf8Ex((PCRTUTF16)pName->NodeName, pName->ActualLength / sizeof (WCHAR), plpszName, 0, NULL);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            rc = VINF_SUCCESS;
    }
    else
    {
        AssertFailed();
        rc = VERR_GENERAL_FAILURE;
    }
    RTMemFree(pName);
    return rc;
}

static int usbLibDevStrRootHubNameGet(HANDLE hCtl, LPSTR* plpszName)
{
    USB_ROOT_HUB_NAME HubName;
    DWORD cbReturned = 0;
    *plpszName = NULL;
    if (!DeviceIoControl(hCtl, IOCTL_USB_GET_ROOT_HUB_NAME, NULL, 0, &HubName, sizeof (HubName), &cbReturned, NULL))
    {
        return VERR_GENERAL_FAILURE;
    }
    PUSB_ROOT_HUB_NAME pHubName = (PUSB_ROOT_HUB_NAME)RTMemAllocZ(HubName.ActualLength);
    if (!pHubName)
        return VERR_OUT_OF_RESOURCES;

    int rc = VINF_SUCCESS;
    if (DeviceIoControl(hCtl, IOCTL_USB_GET_ROOT_HUB_NAME, NULL, 0, pHubName, HubName.ActualLength, &cbReturned, NULL))
    {
        rc = RTUtf16ToUtf8Ex((PCRTUTF16)pHubName->RootHubName, pHubName->ActualLength / sizeof (WCHAR), plpszName, 0, NULL);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            rc = VINF_SUCCESS;
    }
    else
    {
        rc = VERR_GENERAL_FAILURE;
    }
    RTMemFree(pHubName);
    return rc;
}

static int usbLibDevCfgDrGet(HANDLE hHub, ULONG iPort, ULONG iDr, PUSB_CONFIGURATION_DESCRIPTOR *ppDr)
{
    *ppDr = NULL;

    char Buf[sizeof (USB_DESCRIPTOR_REQUEST) + sizeof (USB_CONFIGURATION_DESCRIPTOR)];
    memset(&Buf, 0, sizeof (Buf));

    PUSB_DESCRIPTOR_REQUEST pCfgDrRq = (PUSB_DESCRIPTOR_REQUEST)Buf;
    PUSB_CONFIGURATION_DESCRIPTOR pCfgDr = (PUSB_CONFIGURATION_DESCRIPTOR)(Buf + sizeof (*pCfgDrRq));

    pCfgDrRq->ConnectionIndex = iPort;
    pCfgDrRq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | iDr;
    pCfgDrRq->SetupPacket.wLength = (USHORT)(sizeof (USB_CONFIGURATION_DESCRIPTOR));
    DWORD cbReturned = 0;
    if (!DeviceIoControl(hHub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, pCfgDrRq, sizeof (Buf),
                                pCfgDrRq, sizeof (Buf),
                                &cbReturned, NULL))
    {
        DWORD dwErr = GetLastError();
        LogRelFunc(("DeviceIoControl 1 fail dwErr (%d)\n", dwErr));
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
        AssertFailed();
#endif
        return VERR_GENERAL_FAILURE;
    }

    if (sizeof (Buf) != cbReturned)
    {
        AssertFailed();
        return VERR_GENERAL_FAILURE;
    }

    if (pCfgDr->wTotalLength < sizeof (USB_CONFIGURATION_DESCRIPTOR))
    {
        AssertFailed();
        return VERR_GENERAL_FAILURE;
    }

    DWORD cbRq = sizeof (USB_DESCRIPTOR_REQUEST) + pCfgDr->wTotalLength;
    PUSB_DESCRIPTOR_REQUEST pRq = (PUSB_DESCRIPTOR_REQUEST)RTMemAllocZ(cbRq);
    Assert(pRq);
    if (!pRq)
        return VERR_OUT_OF_RESOURCES;

    int rc = VERR_GENERAL_FAILURE;
    do
    {
        PUSB_CONFIGURATION_DESCRIPTOR pDr = (PUSB_CONFIGURATION_DESCRIPTOR)(pRq + 1);
        pRq->ConnectionIndex = iPort;
        pRq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | iDr;
        pRq->SetupPacket.wLength = (USHORT)(cbRq - sizeof (USB_DESCRIPTOR_REQUEST));
        if (!DeviceIoControl(hHub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, pRq, cbRq,
                                    pRq, cbRq,
                                    &cbReturned, NULL))
        {
            DWORD dwErr = GetLastError();
            LogRelFunc(("DeviceIoControl 2 fail dwErr (%d)\n", dwErr));
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
            AssertFailed();
#endif
            break;
        }

        if (cbRq != cbReturned)
        {
            AssertFailed();
            break;
        }

        if (pDr->wTotalLength != cbRq - sizeof (USB_DESCRIPTOR_REQUEST))
        {
            AssertFailed();
            break;
        }

        *ppDr = pDr;
        return VINF_SUCCESS;
    } while (0);

    RTMemFree(pRq);
    return rc;
}

static void usbLibDevCfgDrFree(PUSB_CONFIGURATION_DESCRIPTOR pDr)
{
    Assert(pDr);
    PUSB_DESCRIPTOR_REQUEST pRq = ((PUSB_DESCRIPTOR_REQUEST)pDr)-1;
    RTMemFree(pRq);
}

static int usbLibDevStrDrEntryGet(HANDLE hHub, ULONG iPort, ULONG iDr, USHORT idLang, PVBOXUSB_STRING_DR_ENTRY *ppList)
{
    char szBuf[sizeof (USB_DESCRIPTOR_REQUEST) + MAXIMUM_USB_STRING_LENGTH];
    RT_ZERO(szBuf);

    PUSB_DESCRIPTOR_REQUEST pRq = (PUSB_DESCRIPTOR_REQUEST)szBuf;
    PUSB_STRING_DESCRIPTOR pDr = (PUSB_STRING_DESCRIPTOR)(szBuf + sizeof (*pRq));
    RT_BZERO(pDr, sizeof(USB_STRING_DESCRIPTOR));

    pRq->ConnectionIndex = iPort;
    pRq->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8) | iDr;
    pRq->SetupPacket.wIndex = idLang;
    pRq->SetupPacket.wLength = sizeof (szBuf) - sizeof (*pRq);

    DWORD cbReturned = 0;
    if (!DeviceIoControl(hHub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, pRq, sizeof (szBuf),
                         pRq, sizeof(szBuf),
                         &cbReturned, NULL))
    {
        DWORD dwErr = GetLastError();
        LogRel(("Getting USB descriptor failed with error %ld\n", dwErr));
        return RTErrConvertFromWin32(dwErr);
    }

    /* Wrong descriptor type at the requested port index? Bail out. */
    if (pDr->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
        return VERR_NOT_FOUND;

    /* Some more sanity checks. */
    if (   (cbReturned < sizeof (*pDr) + 2)
        || (!!(pDr->bLength % 2))
        || (pDr->bLength != cbReturned - sizeof(*pRq)))
    {
        AssertMsgFailed(("Sanity check failed for string descriptor: cbReturned=%RI32, cbDevReq=%zu, type=%RU8, len=%RU8, port=%RU32, index=%RU32, lang=%RU32\n",
                         cbReturned, sizeof(*pRq), pDr->bDescriptorType, pDr->bLength, iPort, iDr, idLang));
        return VERR_INVALID_PARAMETER;
    }

    PVBOXUSB_STRING_DR_ENTRY pEntry =
        (PVBOXUSB_STRING_DR_ENTRY)RTMemAllocZ(sizeof(VBOXUSB_STRING_DR_ENTRY) + pDr->bLength + 2);
    AssertPtr(pEntry);
    if (!pEntry)
        return VERR_NO_MEMORY;

    pEntry->pNext = *ppList;
    pEntry->iDr = iDr;
    pEntry->idLang = idLang;
    memcpy(&pEntry->StrDr, pDr, pDr->bLength);

    *ppList = pEntry;

    return VINF_SUCCESS;
}

static void usbLibDevStrDrEntryFree(PVBOXUSB_STRING_DR_ENTRY pDr)
{
    RTMemFree(pDr);
}

static void usbLibDevStrDrEntryFreeList(PVBOXUSB_STRING_DR_ENTRY pDr)
{
    while (pDr)
    {
        PVBOXUSB_STRING_DR_ENTRY pNext = pDr->pNext;
        usbLibDevStrDrEntryFree(pDr);
        pDr = pNext;
    }
}

static int usbLibDevStrDrEntryGetForLangs(HANDLE hHub, ULONG iPort, ULONG iDr, ULONG cIdLang, const USHORT *pIdLang, PVBOXUSB_STRING_DR_ENTRY *ppList)
{
    for (ULONG i = 0; i < cIdLang; ++i)
    {
        usbLibDevStrDrEntryGet(hHub, iPort, iDr, pIdLang[i], ppList);
    }
    return VINF_SUCCESS;
}

static int usbLibDevStrDrEntryGetAll(HANDLE hHub, ULONG iPort, PUSB_DEVICE_DESCRIPTOR pDevDr, PUSB_CONFIGURATION_DESCRIPTOR pCfgDr, PVBOXUSB_STRING_DR_ENTRY *ppList)
{
    /* Read string descriptor zero to determine what languages are available. */
    int rc = usbLibDevStrDrEntryGet(hHub, iPort, 0, 0, ppList);
    if (RT_FAILURE(rc))
        return rc;

    PUSB_STRING_DESCRIPTOR pLangStrDr = &(*ppList)->StrDr;
    USHORT *pIdLang = pLangStrDr->bString;
    ULONG cIdLang = (pLangStrDr->bLength - RT_OFFSETOF(USB_STRING_DESCRIPTOR, bString)) / sizeof (*pIdLang);

    if (pDevDr->iManufacturer)
    {
        rc = usbLibDevStrDrEntryGetForLangs(hHub, iPort, pDevDr->iManufacturer, cIdLang, pIdLang, ppList);
        AssertRC(rc);
    }

    if (pDevDr->iProduct)
    {
        rc = usbLibDevStrDrEntryGetForLangs(hHub, iPort, pDevDr->iProduct, cIdLang, pIdLang, ppList);
        AssertRC(rc);
    }

    if (pDevDr->iSerialNumber)
    {
        rc = usbLibDevStrDrEntryGetForLangs(hHub, iPort, pDevDr->iSerialNumber, cIdLang, pIdLang, ppList);
        AssertRC(rc);
    }

    PUCHAR pCur = (PUCHAR)pCfgDr;
    PUCHAR pEnd = pCur + pCfgDr->wTotalLength;
    while (pCur + sizeof (USB_COMMON_DESCRIPTOR) <= pEnd)
    {
        PUSB_COMMON_DESCRIPTOR pCmnDr = (PUSB_COMMON_DESCRIPTOR)pCur;
        if (pCur + pCmnDr->bLength > pEnd)
        {
            AssertFailed();
            break;
        }

        /* This is invalid but was seen with a TerraTec Aureon 7.1 USB sound card. */
        if (!pCmnDr->bLength)
            break;

        switch (pCmnDr->bDescriptorType)
        {
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            {
                if (pCmnDr->bLength != sizeof (USB_CONFIGURATION_DESCRIPTOR))
                {
                    AssertFailed();
                    break;
                }
                PUSB_CONFIGURATION_DESCRIPTOR pCurCfgDr = (PUSB_CONFIGURATION_DESCRIPTOR)pCmnDr;
                if (!pCurCfgDr->iConfiguration)
                    break;
                rc = usbLibDevStrDrEntryGetForLangs(hHub, iPort, pCurCfgDr->iConfiguration, cIdLang, pIdLang, ppList);
                AssertRC(rc);
                break;
            }
            case USB_INTERFACE_DESCRIPTOR_TYPE:
            {
                if (pCmnDr->bLength != sizeof (USB_INTERFACE_DESCRIPTOR) && pCmnDr->bLength != sizeof (USB_INTERFACE_DESCRIPTOR2))
                {
                    AssertFailed();
                    break;
                }
                PUSB_INTERFACE_DESCRIPTOR pCurIfDr = (PUSB_INTERFACE_DESCRIPTOR)pCmnDr;
                if (!pCurIfDr->iInterface)
                    break;
                rc = usbLibDevStrDrEntryGetForLangs(hHub, iPort, pCurIfDr->iInterface, cIdLang, pIdLang, ppList);
                AssertRC(rc);
                break;
            }
            default:
                break;
        }

        pCur = pCur + pCmnDr->bLength;
    }

    return VINF_SUCCESS;
}

static int usbLibDevGetHubDevices(LPCSTR lpszName, PUSBDEVICE *ppDevs, uint32_t *pcDevs);

static int usbLibDevGetHubPortDevices(HANDLE hHub, LPCSTR lpcszHubName, ULONG iPort, PUSBDEVICE *ppDevs, uint32_t *pcDevs)
{
    int rc = VINF_SUCCESS;
    char Buf[sizeof (USB_NODE_CONNECTION_INFORMATION_EX) + (sizeof (USB_PIPE_INFO) * 20)];
    PUSB_NODE_CONNECTION_INFORMATION_EX pConInfo = (PUSB_NODE_CONNECTION_INFORMATION_EX)Buf;
    //PUSB_PIPE_INFO paPipeInfo = (PUSB_PIPE_INFO)(Buf + sizeof (PUSB_NODE_CONNECTION_INFORMATION_EX));
    DWORD cbReturned = 0;
    memset(&Buf, 0, sizeof (Buf));
    pConInfo->ConnectionIndex = iPort;
    if (!DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
                                  pConInfo, sizeof (Buf),
                                  pConInfo, sizeof (Buf),
                                  &cbReturned, NULL))
    {
        DWORD dwErr = GetLastError(); NOREF(dwErr);
        AssertMsg(dwErr == ERROR_DEVICE_NOT_CONNECTED, (__FUNCTION__": DeviceIoControl failed dwErr (%d)\n", dwErr));
        return VERR_GENERAL_FAILURE;
    }

    if (pConInfo->ConnectionStatus != DeviceConnected)
    {
        /* just ignore & return success */
        return VWRN_INVALID_HANDLE;
    }

    if (pConInfo->DeviceIsHub)
    {
        LPSTR lpszHubName = NULL;
        rc = usbLibDevStrHubNameGet(hHub, iPort, &lpszHubName);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            rc = usbLibDevGetHubDevices(lpszHubName, ppDevs, pcDevs);
            usbLibDevStrFree(lpszHubName);
            AssertRC(rc);
            return rc;
        }
        /* ignore this err */
        return VINF_SUCCESS;
    }

    bool fFreeNameBuf = true;
    char nameEmptyBuf = '\0';
    LPSTR lpszName = NULL;
    rc = usbLibDevStrDriverKeyGet(hHub, iPort, &lpszName);
    Assert(!!lpszName == !!RT_SUCCESS(rc));
    if (!lpszName)
    {
        lpszName = &nameEmptyBuf;
        fFreeNameBuf = false;
    }

    PUSB_CONFIGURATION_DESCRIPTOR pCfgDr = NULL;
    PVBOXUSB_STRING_DR_ENTRY pList = NULL;
    rc = usbLibDevCfgDrGet(hHub, iPort, 0, &pCfgDr);
    if (pCfgDr)
    {
        rc = usbLibDevStrDrEntryGetAll(hHub, iPort, &pConInfo->DeviceDescriptor, pCfgDr, &pList);
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
        AssertRC(rc);
#endif
    }

    PUSBDEVICE pDev = (PUSBDEVICE)RTMemAllocZ(sizeof (*pDev));
    if (RT_LIKELY(pDev))
    {
        rc = usbLibDevPopulate(pDev, pConInfo, iPort, lpszName, lpcszHubName, pList);
        if (RT_SUCCESS(rc))
        {
            pDev->pNext = *ppDevs;
            *ppDevs = pDev;
            ++*pcDevs;
        }
        else
            RTMemFree(pDev);
    }
    else
        rc = VERR_NO_MEMORY;

    if (pCfgDr)
        usbLibDevCfgDrFree(pCfgDr);
    if (fFreeNameBuf)
    {
        Assert(lpszName);
        usbLibDevStrFree(lpszName);
    }
    if (pList)
        usbLibDevStrDrEntryFreeList(pList);

    return rc;
}

static int usbLibDevGetHubDevices(LPCSTR lpszName, PUSBDEVICE *ppDevs, uint32_t *pcDevs)
{
    LPSTR lpszDevName = (LPSTR)RTMemAllocZ(strlen(lpszName) + sizeof("\\\\.\\"));
    HANDLE hDev = INVALID_HANDLE_VALUE;
    Assert(lpszDevName);
    if (!lpszDevName)
    {
        AssertFailed();
        return VERR_OUT_OF_RESOURCES;
    }

    int rc = VINF_SUCCESS;
    strcpy(lpszDevName, "\\\\.\\");
    strcpy(lpszDevName + sizeof("\\\\.\\") - sizeof (lpszDevName[0]), lpszName);
    do
    {
        DWORD cbReturned = 0;
        hDev = CreateFile(lpszDevName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hDev == INVALID_HANDLE_VALUE)
        {
            AssertFailed();
            break;
        }

        USB_NODE_INFORMATION NodeInfo;
        memset(&NodeInfo, 0, sizeof (NodeInfo));
        if (!DeviceIoControl(hDev, IOCTL_USB_GET_NODE_INFORMATION,
                            &NodeInfo, sizeof (NodeInfo),
                            &NodeInfo, sizeof (NodeInfo),
                            &cbReturned, NULL))
        {
            AssertFailed();
            break;
        }

        for (ULONG i = 1; i <= NodeInfo.u.HubInformation.HubDescriptor.bNumberOfPorts; ++i)
        {
            /* Just skip devices for which we failed to create the device structure. */
            usbLibDevGetHubPortDevices(hDev, lpszName, i, ppDevs, pcDevs);
        }
    } while (0);

    if (hDev != INVALID_HANDLE_VALUE)
        CloseHandle(hDev);

    RTMemFree(lpszDevName);

    return rc;
}

static int usbLibDevGetDevices(PUSBDEVICE *ppDevs, uint32_t *pcDevs)
{
    char CtlName[16];
    int rc = VINF_SUCCESS;

    for (int i = 0; i < 10; ++i)
    {
        sprintf(CtlName, "\\\\.\\HCD%d", i);
        HANDLE hCtl = CreateFile(CtlName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hCtl != INVALID_HANDLE_VALUE)
        {
            char* lpszName;
            rc = usbLibDevStrRootHubNameGet(hCtl, &lpszName);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                rc = usbLibDevGetHubDevices(lpszName, ppDevs, pcDevs);
                AssertRC(rc);
                usbLibDevStrFree(lpszName);
            }
            CloseHandle(hCtl);
            if (RT_FAILURE(rc))
                break;
        }
    }
    return VINF_SUCCESS;
}

#if 0 /* unused */
static PUSBSUP_GET_DEVICES usbLibMonGetDevRqAlloc(uint32_t cDevs, PDWORD pcbRq)
{
    DWORD cbRq = RT_OFFSETOF(USBSUP_GET_DEVICES, aDevices[cDevs]);
    PUSBSUP_GET_DEVICES pRq = (PUSBSUP_GET_DEVICES)RTMemAllocZ(cbRq);
    Assert(pRq);
    if (!pRq)
        return NULL;
    pRq->cDevices = cDevs;
    *pcbRq = cbRq;
    return pRq;
}
#endif

static int usbLibMonDevicesCmp(PUSBDEVICE pDev, PVBOXUSB_DEV pDevInfo)
{
    int iDiff;
    iDiff = strcmp(pDev->pszAddress, pDevInfo->szDriverRegName);
    return iDiff;
}

static int usbLibMonDevicesUpdate(PVBOXUSBGLOBALSTATE pGlobal, PUSBDEVICE pDevs, PVBOXUSB_DEV pDevInfos)
{

    PUSBDEVICE pDevsHead = pDevs;
    for (; pDevInfos; pDevInfos = pDevInfos->pNext)
    {
        for (pDevs = pDevsHead; pDevs; pDevs = pDevs->pNext)
        {
            if (usbLibMonDevicesCmp(pDevs, pDevInfos))
                continue;

            if (!pDevInfos->szDriverRegName[0])
            {
                AssertFailed();
                break;
            }

            USBSUP_GETDEV Dev = {0};
            HANDLE hDev = CreateFile(pDevInfos->szName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
                                                          OPEN_EXISTING,  FILE_ATTRIBUTE_SYSTEM, NULL);
            if (hDev == INVALID_HANDLE_VALUE)
            {
                AssertFailed();
                break;
            }

            DWORD cbReturned = 0;
            if (!DeviceIoControl(hDev, SUPUSB_IOCTL_GET_DEVICE, &Dev, sizeof (Dev), &Dev, sizeof (Dev), &cbReturned, NULL))
            {
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
                 DWORD dwErr = GetLastError(); NOREF(dwErr);
                 /* ERROR_DEVICE_NOT_CONNECTED -> device was removed just now */
                 AssertMsg(dwErr == ERROR_DEVICE_NOT_CONNECTED, (__FUNCTION__": DeviceIoControl failed dwErr (%d)\n", dwErr));
#endif
                 Log(("SUPUSB_IOCTL_GET_DEVICE: DeviceIoControl no longer connected\n"));
                 CloseHandle(hDev);
                 break;
            }

            /* we must not close the handle until we request for the device state from the monitor to ensure
             * the device handle returned by the device driver does not disappear */
            Assert(Dev.hDevice);
            USBSUP_GETDEV_MON MonInfo;
            HVBOXUSBDEVUSR hDevice = Dev.hDevice;
            if (!DeviceIoControl(pGlobal->hMonitor, SUPUSBFLT_IOCTL_GET_DEVICE, &hDevice, sizeof (hDevice), &MonInfo, sizeof (MonInfo), &cbReturned, NULL))
            {
                 DWORD dwErr = GetLastError(); NOREF(dwErr);
                 /* ERROR_DEVICE_NOT_CONNECTED -> device was removed just now */
                 AssertMsgFailed(("Monitor DeviceIoControl failed dwErr (%d)\n", dwErr));
                 Log(("SUPUSBFLT_IOCTL_GET_DEVICE: DeviceIoControl no longer connected\n"));
                 CloseHandle(hDev);
                 break;
            }

            CloseHandle(hDev);

            /* success!! update device info */
            /* ensure the state returned is valid */
            Assert(    MonInfo.enmState == USBDEVICESTATE_USED_BY_HOST
                    || MonInfo.enmState == USBDEVICESTATE_USED_BY_HOST_CAPTURABLE
                    || MonInfo.enmState == USBDEVICESTATE_UNUSED
                    || MonInfo.enmState == USBDEVICESTATE_HELD_BY_PROXY
                    || MonInfo.enmState == USBDEVICESTATE_USED_BY_GUEST);
            pDevs->enmState = MonInfo.enmState;
            if (pDevs->bcdUSB == 0x300)
                /* USB3 spec guarantees this (9.6.1). */
                pDevs->enmSpeed = USBDEVICESPEED_SUPER;
            else
                /* The following is not 100% accurate but we only care about high-speed vs. non-high-speed */
                pDevs->enmSpeed = Dev.fHiSpeed ? USBDEVICESPEED_HIGH : USBDEVICESPEED_FULL;

            if (pDevs->enmState != USBDEVICESTATE_USED_BY_HOST)
            {
                /* only set the interface name if device can be grabbed */
                RTStrFree(pDevs->pszAltAddress);
                pDevs->pszAltAddress = (char*)pDevs->pszAddress;
                pDevs->pszAddress = RTStrDup(pDevInfos->szName);
            }
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
            else
            {
                /* dbg breakpoint */
                AssertFailed();
            }
#endif

            /* we've found the device, break in any way */
            break;
        }
    }

    return VINF_SUCCESS;
}

static int usbLibGetDevices(PVBOXUSBGLOBALSTATE pGlobal, PUSBDEVICE *ppDevs, uint32_t *pcDevs)
{
    *ppDevs = NULL;
    *pcDevs = 0;

    int rc = usbLibDevGetDevices(ppDevs, pcDevs);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        PVBOXUSB_DEV pDevInfos = NULL;
        uint32_t cDevInfos = 0;
        rc = usbLibVuGetDevices(&pDevInfos, &cDevInfos);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            rc = usbLibMonDevicesUpdate(pGlobal, *ppDevs, pDevInfos);
            AssertRC(rc);
            usbLibVuFreeDevices(pDevInfos);
        }

        return VINF_SUCCESS;
    }
    return rc;
}

AssertCompile(INFINITE == RT_INDEFINITE_WAIT);
static int usbLibStateWaitChange(PVBOXUSBGLOBALSTATE pGlobal, RTMSINTERVAL cMillies)
{
    HANDLE ahEvents[] = {pGlobal->hNotifyEvent, pGlobal->hInterruptEvent};
    DWORD dwResult = WaitForMultipleObjects(RT_ELEMENTS(ahEvents), ahEvents,
                        FALSE, /* BOOL bWaitAll */
                        cMillies);

    switch (dwResult)
    {
        case WAIT_OBJECT_0:
            return VINF_SUCCESS;
        case WAIT_OBJECT_0 + 1:
            return VERR_INTERRUPTED;
        case WAIT_TIMEOUT:
            return VERR_TIMEOUT;
        default:
        {
            DWORD dwErr = GetLastError(); NOREF(dwErr);
            AssertMsgFailed(("WaitForMultipleObjects failed, dwErr (%d)\n", dwErr));
            return VERR_GENERAL_FAILURE;
        }
    }
}

AssertCompile(RT_INDEFINITE_WAIT == INFINITE);
AssertCompile(sizeof (RTMSINTERVAL) == sizeof (DWORD));
USBLIB_DECL(int) USBLibWaitChange(RTMSINTERVAL msWaitTimeout)
{
    return usbLibStateWaitChange(&g_VBoxUsbGlobal, msWaitTimeout);
}

static int usbLibInterruptWaitChange(PVBOXUSBGLOBALSTATE pGlobal)
{
    BOOL fRc = SetEvent(pGlobal->hInterruptEvent);
    if (!fRc)
    {
        DWORD dwErr = GetLastError(); NOREF(dwErr);
        AssertMsgFailed(("SetEvent failed, dwErr (%d)\n", dwErr));
        return VERR_GENERAL_FAILURE;
    }
    return VINF_SUCCESS;
}

USBLIB_DECL(int) USBLibInterruptWaitChange(void)
{
    return usbLibInterruptWaitChange(&g_VBoxUsbGlobal);
}

/*
USBLIB_DECL(bool) USBLibHasPendingDeviceChanges(void)
{
    int rc = USBLibWaitChange(0);
    return rc == VINF_SUCCESS;
}
*/

USBLIB_DECL(int) USBLibGetDevices(PUSBDEVICE *ppDevices, uint32_t *pcbNumDevices)
{
    Assert(g_VBoxUsbGlobal.hMonitor != INVALID_HANDLE_VALUE);
    return usbLibGetDevices(&g_VBoxUsbGlobal, ppDevices, pcbNumDevices);
}

USBLIB_DECL(void *) USBLibAddFilter(PCUSBFILTER pFilter)
{
    USBSUP_FLTADDOUT FltAddRc;
    DWORD cbReturned = 0;

    if (g_VBoxUsbGlobal.hMonitor == INVALID_HANDLE_VALUE)
    {
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
        AssertFailed();
#endif
        return NULL;
    }

    Log(("usblibInsertFilter: Manufacturer=%s Product=%s Serial=%s\n",
         USBFilterGetString(pFilter, USBFILTERIDX_MANUFACTURER_STR)  ? USBFilterGetString(pFilter, USBFILTERIDX_MANUFACTURER_STR)  : "<null>",
         USBFilterGetString(pFilter, USBFILTERIDX_PRODUCT_STR)       ? USBFilterGetString(pFilter, USBFILTERIDX_PRODUCT_STR)       : "<null>",
         USBFilterGetString(pFilter, USBFILTERIDX_SERIAL_NUMBER_STR) ? USBFilterGetString(pFilter, USBFILTERIDX_SERIAL_NUMBER_STR) : "<null>"));

    if (!DeviceIoControl(g_VBoxUsbGlobal.hMonitor, SUPUSBFLT_IOCTL_ADD_FILTER,
                (LPVOID)pFilter, sizeof(*pFilter),
                &FltAddRc, sizeof(FltAddRc),
                &cbReturned, NULL))
    {
        DWORD dwErr = GetLastError(); NOREF(dwErr);
        AssertMsgFailed(("DeviceIoControl failed with dwErr (%d(\n", dwErr));
        return NULL;
    }

    if (RT_FAILURE(FltAddRc.rc))
    {
        AssertMsgFailed(("Adding filter failed with %d\n", FltAddRc.rc));
        return NULL;
    }
    return (void *)FltAddRc.uId;
}


USBLIB_DECL(void) USBLibRemoveFilter(void *pvId)
{
    uintptr_t uId;
    DWORD cbReturned = 0;

    if (g_VBoxUsbGlobal.hMonitor == INVALID_HANDLE_VALUE)
    {
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
        AssertFailed();
#endif
        return;
    }

    Log(("usblibRemoveFilter %p\n", pvId));

    uId = (uintptr_t)pvId;
    if (!DeviceIoControl(g_VBoxUsbGlobal.hMonitor, SUPUSBFLT_IOCTL_REMOVE_FILTER, &uId, sizeof(uId),  NULL, 0,&cbReturned, NULL))
        AssertMsgFailed(("DeviceIoControl failed with LastError=%Rwa\n", GetLastError()));
}

USBLIB_DECL(int) USBLibRunFilters(void)
{
    DWORD cbReturned = 0;

    Assert(g_VBoxUsbGlobal.hMonitor != INVALID_HANDLE_VALUE);

    if (!DeviceIoControl(g_VBoxUsbGlobal.hMonitor, SUPUSBFLT_IOCTL_RUN_FILTERS,
                NULL, 0,
                NULL, 0,
                &cbReturned, NULL))
    {
        DWORD dwErr = GetLastError();
        AssertMsgFailed(("DeviceIoControl failed with dwErr (%d(\n", dwErr));
        return RTErrConvertFromWin32(dwErr);
    }

    return VINF_SUCCESS;
}


#ifdef VBOX_USB_USE_DEVICE_NOTIFICATION

static VOID CALLBACK usbLibTimerCallback(__in PVOID lpParameter, __in BOOLEAN TimerOrWaitFired)
{
    RT_NOREF2(lpParameter, TimerOrWaitFired);
    SetEvent(g_VBoxUsbGlobal.hNotifyEvent);
}

static void usbLibOnDeviceChange(void)
{
    /* we're getting series of events like that especially on device re-attach
     * (i.e. first for device detach and then for device attach)
     * unfortunately the event does not tell us what actually happened.
     * To avoid extra notifications, we delay the SetEvent via a timer
     * and update the timer if additional notification comes before the timer fires
     * */
    if (g_VBoxUsbGlobal.hTimer)
    {
        if (!DeleteTimerQueueTimer(g_VBoxUsbGlobal.hTimerQueue, g_VBoxUsbGlobal.hTimer, NULL))
        {
            DWORD dwErr = GetLastError(); NOREF(dwErr);
            AssertMsg(dwErr == ERROR_IO_PENDING, ("DeleteTimerQueueTimer failed, dwErr (%d)\n", dwErr));
        }
    }

    if (!CreateTimerQueueTimer(&g_VBoxUsbGlobal.hTimer, g_VBoxUsbGlobal.hTimerQueue,
                               usbLibTimerCallback,
                               NULL,
                               500, /* ms*/
                               0,
                               WT_EXECUTEONLYONCE))
    {
        DWORD dwErr = GetLastError(); NOREF(dwErr);
        AssertMsgFailed(("CreateTimerQueueTimer failed, dwErr (%d)\n", dwErr));

        /* call it directly */
        usbLibTimerCallback(NULL, FALSE);
    }
}

static LRESULT CALLBACK usbLibWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DEVICECHANGE:
            if (wParam == DBT_DEVNODES_CHANGED)
            {
                /* we notify change any device arivals/removals on the system
                 * and let the client decide whether the usb change actually happened
                 * so far this is more clean than reporting events from the Monitor
                 * because monitor sees only PDO arrivals/removals,
                 * and by the time PDO is created, device can not
                 * be yet started and fully functional,
                 * so usblib won't be able to pick it up
                 * */

                usbLibOnDeviceChange();
            }
            break;
         case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            }
    }
    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}

/** @todo r=bird: Use an IPRT thread? */
static DWORD WINAPI usbLibMsgThreadProc(__in LPVOID lpParameter)
{
    static LPCSTR   s_szVBoxUsbWndClassName = "VBoxUsbLibClass";
    const HINSTANCE hInstance               = (HINSTANCE)GetModuleHandle(NULL);
    RT_NOREF1(lpParameter);

    Assert(g_VBoxUsbGlobal.hWnd == NULL);
    g_VBoxUsbGlobal.hWnd = NULL;

    /*
     * Register the Window Class and create the hidden window.
     */
    WNDCLASS wc;
    wc.style         = 0;
    wc.lpfnWndProc   = usbLibWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(void *);
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = s_szVBoxUsbWndClassName;
    ATOM atomWindowClass = RegisterClass(&wc);
    if (atomWindowClass != 0)
        g_VBoxUsbGlobal.hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                                              s_szVBoxUsbWndClassName, s_szVBoxUsbWndClassName,
                                              WS_POPUPWINDOW,
                                              -200, -200, 100, 100, NULL, NULL, hInstance, NULL);
    else
        AssertMsgFailed(("RegisterClass failed, last error %u\n", GetLastError()));

    /*
     * Signal the creator thread.
     */
    ASMCompilerBarrier();
    SetEvent(g_VBoxUsbGlobal.hNotifyEvent);

    if (g_VBoxUsbGlobal.hWnd)
    {
        /* Make sure it's really hidden. */
        SetWindowPos(g_VBoxUsbGlobal.hWnd, HWND_TOPMOST, -200, -200, 0, 0,
                     SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSIZE);

        /*
         * The message pump.
         */
        MSG msg;
        BOOL fRet;
        while ((fRet = GetMessage(&msg, NULL, 0, 0)) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Assert(fRet >= 0);
    }

    if (atomWindowClass != NULL)
        UnregisterClass(s_szVBoxUsbWndClassName, hInstance);

    return 0;
}

#endif /* VBOX_USB_USE_DEVICE_NOTIFICATION */

/**
 * Initialize the USB library
 *
 * @returns VBox status code.
 */
USBLIB_DECL(int) USBLibInit(void)
{
    int rc = VERR_GENERAL_FAILURE;

    Log(("usbproxy: usbLibInit\n"));

    RT_ZERO(g_VBoxUsbGlobal);
    g_VBoxUsbGlobal.hMonitor = INVALID_HANDLE_VALUE;

    /*
     * Create the notification and interrupt event before opening the device.
     */
    g_VBoxUsbGlobal.hNotifyEvent = CreateEvent(NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes */
                                               FALSE, /* BOOL bManualReset */
#ifndef VBOX_USB_USE_DEVICE_NOTIFICATION
                                               TRUE,  /* BOOL bInitialState */
#else
                                               FALSE, /* set to false since it will be initially used for notification thread startup sync */
#endif
                                               NULL   /* LPCTSTR lpName */);
    if (g_VBoxUsbGlobal.hNotifyEvent)
    {
        g_VBoxUsbGlobal.hInterruptEvent = CreateEvent(NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes */
                                                      FALSE, /* BOOL bManualReset */
                                                      FALSE, /* BOOL bInitialState */
                                                      NULL   /* LPCTSTR lpName */);
        if (g_VBoxUsbGlobal.hInterruptEvent)
        {
            /*
             * Open the USB monitor device, starting if needed.
             */
            g_VBoxUsbGlobal.hMonitor = CreateFile(USBMON_DEVICE_NAME,
                                                  GENERIC_READ | GENERIC_WRITE,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                  NULL,
                                                  OPEN_EXISTING,
                                                  FILE_ATTRIBUTE_SYSTEM,
                                                  NULL);

            if (g_VBoxUsbGlobal.hMonitor == INVALID_HANDLE_VALUE)
            {
                HRESULT hr = VBoxDrvCfgSvcStart(USBMON_SERVICE_NAME_W);
                if (hr == S_OK)
                {
                    g_VBoxUsbGlobal.hMonitor = CreateFile(USBMON_DEVICE_NAME,
                                                          GENERIC_READ | GENERIC_WRITE,
                                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                          NULL,
                                                          OPEN_EXISTING,
                                                          FILE_ATTRIBUTE_SYSTEM,
                                                          NULL);
                    if (g_VBoxUsbGlobal.hMonitor == INVALID_HANDLE_VALUE)
                    {
                        DWORD dwErr = GetLastError();
                        LogRelFunc(("CreateFile failed dwErr(%d)\n", dwErr));
                        rc = VERR_FILE_NOT_FOUND;
                    }
                }
            }

            if (g_VBoxUsbGlobal.hMonitor != INVALID_HANDLE_VALUE)
            {
                /*
                 * Check the USB monitor version.
                 *
                 * Drivers are backwards compatible within the same major
                 * number.  We consider the minor version number this library
                 * is compiled with to be the minimum required by the driver.
                 * This is by reasoning that the library uses the full feature
                 * set of the driver it's written for.
                 */
                USBSUP_VERSION  Version = {0};
                DWORD           cbReturned = 0;
                if (DeviceIoControl(g_VBoxUsbGlobal.hMonitor, SUPUSBFLT_IOCTL_GET_VERSION,
                                    NULL, 0,
                                    &Version, sizeof (Version),
                                    &cbReturned, NULL))
                {
                    if (   Version.u32Major == USBMON_MAJOR_VERSION
#if USBMON_MINOR_VERSION != 0
                        && Version.u32Minor >= USBMON_MINOR_VERSION
#endif
                        )
                    {
#ifndef VBOX_USB_USE_DEVICE_NOTIFICATION
                        /*
                         * Tell the monitor driver which event object to use
                         * for notifications.
                         */
                        USBSUP_SET_NOTIFY_EVENT SetEvent = {0};
                        Assert(g_VBoxUsbGlobal.hNotifyEvent);
                        SetEvent.u.hEvent = g_VBoxUsbGlobal.hNotifyEvent;
                        if (DeviceIoControl(g_VBoxUsbGlobal.hMonitor, SUPUSBFLT_IOCTL_SET_NOTIFY_EVENT,
                                            &SetEvent, sizeof(SetEvent),
                                            &SetEvent, sizeof(SetEvent),
                                            &cbReturned, NULL))
                        {
                            rc = SetEvent.u.rc;
                            if (RT_SUCCESS(rc))
                            {
                                /*
                                 * We're DONE!
                                 */
                                return VINF_SUCCESS;
                            }

                            AssertMsgFailed(("SetEvent failed, %Rrc (%d)\n", rc, rc));
                        }
                        else
                        {
                            DWORD dwErr = GetLastError();
                            AssertMsgFailed(("SetEvent Ioctl failed, dwErr (%d)\n", dwErr));
                            rc = VERR_VERSION_MISMATCH;
                        }
#else
                        /*
                         * We can not use USB Mon for reliable device add/remove tracking
                         * since once USB Mon is notified about PDO creation and/or IRP_MN_START_DEVICE,
                         * the function device driver may still do some initialization, which might result in
                         * notifying too early.
                         * Instead we use WM_DEVICECHANGE + DBT_DEVNODES_CHANGED to make Windows notify us about
                         * device arivals/removals.
                         * Since WM_DEVICECHANGE is a window message, create a dedicated thread to be used for WndProc and stuff.
                         * The thread would create a window, track windows messages and call usbLibOnDeviceChange on WM_DEVICECHANGE arrival.
                         * See comments in usbLibOnDeviceChange function for detail about using the timer queue.
                         */
                        g_VBoxUsbGlobal.hTimerQueue = CreateTimerQueue();
                        if (g_VBoxUsbGlobal.hTimerQueue)
                        {
                            g_VBoxUsbGlobal.hThread = CreateThread(
                              NULL, /*__in_opt   LPSECURITY_ATTRIBUTES lpThreadAttributes, */
                              0, /*__in       SIZE_T dwStackSize, */
                              usbLibMsgThreadProc, /*__in       LPTHREAD_START_ROUTINE lpStartAddress,*/
                              NULL, /*__in_opt   LPVOID lpParameter,*/
                              0, /*__in       DWORD dwCreationFlags,*/
                              NULL /*__out_opt  LPDWORD lpThreadId*/
                            );
                            if (g_VBoxUsbGlobal.hThread)
                            {
                                DWORD dwResult = WaitForSingleObject(g_VBoxUsbGlobal.hNotifyEvent, INFINITE);
                                Assert(dwResult == WAIT_OBJECT_0);
                                if (g_VBoxUsbGlobal.hWnd)
                                {
                                    /*
                                     * We're DONE!
                                     *
                                     * Just ensure that the event is set so the
                                     * first "wait change" request is processed.
                                     */
                                    SetEvent(g_VBoxUsbGlobal.hNotifyEvent);
                                    return VINF_SUCCESS;
                                }

                                dwResult = WaitForSingleObject(g_VBoxUsbGlobal.hThread, INFINITE);
                                Assert(dwResult == WAIT_OBJECT_0);
                                BOOL fRc = CloseHandle(g_VBoxUsbGlobal.hThread); NOREF(fRc);
                                DWORD dwErr = GetLastError(); NOREF(dwErr);
                                AssertMsg(fRc, ("CloseHandle for hThread failed dwErr(%d)\n", dwErr));
                                g_VBoxUsbGlobal.hThread = INVALID_HANDLE_VALUE;
                            }
                            else
                            {
                                DWORD dwErr = GetLastError(); NOREF(dwErr);
                                AssertMsgFailed(("CreateThread failed, dwErr (%d)\n", dwErr));
                                rc = VERR_GENERAL_FAILURE;
                            }

                            DeleteTimerQueueEx(g_VBoxUsbGlobal.hTimerQueue, INVALID_HANDLE_VALUE /* see term */);
                            g_VBoxUsbGlobal.hTimerQueue = NULL;
                        }
                        else
                        {
                            DWORD dwErr = GetLastError(); NOREF(dwErr);
                            AssertMsgFailed(("CreateTimerQueue failed dwErr(%d)\n", dwErr));
                        }
#endif
                    }
                    else
                    {
                        LogRelFunc(("USB Monitor driver version mismatch! driver=%u.%u library=%u.%u\n",
                                Version.u32Major, Version.u32Minor, USBMON_MAJOR_VERSION, USBMON_MINOR_VERSION));
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
                        AssertFailed();
#endif
                        rc = VERR_VERSION_MISMATCH;
                    }
                }
                else
                {
                    DWORD dwErr = GetLastError(); NOREF(dwErr);
                    AssertMsgFailed(("DeviceIoControl failed dwErr(%d)\n", dwErr));
                    rc = VERR_VERSION_MISMATCH;
                }

                CloseHandle(g_VBoxUsbGlobal.hMonitor);
                g_VBoxUsbGlobal.hMonitor = INVALID_HANDLE_VALUE;
            }
            else
            {
                LogRelFunc(("USB Service not found\n"));
#ifdef VBOX_WITH_ANNOYING_USB_ASSERTIONS
                AssertFailed();
#endif
                rc = VERR_FILE_NOT_FOUND;
            }

            CloseHandle(g_VBoxUsbGlobal.hInterruptEvent);
            g_VBoxUsbGlobal.hInterruptEvent = NULL;
        }
        else
        {
            DWORD dwErr = GetLastError(); NOREF(dwErr);
            AssertMsgFailed(("CreateEvent for InterruptEvent failed dwErr(%d)\n", dwErr));
            rc = VERR_GENERAL_FAILURE;
        }

        CloseHandle(g_VBoxUsbGlobal.hNotifyEvent);
        g_VBoxUsbGlobal.hNotifyEvent = NULL;
    }
    else
    {
        DWORD dwErr = GetLastError(); NOREF(dwErr);
        AssertMsgFailed(("CreateEvent for NotifyEvent failed dwErr(%d)\n", dwErr));
        rc = VERR_GENERAL_FAILURE;
    }

    /* since main calls us even if USBLibInit fails,
     * we use hMonitor == INVALID_HANDLE_VALUE as a marker to indicate whether the lib is inited */

    Assert(RT_FAILURE(rc));
    return rc;
}


/**
 * Terminate the USB library
 *
 * @returns VBox status code.
 */
USBLIB_DECL(int) USBLibTerm(void)
{
    if (g_VBoxUsbGlobal.hMonitor == INVALID_HANDLE_VALUE)
    {
        Assert(g_VBoxUsbGlobal.hInterruptEvent == NULL);
        Assert(g_VBoxUsbGlobal.hNotifyEvent == NULL);
        return VINF_SUCCESS;
    }

    BOOL fRc;
#ifdef VBOX_USB_USE_DEVICE_NOTIFICATION
    fRc = PostMessage(g_VBoxUsbGlobal.hWnd, WM_CLOSE, 0, 0);
    AssertMsg(fRc, ("PostMessage for hWnd failed dwErr(%d)\n", GetLastError()));

    if (g_VBoxUsbGlobal.hThread != NULL)
    {
        DWORD dwResult = WaitForSingleObject(g_VBoxUsbGlobal.hThread, INFINITE);
        Assert(dwResult == WAIT_OBJECT_0); NOREF(dwResult);
        fRc = CloseHandle(g_VBoxUsbGlobal.hThread);
        AssertMsg(fRc, ("CloseHandle for hThread failed dwErr(%d)\n", GetLastError()));
    }

    if (g_VBoxUsbGlobal.hTimer)
    {
        fRc = DeleteTimerQueueTimer(g_VBoxUsbGlobal.hTimerQueue, g_VBoxUsbGlobal.hTimer,
                                    INVALID_HANDLE_VALUE); /* <-- to block until the timer is completed */
        AssertMsg(fRc, ("DeleteTimerQueueTimer failed dwErr(%d)\n", GetLastError()));
    }

    if (g_VBoxUsbGlobal.hTimerQueue)
    {
        fRc = DeleteTimerQueueEx(g_VBoxUsbGlobal.hTimerQueue,
                                 INVALID_HANDLE_VALUE); /* <-- to block until all timers are completed */
        AssertMsg(fRc, ("DeleteTimerQueueEx failed dwErr(%d)\n", GetLastError()));
    }
#endif /* VBOX_USB_USE_DEVICE_NOTIFICATION */

    fRc = CloseHandle(g_VBoxUsbGlobal.hMonitor);
    AssertMsg(fRc, ("CloseHandle for hMonitor failed dwErr(%d)\n", GetLastError()));
    g_VBoxUsbGlobal.hMonitor = INVALID_HANDLE_VALUE;

    fRc = CloseHandle(g_VBoxUsbGlobal.hInterruptEvent);
    AssertMsg(fRc, ("CloseHandle for hInterruptEvent failed lasterr=%u\n", GetLastError()));
    g_VBoxUsbGlobal.hInterruptEvent = NULL;

    fRc = CloseHandle(g_VBoxUsbGlobal.hNotifyEvent);
    AssertMsg(fRc, ("CloseHandle for hNotifyEvent failed dwErr(%d)\n", GetLastError()));
    g_VBoxUsbGlobal.hNotifyEvent = NULL;

    return VINF_SUCCESS;
}

