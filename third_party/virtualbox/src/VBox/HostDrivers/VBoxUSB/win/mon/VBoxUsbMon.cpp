/* $Id: VBoxUsbMon.cpp $ */
/** @file
 * VBox USB Monitor
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
#include "VBoxUsbMon.h"
#include "../cmn/VBoxUsbIdc.h"
#include <vbox/err.h>
#include <VBox/usblib.h>
#include <excpt.h>
#include <stdio.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/*
 * Note: Must match the VID & PID in the USB driver .inf file!!
 */
/*
  BusQueryDeviceID USB\Vid_80EE&Pid_CAFE
  BusQueryInstanceID 2
  BusQueryHardwareIDs USB\Vid_80EE&Pid_CAFE&Rev_0100
  BusQueryHardwareIDs USB\Vid_80EE&Pid_CAFE
  BusQueryCompatibleIDs USB\Class_ff&SubClass_00&Prot_00
  BusQueryCompatibleIDs USB\Class_ff&SubClass_00
  BusQueryCompatibleIDs USB\Class_ff
*/

#define szBusQueryDeviceId       L"USB\\Vid_80EE&Pid_CAFE"
#define szBusQueryHardwareIDs    L"USB\\Vid_80EE&Pid_CAFE&Rev_0100\0USB\\Vid_80EE&Pid_CAFE\0\0"
#define szBusQueryCompatibleIDs  L"USB\\Class_ff&SubClass_00&Prot_00\0USB\\Class_ff&SubClass_00\0USB\\Class_ff\0\0"

#define szDeviceTextDescription          L"VirtualBox USB"


#define VBOXUSBMON_MEMTAG 'MUBV'


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct VBOXUSBMONINS
{
    void * pvDummy;
} VBOXUSBMONINS, *PVBOXUSBMONINS;

typedef struct VBOXUSBMONCTX
{
    VBOXUSBFLTCTX FltCtx;
} VBOXUSBMONCTX, *PVBOXUSBMONCTX;

typedef struct VBOXUSBHUB_PNPHOOK
{
    VBOXUSBHOOK_ENTRY Hook;
    bool fUninitFailed;
} VBOXUSBHUB_PNPHOOK, *PVBOXUSBHUB_PNPHOOK;

typedef struct VBOXUSBHUB_PNPHOOK_COMPLETION
{
    VBOXUSBHOOK_REQUEST Rq;
} VBOXUSBHUB_PNPHOOK_COMPLETION, *PVBOXUSBHUB_PNPHOOK_COMPLETION;

/*
 * Comment out VBOX_USB3PORT definition to disable hooking to multiple drivers (#6509)
 */
#define VBOX_USB3PORT

#ifdef VBOX_USB3PORT
#define VBOXUSBMON_MAXDRIVERS 5
typedef struct VBOXUSB_PNPDRIVER
{
    PDRIVER_OBJECT     DriverObject;
    VBOXUSBHUB_PNPHOOK UsbHubPnPHook;
    PDRIVER_DISPATCH   pfnHookStub;
} VBOXUSB_PNPDRIVER, *PVBOXUSB_PNPDRIVER;
#endif /* !VBOX_USB3PORT */

typedef struct VBOXUSBMONGLOBALS
{
    PDEVICE_OBJECT pDevObj;
#ifdef VBOX_USB3PORT
    VBOXUSB_PNPDRIVER pDrivers[VBOXUSBMON_MAXDRIVERS];
#else /* !VBOX_USB3PORT */
    VBOXUSBHUB_PNPHOOK UsbHubPnPHook;
#endif /* !VBOX_USB3PORT */
    KEVENT OpenSynchEvent;
    IO_REMOVE_LOCK RmLock;
    uint32_t cOpens;
    volatile LONG ulPreventUnloadOn;
    PFILE_OBJECT pPreventUnloadFileObj;
} VBOXUSBMONGLOBALS, *PVBOXUSBMONGLOBALS;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static VBOXUSBMONGLOBALS g_VBoxUsbMonGlobals;



PVOID VBoxUsbMonMemAlloc(SIZE_T cbBytes)
{
    PVOID pvMem = ExAllocatePoolWithTag(NonPagedPool, cbBytes, VBOXUSBMON_MEMTAG);
    Assert(pvMem);
    return pvMem;
}

PVOID VBoxUsbMonMemAllocZ(SIZE_T cbBytes)
{
    PVOID pvMem = VBoxUsbMonMemAlloc(cbBytes);
    if (pvMem)
    {
        RtlZeroMemory(pvMem, cbBytes);
    }
    return pvMem;
}

VOID VBoxUsbMonMemFree(PVOID pvMem)
{
    ExFreePoolWithTag(pvMem, VBOXUSBMON_MEMTAG);
}

#define VBOXUSBDBG_STRCASE(_t) \
        case _t: return #_t
#define VBOXUSBDBG_STRCASE_UNKNOWN(_v) \
        default: LOG((__FUNCTION__": Unknown Value (0n%d), (0x%x)", _v, _v)); return "Unknown"

static const char* vboxUsbDbgStrPnPMn(UCHAR uMn)
{
    switch (uMn)
    {
        VBOXUSBDBG_STRCASE(IRP_MN_START_DEVICE);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_REMOVE_DEVICE);
        VBOXUSBDBG_STRCASE(IRP_MN_REMOVE_DEVICE);
        VBOXUSBDBG_STRCASE(IRP_MN_CANCEL_REMOVE_DEVICE);
        VBOXUSBDBG_STRCASE(IRP_MN_STOP_DEVICE);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_STOP_DEVICE);
        VBOXUSBDBG_STRCASE(IRP_MN_CANCEL_STOP_DEVICE);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_DEVICE_RELATIONS);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_INTERFACE);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_CAPABILITIES);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_RESOURCES);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_RESOURCE_REQUIREMENTS);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_DEVICE_TEXT);
        VBOXUSBDBG_STRCASE(IRP_MN_FILTER_RESOURCE_REQUIREMENTS);
        VBOXUSBDBG_STRCASE(IRP_MN_READ_CONFIG);
        VBOXUSBDBG_STRCASE(IRP_MN_WRITE_CONFIG);
        VBOXUSBDBG_STRCASE(IRP_MN_EJECT);
        VBOXUSBDBG_STRCASE(IRP_MN_SET_LOCK);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_ID);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_PNP_DEVICE_STATE);
        VBOXUSBDBG_STRCASE(IRP_MN_QUERY_BUS_INFORMATION);
        VBOXUSBDBG_STRCASE(IRP_MN_DEVICE_USAGE_NOTIFICATION);
        VBOXUSBDBG_STRCASE(IRP_MN_SURPRISE_REMOVAL);
        VBOXUSBDBG_STRCASE_UNKNOWN(uMn);
    }
}

void vboxUsbDbgPrintUnicodeString(PUNICODE_STRING pUnicodeString)
{
    RT_NOREF1(pUnicodeString);
    Log(("%.*ls", pUnicodeString->Length / 2, pUnicodeString->Buffer));
}

/**
 * Send IRP_MN_QUERY_DEVICE_RELATIONS
 *
 * @returns NT Status
 * @param   pDevObj         USB device pointer
 * @param   pFileObj        Valid file object pointer
 * @param   pDevRelations   Pointer to DEVICE_RELATIONS pointer (out)
 */
NTSTATUS VBoxUsbMonQueryBusRelations(PDEVICE_OBJECT pDevObj, PFILE_OBJECT pFileObj, PDEVICE_RELATIONS *pDevRelations)
{
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    NTSTATUS Status;
    PIRP pIrp;
    PIO_STACK_LOCATION pSl;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Assert(pDevRelations);
    *pDevRelations = NULL;

    pIrp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, pDevObj, NULL, 0, NULL, &Event, &IoStatus);
    if (!pIrp)
    {
        WARN(("IoBuildDeviceIoControlRequest failed!!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    pSl = IoGetNextIrpStackLocation(pIrp);
    pSl->MajorFunction = IRP_MJ_PNP;
    pSl->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    pSl->Parameters.QueryDeviceRelations.Type = BusRelations;
    pSl->FileObject = pFileObj;

    Status = IoCallDriver(pDevObj, pIrp);
    if (Status == STATUS_PENDING)
    {
        LOG(("IoCallDriver returned STATUS_PENDING!!"));
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (Status == STATUS_SUCCESS)
    {
        PDEVICE_RELATIONS pRel = (PDEVICE_RELATIONS)IoStatus.Information;
        LOG(("pRel = %p", pRel));
        if (VALID_PTR(pRel))
        {
            *pDevRelations = pRel;
        }
        else
        {
            WARN(("Invalid pointer %p", pRel));
        }
    }
    else
    {
        WARN(("IRP_MN_QUERY_DEVICE_RELATIONS failed Status(0x%x)", Status));
    }

    LOG(("IoCallDriver returned %x", Status));
    return Status;
}

RT_C_DECLS_BEGIN
/* these two come from IFS Kit, which is not included in 2K DDK we use,
 * although they are documented and exported in ntoskrnl,
 * and both should be present for >= XP according to MSDN */
NTKERNELAPI
NTSTATUS
ObQueryNameString(
    __in PVOID Object,
    __out_bcount_opt(Length) POBJECT_NAME_INFORMATION ObjectNameInfo,
    __in ULONG Length,
    __out PULONG ReturnLength
    );

NTKERNELAPI
PDEVICE_OBJECT
IoGetLowerDeviceObject(
    __in  PDEVICE_OBJECT  DeviceObject
    );

RT_C_DECLS_END

typedef DECLCALLBACK(VOID) FNVBOXUSBDEVNAMEMATCHER(PDEVICE_OBJECT pDo, PUNICODE_STRING pName, PVOID pvMatcher);
typedef FNVBOXUSBDEVNAMEMATCHER *PFNVBOXUSBDEVNAMEMATCHER;

static NTSTATUS vboxUsbObjCheckName(PDEVICE_OBJECT pDo, PFNVBOXUSBDEVNAMEMATCHER pfnMatcher, PVOID pvMatcher)
{
    union
    {
        OBJECT_NAME_INFORMATION Info;
        char buf[1024];
    } buf;
    ULONG cbLength = 0;

    POBJECT_NAME_INFORMATION pInfo = &buf.Info;
    NTSTATUS Status = ObQueryNameString(pDo, &buf.Info, sizeof (buf), &cbLength);
    if (!NT_SUCCESS(Status))
    {
        if (STATUS_INFO_LENGTH_MISMATCH != Status)
        {
            WARN(("ObQueryNameString failed 0x%x", Status));
            return Status;
        }

        LOG(("ObQueryNameString returned STATUS_INFO_LENGTH_MISMATCH, required size %d", cbLength));

        pInfo = (POBJECT_NAME_INFORMATION)VBoxUsbMonMemAlloc(cbLength);
        if (!pInfo)
        {
            WARN(("VBoxUsbMonMemAlloc failed"));
            return STATUS_NO_MEMORY;
        }
        Status = ObQueryNameString(pDo, pInfo, cbLength, &cbLength);
        if (!NT_SUCCESS(Status))
        {
            WARN(("ObQueryNameString second try failed 0x%x", Status));
            VBoxUsbMonMemFree(pInfo);
            return Status;
        }
    }

    /* we've got the name! */
    LOG(("got the name:"));
    LOG_USTR(&pInfo->Name);
    pfnMatcher(pDo, &pInfo->Name, pvMatcher);

    if (&buf.Info != pInfo)
    {
        LOG(("freeing allocated pInfo(0x%p)", pInfo));
        VBoxUsbMonMemFree(pInfo);
    }
    else
    {
        LOG(("no freeing info needed"));
    }

    return STATUS_SUCCESS;
}


typedef DECLCALLBACK(BOOLEAN) FNVBOXUSBDEVSTACKWALKER(PDEVICE_OBJECT pTopDo, PDEVICE_OBJECT pCurDo, PVOID pvContext);
typedef FNVBOXUSBDEVSTACKWALKER *PFNVBOXUSBDEVSTACKWALKER;

VOID vboxUsbObjDevStackWalk(PDEVICE_OBJECT pDo, PFNVBOXUSBDEVSTACKWALKER pfnWalker, PVOID pvWalker)
{
    LOG(("==>tree walk for Do 0x%p", pDo));
    PDEVICE_OBJECT pCurDo = pDo;
    ObReferenceObject(pCurDo); /* <- to make sure the dereferencing logic below works correctly */
    do
    {
        LOG(("==Do 0x%p", pCurDo));
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
        {
            union
            {
                OBJECT_NAME_INFORMATION Info;
                char buf[1024];
            } buf;
            ULONG cbLength = 0;

            NTSTATUS tmpStatus = ObQueryNameString(pCurDo, &buf.Info, sizeof (buf), &cbLength);
            if (NT_SUCCESS(tmpStatus))
            {
                LOG(("  Obj name:"));
                LOG_USTR(&buf.Info.Name);
            }
            else
            {
                if (STATUS_INFO_LENGTH_MISMATCH != tmpStatus)
                {
                    WARN(("ObQueryNameString failed 0x%x", tmpStatus));
                }
                else
                {
                    WARN(("ObQueryNameString STATUS_INFO_LENGTH_MISMATCH, required %d", cbLength));
                }
            }

            if (pCurDo->DriverObject
                && pCurDo->DriverObject->DriverName.Buffer
                && pCurDo->DriverObject->DriverName.Length)
            {
                LOG(("  Drv Obj(0x%p), name:", pCurDo->DriverObject));
                LOG_USTR(&pCurDo->DriverObject->DriverName);
            }
            else
            {
                LOG(("  No Drv Name, Drv Obj(0x%p)", pCurDo->DriverObject));
                if (pCurDo->DriverObject)
                {
                    LOG(("  driver name is zero, Length(%d), Buffer(0x%p)",
                                pCurDo->DriverObject->DriverName.Length, pCurDo->DriverObject->DriverName.Buffer));
                }
                else
                {
                    LOG(("  driver object is NULL"));
                }
            }
        }
#endif
        if (!pfnWalker(pDo, pCurDo, pvWalker))
        {
            LOG(("the walker said to stop"));
            ObDereferenceObject(pCurDo);
            break;
        }

        PDEVICE_OBJECT pLowerDo = IoGetLowerDeviceObject(pCurDo);
        ObDereferenceObject(pCurDo);
        if (!pLowerDo)
        {
            LOG(("IoGetLowerDeviceObject returnned NULL, stop"));
            break;
        }
        pCurDo = pLowerDo;
    } while (1);

    LOG(("<==tree walk"));
}

static DECLCALLBACK(BOOLEAN) vboxUsbObjNamePrefixMatch(PUNICODE_STRING pName, PUNICODE_STRING pNamePrefix, BOOLEAN fCaseInSensitive)
{
    LOG(("Matching prefix:"));
    LOG_USTR(pNamePrefix);
    if (pNamePrefix->Length > pName->Length)
    {
        LOG(("Pregix Length(%d) > Name Length(%d)", pNamePrefix->Length, pName->Length));
        return FALSE;
    }

    LOG(("Pregix Length(%d) <= Name Length(%d)", pNamePrefix->Length, pName->Length));

    UNICODE_STRING NamePrefix = *pName;
    NamePrefix.Length = pNamePrefix->Length;
    LONG rc = RtlCompareUnicodeString(&NamePrefix, pNamePrefix, fCaseInSensitive);

    if (!rc)
    {
        LOG(("prefix MATCHED!"));
        return TRUE;
    }

    LOG(("prefix NOT matched!"));
    return FALSE;
}

typedef struct VBOXUSBOBJNAMEPREFIXMATCHER
{
    PUNICODE_STRING pNamePrefix;
    BOOLEAN fMatched;
} VBOXUSBOBJNAMEPREFIXMATCHER, *PVBOXUSBOBJNAMEPREFIXMATCHER;

static DECLCALLBACK(VOID) vboxUsbObjDevNamePrefixMatcher(PDEVICE_OBJECT pDo, PUNICODE_STRING pName, PVOID pvMatcher)
{
    RT_NOREF1(pDo);
    PVBOXUSBOBJNAMEPREFIXMATCHER pData = (PVBOXUSBOBJNAMEPREFIXMATCHER)pvMatcher;
    PUNICODE_STRING pNamePrefix = pData->pNamePrefix;
    ASSERT_WARN(!pData->fMatched, ("match flag already set!"));
    pData->fMatched = vboxUsbObjNamePrefixMatch(pName, pNamePrefix, TRUE /* fCaseInSensitive */);
    LOG(("match result (%d)", (int)pData->fMatched));
}

typedef struct VBOXUSBOBJDRVOBJSEARCHER
{
    PDEVICE_OBJECT pDevObj;
    PUNICODE_STRING pDrvName;
    PUNICODE_STRING pPdoNamePrefix;
    ULONG fFlags;
} VBOXUSBOBJDRVOBJSEARCHER, *PVBOXUSBOBJDRVOBJSEARCHER;

static DECLCALLBACK(BOOLEAN) vboxUsbObjDevObjSearcherWalker(PDEVICE_OBJECT pTopDo, PDEVICE_OBJECT pCurDo, PVOID pvContext)
{
    RT_NOREF1(pTopDo);
    PVBOXUSBOBJDRVOBJSEARCHER pData = (PVBOXUSBOBJDRVOBJSEARCHER)pvContext;
    ASSERT_WARN(!pData->pDevObj, ("non-null dev object (0x%p) on enter", pData->pDevObj));
    pData->pDevObj = NULL;
    if (pCurDo->DriverObject
        && pCurDo->DriverObject->DriverName.Buffer
        && pCurDo->DriverObject->DriverName.Length
        && !RtlCompareUnicodeString(pData->pDrvName, &pCurDo->DriverObject->DriverName, TRUE /* case insensitive */))
    {
        LOG(("MATCHED driver:"));
        LOG_USTR(&pCurDo->DriverObject->DriverName);
        if ((pData->fFlags & VBOXUSBMONHUBWALK_F_ALL) != VBOXUSBMONHUBWALK_F_ALL)
        {
            VBOXUSBOBJNAMEPREFIXMATCHER Data = {0};
            Data.pNamePrefix = pData->pPdoNamePrefix;
            NTSTATUS Status = vboxUsbObjCheckName(pCurDo, vboxUsbObjDevNamePrefixMatcher, &Data);
            if (!NT_SUCCESS(Status))
            {
                WARN(("vboxUsbObjCheckName failed Status (0x%x)", Status));
                return TRUE;
            }


            LOG(("prefix match result (%d)", Data.fMatched));
            if ((pData->fFlags & VBOXUSBMONHUBWALK_F_FDO) == VBOXUSBMONHUBWALK_F_FDO)
            {
                LOG(("VBOXUSBMONHUBWALK_F_FDO"));
                if (Data.fMatched)
                {
                    LOG(("this is a PDO object, skip it and stop search"));
                    /* stop search as we will not find FDO here */
                    return FALSE;
                }

                LOG(("this is a FDO object, MATCHED!!"));
            }
            else if ((pData->fFlags & VBOXUSBMONHUBWALK_F_PDO) == VBOXUSBMONHUBWALK_F_PDO)
            {
                LOG(("VBOXUSBMONHUBWALK_F_PDO"));
                if (!Data.fMatched)
                {
                    LOG(("this is a FDO object, skip it and continue search"));
                    /* continue seach since since this could be a nested hub that would have a usbhub-originated PDO */
                    return TRUE;
                }

                LOG(("this is a PDO object, MATCHED!!"));
            }

        }
        else
        {
            LOG(("VBOXUSBMONHUBWALK_F_ALL"));
            LOG(("either PDO or FDO, MATCHED!!"));
        }

        /* ensure the dev object is not destroyed */
        ObReferenceObject(pCurDo);
        pData->pDevObj = pCurDo;
        /* we are done */
        return FALSE;
    }
    else
    {
        LOG(("driver object (0x%p) no match", pCurDo->DriverObject));
        if (pCurDo->DriverObject)
        {
            if (   pCurDo->DriverObject->DriverName.Buffer
                && pCurDo->DriverObject->DriverName.Length)
            {
                LOG(("driver name not match, was:"));
                LOG_USTR(&pCurDo->DriverObject->DriverName);
                LOG(("but expected:"));
                LOG_USTR(pData->pDrvName);
            }
            else
            {
                LOG(("driver name is zero, Length(%d), Buffer(0x%p)",
                        pCurDo->DriverObject->DriverName.Length, pCurDo->DriverObject->DriverName.Buffer));
            }
        }
        else
        {
            LOG(("driver object is NULL"));
        }
    }
    return TRUE;
}

VOID vboxUsbMonHubDevWalk(PFNVBOXUSBMONDEVWALKER pfnWalker, PVOID pvWalker, ULONG fFlags)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
#ifndef VBOX_USB3PORT
    UNICODE_STRING szStandardHubName;
    PDRIVER_OBJECT pDrvObj = NULL;
    szStandardHubName.Length = 0;
    szStandardHubName.MaximumLength = 0;
    szStandardHubName.Buffer = 0;
    RtlInitUnicodeString(&szStandardHubName, L"\\Driver\\usbhub");
    UNICODE_STRING szStandardHubPdoNamePrefix;
    szStandardHubPdoNamePrefix.Length = 0;
    szStandardHubPdoNamePrefix.MaximumLength = 0;
    szStandardHubPdoNamePrefix.Buffer = 0;
    RtlInitUnicodeString(&szStandardHubPdoNamePrefix, L"\\Device\\USBPDO-");

    for (int i = 0; i < 16; i++)
    {
        WCHAR           szwHubName[32] = {0};
        char            szHubName[32] = {0};
        ANSI_STRING     AnsiName;
        UNICODE_STRING  UnicodeName;
        PDEVICE_OBJECT  pHubDevObj;
        PFILE_OBJECT    pHubFileObj;

        sprintf(szHubName, "\\Device\\USBPDO-%d", i);

        RtlInitAnsiString(&AnsiName, szHubName);

        UnicodeName.Length = 0;
        UnicodeName.MaximumLength = sizeof (szwHubName);
        UnicodeName.Buffer = szwHubName;

        RtlInitAnsiString(&AnsiName, szHubName);
        Status = RtlAnsiStringToUnicodeString(&UnicodeName, &AnsiName, FALSE);
        if (Status == STATUS_SUCCESS)
        {
            Status = IoGetDeviceObjectPointer(&UnicodeName, FILE_READ_DATA, &pHubFileObj, &pHubDevObj);
            if (Status == STATUS_SUCCESS)
            {
                LOG(("IoGetDeviceObjectPointer for \\Device\\USBPDO-%d returned %p %p", i, pHubDevObj, pHubFileObj));

                VBOXUSBOBJDRVOBJSEARCHER Data = {0};
                Data.pDrvName = &szStandardHubName;
                Data.pPdoNamePrefix = &szStandardHubPdoNamePrefix;
                Data.fFlags = fFlags;

                vboxUsbObjDevStackWalk(pHubDevObj, vboxUsbObjDevObjSearcherWalker, &Data);
                if (Data.pDevObj)
                {
                    LOG(("found hub dev obj (0x%p)", Data.pDevObj));
                    if (!pfnWalker(pHubFileObj, pHubDevObj, Data.pDevObj, pvWalker))
                    {
                        LOG(("the walker said to stop"));
                        ObDereferenceObject(Data.pDevObj);
                        ObDereferenceObject(pHubFileObj);
                        break;
                    }

                    LOG(("going forward.."));
                    ObDereferenceObject(Data.pDevObj);
                }
                else
                {
                    LOG(("no hub driver obj found"));
                    ASSERT_WARN(!Data.pDevObj, ("non-null dev obj poiter returned (0x%p)", Data.pDevObj));
                }

                /* this will dereference both file and dev obj */
                ObDereferenceObject(pHubFileObj);
            }
            else
            {
                LOG(("IoGetDeviceObjectPointer returned Status (0x%x) for (\\Device\\USBPDO-%d)", Status, i));
            }
        }
        else
        {
            WARN(("RtlAnsiStringToUnicodeString failed, Status (0x%x) for Ansu name (\\Device\\USBPDO-%d)", Status, i));
        }
    }
#else /* VBOX_USB3PORT */
    RT_NOREF1(fFlags);
    PWSTR szwHubList;
    Status = IoGetDeviceInterfaces(&GUID_DEVINTERFACE_USB_HUB, NULL, 0, &szwHubList);
    if (Status != STATUS_SUCCESS)
    {
        LOG(("IoGetDeviceInterfaces failed with %d\n", Status));
        return;
    }
    if (szwHubList)
    {
        UNICODE_STRING  UnicodeName;
        PDEVICE_OBJECT  pHubDevObj;
        PFILE_OBJECT    pHubFileObj;
        PWSTR           szwHubName = szwHubList;
        while (*szwHubName != UNICODE_NULL)
        {
            RtlInitUnicodeString(&UnicodeName, szwHubName);
            Status = IoGetDeviceObjectPointer(&UnicodeName, FILE_READ_DATA, &pHubFileObj, &pHubDevObj);
            if (Status == STATUS_SUCCESS)
            {
                /* We could not log hub name here.
                 * It is the paged memory and we cannot use it in logger cause it increases the IRQL
                 */
                LOG(("IoGetDeviceObjectPointer returned %p %p", pHubDevObj, pHubFileObj));
                if (!pfnWalker(pHubFileObj, pHubDevObj, pHubDevObj, pvWalker))
                {
                    LOG(("the walker said to stop"));
                    ObDereferenceObject(pHubFileObj);
                    break;
                }

                LOG(("going forward.."));
                ObDereferenceObject(pHubFileObj);
            }
            szwHubName += wcslen(szwHubName) + 1;
        }
        ExFreePool(szwHubList);
    }
#endif /* VBOX_USB3PORT */
}

typedef struct VBOXUSBMONFINDHUBWALKER
{
    PDRIVER_OBJECT pDrvObj;
} VBOXUSBMONFINDHUBWALKER, *PVBOXUSBMONFINDHUBWALKER;

static DECLCALLBACK(BOOLEAN) vboxUsbMonFindHubDrvObjWalker(PFILE_OBJECT pFile, PDEVICE_OBJECT pTopDo, PDEVICE_OBJECT pHubDo, PVOID pvContext)
{
    RT_NOREF2(pFile, pTopDo);
    PVBOXUSBMONFINDHUBWALKER pData = (PVBOXUSBMONFINDHUBWALKER)pvContext;
    PDRIVER_OBJECT pDrvObj = pHubDo->DriverObject;

    ASSERT_WARN(!pData->pDrvObj, ("pDrvObj expected null on enter, but was(0x%p)", pData->pDrvObj));
    if (pDrvObj)
    {
        LOG(("found driver object 0x%p", pDrvObj));
        ObReferenceObject(pDrvObj);
        pData->pDrvObj = pDrvObj;
        return FALSE;
    }

    WARN(("null pDrvObj!"));
    return TRUE;
}

static PDRIVER_OBJECT vboxUsbMonHookFindHubDrvObj()
{
    UNICODE_STRING szStandardHubName;
    szStandardHubName.Length = 0;
    szStandardHubName.MaximumLength = 0;
    szStandardHubName.Buffer = 0;
    RtlInitUnicodeString(&szStandardHubName, L"\\Driver\\usbhub");

    LOG(("Search USB hub"));
    VBOXUSBMONFINDHUBWALKER Data = {0};
    vboxUsbMonHubDevWalk(vboxUsbMonFindHubDrvObjWalker, &Data, VBOXUSBMONHUBWALK_F_ALL);
    if (Data.pDrvObj)
        LOG(("returning driver object 0x%p", Data.pDrvObj));
    else
        WARN(("no hub driver object found!"));
    return Data.pDrvObj;
}

/* NOTE: the stack location data is not the "actual" IRP stack location,
 * but a copy being preserved on the IRP way down.
 * See the note in VBoxUsbPnPCompletion for detail */
static NTSTATUS vboxUsbMonHandlePnPIoctl(PDEVICE_OBJECT pDevObj, PIO_STACK_LOCATION pSl, PIO_STATUS_BLOCK pIoStatus)
{
    LOG(("IRQL = %d", KeGetCurrentIrql()));
    switch(pSl->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_TEXT:
        {
            LOG(("IRP_MN_QUERY_DEVICE_TEXT: pIoStatus->Status = %x", pIoStatus->Status));
            if (pIoStatus->Status == STATUS_SUCCESS)
            {
                WCHAR *pId = (WCHAR *)pIoStatus->Information;
                if (VALID_PTR(pId))
                {
                    KIRQL Iqrl = KeGetCurrentIrql();
                    /* IRQL should be always passive here */
                    ASSERT_WARN(Iqrl == PASSIVE_LEVEL, ("irql is not PASSIVE"));
                    switch(pSl->Parameters.QueryDeviceText.DeviceTextType)
                    {
                        case DeviceTextLocationInformation:
                            LOG(("DeviceTextLocationInformation"));
                            LOG_STRW(pId);
                            break;

                        case DeviceTextDescription:
                            LOG(("DeviceTextDescription"));
                            LOG_STRW(pId);
                            if (VBoxUsbFltPdoIsFiltered(pDevObj))
                            {
                                LOG(("PDO (0x%p) is filtered", pDevObj));
                                WCHAR *pId = (WCHAR *)ExAllocatePool(PagedPool, sizeof(szDeviceTextDescription));
                                if (!pId)
                                {
                                    AssertFailed();
                                    break;
                                }
                                memcpy(pId, szDeviceTextDescription, sizeof(szDeviceTextDescription));
                                LOG(("NEW szDeviceTextDescription"));
                                LOG_STRW(pId);
                                ExFreePool((PVOID)pIoStatus->Information);
                                pIoStatus->Information = (ULONG_PTR)pId;
                            }
                            else
                            {
                                LOG(("PDO (0x%p) is NOT filtered", pDevObj));
                            }
                            break;
                        default:
                            LOG(("DeviceText %d", pSl->Parameters.QueryDeviceText.DeviceTextType));
                            break;
                    }
                }
                else
                    LOG(("Invalid pointer %p", pId));
            }
            break;
        }

        case IRP_MN_QUERY_ID:
        {
            LOG(("IRP_MN_QUERY_ID: Irp->pIoStatus->Status = %x", pIoStatus->Status));
            if (pIoStatus->Status == STATUS_SUCCESS &&  pDevObj)
            {
                WCHAR *pId = (WCHAR *)pIoStatus->Information;
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
                WCHAR *pTmp;
#endif
                if (VALID_PTR(pId))
                {
                    KIRQL Iqrl = KeGetCurrentIrql();
                    /* IRQL should be always passive here */
                    ASSERT_WARN(Iqrl == PASSIVE_LEVEL, ("irql is not PASSIVE"));

                    switch (pSl->Parameters.QueryId.IdType)
                    {
                        case BusQueryInstanceID:
                            LOG(("BusQueryInstanceID"));
                            LOG_STRW(pId);
                            break;

                        case BusQueryDeviceID:
                        {
                            LOG(("BusQueryDeviceID"));
                            pId = (WCHAR *)ExAllocatePool(PagedPool, sizeof(szBusQueryDeviceId));
                            if (!pId)
                            {
                                WARN(("ExAllocatePool failed"));
                                break;
                            }

                            BOOLEAN bFiltered = FALSE;
                            NTSTATUS Status = VBoxUsbFltPdoAdd(pDevObj, &bFiltered);
                            if (Status != STATUS_SUCCESS || !bFiltered)
                            {
                                if (Status == STATUS_SUCCESS)
                                {
                                    LOG(("PDO (0x%p) is NOT filtered", pDevObj));
                                }
                                else
                                {
                                    WARN(("VBoxUsbFltPdoAdd for PDO (0x%p) failed Status 0x%x", pDevObj, Status));
                                }
                                ExFreePool(pId);
                                break;
                            }

                            LOG(("PDO (0x%p) is filtered", pDevObj));
                            ExFreePool((PVOID)pIoStatus->Information);
                            memcpy(pId, szBusQueryDeviceId, sizeof(szBusQueryDeviceId));
                            pIoStatus->Information = (ULONG_PTR)pId;
                            break;
                        }
                    case BusQueryHardwareIDs:
                    {
                        LOG(("BusQueryHardwareIDs"));
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
                        while (*pId) //MULTI_SZ
                        {
                            LOG_STRW(pId);
                            while (*pId) pId++;
                            pId++;
                        }
#endif
                        pId = (WCHAR *)ExAllocatePool(PagedPool, sizeof(szBusQueryHardwareIDs));
                        if (!pId)
                        {
                            WARN(("ExAllocatePool failed"));
                            break;
                        }

                        BOOLEAN bFiltered = FALSE;
                        NTSTATUS Status = VBoxUsbFltPdoAdd(pDevObj, &bFiltered);
                        if (Status != STATUS_SUCCESS || !bFiltered)
                        {
                            if (Status == STATUS_SUCCESS)
                            {
                                LOG(("PDO (0x%p) is NOT filtered", pDevObj));
                            }
                            else
                            {
                                WARN(("VBoxUsbFltPdoAdd for PDO (0x%p) failed Status 0x%x", pDevObj, Status));
                            }
                            ExFreePool(pId);
                            break;
                        }

                        LOG(("PDO (0x%p) is filtered", pDevObj));

                        memcpy(pId, szBusQueryHardwareIDs, sizeof(szBusQueryHardwareIDs));
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
                        LOG(("NEW BusQueryHardwareIDs"));
                        pTmp = pId;
                        while (*pTmp) //MULTI_SZ
                        {

                            LOG_STRW(pTmp);
                            while (*pTmp) pTmp++;
                            pTmp++;
                        }
#endif
                        ExFreePool((PVOID)pIoStatus->Information);
                        pIoStatus->Information = (ULONG_PTR)pId;
                        break;
                    }
                    case BusQueryCompatibleIDs:
                        LOG(("BusQueryCompatibleIDs"));
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
                        while (*pId) //MULTI_SZ
                        {
                            LOG_STRW(pId);
                            while (*pId) pId++;
                            pId++;
                        }
#endif
                        if (VBoxUsbFltPdoIsFiltered(pDevObj))
                        {
                            LOG(("PDO (0x%p) is filtered", pDevObj));
                            pId = (WCHAR *)ExAllocatePool(PagedPool, sizeof(szBusQueryCompatibleIDs));
                            if (!pId)
                            {
                                WARN(("ExAllocatePool failed"));
                                break;
                            }
                            memcpy(pId, szBusQueryCompatibleIDs, sizeof(szBusQueryCompatibleIDs));
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
                            LOG(("NEW BusQueryCompatibleIDs"));
                            pTmp = pId;
                            while (*pTmp) //MULTI_SZ
                            {
                                LOG_STRW(pTmp);
                                while (*pTmp) pTmp++;
                                pTmp++;
                            }
#endif
                            ExFreePool((PVOID)pIoStatus->Information);
                            pIoStatus->Information = (ULONG_PTR)pId;
                        }
                        else
                        {
                            LOG(("PDO (0x%p) is NOT filtered", pDevObj));
                        }
                        break;

                        default:
                            /** @todo r=bird: handle BusQueryContainerID and whatever else we might see  */
                            break;
                    }
                }
                else
                {
                    LOG(("Invalid pointer %p", pId));
                }
            }
            break;
        }

#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            switch(pSl->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                    LOG(("BusRelations"));

                    if (pIoStatus->Status == STATUS_SUCCESS)
                    {
                        PDEVICE_RELATIONS pRel = (PDEVICE_RELATIONS)pIoStatus->Information;
                        LOG(("pRel = %p", pRel));
                        if (VALID_PTR(pRel))
                        {
                            for (unsigned i=0;i<pRel->Count;i++)
                            {
                                if (VBoxUsbFltPdoIsFiltered(pDevObj))
                                    LOG(("New PDO %p", pRel->Objects[i]));
                            }
                        }
                        else
                            LOG(("Invalid pointer %p", pRel));
                    }
                    break;
                case TargetDeviceRelation:
                    LOG(("TargetDeviceRelation"));
                    break;
                case RemovalRelations:
                    LOG(("RemovalRelations"));
                    break;
                case EjectionRelations:
                    LOG(("EjectionRelations"));
                    break;
                default:
                    LOG(("QueryDeviceRelations.Type=%d", pSl->Parameters.QueryDeviceRelations.Type));
            }
            break;
        }

        case IRP_MN_QUERY_CAPABILITIES:
        {
            LOG(("IRP_MN_QUERY_CAPABILITIES: pIoStatus->Status = %x", pIoStatus->Status));
            if (pIoStatus->Status == STATUS_SUCCESS)
            {
                PDEVICE_CAPABILITIES pCaps = pSl->Parameters.DeviceCapabilities.Capabilities;
                if (VALID_PTR(pCaps))
                {
                    LOG(("Caps.SilentInstall  = %d", pCaps->SilentInstall));
                    LOG(("Caps.UniqueID       = %d", pCaps->UniqueID ));
                    LOG(("Caps.Address        = %d", pCaps->Address ));
                    LOG(("Caps.UINumber       = %d", pCaps->UINumber ));
                }
                else
                    LOG(("Invalid pointer %p", pCaps));
            }
            break;
        }

        default:
            break;
#endif
    } /*switch */

    LOG(("Done returns %x (IRQL = %d)", pIoStatus->Status, KeGetCurrentIrql()));
    return pIoStatus->Status;
}

NTSTATUS _stdcall VBoxUsbPnPCompletion(DEVICE_OBJECT *pDevObj, IRP *pIrp, void *pvContext)
{
    LOG(("Completion PDO(0x%p), IRP(0x%p), Status(0x%x)", pDevObj, pIrp, pIrp->IoStatus.Status));
    ASSERT_WARN(pvContext, ("zero context"));

    PVBOXUSBHOOK_REQUEST pRequest = (PVBOXUSBHOOK_REQUEST)pvContext;
    /* NOTE: despite a regular IRP processing the stack location in our completion
     * differs from those of the PnP hook since the hook is invoked in the "context" of the calle,
     * while the completion is in the "coller" context in terms of IRP,
     * so the completion stack location is one level "up" here.
     *
     * Moreover we CAN NOT access irp stack location in the completion because we might not have one at all
     * in case the hooked driver is at the top of the irp call stack
     *
     * This is why we use the stack location we saved on IRP way down.
     * */
    PIO_STACK_LOCATION pSl = &pRequest->OldLocation;
    ASSERT_WARN(pIrp == pRequest->pIrp, ("completed IRP(0x%x) not match request IRP(0x%x)", pIrp, pRequest->pIrp));
    /* NOTE: we can not rely on pDevObj passed in IoCompletion since it may be zero
     * in case IRP was created with extra stack locations and the caller did not initialize
     * the IO_STACK_LOCATION::DeviceObject */
    DEVICE_OBJECT *pRealDevObj = pRequest->pDevObj;
//    Assert(!pDevObj || pDevObj == pRealDevObj);
//    Assert(pSl->DeviceObject == pDevObj);

    switch(pSl->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_TEXT:
        case IRP_MN_QUERY_ID:
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        case IRP_MN_QUERY_CAPABILITIES:
#endif
            if (NT_SUCCESS(pIrp->IoStatus.Status))
            {
                vboxUsbMonHandlePnPIoctl(pRealDevObj, pSl, &pIrp->IoStatus);
            }
            else
            {
                ASSERT_WARN(pIrp->IoStatus.Status == STATUS_NOT_SUPPORTED, ("Irp failed with status(0x%x)", pIrp->IoStatus.Status));
            }
            break;

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_REMOVE_DEVICE:
            if (NT_SUCCESS(pIrp->IoStatus.Status))
            {
                VBoxUsbFltPdoRemove(pRealDevObj);
            }
            else
            {
                AssertFailed();
            }
            break;

        /* These two IRPs are received when the PnP subsystem has determined the id of the newly arrived device */
        /* IRP_MN_START_DEVICE only arrives if it's a USB device of a known class or with a present host driver */
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        case IRP_MN_QUERY_RESOURCES:
            if (NT_SUCCESS(pIrp->IoStatus.Status) || pIrp->IoStatus.Status == STATUS_NOT_SUPPORTED)
            {
                VBoxUsbFltPdoAddCompleted(pRealDevObj);
            }
            else
            {
                AssertFailed();
            }
            break;

        default:
            break;
    }

    LOG(("<==PnP: Mn(%s), PDO(0x%p), IRP(0x%p), Status(0x%x), Sl PDO(0x%p), Compl PDO(0x%p)",
                            vboxUsbDbgStrPnPMn(pSl->MinorFunction),
                            pRealDevObj, pIrp, pIrp->IoStatus.Status,
                            pSl->DeviceObject, pDevObj));
#ifdef DEBUG_misha
    NTSTATUS tmpStatus = pIrp->IoStatus.Status;
#endif
#ifdef VBOX_USB3PORT
    PVBOXUSBHOOK_ENTRY pHook = pRequest->pHook;
#else /* !VBOX_USB3PORT */
    PVBOXUSBHOOK_ENTRY pHook = &g_VBoxUsbMonGlobals.UsbHubPnPHook.Hook;
#endif /* !VBOX_USB3PORT */
    NTSTATUS Status = VBoxUsbHookRequestComplete(pHook, pDevObj, pIrp, pRequest);
    VBoxUsbMonMemFree(pRequest);
#ifdef DEBUG_misha
    if (Status != STATUS_MORE_PROCESSING_REQUIRED)
    {
        Assert(pIrp->IoStatus.Status == tmpStatus);
    }
#endif
    VBoxUsbHookRelease(pHook);
    return Status;
}

/**
 * Device PnP hook
 *
 * @param   pDevObj     Device object.
 * @param   pIrp         Request packet.
 */
#ifdef VBOX_USB3PORT
static NTSTATUS vboxUsbMonPnPHook(IN PVBOXUSBHOOK_ENTRY pHook, IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
#else /* !VBOX_USB3PORT */
NTSTATUS _stdcall VBoxUsbMonPnPHook(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
#endif /* !VBOX_USB3PORT */
{
#ifndef VBOX_USB3PORT
    PVBOXUSBHOOK_ENTRY pHook = &g_VBoxUsbMonGlobals.UsbHubPnPHook.Hook;
#endif /* !VBOX_USB3PORT */
    LOG(("==>PnP: Mn(%s), PDO(0x%p), IRP(0x%p), Status(0x%x)", vboxUsbDbgStrPnPMn(IoGetCurrentIrpStackLocation(pIrp)->MinorFunction), pDevObj, pIrp, pIrp->IoStatus.Status));

    if (!VBoxUsbHookRetain(pHook))
    {
        WARN(("VBoxUsbHookRetain failed"));
        return VBoxUsbHookRequestPassDownHookSkip(pHook, pDevObj, pIrp);
    }

    PVBOXUSBHUB_PNPHOOK_COMPLETION pCompletion = (PVBOXUSBHUB_PNPHOOK_COMPLETION)VBoxUsbMonMemAlloc(sizeof (*pCompletion));
    if (!pCompletion)
    {
        WARN(("VBoxUsbMonMemAlloc failed"));
        VBoxUsbHookRelease(pHook);
        pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS Status = VBoxUsbHookRequestPassDownHookCompletion(pHook, pDevObj, pIrp, VBoxUsbPnPCompletion, &pCompletion->Rq);
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
    if (Status != STATUS_PENDING)
    {
        LOG(("Request completed, Status(0x%x)", Status));
        VBoxUsbHookVerifyCompletion(pHook, &pCompletion->Rq, pIrp);
    }
    else
    {
        LOG(("Request pending"));
    }
#endif
    return Status;
}

#ifdef VBOX_USB3PORT
/**
 * Device PnP hook stubs.
 *
 * @param   pDevObj     Device object.
 * @param   pIrp         Request packet.
 */
#define VBOX_PNPHOOKSTUB(n) NTSTATUS _stdcall VBoxUsbMonPnPHook##n(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp) \
{ \
    return vboxUsbMonPnPHook(&g_VBoxUsbMonGlobals.pDrivers[n].UsbHubPnPHook.Hook, pDevObj, pIrp); \
}

#define VBOX_PNPHOOKSTUB_INIT(n) g_VBoxUsbMonGlobals.pDrivers[n].pfnHookStub = VBoxUsbMonPnPHook##n

VBOX_PNPHOOKSTUB(0)
VBOX_PNPHOOKSTUB(1)
VBOX_PNPHOOKSTUB(2)
VBOX_PNPHOOKSTUB(3)
VBOX_PNPHOOKSTUB(4)
AssertCompile(VBOXUSBMON_MAXDRIVERS == 5);

typedef struct VBOXUSBMONHOOKDRIVERWALKER
{
    PDRIVER_OBJECT pDrvObj;
} VBOXUSBMONHOOKDRIVERWALKER, *PVBOXUSBMONHOOKDRIVERWALKER;

/**
 * Logs an error to the system event log.
 *
 * @param   ErrCode        Error to report to event log.
 * @param   ReturnedStatus Error that was reported by the driver to the caller.
 * @param   uErrId         Unique error id representing the location in the driver.
 * @param   cbDumpData     Number of bytes at pDumpData.
 * @param   pDumpData      Pointer to data that will be added to the message (see 'details' tab).
 */
static void vboxUsbMonLogError(NTSTATUS ErrCode, NTSTATUS ReturnedStatus, ULONG uErrId, USHORT cbDumpData, PVOID pDumpData)
{
    PIO_ERROR_LOG_PACKET pErrEntry;


    /* Truncate dumps that do not fit into IO_ERROR_LOG_PACKET. */
    if (FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + cbDumpData > ERROR_LOG_MAXIMUM_SIZE)
        cbDumpData = ERROR_LOG_MAXIMUM_SIZE - FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);

    pErrEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(g_VBoxUsbMonGlobals.pDevObj,
                                                              FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + cbDumpData);
    if (pErrEntry)
    {
        uint8_t *pDump = (uint8_t *)pErrEntry->DumpData;
        if (cbDumpData)
            memcpy(pDump, pDumpData, cbDumpData);
        pErrEntry->MajorFunctionCode = 0;
        pErrEntry->RetryCount = 0;
        pErrEntry->DumpDataSize = cbDumpData;
        pErrEntry->NumberOfStrings = 0;
        pErrEntry->StringOffset = 0;
        pErrEntry->ErrorCode = ErrCode;
        pErrEntry->UniqueErrorValue = uErrId;
        pErrEntry->FinalStatus = ReturnedStatus;
        pErrEntry->IoControlCode = 0;
        IoWriteErrorLogEntry(pErrEntry);
    }
    else
    {
        LOG(("Failed to allocate error log entry (cb=%d)\n", FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + cbDumpData));
    }
}

static DECLCALLBACK(BOOLEAN) vboxUsbMonHookDrvObjWalker(PFILE_OBJECT pFile, PDEVICE_OBJECT pTopDo, PDEVICE_OBJECT pHubDo, PVOID pvContext)
{
    RT_NOREF3(pFile, pTopDo, pvContext);
    PDRIVER_OBJECT pDrvObj = pHubDo->DriverObject;

    /* First we try to figure out if we are already hooked to this driver. */
    for (int i = 0; i < VBOXUSBMON_MAXDRIVERS; i++)
        if (pDrvObj == g_VBoxUsbMonGlobals.pDrivers[i].DriverObject)
        {
            LOG(("Found %p at pDrivers[%d]\n", pDrvObj, i));
            /* We've already hooked to this one -- nothing to do. */
            return TRUE;
        }
    /* We are not hooked yet, find an empty slot. */
    for (int i = 0; i < VBOXUSBMON_MAXDRIVERS; i++)
    {
        if (!g_VBoxUsbMonGlobals.pDrivers[i].DriverObject)
        {
            /* Found an emtpy slot, use it. */
            g_VBoxUsbMonGlobals.pDrivers[i].DriverObject = pDrvObj;
            ObReferenceObject(pDrvObj);
            LOG(("pDrivers[%d] = %p, installing the hook...\n", i, pDrvObj));
            VBoxUsbHookInit(&g_VBoxUsbMonGlobals.pDrivers[i].UsbHubPnPHook.Hook,
                            pDrvObj,
                            IRP_MJ_PNP,
                            g_VBoxUsbMonGlobals.pDrivers[i].pfnHookStub);
            VBoxUsbHookInstall(&g_VBoxUsbMonGlobals.pDrivers[i].UsbHubPnPHook.Hook);
            return TRUE; /* Must continue to find all drivers. */
        }
        if (pDrvObj == g_VBoxUsbMonGlobals.pDrivers[i].DriverObject)
        {
            LOG(("Found %p at pDrivers[%d]\n", pDrvObj, i));
            /* We've already hooked to this one -- nothing to do. */
            return TRUE;
        }
    }
    /* No empty slots! No reason to continue. */
    LOG(("No empty slots!\n"));
    ANSI_STRING ansiDrvName;
    NTSTATUS Status = RtlUnicodeStringToAnsiString(&ansiDrvName, &pDrvObj->DriverName, true);
    if (Status != STATUS_SUCCESS)
    {
        ansiDrvName.Length = 0;
        LOG(("RtlUnicodeStringToAnsiString failed with 0x%x", Status));
    }
    vboxUsbMonLogError(IO_ERR_INSUFFICIENT_RESOURCES, STATUS_SUCCESS, 1, ansiDrvName.Length, ansiDrvName.Buffer);
    if (Status == STATUS_SUCCESS)
        RtlFreeAnsiString(&ansiDrvName);
    return FALSE;
}

/**
 * Finds all USB drivers in the system and installs hooks if haven't done already.
 */
static NTSTATUS vboxUsbMonInstallAllHooks()
{
    vboxUsbMonHubDevWalk(vboxUsbMonHookDrvObjWalker, NULL, VBOXUSBMONHUBWALK_F_ALL);
    return STATUS_SUCCESS;
}
#endif /* VBOX_USB3PORT */

static NTSTATUS vboxUsbMonHookCheckInit()
{
    static bool fIsHookInited = false;
    if (fIsHookInited)
    {
        LOG(("hook inited already, success"));
        return STATUS_SUCCESS;
    }
#ifdef VBOX_USB3PORT
    return vboxUsbMonInstallAllHooks();
#else /* !VBOX_USB3PORT */
    PDRIVER_OBJECT pDrvObj = vboxUsbMonHookFindHubDrvObj();
    if (pDrvObj)
    {
        VBoxUsbHookInit(&g_VBoxUsbMonGlobals.UsbHubPnPHook.Hook, pDrvObj, IRP_MJ_PNP, VBoxUsbMonPnPHook);
        fIsHookInited = true;
        LOG(("SUCCESS"));
        return STATUS_SUCCESS;
    }
    WARN(("hub drv obj not found, fail"));
    return STATUS_UNSUCCESSFUL;
#endif /* !VBOX_USB3PORT */
}

static NTSTATUS vboxUsbMonHookInstall()
{
#ifdef VBOX_USB3PORT
    /* Nothing to do here as we have already installed all hooks in vboxUsbMonHookCheckInit(). */
    return STATUS_SUCCESS;
#else /* !VBOX_USB3PORT */
#ifdef VBOXUSBMON_DBG_NO_PNPHOOK
    return STATUS_SUCCESS;
#else
    if (g_VBoxUsbMonGlobals.UsbHubPnPHook.fUninitFailed)
    {
        WARN(("trying to hook usbhub pnp after the unhook failed, do nothing & pretend success"));
        return STATUS_SUCCESS;
    }
    return VBoxUsbHookInstall(&g_VBoxUsbMonGlobals.UsbHubPnPHook.Hook);
#endif
#endif /* !VBOX_USB3PORT */
}

static NTSTATUS vboxUsbMonHookUninstall()
{
#ifdef VBOXUSBMON_DBG_NO_PNPHOOK
    return STATUS_SUCCESS;
#else
#ifdef VBOX_USB3PORT
    NTSTATUS Status = STATUS_SUCCESS;
    for (int i = 0; i < VBOXUSBMON_MAXDRIVERS; i++)
    {
        if (g_VBoxUsbMonGlobals.pDrivers[i].DriverObject)
        {
            Assert(g_VBoxUsbMonGlobals.pDrivers[i].DriverObject == g_VBoxUsbMonGlobals.pDrivers[i].UsbHubPnPHook.Hook.pDrvObj);
            LOG(("Unhooking from %p...\n", g_VBoxUsbMonGlobals.pDrivers[i].DriverObject));
            Status = VBoxUsbHookUninstall(&g_VBoxUsbMonGlobals.pDrivers[i].UsbHubPnPHook.Hook);
            if (!NT_SUCCESS(Status))
            {
                /*
                 * We failed to uninstall the hook, so we keep the reference to the driver
                 * in order to prevent another driver re-using this slot because we are
                 * going to mark this hook as fUninitFailed.
                 */
                //AssertMsgFailed(("usbhub pnp unhook failed, setting the fUninitFailed flag, the current value of fUninitFailed (%d)", g_VBoxUsbMonGlobals.UsbHubPnPHook.fUninitFailed));
                LOG(("usbhub pnp unhook failed, setting the fUninitFailed flag, the current value of fUninitFailed (%d)", g_VBoxUsbMonGlobals.pDrivers[i].UsbHubPnPHook.fUninitFailed));
                g_VBoxUsbMonGlobals.pDrivers[i].UsbHubPnPHook.fUninitFailed = true;
            }
            else
            {
                /* The hook was removed successfully, now we can forget about this driver. */
                ObDereferenceObject(g_VBoxUsbMonGlobals.pDrivers[i].DriverObject);
                g_VBoxUsbMonGlobals.pDrivers[i].DriverObject = NULL;
            }
        }
    }
#else /* !VBOX_USB3PORT */
    NTSTATUS Status = VBoxUsbHookUninstall(&g_VBoxUsbMonGlobals.UsbHubPnPHook.Hook);
    if (!NT_SUCCESS(Status))
    {
        AssertMsgFailed(("usbhub pnp unhook failed, setting the fUninitFailed flag, the current value of fUninitFailed (%d)", g_VBoxUsbMonGlobals.UsbHubPnPHook.fUninitFailed));
        g_VBoxUsbMonGlobals.UsbHubPnPHook.fUninitFailed = true;
    }
#endif /* !VBOX_USB3PORT */
    return Status;
#endif
}


static NTSTATUS vboxUsbMonCheckTermStuff()
{
    NTSTATUS Status = KeWaitForSingleObject(&g_VBoxUsbMonGlobals.OpenSynchEvent,
            Executive, KernelMode,
            FALSE, /* BOOLEAN Alertable */
            NULL /* IN PLARGE_INTEGER Timeout */
            );
    AssertRelease(Status == STATUS_SUCCESS);

    do
    {
        if (--g_VBoxUsbMonGlobals.cOpens)
            break;

        Status = vboxUsbMonHookUninstall();

        NTSTATUS tmpStatus = VBoxUsbFltTerm();
        if (!NT_SUCCESS(tmpStatus))
        {
            /* this means a driver state is screwed up, KeBugCheckEx here ? */
            AssertReleaseFailed();
        }
    } while (0);

    KeSetEvent(&g_VBoxUsbMonGlobals.OpenSynchEvent, 0, FALSE);

    return Status;
}

static NTSTATUS vboxUsbMonCheckInitStuff()
{
    NTSTATUS Status = KeWaitForSingleObject(&g_VBoxUsbMonGlobals.OpenSynchEvent,
            Executive, KernelMode,
            FALSE, /* BOOLEAN Alertable */
            NULL /* IN PLARGE_INTEGER Timeout */
        );
    if (Status == STATUS_SUCCESS)
    {
        do
        {
            if (g_VBoxUsbMonGlobals.cOpens++)
            {
                LOG(("opens: %d, success", g_VBoxUsbMonGlobals.cOpens));
                break;
            }

            Status = VBoxUsbFltInit();
            if (NT_SUCCESS(Status))
            {
                Status = vboxUsbMonHookCheckInit();
                if (NT_SUCCESS(Status))
                {
                    Status = vboxUsbMonHookInstall();
                    if (NT_SUCCESS(Status))
                    {
                        Status = STATUS_SUCCESS;
                        LOG(("succeded!!"));
                        break;
                    }
                    else
                    {
                        WARN(("vboxUsbMonHookInstall failed, Status (0x%x)", Status));
                    }
                }
                else
                {
                    WARN(("vboxUsbMonHookCheckInit failed, Status (0x%x)", Status));
                }
                VBoxUsbFltTerm();
            }
            else
            {
                WARN(("VBoxUsbFltInit failed, Status (0x%x)", Status));
            }

            --g_VBoxUsbMonGlobals.cOpens;
            Assert(!g_VBoxUsbMonGlobals.cOpens);
        } while (0);

        KeSetEvent(&g_VBoxUsbMonGlobals.OpenSynchEvent, 0, FALSE);
    }
    else
    {
        WARN(("KeWaitForSingleObject failed, Status (0x%x)", Status));
    }
    return Status;
}

static NTSTATUS vboxUsbMonContextCreate(PVBOXUSBMONCTX *ppCtx)
{
    NTSTATUS Status;
    *ppCtx = NULL;
    PVBOXUSBMONCTX pFileCtx = (PVBOXUSBMONCTX)VBoxUsbMonMemAllocZ(sizeof (*pFileCtx));
    if (pFileCtx)
    {
        Status = vboxUsbMonCheckInitStuff();
        if (Status == STATUS_SUCCESS)
        {
            Status = VBoxUsbFltCreate(&pFileCtx->FltCtx);
            if (Status == STATUS_SUCCESS)
            {
                *ppCtx = pFileCtx;
                LOG(("succeeded!!"));
                return STATUS_SUCCESS;
            }
            else
            {
                WARN(("VBoxUsbFltCreate failed"));
            }
            vboxUsbMonCheckTermStuff();
        }
        else
        {
            WARN(("vboxUsbMonCheckInitStuff failed"));
        }
        VBoxUsbMonMemFree(pFileCtx);
    }
    else
    {
        WARN(("VBoxUsbMonMemAllocZ failed"));
        Status = STATUS_NO_MEMORY;
    }

    return Status;
}

static NTSTATUS vboxUsbMonContextClose(PVBOXUSBMONCTX pCtx)
{
    NTSTATUS Status = VBoxUsbFltClose(&pCtx->FltCtx);
    if (Status == STATUS_SUCCESS)
    {
        Status = vboxUsbMonCheckTermStuff();
        Assert(Status == STATUS_SUCCESS);
        /* ignore the failure */
        VBoxUsbMonMemFree(pCtx);
    }

    return Status;
}

static NTSTATUS _stdcall VBoxUsbMonClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);
    PFILE_OBJECT pFileObj = pStack->FileObject;
    Assert(pFileObj->FsContext);
    PVBOXUSBMONCTX pCtx = (PVBOXUSBMONCTX)pFileObj->FsContext;

    LOG(("VBoxUsbMonClose"));

    NTSTATUS Status = vboxUsbMonContextClose(pCtx);
    if (Status != STATUS_SUCCESS)
    {
        WARN(("vboxUsbMonContextClose failed, Status (0x%x), prefent unload", Status));
        if (!InterlockedExchange(&g_VBoxUsbMonGlobals.ulPreventUnloadOn, 1))
        {
            LOGREL(("ulPreventUnloadOn not set, preventing unload"));
            UNICODE_STRING UniName;
            PDEVICE_OBJECT pTmpDevObj;
            RtlInitUnicodeString(&UniName, USBMON_DEVICE_NAME_NT);
            NTSTATUS tmpStatus = IoGetDeviceObjectPointer(&UniName, FILE_ALL_ACCESS, &g_VBoxUsbMonGlobals.pPreventUnloadFileObj, &pTmpDevObj);
            AssertRelease(NT_SUCCESS(tmpStatus));
            AssertRelease(pTmpDevObj == pDevObj);
        }
        else
        {
            WARN(("ulPreventUnloadOn already set"));
        }
        LOG(("success!!"));
        Status = STATUS_SUCCESS;
    }
    pFileObj->FsContext = NULL;
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information  = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}


static NTSTATUS _stdcall VBoxUsbMonCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    RT_NOREF1(pDevObj);
    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);
    PFILE_OBJECT pFileObj = pStack->FileObject;
    NTSTATUS Status;

    LOG(("VBoxUSBMonCreate"));

    if (pStack->Parameters.Create.Options & FILE_DIRECTORY_FILE)
    {
        WARN(("trying to open as a directory"));
        pIrp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return STATUS_NOT_A_DIRECTORY;
    }

    pFileObj->FsContext = NULL;
    PVBOXUSBMONCTX pCtx = NULL;
    Status = vboxUsbMonContextCreate(&pCtx);
    if (Status == STATUS_SUCCESS)
    {
        Assert(pCtx);
        pFileObj->FsContext = pCtx;
    }
    else
    {
        WARN(("vboxUsbMonContextCreate failed Status (0x%x)", Status));
    }

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information  = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}

static int VBoxUsbMonSetNotifyEvent(PVBOXUSBMONCTX pContext, HANDLE hEvent)
{
    int rc = VBoxUsbFltSetNotifyEvent(&pContext->FltCtx, hEvent);
    return rc;
}

static int VBoxUsbMonFltAdd(PVBOXUSBMONCTX pContext, PUSBFILTER pFilter, uintptr_t *pId)
{
#ifdef VBOXUSBMON_DBG_NO_FILTERS
    static uintptr_t idDummy = 1;
    *pId = idDummy;
    ++idDummy;
    return VINF_SUCCESS;
#else
    int rc = VBoxUsbFltAdd(&pContext->FltCtx, pFilter, pId);
    return rc;
#endif
}

static int VBoxUsbMonFltRemove(PVBOXUSBMONCTX pContext, uintptr_t uId)
{
#ifdef VBOXUSBMON_DBG_NO_FILTERS
    return VINF_SUCCESS;
#else
    int rc = VBoxUsbFltRemove(&pContext->FltCtx, uId);
    return rc;
#endif
}

static NTSTATUS VBoxUsbMonRunFilters(PVBOXUSBMONCTX pContext)
{
    NTSTATUS Status = VBoxUsbFltFilterCheck(&pContext->FltCtx);
    return Status;
}

static NTSTATUS VBoxUsbMonGetDevice(PVBOXUSBMONCTX pContext, HVBOXUSBDEVUSR hDevice, PUSBSUP_GETDEV_MON pInfo)
{
    NTSTATUS Status = VBoxUsbFltGetDevice(&pContext->FltCtx, hDevice, pInfo);
    return Status;
}

static NTSTATUS vboxUsbMonIoctlDispatch(PVBOXUSBMONCTX pContext, ULONG Ctl, PVOID pvBuffer, ULONG cbInBuffer,
                                        ULONG cbOutBuffer, ULONG_PTR *pInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Info = 0;
    switch (Ctl)
    {
        case SUPUSBFLT_IOCTL_GET_VERSION:
        {
            PUSBSUP_VERSION pOut = (PUSBSUP_VERSION)pvBuffer;

            LOG(("SUPUSBFLT_IOCTL_GET_VERSION"));
            if (!pvBuffer || cbOutBuffer != sizeof(*pOut) || cbInBuffer != 0)
            {
                WARN(("SUPUSBFLT_IOCTL_GET_VERSION: Invalid input/output sizes. cbIn=%d expected %d. cbOut=%d expected %d.",
                        cbInBuffer, 0, cbOutBuffer, sizeof (*pOut)));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            pOut->u32Major = USBMON_MAJOR_VERSION;
            pOut->u32Minor = USBMON_MINOR_VERSION;
            Info = sizeof (*pOut);
            ASSERT_WARN(Status == STATUS_SUCCESS, ("unexpected status, 0x%x", Status));
            break;
        }

        case SUPUSBFLT_IOCTL_ADD_FILTER:
        {
            PUSBFILTER pFilter = (PUSBFILTER)pvBuffer;
            PUSBSUP_FLTADDOUT pOut = (PUSBSUP_FLTADDOUT)pvBuffer;
            uintptr_t uId = 0;
            int rc;
            if (RT_UNLIKELY(!pvBuffer || cbInBuffer != sizeof (*pFilter) || cbOutBuffer != sizeof (*pOut)))
            {
                WARN(("SUPUSBFLT_IOCTL_ADD_FILTER: Invalid input/output sizes. cbIn=%d expected %d. cbOut=%d expected %d.",
                        cbInBuffer, sizeof (*pFilter), cbOutBuffer, sizeof (*pOut)));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            rc = VBoxUsbMonFltAdd(pContext, pFilter, &uId);
            pOut->rc  = rc;
            pOut->uId = uId;
            Info = sizeof (*pOut);
            ASSERT_WARN(Status == STATUS_SUCCESS, ("unexpected status, 0x%x", Status));
            break;
        }

        case SUPUSBFLT_IOCTL_REMOVE_FILTER:
        {
            uintptr_t *pIn = (uintptr_t *)pvBuffer;
            int *pRc = (int *)pvBuffer;

            if (!pvBuffer || cbInBuffer != sizeof (*pIn) || (cbOutBuffer && cbOutBuffer != sizeof (*pRc)))
            {
                WARN(("SUPUSBFLT_IOCTL_REMOVE_FILTER: Invalid input/output sizes. cbIn=%d expected %d. cbOut=%d expected %d.",
                        cbInBuffer, sizeof (*pIn), cbOutBuffer, 0));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            LOG(("SUPUSBFLT_IOCTL_REMOVE_FILTER %x", *pIn));
            int rc = VBoxUsbMonFltRemove(pContext, *pIn);
            if (cbOutBuffer)
            {
                /* we've validated that already */
                Assert(cbOutBuffer == (ULONG)*pRc);
                *pRc = rc;
                Info = sizeof (*pRc);
            }
            ASSERT_WARN(Status == STATUS_SUCCESS, ("unexpected status, 0x%x", Status));
            break;
        }

        case SUPUSBFLT_IOCTL_RUN_FILTERS:
        {
            if (pvBuffer || cbInBuffer || cbOutBuffer)
            {
                WARN(("SUPUSBFLT_IOCTL_RUN_FILTERS: Invalid input/output sizes. cbIn=%d expected %d. cbOut=%d expected %d.",
                        cbInBuffer, 0, cbOutBuffer, 0));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            LOG(("SUPUSBFLT_IOCTL_RUN_FILTERS "));
            Status = VBoxUsbMonRunFilters(pContext);
            ASSERT_WARN(Status != STATUS_PENDING, ("status pending!"));
            break;
        }

        case SUPUSBFLT_IOCTL_GET_DEVICE:
        {
            HVBOXUSBDEVUSR hDevice = *((HVBOXUSBDEVUSR*)pvBuffer);
            PUSBSUP_GETDEV_MON pOut = (PUSBSUP_GETDEV_MON)pvBuffer;
            if (!pvBuffer || cbInBuffer != sizeof (hDevice) || cbOutBuffer < sizeof (*pOut))
            {
                WARN(("SUPUSBFLT_IOCTL_GET_DEVICE: Invalid input/output sizes. cbIn=%d expected %d. cbOut=%d expected >= %d.",
                        cbInBuffer, sizeof (hDevice), cbOutBuffer, sizeof (*pOut)));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            Status = VBoxUsbMonGetDevice(pContext, hDevice, pOut);

            if (NT_SUCCESS(Status))
            {
                Info = sizeof (*pOut);
            }
            else
            {
                WARN(("VBoxUsbMonGetDevice fail 0x%x", Status));
            }
            break;
        }

        case SUPUSBFLT_IOCTL_SET_NOTIFY_EVENT:
        {
            PUSBSUP_SET_NOTIFY_EVENT pSne = (PUSBSUP_SET_NOTIFY_EVENT)pvBuffer;
            if (!pvBuffer || cbInBuffer != sizeof (*pSne) || cbOutBuffer != sizeof (*pSne))
            {
                WARN(("SUPUSBFLT_IOCTL_SET_NOTIFY_EVENT: Invalid input/output sizes. cbIn=%d expected %d. cbOut=%d expected %d.",
                        cbInBuffer, sizeof (*pSne), cbOutBuffer, sizeof (*pSne)));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            pSne->u.rc = VBoxUsbMonSetNotifyEvent(pContext, pSne->u.hEvent);
            Info = sizeof (*pSne);
            ASSERT_WARN(Status == STATUS_SUCCESS, ("unexpected status, 0x%x", Status));
            break;
        }

        default:
            WARN(("Unknown code 0x%x", Ctl));
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    ASSERT_WARN(Status != STATUS_PENDING, ("Status pending!"));

    *pInfo = Info;
    return Status;
}

static NTSTATUS _stdcall VBoxUsbMonDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    ULONG_PTR Info = 0;
    NTSTATUS Status = IoAcquireRemoveLock(&g_VBoxUsbMonGlobals.RmLock, pDevObj);
    if (NT_SUCCESS(Status))
    {
        PIO_STACK_LOCATION pSl = IoGetCurrentIrpStackLocation(pIrp);
        PFILE_OBJECT pFileObj = pSl->FileObject;
        Assert(pFileObj);
        Assert(pFileObj->FsContext);
        PVBOXUSBMONCTX pCtx = (PVBOXUSBMONCTX)pFileObj->FsContext;
        Assert(pCtx);
        Status = vboxUsbMonIoctlDispatch(pCtx,
                    pSl->Parameters.DeviceIoControl.IoControlCode,
                    pIrp->AssociatedIrp.SystemBuffer,
                    pSl->Parameters.DeviceIoControl.InputBufferLength,
                    pSl->Parameters.DeviceIoControl.OutputBufferLength,
                    &Info);
        ASSERT_WARN(Status != STATUS_PENDING, ("Status pending"));

        IoReleaseRemoveLock(&g_VBoxUsbMonGlobals.RmLock, pDevObj);
    }
    else
    {
        WARN(("IoAcquireRemoveLock failed Status (0x%x)", Status));
    }

    pIrp->IoStatus.Information = Info;
    pIrp->IoStatus.Status = Status;
    IoCompleteRequest (pIrp, IO_NO_INCREMENT);
    return Status;
}

static NTSTATUS vboxUsbMonInternalIoctlDispatch(ULONG Ctl, PVOID pvBuffer,  ULONG_PTR *pInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    *pInfo = 0;
    switch (Ctl)
    {
        case VBOXUSBIDC_INTERNAL_IOCTL_GET_VERSION:
        {
            PVBOXUSBIDC_VERSION pOut = (PVBOXUSBIDC_VERSION)pvBuffer;

            LOG(("VBOXUSBIDC_INTERNAL_IOCTL_GET_VERSION"));
            if (!pvBuffer)
            {
                WARN(("VBOXUSBIDC_INTERNAL_IOCTL_GET_VERSION: Buffer is NULL"));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            pOut->u32Major = VBOXUSBIDC_VERSION_MAJOR;
            pOut->u32Minor = VBOXUSBIDC_VERSION_MINOR;
            ASSERT_WARN(Status == STATUS_SUCCESS, ("unexpected status, 0x%x", Status));
            break;
        }

        case VBOXUSBIDC_INTERNAL_IOCTL_PROXY_STARTUP:
        {
            PVBOXUSBIDC_PROXY_STARTUP pOut = (PVBOXUSBIDC_PROXY_STARTUP)pvBuffer;

            LOG(("VBOXUSBIDC_INTERNAL_IOCTL_PROXY_STARTUP"));
            if (!pvBuffer)
            {
                WARN(("VBOXUSBIDC_INTERNAL_IOCTL_PROXY_STARTUP: Buffer is NULL"));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            pOut->u.hDev = VBoxUsbFltProxyStarted(pOut->u.pPDO);
            ASSERT_WARN(pOut->u.hDev, ("zero hDev"));
            ASSERT_WARN(Status == STATUS_SUCCESS, ("unexpected status, 0x%x", Status));
            break;
        }

        case VBOXUSBIDC_INTERNAL_IOCTL_PROXY_TEARDOWN:
        {
            PVBOXUSBIDC_PROXY_TEARDOWN pOut = (PVBOXUSBIDC_PROXY_TEARDOWN)pvBuffer;

            LOG(("VBOXUSBIDC_INTERNAL_IOCTL_PROXY_TEARDOWN"));
            if (!pvBuffer)
            {
                WARN(("VBOXUSBIDC_INTERNAL_IOCTL_PROXY_TEARDOWN: Buffer is NULL"));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            ASSERT_WARN(pOut->hDev, ("zero hDev"));
            VBoxUsbFltProxyStopped(pOut->hDev);
            ASSERT_WARN(Status == STATUS_SUCCESS, ("unexpected status, 0x%x", Status));
            break;
        }

        default:
        {
            WARN(("Unknown code 0x%x", Ctl));
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    return Status;
}

static NTSTATUS _stdcall VBoxUsbMonInternalDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    ULONG_PTR Info = 0;
    NTSTATUS Status = IoAcquireRemoveLock(&g_VBoxUsbMonGlobals.RmLock, pDevObj);
    if (NT_SUCCESS(Status))
    {
        PIO_STACK_LOCATION pSl = IoGetCurrentIrpStackLocation(pIrp);
        Status = vboxUsbMonInternalIoctlDispatch(pSl->Parameters.DeviceIoControl.IoControlCode,
                        pSl->Parameters.Others.Argument1,
                        &Info);
        Assert(Status != STATUS_PENDING);

        IoReleaseRemoveLock(&g_VBoxUsbMonGlobals.RmLock, pDevObj);
    }

    pIrp->IoStatus.Information = Info;
    pIrp->IoStatus.Status = Status;
    IoCompleteRequest (pIrp, IO_NO_INCREMENT);
    return Status;
}

/**
 * Unload the driver.
 *
 * @param   pDrvObj     Driver object.
 */
static void _stdcall VBoxUsbMonUnload(PDRIVER_OBJECT pDrvObj)
{
    RT_NOREF1(pDrvObj);
    LOG(("VBoxUSBMonUnload pDrvObj (0x%p)", pDrvObj));

    IoReleaseRemoveLockAndWait(&g_VBoxUsbMonGlobals.RmLock, &g_VBoxUsbMonGlobals);

    Assert(!g_VBoxUsbMonGlobals.cOpens);

    UNICODE_STRING DosName;
    RtlInitUnicodeString(&DosName, USBMON_DEVICE_NAME_DOS);
    IoDeleteSymbolicLink(&DosName);

    IoDeleteDevice(g_VBoxUsbMonGlobals.pDevObj);

    /* cleanup the logger */
    PRTLOGGER pLogger = RTLogRelSetDefaultInstance(NULL);
    if (pLogger)
        RTLogDestroy(pLogger);
    pLogger = RTLogSetDefaultInstance(NULL);
    if (pLogger)
        RTLogDestroy(pLogger);
}

RT_C_DECLS_BEGIN
NTSTATUS _stdcall DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath);
RT_C_DECLS_END

/**
 * Driver entry point.
 *
 * @returns appropriate status code.
 * @param   pDrvObj     Pointer to driver object.
 * @param   pRegPath    Registry base path.
 */
NTSTATUS _stdcall DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
    RT_NOREF1(pRegPath);
#ifdef VBOX_USB_WITH_VERBOSE_LOGGING
    RTLogGroupSettings(0, "+default.e.l.f.l2.l3");
    RTLogDestinations(0, "debugger");
#endif

    LOGREL(("Built %s %s", __DATE__, __TIME__));

    memset (&g_VBoxUsbMonGlobals, 0, sizeof (g_VBoxUsbMonGlobals));
#ifdef VBOX_USB3PORT
    VBOX_PNPHOOKSTUB_INIT(0);
    VBOX_PNPHOOKSTUB_INIT(1);
    VBOX_PNPHOOKSTUB_INIT(2);
    VBOX_PNPHOOKSTUB_INIT(3);
    VBOX_PNPHOOKSTUB_INIT(4);
    AssertCompile(VBOXUSBMON_MAXDRIVERS == 5);
#endif /* VBOX_USB3PORT */
    KeInitializeEvent(&g_VBoxUsbMonGlobals.OpenSynchEvent, SynchronizationEvent, TRUE /* signaled */);
    IoInitializeRemoveLock(&g_VBoxUsbMonGlobals.RmLock, VBOXUSBMON_MEMTAG, 1, 100);
    UNICODE_STRING DevName;
    PDEVICE_OBJECT pDevObj;
    /* create the device */
    RtlInitUnicodeString(&DevName, USBMON_DEVICE_NAME_NT);
    NTSTATUS Status = IoAcquireRemoveLock(&g_VBoxUsbMonGlobals.RmLock, &g_VBoxUsbMonGlobals);
    if (NT_SUCCESS(Status))
    {
        Status = IoCreateDevice(pDrvObj, sizeof (VBOXUSBMONINS), &DevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDevObj);
        if (NT_SUCCESS(Status))
        {
            UNICODE_STRING DosName;
            RtlInitUnicodeString(&DosName, USBMON_DEVICE_NAME_DOS);
            Status = IoCreateSymbolicLink(&DosName, &DevName);
            if (NT_SUCCESS(Status))
            {
                PVBOXUSBMONINS pDevExt = (PVBOXUSBMONINS)pDevObj->DeviceExtension;
                memset(pDevExt, 0, sizeof(*pDevExt));

                pDrvObj->DriverUnload = VBoxUsbMonUnload;
                pDrvObj->MajorFunction[IRP_MJ_CREATE] = VBoxUsbMonCreate;
                pDrvObj->MajorFunction[IRP_MJ_CLOSE] = VBoxUsbMonClose;
                pDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VBoxUsbMonDeviceControl;
                pDrvObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = VBoxUsbMonInternalDeviceControl;

                g_VBoxUsbMonGlobals.pDevObj = pDevObj;
                LOG(("VBoxUSBMon::DriverEntry returning STATUS_SUCCESS"));
                return STATUS_SUCCESS;
            }
            IoDeleteDevice(pDevObj);
        }
        IoReleaseRemoveLockAndWait(&g_VBoxUsbMonGlobals.RmLock, &g_VBoxUsbMonGlobals);
    }

    return Status;
}
