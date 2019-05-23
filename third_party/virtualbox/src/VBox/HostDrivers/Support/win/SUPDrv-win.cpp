/* $Id: SUPDrv-win.cpp $ */
/** @file
 * VBoxDrv - The VirtualBox Support Driver - Windows NT specifics.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#define IPRT_NT_MAP_TO_ZW
#define LOG_GROUP LOG_GROUP_SUP_DRV
#include "../SUPDrvInternal.h"
#include <excpt.h>
#include <ntimage.h>

#include <iprt/assert.h>
#include <iprt/avl.h>
#include <iprt/ctype.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/process.h>
#include <iprt/power.h>
#include <iprt/rand.h>
#include <iprt/semaphore.h>
#include <iprt/spinlock.h>
#include <iprt/string.h>
#include <iprt/x86.h>
#include <VBox/log.h>
#include <VBox/err.h>

#include <iprt/asm-amd64-x86.h>

#ifdef VBOX_WITH_HARDENING
# include "SUPHardenedVerify-win.h"
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The support service name. */
#define SERVICE_NAME    "VBoxDrv"
/** The Pool tag (VBox). */
#define SUPDRV_NT_POOL_TAG  'xoBV'

/** NT device name for user access. */
#define DEVICE_NAME_NT_USR          L"\\Device\\VBoxDrvU"
#ifdef VBOX_WITH_HARDENING
/** Macro for checking for deflecting calls to the stub device. */
# define VBOXDRV_COMPLETE_IRP_AND_RETURN_IF_STUB_DEV(a_pDevObj, a_pIrp) \
    do { if ((a_pDevObj) == g_pDevObjStub) \
            return supdrvNtCompleteRequest(STATUS_ACCESS_DENIED, a_pIrp); \
    } while (0)
/** Macro for checking for deflecting calls to the stub and error info
 *  devices. */
# define VBOXDRV_COMPLETE_IRP_AND_RETURN_IF_STUB_OR_ERROR_INFO_DEV(a_pDevObj, a_pIrp) \
    do { if ((a_pDevObj) == g_pDevObjStub || (a_pDevObj) == g_pDevObjErrorInfo) \
            return supdrvNtCompleteRequest(STATUS_ACCESS_DENIED, a_pIrp); \
    } while (0)
#else
# define VBOXDRV_COMPLETE_IRP_AND_RETURN_IF_STUB_DEV(a_pDevObj, a_pIrp)                 do {} while (0)
# define VBOXDRV_COMPLETE_IRP_AND_RETURN_IF_STUB_OR_ERROR_INFO_DEV(a_pDevObj, a_pIrp)   do {} while (0)
#endif

/** Enables the fast I/O control code path. */
#define VBOXDRV_WITH_FAST_IO


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Device extension used by VBoxDrvU.
 */
typedef struct SUPDRVDEVEXTUSR
{
    /** Global cookie (same location as in SUPDRVDEVEXT, different value). */
    uint32_t                        u32Cookie;
    /** Pointer to the main driver extension. */
    PSUPDRVDEVEXT                   pMainDrvExt;
} SUPDRVDEVEXTUSR;
AssertCompileMembersAtSameOffset(SUPDRVDEVEXT, u32Cookie, SUPDRVDEVEXTUSR, u32Cookie);
/** Pointer to the VBoxDrvU device extension. */
typedef SUPDRVDEVEXTUSR *PSUPDRVDEVEXTUSR;
/** Value of SUPDRVDEVEXTUSR::u32Cookie. */
#define SUPDRVDEVEXTUSR_COOKIE      UINT32_C(0x12345678)

/** Get the main device extension. */
#define SUPDRVNT_GET_DEVEXT(pDevObj) \
    (  pDevObj != g_pDevObjUsr \
     ? (PSUPDRVDEVEXT)pDevObj->DeviceExtension \
     : ((PSUPDRVDEVEXTUSR)pDevObj->DeviceExtension)->pMainDrvExt )

#ifdef VBOX_WITH_HARDENING

/**
 * Device extension used by VBoxDrvStub.
 */
typedef struct SUPDRVDEVEXTSTUB
{
    /** Common header. */
    SUPDRVDEVEXTUSR     Common;
} SUPDRVDEVEXTSTUB;
/** Pointer to the VBoxDrvStub device extension. */
typedef SUPDRVDEVEXTSTUB *PSUPDRVDEVEXTSTUB;
/** Value of SUPDRVDEVEXTSTUB::Common.u32Cookie. */
#define SUPDRVDEVEXTSTUB_COOKIE      UINT32_C(0x90abcdef)


/**
 * Device extension used by VBoxDrvErrorInfo.
 */
typedef struct SUPDRVDEVEXTERRORINFO
{
    /** Common header. */
    SUPDRVDEVEXTUSR     Common;
} SUPDRVDEVEXTERRORINFO;
/** Pointer to the VBoxDrvErrorInfo device extension. */
typedef SUPDRVDEVEXTERRORINFO *PSUPDRVDEVEXTERRORINFO;
/** Value of SUPDRVDEVEXTERRORINFO::Common.u32Cookie. */
#define SUPDRVDEVEXTERRORINFO_COOKIE      UINT32_C(0xBadC0ca0)

/**
 * Error info for a failed VBoxDrv or VBoxDrvStub open attempt.
 */
typedef struct SUPDRVNTERRORINFO
{
    /** The list entry (in g_ErrorInfoHead). */
    RTLISTNODE      ListEntry;
    /** The ID of the process this error info belongs to.  */
    HANDLE          hProcessId;
    /** The ID of the thread owning this info. */
    HANDLE          hThreadId;
    /** Milliseconds createion timestamp (for cleaning up).  */
    uint64_t        uCreatedMsTs;
    /** Number of bytes of valid info. */
    uint32_t        cchErrorInfo;
    /** The error info. */
    char            szErrorInfo[16384 - sizeof(RTLISTNODE) - sizeof(HANDLE)*2 - sizeof(uint64_t) - sizeof(uint32_t) - 0x20];
} SUPDRVNTERRORINFO;
/** Pointer to error info. */
typedef SUPDRVNTERRORINFO *PSUPDRVNTERRORINFO;


/**
 * The kind of process we're protecting.
 */
typedef enum SUPDRVNTPROTECTKIND
{
    kSupDrvNtProtectKind_Invalid = 0,

    /** Stub process protection while performing process verification.
     * Next: StubSpawning (or free)  */
    kSupDrvNtProtectKind_StubUnverified,
    /** Stub process protection before it creates the VM process.
     * Next: StubParent, StubDead. */
    kSupDrvNtProtectKind_StubSpawning,
    /** Stub process protection while having a VM process as child.
     * Next: StubDead  */
    kSupDrvNtProtectKind_StubParent,
    /** Dead stub process. */
    kSupDrvNtProtectKind_StubDead,

    /** Potential VM process.
     * Next: VmProcessConfirmed, VmProcessDead. */
    kSupDrvNtProtectKind_VmProcessUnconfirmed,
    /** Confirmed VM process.
     * Next: VmProcessDead. */
    kSupDrvNtProtectKind_VmProcessConfirmed,
    /** Dead VM process. */
    kSupDrvNtProtectKind_VmProcessDead,

    /** End of valid protection kinds. */
    kSupDrvNtProtectKind_End
} SUPDRVNTPROTECTKIND;

/**
 * A NT process protection structure.
 */
typedef struct SUPDRVNTPROTECT
{
    /** The AVL node core structure.  The process ID is the pid. */
    AVLPVNODECORE       AvlCore;
    /** Magic value (SUPDRVNTPROTECT_MAGIC). */
    uint32_t volatile   u32Magic;
    /** Reference counter. */
    uint32_t volatile   cRefs;
    /** The kind of process we're protecting. */
    SUPDRVNTPROTECTKIND volatile enmProcessKind;
    /** Whether this structure is in the tree. */
    bool                fInTree : 1;
    /** 7,: Hack to allow the supid themes service duplicate handle privileges to
     *  our process. */
    bool                fThemesFirstProcessCreateHandle : 1;
    /** Vista, 7 & 8: Hack to allow more rights to the handle returned by
     *  NtCreateUserProcess. Only applicable to VmProcessUnconfirmed. */
    bool                fFirstProcessCreateHandle : 1;
    /** Vista, 7 & 8: Hack to allow more rights to the handle returned by
     *  NtCreateUserProcess. Only applicable to VmProcessUnconfirmed. */
    bool                fFirstThreadCreateHandle : 1;
    /** 8.1: Hack to allow more rights to the handle returned by
     *  NtCreateUserProcess. Only applicable to VmProcessUnconfirmed. */
    bool                fCsrssFirstProcessCreateHandle : 1;
    /** Vista, 7 & 8: Hack to allow more rights to the handle duplicated by CSRSS
     * during process creation. Only applicable to VmProcessUnconfirmed.  On
     * 32-bit systems we allow two as ZoneAlarm's system call hooks has been
     * observed to do some seemingly unnecessary duplication work. */
    int32_t volatile    cCsrssFirstProcessDuplicateHandle;

    /** The parent PID for VM processes, otherwise NULL. */
    HANDLE              hParentPid;
    /** The TID of the thread opening VBoxDrv or VBoxDrvStub, NULL if not opened. */
    HANDLE              hOpenTid;
    /** The PID of the CSRSS process associated with this process. */
    HANDLE              hCsrssPid;
    /** Pointer to the CSRSS process structure (referenced). */
    PEPROCESS           pCsrssProcess;
    /** State dependent data. */
    union
    {
        /** A stub process in the StubParent state will keep a reference to a child
         * while it's in the VmProcessUnconfirmed state so that it can be cleaned up
         * correctly if things doesn't work out. */
        struct SUPDRVNTPROTECT *pChild;
        /** A process in the VmProcessUnconfirmed state will keep a weak
         * reference to the parent's protection structure so it can clean up the pChild
         * reference the parent has to it. */
        struct SUPDRVNTPROTECT *pParent;
    } u;
} SUPDRVNTPROTECT;
/** Pointer to a NT process protection record. */
typedef SUPDRVNTPROTECT *PSUPDRVNTPROTECT;
/** The SUPDRVNTPROTECT::u32Magic value (Robert A. Heinlein). */
# define SUPDRVNTPROTECT_MAGIC      UINT32_C(0x19070707)
/** The SUPDRVNTPROTECT::u32Magic value of a dead structure. */
# define SUPDRVNTPROTECT_MAGIC_DEAD UINT32_C(0x19880508)

/** Pointer to ObGetObjectType. */
typedef POBJECT_TYPE (NTAPI *PFNOBGETOBJECTTYPE)(PVOID);
/** Pointer to ObRegisterCallbacks. */
typedef NTSTATUS (NTAPI *PFNOBREGISTERCALLBACKS)(POB_CALLBACK_REGISTRATION, PVOID *);
/** Pointer to ObUnregisterCallbacks. */
typedef VOID     (NTAPI *PFNOBUNREGISTERCALLBACKS)(PVOID);
/** Pointer to PsSetCreateProcessNotifyRoutineEx. */
typedef NTSTATUS (NTAPI *PFNPSSETCREATEPROCESSNOTIFYROUTINEEX)(PCREATE_PROCESS_NOTIFY_ROUTINE_EX, BOOLEAN);
/** Pointer to PsReferenceProcessFilePointer. */
typedef NTSTATUS (NTAPI *PFNPSREFERENCEPROCESSFILEPOINTER)(PEPROCESS, PFILE_OBJECT *);
/** Pointer to PsIsProtectedProcessLight. */
typedef BOOLEAN  (NTAPI *PFNPSISPROTECTEDPROCESSLIGHT)(PEPROCESS);
/** Pointer to ZwAlpcCreatePort. */
typedef NTSTATUS (NTAPI *PFNZWALPCCREATEPORT)(PHANDLE, POBJECT_ATTRIBUTES, struct _ALPC_PORT_ATTRIBUTES *);

#endif /* VBOX_WITH_HARDENINIG */


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void     _stdcall   VBoxDrvNtUnload(PDRIVER_OBJECT pDrvObj);
static NTSTATUS _stdcall   VBoxDrvNtCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp);
static NTSTATUS _stdcall   VBoxDrvNtCleanup(PDEVICE_OBJECT pDevObj, PIRP pIrp);
static NTSTATUS _stdcall   VBoxDrvNtClose(PDEVICE_OBJECT pDevObj, PIRP pIrp);
#ifdef VBOXDRV_WITH_FAST_IO
static BOOLEAN  _stdcall   VBoxDrvNtFastIoDeviceControl(PFILE_OBJECT pFileObj, BOOLEAN fWait, PVOID pvInput, ULONG cbInput,
                                                        PVOID pvOutput, ULONG cbOutput, ULONG uCmd,
                                                        PIO_STATUS_BLOCK pIoStatus, PDEVICE_OBJECT pDevObj);
#endif
static NTSTATUS _stdcall   VBoxDrvNtDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp);
static int                 VBoxDrvNtDeviceControlSlow(PSUPDRVDEVEXT pDevExt, PSUPDRVSESSION pSession, PIRP pIrp, PIO_STACK_LOCATION pStack);
static NTSTATUS _stdcall   VBoxDrvNtInternalDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp);
static VOID     _stdcall   VBoxPowerDispatchCallback(PVOID pCallbackContext, PVOID pArgument1, PVOID pArgument2);
static NTSTATUS _stdcall   VBoxDrvNtRead(PDEVICE_OBJECT pDevObj, PIRP pIrp);
static NTSTATUS _stdcall   VBoxDrvNtNotSupportedStub(PDEVICE_OBJECT pDevObj, PIRP pIrp);
static NTSTATUS            VBoxDrvNtErr2NtStatus(int rc);
#ifdef VBOX_WITH_HARDENING
static NTSTATUS             supdrvNtProtectInit(void);
static void                 supdrvNtProtectTerm(void);
static int                  supdrvNtProtectCreate(PSUPDRVNTPROTECT *ppNtProtect, HANDLE hPid,
                                                  SUPDRVNTPROTECTKIND enmProcessKind, bool fLink);
static void                 supdrvNtProtectRelease(PSUPDRVNTPROTECT pNtProtect);
static PSUPDRVNTPROTECT     supdrvNtProtectLookup(HANDLE hPid);
static int                  supdrvNtProtectFindAssociatedCsrss(PSUPDRVNTPROTECT pNtProtect);
static int                  supdrvNtProtectVerifyProcess(PSUPDRVNTPROTECT pNtProtect);

static bool                 supdrvNtIsDebuggerAttached(void);
static void                 supdrvNtErrorInfoCleanupProcess(HANDLE hProcessId);

#endif


/*********************************************************************************************************************************
*   Exported Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
NTSTATUS _stdcall DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath);
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Pointer to the system device instance. */
static PDEVICE_OBJECT g_pDevObjSys = NULL;
/** Pointer to the user device instance. */
static PDEVICE_OBJECT g_pDevObjUsr = NULL;
#ifdef VBOXDRV_WITH_FAST_IO
/** Fast I/O dispatch table. */
static FAST_IO_DISPATCH const g_VBoxDrvFastIoDispatch =
{
    /* .SizeOfFastIoDispatch            = */ sizeof(g_VBoxDrvFastIoDispatch),
    /* .FastIoCheckIfPossible           = */ NULL,
    /* .FastIoRead                      = */ NULL,
    /* .FastIoWrite                     = */ NULL,
    /* .FastIoQueryBasicInfo            = */ NULL,
    /* .FastIoQueryStandardInfo         = */ NULL,
    /* .FastIoLock                      = */ NULL,
    /* .FastIoUnlockSingle              = */ NULL,
    /* .FastIoUnlockAll                 = */ NULL,
    /* .FastIoUnlockAllByKey            = */ NULL,
    /* .FastIoDeviceControl             = */ VBoxDrvNtFastIoDeviceControl,
    /* .AcquireFileForNtCreateSection   = */ NULL,
    /* .ReleaseFileForNtCreateSection   = */ NULL,
    /* .FastIoDetachDevice              = */ NULL,
    /* .FastIoQueryNetworkOpenInfo      = */ NULL,
    /* .AcquireForModWrite              = */ NULL,
    /* .MdlRead                         = */ NULL,
    /* .MdlReadComplete                 = */ NULL,
    /* .PrepareMdlWrite                 = */ NULL,
    /* .MdlWriteComplete                = */ NULL,
    /* .FastIoReadCompressed            = */ NULL,
    /* .FastIoWriteCompressed           = */ NULL,
    /* .MdlReadCompleteCompressed       = */ NULL,
    /* .MdlWriteCompleteCompressed      = */ NULL,
    /* .FastIoQueryOpen                 = */ NULL,
    /* .ReleaseForModWrite              = */ NULL,
    /* .AcquireForCcFlush               = */ NULL,
    /* .ReleaseForCcFlush               = */ NULL,
};
#endif /* VBOXDRV_WITH_FAST_IO */

/** Default ZERO value. */
static ULONG                        g_fOptDefaultZero = 0;
/** Registry values.
 * We wrap these in a struct to ensure they are followed by a little zero
 * padding in order to limit the chance of trouble on unpatched systems.  */
struct
{
    /** The ForceAsync registry value. */
    ULONG                           fOptForceAsyncTsc;
    /** Padding. */
    uint64_t                        au64Padding[2];
}                                   g_Options = { FALSE, 0, 0 };
/** Registry query table for RtlQueryRegistryValues. */
static RTL_QUERY_REGISTRY_TABLE     g_aRegValues[] =
{
    {
        /* .QueryRoutine = */   NULL,
        /* .Flags = */          RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK,
        /* .Name = */           L"ForceAsyncTsc",
        /* .EntryContext = */   &g_Options.fOptForceAsyncTsc,
        /* .DefaultType = */    (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_DWORD,
        /* .DefaultData = */    &g_fOptDefaultZero,
        /* .DefaultLength = */  sizeof(g_fOptDefaultZero),
    },
    {   NULL, 0, NULL, NULL, 0, NULL, 0 } /* terminator entry. */
};

/** Pointer to KeQueryMaximumGroupCount. */
static PFNKEQUERYMAXIMUMGROUPCOUNT      g_pfnKeQueryMaximumGroupCount = NULL;
/** Pointer to KeGetProcessorIndexFromNumber. */
static PFNKEGETPROCESSORINDEXFROMNUMBER g_pfnKeGetProcessorIndexFromNumber = NULL;
/** Pointer to KeGetProcessorNumberFromIndex. */
static PFNKEGETPROCESSORNUMBERFROMINDEX g_pfnKeGetProcessorNumberFromIndex = NULL;

#ifdef VBOX_WITH_HARDENING
/** Pointer to the stub device instance. */
static PDEVICE_OBJECT               g_pDevObjStub = NULL;
/** Spinlock protecting g_NtProtectTree as well as the releasing of protection
 *  structures. */
static RTSPINLOCK                   g_hNtProtectLock = NIL_RTSPINLOCK;
/** AVL tree of SUPDRVNTPROTECT structures. */
static AVLPVTREE                    g_NtProtectTree  = NULL;
/** Cookie returned by ObRegisterCallbacks for the callbacks. */
static PVOID                        g_pvObCallbacksCookie = NULL;
/** Combined windows NT version number.  See SUP_MAKE_NT_VER_COMBINED. */
uint32_t                            g_uNtVerCombined = 0;
/** Pointer to ObGetObjectType if available.. */
static PFNOBGETOBJECTTYPE           g_pfnObGetObjectType = NULL;
/** Pointer to ObRegisterCallbacks if available.. */
static PFNOBREGISTERCALLBACKS       g_pfnObRegisterCallbacks = NULL;
/** Pointer to ObUnregisterCallbacks if available.. */
static PFNOBUNREGISTERCALLBACKS     g_pfnObUnRegisterCallbacks = NULL;
/** Pointer to PsSetCreateProcessNotifyRoutineEx if available.. */
static PFNPSSETCREATEPROCESSNOTIFYROUTINEEX g_pfnPsSetCreateProcessNotifyRoutineEx = NULL;
/** Pointer to PsReferenceProcessFilePointer if available. */
static PFNPSREFERENCEPROCESSFILEPOINTER g_pfnPsReferenceProcessFilePointer = NULL;
/** Pointer to PsIsProtectedProcessLight. */
static PFNPSISPROTECTEDPROCESSLIGHT g_pfnPsIsProtectedProcessLight = NULL;
/** Pointer to ZwAlpcCreatePort. */
static PFNZWALPCCREATEPORT          g_pfnZwAlpcCreatePort = NULL;

# ifdef RT_ARCH_AMD64
extern "C" {
/** Pointer to KiServiceLinkage (used to fake missing ZwQueryVirtualMemory on
 *  XP64 / W2K3-64). */
PFNRT                               g_pfnKiServiceLinkage  = NULL;
/** Pointer to KiServiceInternal (used to fake missing ZwQueryVirtualMemory on
 *  XP64 / W2K3-64) */
PFNRT                               g_pfnKiServiceInternal = NULL;
}
# endif
/** The primary ALPC port object type. (LpcPortObjectType at init time.) */
static POBJECT_TYPE                 g_pAlpcPortObjectType1 = NULL;
/** The secondary ALPC port object type. (Sampled at runtime.) */
static POBJECT_TYPE volatile        g_pAlpcPortObjectType2 = NULL;

/** Pointer to the error information device instance. */
static PDEVICE_OBJECT               g_pDevObjErrorInfo = NULL;
/** Fast mutex semaphore protecting the error info list. */
static RTSEMMUTEX                   g_hErrorInfoLock = NIL_RTSEMMUTEX;
/** Head of the error info (SUPDRVNTERRORINFO). */
static RTLISTANCHOR                 g_ErrorInfoHead;

#endif


/**
 * Takes care of creating the devices and their symbolic links.
 *
 * @returns NT status code.
 * @param   pDrvObj     Pointer to driver object.
 */
static NTSTATUS vboxdrvNtCreateDevices(PDRIVER_OBJECT pDrvObj)
{
    /*
     * System device.
     */
    UNICODE_STRING DevName;
    RtlInitUnicodeString(&DevName, SUPDRV_NT_DEVICE_NAME_SYS);
    NTSTATUS rcNt = IoCreateDevice(pDrvObj, sizeof(SUPDRVDEVEXT), &DevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &g_pDevObjSys);
    if (NT_SUCCESS(rcNt))
    {
        /*
         * User device.
         */
        RtlInitUnicodeString(&DevName, SUPDRV_NT_DEVICE_NAME_USR);
        rcNt = IoCreateDevice(pDrvObj, sizeof(SUPDRVDEVEXTUSR), &DevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &g_pDevObjUsr);
        if (NT_SUCCESS(rcNt))
        {
            PSUPDRVDEVEXTUSR pDevExtUsr = (PSUPDRVDEVEXTUSR)g_pDevObjUsr->DeviceExtension;
            pDevExtUsr->pMainDrvExt = (PSUPDRVDEVEXT)g_pDevObjSys->DeviceExtension;
            pDevExtUsr->u32Cookie   = SUPDRVDEVEXTUSR_COOKIE;

#ifdef VBOX_WITH_HARDENING
            /*
             * Hardened stub device.
             */
            RtlInitUnicodeString(&DevName, SUPDRV_NT_DEVICE_NAME_STUB);
            rcNt = IoCreateDevice(pDrvObj, sizeof(SUPDRVDEVEXTSTUB), &DevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &g_pDevObjStub);
            if (NT_SUCCESS(rcNt))
            {
                if (NT_SUCCESS(rcNt))
                {
                    PSUPDRVDEVEXTSTUB pDevExtStub = (PSUPDRVDEVEXTSTUB)g_pDevObjStub->DeviceExtension;
                    pDevExtStub->Common.pMainDrvExt = (PSUPDRVDEVEXT)g_pDevObjSys->DeviceExtension;
                    pDevExtStub->Common.u32Cookie   = SUPDRVDEVEXTSTUB_COOKIE;

                    /*
                     * Hardened error information device.
                     */
                    RtlInitUnicodeString(&DevName, SUPDRV_NT_DEVICE_NAME_ERROR_INFO);
                    rcNt = IoCreateDevice(pDrvObj, sizeof(SUPDRVDEVEXTERRORINFO), &DevName, FILE_DEVICE_UNKNOWN, 0, FALSE,
                                          &g_pDevObjErrorInfo);
                    if (NT_SUCCESS(rcNt))
                    {
                        g_pDevObjErrorInfo->Flags |= DO_BUFFERED_IO;

                        if (NT_SUCCESS(rcNt))
                        {
                            PSUPDRVDEVEXTERRORINFO pDevExtStub = (PSUPDRVDEVEXTERRORINFO)g_pDevObjStub->DeviceExtension;
                            pDevExtStub->Common.pMainDrvExt = (PSUPDRVDEVEXT)g_pDevObjSys->DeviceExtension;
                            pDevExtStub->Common.u32Cookie   = SUPDRVDEVEXTERRORINFO_COOKIE;

#endif
                            /* Done. */
                            return rcNt;
#ifdef VBOX_WITH_HARDENING
                        }

                        /* Bail out. */
                        IoDeleteDevice(g_pDevObjErrorInfo);
                        g_pDevObjErrorInfo = NULL;
                    }
                }

                /* Bail out. */
                IoDeleteDevice(g_pDevObjStub);
                g_pDevObjUsr = NULL;
            }
            IoDeleteDevice(g_pDevObjUsr);
            g_pDevObjUsr = NULL;
#endif
        }
        IoDeleteDevice(g_pDevObjSys);
        g_pDevObjSys = NULL;
    }
    return rcNt;
}

/**
 * Destroys the devices and links created by vboxdrvNtCreateDevices.
 */
static void vboxdrvNtDestroyDevices(void)
{
    if (g_pDevObjUsr)
    {
        PSUPDRVDEVEXTUSR pDevExtUsr = (PSUPDRVDEVEXTUSR)g_pDevObjUsr->DeviceExtension;
        pDevExtUsr->pMainDrvExt = NULL;
    }
#ifdef VBOX_WITH_HARDENING
    if (g_pDevObjStub)
    {
        PSUPDRVDEVEXTSTUB pDevExtStub = (PSUPDRVDEVEXTSTUB)g_pDevObjStub->DeviceExtension;
        pDevExtStub->Common.pMainDrvExt = NULL;
    }
    if (g_pDevObjErrorInfo)
    {
        PSUPDRVDEVEXTERRORINFO pDevExtErrorInfo = (PSUPDRVDEVEXTERRORINFO)g_pDevObjStub->DeviceExtension;
        pDevExtErrorInfo->Common.pMainDrvExt = NULL;
    }
#endif

#ifdef VBOX_WITH_HARDENING
    IoDeleteDevice(g_pDevObjErrorInfo);
    g_pDevObjErrorInfo = NULL;
    IoDeleteDevice(g_pDevObjStub);
    g_pDevObjStub = NULL;
#endif
    IoDeleteDevice(g_pDevObjUsr);
    g_pDevObjUsr = NULL;
    IoDeleteDevice(g_pDevObjSys);
    g_pDevObjSys = NULL;
}


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

    /*
     * Sanity checks.
     */
#ifdef VBOXDRV_WITH_FAST_IO
    if (g_VBoxDrvFastIoDispatch.FastIoDeviceControl != VBoxDrvNtFastIoDeviceControl)
    {
        DbgPrint("VBoxDrv: FastIoDeviceControl=%p instead of %p\n",
                 g_VBoxDrvFastIoDispatch.FastIoDeviceControl, VBoxDrvNtFastIoDeviceControl);
        return STATUS_INTERNAL_ERROR;
    }
#endif

    /*
     * Query options first so any overflows on unpatched machines will do less
     * harm (see MS11-011 / 2393802 / 2011-03-18).
     *
     * Unfortunately, pRegPath isn't documented as zero terminated, even if it
     * quite likely always is, so we have to make a copy here.
     */
    NTSTATUS rcNt;
    PWSTR pwszCopy = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, pRegPath->Length + sizeof(WCHAR), 'VBox');
    if (pwszCopy)
    {
        memcpy(pwszCopy, pRegPath->Buffer, pRegPath->Length);
        pwszCopy[pRegPath->Length / sizeof(WCHAR)] = '\0';
        rcNt = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL, pwszCopy,
                                      g_aRegValues, NULL /*pvContext*/, NULL /*pvEnv*/);
        ExFreePoolWithTag(pwszCopy, 'VBox');
        /* Probably safe to ignore rcNt here. */
    }

    /*
     * Resolve methods we want but isn't available everywhere.
     */
    UNICODE_STRING RoutineName;
    RtlInitUnicodeString(&RoutineName, L"KeQueryMaximumGroupCount");
    g_pfnKeQueryMaximumGroupCount = (PFNKEQUERYMAXIMUMGROUPCOUNT)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"KeGetProcessorIndexFromNumber");
    g_pfnKeGetProcessorIndexFromNumber = (PFNKEGETPROCESSORINDEXFROMNUMBER)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"KeGetProcessorNumberFromIndex");
    g_pfnKeGetProcessorNumberFromIndex = (PFNKEGETPROCESSORNUMBERFROMINDEX)MmGetSystemRoutineAddress(&RoutineName);

    Assert(   (g_pfnKeGetProcessorNumberFromIndex != NULL) == (g_pfnKeGetProcessorIndexFromNumber != NULL)
           && (g_pfnKeGetProcessorNumberFromIndex != NULL) == (g_pfnKeQueryMaximumGroupCount != NULL)); /* all or nothing. */

    /*
     * Initialize the runtime (IPRT).
     */
    int vrc = RTR0Init(0);
    if (RT_SUCCESS(vrc))
    {
        Log(("VBoxDrv::DriverEntry\n"));

#ifdef VBOX_WITH_HARDENING
        /*
         * Initialize process protection.
         */
        rcNt = supdrvNtProtectInit();
        if (NT_SUCCESS(rcNt))
#endif
        {
            /*
             * Create device.
             * (That means creating a device object and a symbolic link so the DOS
             * subsystems (OS/2, win32, ++) can access the device.)
             */
            rcNt = vboxdrvNtCreateDevices(pDrvObj);
            if (NT_SUCCESS(rcNt))
            {
                /*
                 * Initialize the device extension.
                 */
                PSUPDRVDEVEXT pDevExt = (PSUPDRVDEVEXT)g_pDevObjSys->DeviceExtension;
                memset(pDevExt, 0, sizeof(*pDevExt));

                vrc = supdrvInitDevExt(pDevExt, sizeof(SUPDRVSESSION));
                if (!vrc)
                {
                    /*
                     * Setup the driver entry points in pDrvObj.
                     */
                    pDrvObj->DriverUnload                                   = VBoxDrvNtUnload;
                    pDrvObj->MajorFunction[IRP_MJ_CREATE]                   = VBoxDrvNtCreate;
                    pDrvObj->MajorFunction[IRP_MJ_CLEANUP]                  = VBoxDrvNtCleanup;
                    pDrvObj->MajorFunction[IRP_MJ_CLOSE]                    = VBoxDrvNtClose;
                    pDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = VBoxDrvNtDeviceControl;
                    pDrvObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]  = VBoxDrvNtInternalDeviceControl;
                    pDrvObj->MajorFunction[IRP_MJ_READ]                     = VBoxDrvNtRead;
                    pDrvObj->MajorFunction[IRP_MJ_WRITE]                    = VBoxDrvNtNotSupportedStub;

#ifdef VBOXDRV_WITH_FAST_IO
                    /* Fast I/O to speed up guest execution roundtrips. */
                    pDrvObj->FastIoDispatch = (PFAST_IO_DISPATCH)&g_VBoxDrvFastIoDispatch;
#endif

                    /*
                     * Register ourselves for power state changes.  We don't
                     * currently care if this fails.
                     */
                    UNICODE_STRING      CallbackName;
                    RtlInitUnicodeString(&CallbackName, L"\\Callback\\PowerState");

                    OBJECT_ATTRIBUTES   Attr;
                    InitializeObjectAttributes(&Attr, &CallbackName, OBJ_CASE_INSENSITIVE, NULL, NULL);

                    rcNt = ExCreateCallback(&pDevExt->pObjPowerCallback, &Attr, TRUE, TRUE);
                    if (rcNt == STATUS_SUCCESS)
                        pDevExt->hPowerCallback = ExRegisterCallback(pDevExt->pObjPowerCallback,
                                                                     VBoxPowerDispatchCallback,
                                                                     g_pDevObjSys);

                    /*
                     * Done! Returning success!
                     */
                    Log(("VBoxDrv::DriverEntry returning STATUS_SUCCESS\n"));
                    return STATUS_SUCCESS;
                }

                Log(("supdrvInitDevExit failed with vrc=%d!\n", vrc));
                rcNt = VBoxDrvNtErr2NtStatus(vrc);

                vboxdrvNtDestroyDevices();
            }
#ifdef VBOX_WITH_HARDENING
            supdrvNtProtectTerm();
#endif
        }
        RTTermRunCallbacks(RTTERMREASON_UNLOAD, 0);
        RTR0Term();
    }
    else
    {
        Log(("RTR0Init failed with vrc=%d!\n", vrc));
        rcNt = VBoxDrvNtErr2NtStatus(vrc);
    }
    if (NT_SUCCESS(rcNt))
        rcNt = STATUS_INVALID_PARAMETER;
    return rcNt;
}


/**
 * Unload the driver.
 *
 * @param   pDrvObj     Driver object.
 */
void _stdcall VBoxDrvNtUnload(PDRIVER_OBJECT pDrvObj)
{
    PSUPDRVDEVEXT pDevExt = (PSUPDRVDEVEXT)g_pDevObjSys->DeviceExtension;

    Log(("VBoxDrvNtUnload at irql %d\n", KeGetCurrentIrql()));

    /* Clean up the power callback registration. */
    if (pDevExt->hPowerCallback)
        ExUnregisterCallback(pDevExt->hPowerCallback);
    if (pDevExt->pObjPowerCallback)
        ObDereferenceObject(pDevExt->pObjPowerCallback);

    /*
     * We ASSUME that it's not possible to unload a driver with open handles.
     */
    supdrvDeleteDevExt(pDevExt);
#ifdef VBOX_WITH_HARDENING
    supdrvNtProtectTerm();
#endif
    RTTermRunCallbacks(RTTERMREASON_UNLOAD, 0);
    RTR0Term();
    vboxdrvNtDestroyDevices();

    NOREF(pDrvObj);
}


/**
 * For simplifying request completion into a simple return statement, extended
 * version.
 *
 * @returns rcNt
 * @param   rcNt                The status code.
 * @param   uInfo               Extra info value.
 * @param   pIrp                The IRP.
 */
DECLINLINE(NTSTATUS) supdrvNtCompleteRequestEx(NTSTATUS rcNt, ULONG_PTR uInfo, PIRP pIrp)
{
    pIrp->IoStatus.Status       = rcNt;
    pIrp->IoStatus.Information  = uInfo;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return rcNt;
}


/**
 * For simplifying request completion into a simple return statement.
 *
 * @returns rcNt
 * @param   rcNt                The status code.
 * @param   pIrp                The IRP.
 */
DECLINLINE(NTSTATUS) supdrvNtCompleteRequest(NTSTATUS rcNt, PIRP pIrp)
{
    return supdrvNtCompleteRequestEx(rcNt, 0 /*uInfo*/, pIrp);
}


/**
 * Create (i.e. Open) file entry point.
 *
 * @param   pDevObj     Device object.
 * @param   pIrp        Request packet.
 */
NTSTATUS _stdcall VBoxDrvNtCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    Log(("VBoxDrvNtCreate: RequestorMode=%d\n", pIrp->RequestorMode));
    PIO_STACK_LOCATION  pStack   = IoGetCurrentIrpStackLocation(pIrp);
    PFILE_OBJECT        pFileObj = pStack->FileObject;
    PSUPDRVDEVEXT       pDevExt  = SUPDRVNT_GET_DEVEXT(pDevObj);

    /*
     * We are not remotely similar to a directory...
     * (But this is possible.)
     */
    if (pStack->Parameters.Create.Options & FILE_DIRECTORY_FILE)
        return supdrvNtCompleteRequest(STATUS_NOT_A_DIRECTORY, pIrp);

    /*
     * Don't create a session for kernel clients, they'll close the handle
     * immediately and work with the file object via
     * VBoxDrvNtInternalDeviceControl.  The first request will be one to
     * create a session.
     */
    NTSTATUS rcNt;
    if (pIrp->RequestorMode == KernelMode)
    {
        if (pDevObj == g_pDevObjSys)
            return supdrvNtCompleteRequestEx(STATUS_SUCCESS, FILE_OPENED, pIrp);

        rcNt = STATUS_ACCESS_DENIED;
    }
#ifdef VBOX_WITH_HARDENING
    /*
     * Anyone can open the error device.
     */
    else if (pDevObj == g_pDevObjErrorInfo)
    {
        pFileObj->FsContext = NULL;
        return supdrvNtCompleteRequestEx(STATUS_SUCCESS, FILE_OPENED, pIrp);
    }
#endif
    else
    {
#if defined(VBOX_WITH_HARDENING) && !defined(VBOX_WITHOUT_DEBUGGER_CHECKS)
        /*
         * Make sure no debuggers are attached to non-user processes.
         */
        if (   pDevObj != g_pDevObjUsr
            && supdrvNtIsDebuggerAttached())
        {
            LogRel(("vboxdrv: Process %p is being debugged, access to vboxdrv / vboxdrvu declined.\n",
                    PsGetProcessId(PsGetCurrentProcess())));
            rcNt = STATUS_TRUST_FAILURE;
        }
        else
#endif
        {
            int rc = VINF_SUCCESS;

#ifdef VBOX_WITH_HARDENING
            /*
             * Access to the stub device is only granted to processes which
             * passes verification.
             *
             * Note! The stub device has no need for a SUPDRVSESSION structure,
             *       so the it uses the SUPDRVNTPROTECT directly instead.
             */
            if (pDevObj == g_pDevObjStub)
            {
                PSUPDRVNTPROTECT pNtProtect = NULL;
                rc = supdrvNtProtectCreate(&pNtProtect, PsGetProcessId(PsGetCurrentProcess()),
                                           kSupDrvNtProtectKind_StubUnverified, true /*fLink*/);
                if (RT_SUCCESS(rc))
                {
                    rc = supdrvNtProtectFindAssociatedCsrss(pNtProtect);
                    if (RT_SUCCESS(rc))
                        rc = supdrvNtProtectVerifyProcess(pNtProtect);
                    if (RT_SUCCESS(rc))
                    {
                        pFileObj->FsContext = pNtProtect; /* Keeps reference. */
                        return supdrvNtCompleteRequestEx(STATUS_SUCCESS, FILE_OPENED, pIrp);
                    }

                    supdrvNtProtectRelease(pNtProtect);
                }
                LogRel(("vboxdrv: Declined %p access to VBoxDrvStub: rc=%d\n", PsGetProcessId(PsGetCurrentProcess()), rc));
            }
            /*
             * Unrestricted access is only granted to a process in the
             * VmProcessUnconfirmed state that checks out correctly and is
             * allowed to transition to VmProcessConfirmed.  Again, only one
             * session per process.
             */
            else if (pDevObj != g_pDevObjUsr)
            {
                PSUPDRVNTPROTECT pNtProtect = supdrvNtProtectLookup(PsGetProcessId(PsGetCurrentProcess()));
                if (pNtProtect)
                {
                    if (pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed)
                    {
                        rc = supdrvNtProtectVerifyProcess(pNtProtect);
                        if (RT_SUCCESS(rc))
                        {
                            /* Create a session. */
                            PSUPDRVSESSION pSession;
                            rc = supdrvCreateSession(pDevExt, true /*fUser*/, pDevObj == g_pDevObjSys /*fUnrestricted*/,
                                                     &pSession);
                            if (RT_SUCCESS(rc))
                            {
                                rc = supdrvSessionHashTabInsert(pDevExt, pSession, (PSUPDRVSESSION *)&pFileObj->FsContext, NULL);
                                supdrvSessionRelease(pSession);
                                if (RT_SUCCESS(rc))
                                {
                                    pSession->pNtProtect = pNtProtect; /* Keeps reference. */
                                    return supdrvNtCompleteRequestEx(STATUS_SUCCESS, FILE_OPENED, pIrp);
                                }
                            }

                            /* No second attempt. */
                            RTSpinlockAcquire(g_hNtProtectLock);
                            if (pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessConfirmed)
                                pNtProtect->enmProcessKind = kSupDrvNtProtectKind_VmProcessDead;
                            RTSpinlockRelease(g_hNtProtectLock);

                            LogRel(("vboxdrv: supdrvCreateSession failed for process %p: rc=%d.\n",
                                    PsGetProcessId(PsGetCurrentProcess()), rc));
                        }
                        else
                            LogRel(("vboxdrv: Process %p failed process verification: rc=%d.\n",
                                    PsGetProcessId(PsGetCurrentProcess()), rc));
                    }
                    else
                    {
                        LogRel(("vboxdrv: %p is not a budding VM process (enmProcessKind=%d).\n",
                                PsGetProcessId(PsGetCurrentProcess()), pNtProtect->enmProcessKind));
                        rc = VERR_SUPDRV_NOT_BUDDING_VM_PROCESS_2;
                    }
                    supdrvNtProtectRelease(pNtProtect);
                }
                else
                {
                    LogRel(("vboxdrv: %p is not a budding VM process.\n", PsGetProcessId(PsGetCurrentProcess())));
                    rc = VERR_SUPDRV_NOT_BUDDING_VM_PROCESS_1;
                }
            }
            /*
             * Call common code to create an unprivileged session.
             */
            else
            {
                PSUPDRVSESSION pSession;
                rc = supdrvCreateSession(pDevExt, true /*fUser*/, false /*fUnrestricted*/, &pSession);
                if (RT_SUCCESS(rc))
                {
                    rc = supdrvSessionHashTabInsert(pDevExt, pSession, (PSUPDRVSESSION *)&pFileObj->FsContext, NULL);
                    supdrvSessionRelease(pSession);
                    if (RT_SUCCESS(rc))
                    {
                        pFileObj->FsContext  = pSession; /* Keeps reference. No race. */
                        pSession->pNtProtect = NULL;
                        return supdrvNtCompleteRequestEx(STATUS_SUCCESS, FILE_OPENED, pIrp);
                    }
                }
            }

#else  /* !VBOX_WITH_HARDENING */
            /*
             * Call common code to create a session.
             */
            pFileObj->FsContext = NULL;
            PSUPDRVSESSION pSession;
            rc = supdrvCreateSession(pDevExt, true /*fUser*/, pDevObj == g_pDevObjSys /*fUnrestricted*/, &pSession);
            if (RT_SUCCESS(rc))
            {
                rc = supdrvSessionHashTabInsert(pDevExt, pSession, (PSUPDRVSESSION *)&pFileObj->FsContext, NULL);
                supdrvSessionRelease(pSession);
                if (RT_SUCCESS(rc))
                    return supdrvNtCompleteRequestEx(STATUS_SUCCESS, FILE_OPENED, pIrp);

            }
#endif /* !VBOX_WITH_HARDENING */

            /* bail out */
            rcNt = VBoxDrvNtErr2NtStatus(rc);
        }
    }

    Assert(!NT_SUCCESS(rcNt));
    pFileObj->FsContext = NULL;
    return supdrvNtCompleteRequest(rcNt, pIrp); /* Note. the IoStatus is completely ignored on error. */
}


/**
 * Clean up file handle entry point.
 *
 * This is called when the last handle reference is released, or something like
 * that.  In the case of IoGetDeviceObjectPointer, this is called as it closes
 * the handle, however it will go on using the file object afterwards...
 *
 * @param   pDevObj     Device object.
 * @param   pIrp        Request packet.
 */
NTSTATUS _stdcall VBoxDrvNtCleanup(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    PSUPDRVDEVEXT       pDevExt  = SUPDRVNT_GET_DEVEXT(pDevObj);
    PIO_STACK_LOCATION  pStack   = IoGetCurrentIrpStackLocation(pIrp);
    PFILE_OBJECT        pFileObj = pStack->FileObject;

#ifdef VBOX_WITH_HARDENING
    if (pDevObj == g_pDevObjStub)
    {
        PSUPDRVNTPROTECT pNtProtect = (PSUPDRVNTPROTECT)pFileObj->FsContext;
        Log(("VBoxDrvNtCleanup: pDevExt=%p pFileObj=%p pNtProtect=%p\n", pDevExt, pFileObj, pNtProtect));
        if (pNtProtect)
        {
            supdrvNtProtectRelease(pNtProtect);
            pFileObj->FsContext = NULL;
        }
    }
    else if (pDevObj == g_pDevObjErrorInfo)
        supdrvNtErrorInfoCleanupProcess(PsGetCurrentProcessId());
    else
#endif
    {
        PSUPDRVSESSION pSession = supdrvSessionHashTabLookup(pDevExt, RTProcSelf(), RTR0ProcHandleSelf(),
                                                             (PSUPDRVSESSION *)&pFileObj->FsContext);
        Log(("VBoxDrvNtCleanup: pDevExt=%p pFileObj=%p pSession=%p\n", pDevExt, pFileObj, pSession));
        if (pSession)
        {
            supdrvSessionHashTabRemove(pDevExt, pSession, NULL);
            supdrvSessionRelease(pSession); /* Drops the reference from supdrvSessionHashTabLookup. */
        }
    }

    return supdrvNtCompleteRequest(STATUS_SUCCESS, pIrp);
}


/**
 * Close file entry point.
 *
 * @param   pDevObj     Device object.
 * @param   pIrp        Request packet.
 */
NTSTATUS _stdcall VBoxDrvNtClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    PSUPDRVDEVEXT       pDevExt  = SUPDRVNT_GET_DEVEXT(pDevObj);
    PIO_STACK_LOCATION  pStack   = IoGetCurrentIrpStackLocation(pIrp);
    PFILE_OBJECT        pFileObj = pStack->FileObject;

#ifdef VBOX_WITH_HARDENING
    if (pDevObj == g_pDevObjStub)
    {
        PSUPDRVNTPROTECT pNtProtect = (PSUPDRVNTPROTECT)pFileObj->FsContext;
        Log(("VBoxDrvNtClose: pDevExt=%p pFileObj=%p pNtProtect=%p\n", pDevExt, pFileObj, pNtProtect));
        if (pNtProtect)
        {
            supdrvNtProtectRelease(pNtProtect);
            pFileObj->FsContext = NULL;
        }
    }
    else if (pDevObj == g_pDevObjErrorInfo)
        supdrvNtErrorInfoCleanupProcess(PsGetCurrentProcessId());
    else
#endif
    {
        PSUPDRVSESSION pSession = supdrvSessionHashTabLookup(pDevExt, RTProcSelf(), RTR0ProcHandleSelf(),
                                                             (PSUPDRVSESSION *)&pFileObj->FsContext);
        Log(("VBoxDrvNtCleanup: pDevExt=%p pFileObj=%p pSession=%p\n", pDevExt, pFileObj, pSession));
        if (pSession)
        {
            supdrvSessionHashTabRemove(pDevExt, pSession, NULL);
            supdrvSessionRelease(pSession); /* Drops the reference from supdrvSessionHashTabLookup. */
        }
    }

    return supdrvNtCompleteRequest(STATUS_SUCCESS, pIrp);
}


#ifdef VBOXDRV_WITH_FAST_IO
/**
 * Fast I/O device control callback.
 *
 * This performs no buffering, neither on the way in or out.
 *
 * @returns TRUE if handled, FALSE if the normal I/O control routine should be
 *          called.
 * @param   pFileObj            The file object.
 * @param   fWait               Whether it's a blocking call
 * @param   pvInput             The input buffer as specified by the user.
 * @param   cbInput             The size of the input buffer.
 * @param   pvOutput            The output buffer as specfied by the user.
 * @param   cbOutput            The size of the output buffer.
 * @param   uCmd                The I/O command/function being invoked.
 * @param   pIoStatus           Where to return the status of the operation.
 * @param   pDevObj             The device object..
 */
static BOOLEAN _stdcall VBoxDrvNtFastIoDeviceControl(PFILE_OBJECT pFileObj, BOOLEAN fWait, PVOID pvInput, ULONG cbInput,
                                                     PVOID pvOutput, ULONG cbOutput, ULONG uCmd,
                                                     PIO_STATUS_BLOCK pIoStatus, PDEVICE_OBJECT pDevObj)
{
    RT_NOREF1(fWait);

    /*
     * Only the normal devices, not the stub or error info ones.
     */
    if (pDevObj != g_pDevObjSys && pDevObj != g_pDevObjUsr)
    {
        pIoStatus->Status      = STATUS_NOT_SUPPORTED;
        pIoStatus->Information = 0;
        return TRUE;
    }

    /*
     * Check the input a little bit and get a the session references.
     */
    PSUPDRVDEVEXT  pDevExt  = SUPDRVNT_GET_DEVEXT(pDevObj);
    PSUPDRVSESSION pSession = supdrvSessionHashTabLookup(pDevExt, RTProcSelf(), RTR0ProcHandleSelf(),
                                                         (PSUPDRVSESSION *)&pFileObj->FsContext);
    if (!pSession)
    {
        pIoStatus->Status      = STATUS_TRUST_FAILURE;
        pIoStatus->Information = 0;
        return TRUE;
    }

    if (pSession->fUnrestricted)
    {
#if defined(VBOX_WITH_HARDENING) && !defined(VBOX_WITHOUT_DEBUGGER_CHECKS)
        if (supdrvNtIsDebuggerAttached())
        {
            pIoStatus->Status      = STATUS_TRUST_FAILURE;
            pIoStatus->Information = 0;
            supdrvSessionRelease(pSession);
            return TRUE;
        }
#endif

        /*
         * Deal with the 2-3 high-speed IOCtl that takes their arguments from
         * the session and iCmd, and does not return anything.
         */
        if (   uCmd == SUP_IOCTL_FAST_DO_RAW_RUN
            || uCmd == SUP_IOCTL_FAST_DO_HM_RUN
            || uCmd == SUP_IOCTL_FAST_DO_NOP)
        {
            int rc = supdrvIOCtlFast(uCmd, (unsigned)(uintptr_t)pvOutput/* VMCPU id */, pDevExt, pSession);
            pIoStatus->Status      = RT_SUCCESS(rc) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
            pIoStatus->Information = 0; /* Could be used to pass rc if we liked. */
            supdrvSessionRelease(pSession);
            return TRUE;
        }
    }

    /*
     * The normal path.
     */
    NTSTATUS    rcNt;
    unsigned    cbOut = 0;
    int         rc = 0;
    Log2(("VBoxDrvNtFastIoDeviceControl(%p): ioctl=%#x pvIn=%p cbIn=%#x pvOut=%p cbOut=%#x pSession=%p\n",
          pDevExt, uCmd, pvInput, cbInput, pvOutput, cbOutput, pSession));

# ifdef RT_ARCH_AMD64
    /* Don't allow 32-bit processes to do any I/O controls. */
    if (!IoIs32bitProcess(NULL))
# endif
    {
        /*
         * In this fast I/O device control path we have to do our own buffering.
         */
        /* Verify that the I/O control function matches our pattern. */
        if ((uCmd & 0x3) == METHOD_BUFFERED)
        {
            /* Get the header so we can validate it a little bit against the
               parameters before allocating any memory kernel for the reqest. */
            SUPREQHDR Hdr;
            if (cbInput >= sizeof(Hdr) && cbOutput >= sizeof(Hdr))
            {
                __try
                {
                    RtlCopyMemory(&Hdr, pvInput, sizeof(Hdr));
                    rcNt = STATUS_SUCCESS;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rcNt = GetExceptionCode();
                    Hdr.cbIn = Hdr.cbOut = 0; /* shut up MSC */
                }
            }
            else
            {
                Hdr.cbIn = Hdr.cbOut = 0; /* shut up MSC */
                rcNt = STATUS_INVALID_PARAMETER;
            }
            if (NT_SUCCESS(rcNt))
            {
                /* Verify that the sizes in the request header are correct. */
                ULONG cbBuf = RT_MAX(cbInput, cbOutput);
                if (   cbInput  == Hdr.cbIn
                    && cbOutput == Hdr.cbOut
                    && cbBuf < _1M*16)
                {
                    /* Allocate a buffer and copy all the input into it. */
                    PSUPREQHDR pHdr = (PSUPREQHDR)ExAllocatePoolWithTag(NonPagedPool, cbBuf, 'VBox');
                    if (pHdr)
                    {
                        __try
                        {
                            RtlCopyMemory(pHdr, pvInput, cbInput);
                            if (cbInput < cbBuf)
                                RtlZeroMemory((uint8_t *)pHdr + cbInput, cbBuf - cbInput);
                            if (!memcmp(pHdr, &Hdr, sizeof(Hdr)))
                                rcNt = STATUS_SUCCESS;
                            else
                                rcNt = STATUS_INVALID_PARAMETER;
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            rcNt = GetExceptionCode();
                        }
                        if (NT_SUCCESS(rcNt))
                        {
                            /*
                             * Now call the common code to do the real work.
                             */
                            rc = supdrvIOCtl(uCmd, pDevExt, pSession, pHdr, cbBuf);
                            if (RT_SUCCESS(rc))
                            {
                                /*
                                 * Copy back the result.
                                 */
                                cbOut = pHdr->cbOut;
                                if (cbOut > cbOutput)
                                {
                                    cbOut = cbOutput;
                                    OSDBGPRINT(("VBoxDrvNtFastIoDeviceControl: too much output! %#x > %#x; uCmd=%#x!\n",
                                                pHdr->cbOut, cbOut, uCmd));
                                }
                                if (cbOut)
                                {
                                    __try
                                    {
                                        RtlCopyMemory(pvOutput, pHdr, cbOut);
                                        rcNt = STATUS_SUCCESS;
                                    }
                                    __except(EXCEPTION_EXECUTE_HANDLER)
                                    {
                                        rcNt = GetExceptionCode();
                                    }
                                }
                                else
                                    rcNt = STATUS_SUCCESS;
                            }
                            else if (rc == VERR_INVALID_PARAMETER)
                                rcNt = STATUS_INVALID_PARAMETER;
                            else
                                rcNt = STATUS_NOT_SUPPORTED;
                            Log2(("VBoxDrvNtFastIoDeviceControl: returns %#x cbOut=%d rc=%#x\n", rcNt, cbOut, rc));
                        }
                        else
                            Log(("VBoxDrvNtFastIoDeviceControl: Error reading %u bytes of user memory at %p (uCmd=%#x)\n",
                                 cbInput, pvInput, uCmd));
                        ExFreePoolWithTag(pHdr, 'VBox');
                    }
                    else
                        rcNt = STATUS_NO_MEMORY;
                }
                else
                {
                    Log(("VBoxDrvNtFastIoDeviceControl: Mismatching sizes (%#x) - Hdr=%#lx/%#lx Irp=%#lx/%#lx!\n",
                         uCmd, Hdr.cbIn, Hdr.cbOut, cbInput, cbOutput));
                    rcNt = STATUS_INVALID_PARAMETER;
                }
            }
        }
        else
        {
            Log(("VBoxDrvNtFastIoDeviceControl: not buffered request (%#x) - not supported\n", uCmd));
            rcNt = STATUS_NOT_SUPPORTED;
        }
    }
# ifdef RT_ARCH_AMD64
    else
    {
        Log(("VBoxDrvNtFastIoDeviceControl: WOW64 req - not supported\n"));
        rcNt = STATUS_NOT_SUPPORTED;
    }
# endif

    /* complete the request. */
    pIoStatus->Status = rcNt;
    pIoStatus->Information = cbOut;
    supdrvSessionRelease(pSession);
    return TRUE; /* handled. */
}
#endif /* VBOXDRV_WITH_FAST_IO */


/**
 * Device I/O Control entry point.
 *
 * @param   pDevObj     Device object.
 * @param   pIrp        Request packet.
 */
NTSTATUS _stdcall VBoxDrvNtDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    VBOXDRV_COMPLETE_IRP_AND_RETURN_IF_STUB_OR_ERROR_INFO_DEV(pDevObj, pIrp);

    PSUPDRVDEVEXT       pDevExt  = SUPDRVNT_GET_DEVEXT(pDevObj);
    PIO_STACK_LOCATION  pStack   = IoGetCurrentIrpStackLocation(pIrp);
    PSUPDRVSESSION      pSession = supdrvSessionHashTabLookup(pDevExt, RTProcSelf(), RTR0ProcHandleSelf(),
                                                              (PSUPDRVSESSION *)&pStack->FileObject->FsContext);

    if (!RT_VALID_PTR(pSession))
        return supdrvNtCompleteRequest(STATUS_TRUST_FAILURE, pIrp);

    /*
     * Deal with the 2-3 high-speed IOCtl that takes their arguments from
     * the session and iCmd, and does not return anything.
     */
    if (pSession->fUnrestricted)
    {
#if defined(VBOX_WITH_HARDENING) && !defined(VBOX_WITHOUT_DEBUGGER_CHECKS)
        if (supdrvNtIsDebuggerAttached())
        {
            supdrvSessionRelease(pSession);
            return supdrvNtCompleteRequest(STATUS_TRUST_FAILURE, pIrp);
        }
#endif

        ULONG ulCmd = pStack->Parameters.DeviceIoControl.IoControlCode;
        if (   ulCmd == SUP_IOCTL_FAST_DO_RAW_RUN
            || ulCmd == SUP_IOCTL_FAST_DO_HM_RUN
            || ulCmd == SUP_IOCTL_FAST_DO_NOP)
        {
            int rc = supdrvIOCtlFast(ulCmd, (unsigned)(uintptr_t)pIrp->UserBuffer /* VMCPU id */, pDevExt, pSession);

            /* Complete the I/O request. */
            supdrvSessionRelease(pSession);
            return supdrvNtCompleteRequest(RT_SUCCESS(rc) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER, pIrp);
        }
    }

    return VBoxDrvNtDeviceControlSlow(pDevExt, pSession, pIrp, pStack);
}


/**
 * Worker for VBoxDrvNtDeviceControl that takes the slow IOCtl functions.
 *
 * @returns NT status code.
 *
 * @param   pDevExt     Device extension.
 * @param   pSession    The session.
 * @param   pIrp        Request packet.
 * @param   pStack      The stack location containing the DeviceControl parameters.
 */
static int VBoxDrvNtDeviceControlSlow(PSUPDRVDEVEXT pDevExt, PSUPDRVSESSION pSession, PIRP pIrp, PIO_STACK_LOCATION pStack)
{
    NTSTATUS    rcNt;
    uint32_t    cbOut = 0;
    int         rc = 0;
    Log2(("VBoxDrvNtDeviceControlSlow(%p,%p): ioctl=%#x pBuf=%p cbIn=%#x cbOut=%#x pSession=%p\n",
          pDevExt, pIrp, pStack->Parameters.DeviceIoControl.IoControlCode,
          pIrp->AssociatedIrp.SystemBuffer, pStack->Parameters.DeviceIoControl.InputBufferLength,
          pStack->Parameters.DeviceIoControl.OutputBufferLength, pSession));

#ifdef RT_ARCH_AMD64
    /* Don't allow 32-bit processes to do any I/O controls. */
    if (!IoIs32bitProcess(pIrp))
#endif
    {
        /* Verify that it's a buffered CTL. */
        if ((pStack->Parameters.DeviceIoControl.IoControlCode & 0x3) == METHOD_BUFFERED)
        {
            /* Verify that the sizes in the request header are correct. */
            PSUPREQHDR pHdr = (PSUPREQHDR)pIrp->AssociatedIrp.SystemBuffer;
            if (    pStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(*pHdr)
                &&  pStack->Parameters.DeviceIoControl.InputBufferLength ==  pHdr->cbIn
                &&  pStack->Parameters.DeviceIoControl.OutputBufferLength ==  pHdr->cbOut)
            {
                /* Zero extra output bytes to make sure we don't leak anything. */
                if (pHdr->cbIn < pHdr->cbOut)
                    RtlZeroMemory((uint8_t *)pHdr + pHdr->cbIn, pHdr->cbOut - pHdr->cbIn);

                /*
                 * Do the job.
                 */
                rc = supdrvIOCtl(pStack->Parameters.DeviceIoControl.IoControlCode, pDevExt, pSession, pHdr,
                                 RT_MAX(pHdr->cbIn, pHdr->cbOut));
                if (!rc)
                {
                    rcNt = STATUS_SUCCESS;
                    cbOut = pHdr->cbOut;
                    if (cbOut > pStack->Parameters.DeviceIoControl.OutputBufferLength)
                    {
                        cbOut = pStack->Parameters.DeviceIoControl.OutputBufferLength;
                        OSDBGPRINT(("VBoxDrvNtDeviceControlSlow: too much output! %#x > %#x; uCmd=%#x!\n",
                                    pHdr->cbOut, cbOut, pStack->Parameters.DeviceIoControl.IoControlCode));
                    }
                }
                else
                    rcNt = STATUS_INVALID_PARAMETER;
                Log2(("VBoxDrvNtDeviceControlSlow: returns %#x cbOut=%d rc=%#x\n", rcNt, cbOut, rc));
            }
            else
            {
                Log(("VBoxDrvNtDeviceControlSlow: Mismatching sizes (%#x) - Hdr=%#lx/%#lx Irp=%#lx/%#lx!\n",
                     pStack->Parameters.DeviceIoControl.IoControlCode,
                     pStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(*pHdr) ? pHdr->cbIn : 0,
                     pStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(*pHdr) ? pHdr->cbOut : 0,
                     pStack->Parameters.DeviceIoControl.InputBufferLength,
                     pStack->Parameters.DeviceIoControl.OutputBufferLength));
                rcNt = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            Log(("VBoxDrvNtDeviceControlSlow: not buffered request (%#x) - not supported\n",
                 pStack->Parameters.DeviceIoControl.IoControlCode));
            rcNt = STATUS_NOT_SUPPORTED;
        }
    }
#ifdef RT_ARCH_AMD64
    else
    {
        Log(("VBoxDrvNtDeviceControlSlow: WOW64 req - not supported\n"));
        rcNt = STATUS_NOT_SUPPORTED;
    }
#endif

    /* complete the request. */
    pIrp->IoStatus.Status = rcNt;
    pIrp->IoStatus.Information = cbOut;
    supdrvSessionRelease(pSession);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return rcNt;
}


/**
 * Internal Device I/O Control entry point, used for IDC.
 *
 * @param   pDevObj     Device object.
 * @param   pIrp        Request packet.
 */
NTSTATUS _stdcall VBoxDrvNtInternalDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    VBOXDRV_COMPLETE_IRP_AND_RETURN_IF_STUB_OR_ERROR_INFO_DEV(pDevObj, pIrp);

    PSUPDRVDEVEXT       pDevExt  = SUPDRVNT_GET_DEVEXT(pDevObj);
    PIO_STACK_LOCATION  pStack   = IoGetCurrentIrpStackLocation(pIrp);
    PFILE_OBJECT        pFileObj = pStack ? pStack->FileObject : NULL;
    PSUPDRVSESSION      pSession = pFileObj ? (PSUPDRVSESSION)pFileObj->FsContext : NULL;
    NTSTATUS            rcNt;
    unsigned            cbOut = 0;
    int                 rc = 0;
    Log2(("VBoxDrvNtInternalDeviceControl(%p,%p): ioctl=%#x pBuf=%p cbIn=%#x cbOut=%#x pSession=%p\n",
          pDevExt, pIrp, pStack->Parameters.DeviceIoControl.IoControlCode,
          pIrp->AssociatedIrp.SystemBuffer, pStack->Parameters.DeviceIoControl.InputBufferLength,
          pStack->Parameters.DeviceIoControl.OutputBufferLength, pSession));

    /* Verify that it's a buffered CTL. */
    if ((pStack->Parameters.DeviceIoControl.IoControlCode & 0x3) == METHOD_BUFFERED)
    {
        /* Verify the pDevExt in the session. */
        if (  pStack->Parameters.DeviceIoControl.IoControlCode != SUPDRV_IDC_REQ_CONNECT
            ? VALID_PTR(pSession) && pSession->pDevExt == pDevExt
            : !pSession
           )
        {
            /* Verify that the size in the request header is correct. */
            PSUPDRVIDCREQHDR pHdr = (PSUPDRVIDCREQHDR)pIrp->AssociatedIrp.SystemBuffer;
            if (    pStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(*pHdr)
                &&  pStack->Parameters.DeviceIoControl.InputBufferLength  == pHdr->cb
                &&  pStack->Parameters.DeviceIoControl.OutputBufferLength == pHdr->cb)
            {
                /*
                 * Call the generic code.
                 *
                 * Note! Connect and disconnect requires some extra attention
                 *       in order to get the session handling right.
                 */
                if (pStack->Parameters.DeviceIoControl.IoControlCode == SUPDRV_IDC_REQ_DISCONNECT)
                    pFileObj->FsContext = NULL;

                rc = supdrvIDC(pStack->Parameters.DeviceIoControl.IoControlCode, pDevExt, pSession, pHdr);
                if (!rc)
                {
                    if (pStack->Parameters.DeviceIoControl.IoControlCode == SUPDRV_IDC_REQ_CONNECT)
                        pFileObj->FsContext = ((PSUPDRVIDCREQCONNECT)pHdr)->u.Out.pSession;

                    rcNt = STATUS_SUCCESS;
                    cbOut = pHdr->cb;
                }
                else
                {
                    rcNt = STATUS_INVALID_PARAMETER;
                    if (pStack->Parameters.DeviceIoControl.IoControlCode == SUPDRV_IDC_REQ_DISCONNECT)
                        pFileObj->FsContext = pSession;
                }
                Log2(("VBoxDrvNtInternalDeviceControl: returns %#x/rc=%#x\n", rcNt, rc));
            }
            else
            {
                Log(("VBoxDrvNtInternalDeviceControl: Mismatching sizes (%#x) - Hdr=%#lx Irp=%#lx/%#lx!\n",
                     pStack->Parameters.DeviceIoControl.IoControlCode,
                     pStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(*pHdr) ? pHdr->cb : 0,
                     pStack->Parameters.DeviceIoControl.InputBufferLength,
                     pStack->Parameters.DeviceIoControl.OutputBufferLength));
                rcNt = STATUS_INVALID_PARAMETER;
            }
        }
        else
            rcNt = STATUS_NOT_SUPPORTED;
    }
    else
    {
        Log(("VBoxDrvNtInternalDeviceControl: not buffered request (%#x) - not supported\n",
             pStack->Parameters.DeviceIoControl.IoControlCode));
        rcNt = STATUS_NOT_SUPPORTED;
    }

    /* complete the request. */
    pIrp->IoStatus.Status = rcNt;
    pIrp->IoStatus.Information = cbOut;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return rcNt;
}


/**
 * Implementation of the read major function for VBoxDrvErrorInfo.
 *
 * This is a stub function for the other devices.
 *
 * @returns NT status code.
 * @param   pDevObj             The device object.
 * @param   pIrp                The I/O request packet.
 */
NTSTATUS _stdcall VBoxDrvNtRead(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    Log(("VBoxDrvNtRead\n"));
    RT_NOREF1(pDevObj);

    NTSTATUS rcNt;
    pIrp->IoStatus.Information = 0;

#ifdef VBOX_WITH_HARDENING
    /*
     * VBoxDrvErrorInfo?
     */
    if (pDevObj == g_pDevObjErrorInfo)
    {
        PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);
        if (   pStack
            && (pIrp->Flags & IRP_BUFFERED_IO))
        {
            /*
             * Look up the process error information.
             */
            HANDLE hCurThreadId  = PsGetCurrentThreadId();
            HANDLE hCurProcessId = PsGetCurrentProcessId();
            int rc = RTSemMutexRequestNoResume(g_hErrorInfoLock, RT_INDEFINITE_WAIT);
            if (RT_SUCCESS(rc))
            {
                PSUPDRVNTERRORINFO  pCur;
                RTListForEach(&g_ErrorInfoHead, pCur, SUPDRVNTERRORINFO, ListEntry)
                {
                    if (   pCur->hProcessId == hCurProcessId
                        && pCur->hThreadId  == hCurThreadId)
                        break;
                }

                /*
                 * Did we find error info and is the caller requesting data within it?
                 * If so, check the destination buffer and copy the data into it.
                 */
                if (   pCur
                    && pStack->Parameters.Read.ByteOffset.QuadPart < pCur->cchErrorInfo
                    && pStack->Parameters.Read.ByteOffset.QuadPart >= 0)
                {
                    PVOID pvDstBuf = pIrp->AssociatedIrp.SystemBuffer;
                    if (pvDstBuf)
                    {
                        uint32_t offRead  = (uint32_t)pStack->Parameters.Read.ByteOffset.QuadPart;
                        uint32_t cbToRead = pCur->cchErrorInfo - offRead;
                        if (cbToRead < pStack->Parameters.Read.Length)
                            RT_BZERO((uint8_t *)pvDstBuf + cbToRead, pStack->Parameters.Read.Length - cbToRead);
                        else
                            cbToRead = pStack->Parameters.Read.Length;
                        memcpy(pvDstBuf, &pCur->szErrorInfo[offRead], cbToRead);
                        pIrp->IoStatus.Information = cbToRead;

                        rcNt = STATUS_SUCCESS;
                    }
                    else
                        rcNt = STATUS_INVALID_ADDRESS;
                }
                /*
                 * End of file. Free the info.
                 */
                else if (pCur)
                {
                    RTListNodeRemove(&pCur->ListEntry);
                    RTMemFree(pCur);
                    rcNt = STATUS_END_OF_FILE;
                }
                /*
                 * We found no error info. Return EOF.
                 */
                else
                    rcNt = STATUS_END_OF_FILE;

                RTSemMutexRelease(g_hErrorInfoLock);
            }
            else
                rcNt = STATUS_UNSUCCESSFUL;

            /* Paranoia: Clear the buffer on failure. */
            if (!NT_SUCCESS(rcNt))
            {
                PVOID pvDstBuf = pIrp->AssociatedIrp.SystemBuffer;
                if (   pvDstBuf
                    && pStack->Parameters.Read.Length)
                    RT_BZERO(pvDstBuf, pStack->Parameters.Read.Length);
            }
        }
        else
            rcNt = STATUS_INVALID_PARAMETER;
    }
    else
#endif /* VBOX_WITH_HARDENING */
    {
        /*
         * Stub.
         */
        rcNt = STATUS_NOT_SUPPORTED;
    }

    /*
     * Complete the request.
     */
    pIrp->IoStatus.Status = rcNt;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return rcNt;
}


/**
 * Stub function for functions we don't implemented.
 *
 * @returns STATUS_NOT_SUPPORTED
 * @param   pDevObj     Device object.
 * @param   pIrp        IRP.
 */
NTSTATUS _stdcall VBoxDrvNtNotSupportedStub(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    Log(("VBoxDrvNtNotSupportedStub\n"));
    NOREF(pDevObj);

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_NOT_SUPPORTED;
}


/**
 * ExRegisterCallback handler for power events
 *
 * @param   pCallbackContext    User supplied parameter (pDevObj)
 * @param   pvArgument1         First argument
 * @param   pvArgument2         Second argument
 */
VOID _stdcall VBoxPowerDispatchCallback(PVOID pCallbackContext, PVOID pvArgument1, PVOID pvArgument2)
{
    /*PDEVICE_OBJECT pDevObj = (PDEVICE_OBJECT)pCallbackContext;*/ RT_NOREF1(pCallbackContext);
    Log(("VBoxPowerDispatchCallback: %x %x\n", pvArgument1, pvArgument2));

    /* Power change imminent? */
    if ((uintptr_t)pvArgument1 == PO_CB_SYSTEM_STATE_LOCK)
    {
        if (pvArgument2 == NULL)
            Log(("VBoxPowerDispatchCallback: about to go into suspend mode!\n"));
        else
            Log(("VBoxPowerDispatchCallback: resumed!\n"));

        /* Inform any clients that have registered themselves with IPRT. */
        RTPowerSignalEvent(pvArgument2 == NULL ? RTPOWEREVENT_SUSPEND : RTPOWEREVENT_RESUME);
    }
}


/**
 * Called to clean up the session structure before it's freed.
 *
 * @param   pDevExt             The device globals.
 * @param   pSession            The session that's being cleaned up.
 */
void VBOXCALL supdrvOSCleanupSession(PSUPDRVDEVEXT pDevExt, PSUPDRVSESSION pSession)
{
#ifdef VBOX_WITH_HARDENING
    if (pSession->pNtProtect)
    {
        supdrvNtProtectRelease(pSession->pNtProtect);
        pSession->pNtProtect = NULL;
    }
    RT_NOREF1(pDevExt);
#else
    RT_NOREF2(pDevExt, pSession);
#endif
}


void VBOXCALL supdrvOSSessionHashTabInserted(PSUPDRVDEVEXT pDevExt, PSUPDRVSESSION pSession, void *pvUser)
{
    NOREF(pDevExt); NOREF(pSession); NOREF(pvUser);
}


void VBOXCALL supdrvOSSessionHashTabRemoved(PSUPDRVDEVEXT pDevExt, PSUPDRVSESSION pSession, void *pvUser)
{
    NOREF(pDevExt); NOREF(pSession); NOREF(pvUser);
}


size_t VBOXCALL supdrvOSGipGetGroupTableSize(PSUPDRVDEVEXT pDevExt)
{
    NOREF(pDevExt);
    uint32_t cMaxCpus = RTMpGetCount();
    uint32_t cGroups  = RTMpGetMaxCpuGroupCount();

    return cGroups * RT_OFFSETOF(SUPGIPCPUGROUP, aiCpuSetIdxs)
         + RT_SIZEOFMEMB(SUPGIPCPUGROUP, aiCpuSetIdxs[0]) * cMaxCpus;
}


int VBOXCALL supdrvOSInitGipGroupTable(PSUPDRVDEVEXT pDevExt, PSUPGLOBALINFOPAGE pGip, size_t cbGipCpuGroups)
{
    Assert(cbGipCpuGroups > 0); NOREF(cbGipCpuGroups); NOREF(pDevExt);

    unsigned const  cGroups = RTMpGetMaxCpuGroupCount();
    AssertReturn(cGroups > 0 && cGroups < RT_ELEMENTS(pGip->aoffCpuGroup), VERR_INTERNAL_ERROR_2);
    pGip->cPossibleCpuGroups = cGroups;

    PSUPGIPCPUGROUP pGroup = (PSUPGIPCPUGROUP)&pGip->aCPUs[pGip->cCpus];
    for (uint32_t idxGroup = 0; idxGroup < cGroups; idxGroup++)
    {
        uint32_t cActive  = 0;
        uint32_t cMax     = RTMpGetCpuGroupCounts(idxGroup, &cActive);
        uint32_t cbNeeded = RT_OFFSETOF(SUPGIPCPUGROUP, aiCpuSetIdxs[cMax]);
        AssertReturn(cbNeeded <= cbGipCpuGroups, VERR_INTERNAL_ERROR_3);
        AssertReturn(cActive <= cMax, VERR_INTERNAL_ERROR_4);

        pGip->aoffCpuGroup[idxGroup] = (uint16_t)((uintptr_t)pGroup - (uintptr_t)pGip);
        pGroup->cMembers    = cActive;
        pGroup->cMaxMembers = cMax;
        for (uint32_t idxMember = 0; idxMember < cMax; idxMember++)
        {
            pGroup->aiCpuSetIdxs[idxMember] = RTMpSetIndexFromCpuGroupMember(idxGroup, idxMember);
            Assert((unsigned)pGroup->aiCpuSetIdxs[idxMember] < pGip->cPossibleCpus);
        }

        /* advance. */
        cbGipCpuGroups -= cbNeeded;
        pGroup = (PSUPGIPCPUGROUP)&pGroup->aiCpuSetIdxs[cMax];
    }

    return VINF_SUCCESS;
}


void VBOXCALL supdrvOSGipInitGroupBitsForCpu(PSUPDRVDEVEXT pDevExt, PSUPGLOBALINFOPAGE pGip, PSUPGIPCPU pGipCpu)
{
    NOREF(pDevExt);

    /*
     * Translate the CPU index into a group and member.
     */
    PROCESSOR_NUMBER ProcNum = { 0, pGipCpu->iCpuSet, 0 };
    if (g_pfnKeGetProcessorNumberFromIndex)
    {
        NTSTATUS rcNt = g_pfnKeGetProcessorNumberFromIndex(pGipCpu->iCpuSet, &ProcNum);
        if (NT_SUCCESS(rcNt))
            Assert(ProcNum.Group < g_pfnKeQueryMaximumGroupCount());
        else
        {
            AssertFailed();
            ProcNum.Group  = 0;
            ProcNum.Number = pGipCpu->iCpuSet;
        }
    }
    pGipCpu->iCpuGroup       = ProcNum.Group;
    pGipCpu->iCpuGroupMember = ProcNum.Number;

    /*
     * Update the group info.  Just do this wholesale for now (doesn't scale well).
     */
    for (uint32_t idxGroup = 0; idxGroup < pGip->cPossibleCpuGroups; idxGroup++)
        if (pGip->aoffCpuGroup[idxGroup] != UINT16_MAX)
        {
            PSUPGIPCPUGROUP pGroup = (PSUPGIPCPUGROUP)((uintptr_t)pGip + pGip->aoffCpuGroup[idxGroup]);

            uint32_t cActive  = 0;
            uint32_t cMax     = RTMpGetCpuGroupCounts(idxGroup, &cActive);
            AssertStmt(cMax == pGroup->cMaxMembers, cMax = pGroup->cMaxMembers);
            AssertStmt(cActive <= cMax, cActive = cMax);
            if (pGroup->cMembers != cActive)
                pGroup->cMembers = cActive;

            for (uint32_t idxMember = 0; idxMember < cMax; idxMember++)
            {
                int idxCpuSet = RTMpSetIndexFromCpuGroupMember(idxGroup, idxMember);
                AssertMsg((unsigned)idxCpuSet < pGip->cPossibleCpus,
                          ("%d vs %d for %u.%u\n", idxCpuSet, pGip->cPossibleCpus, idxGroup, idxMember));

                if (pGroup->aiCpuSetIdxs[idxMember] != idxCpuSet)
                    pGroup->aiCpuSetIdxs[idxMember] = idxCpuSet;
            }
        }
}


/**
 * Initializes any OS specific object creator fields.
 */
void VBOXCALL   supdrvOSObjInitCreator(PSUPDRVOBJ pObj, PSUPDRVSESSION pSession)
{
    NOREF(pObj);
    NOREF(pSession);
}


/**
 * Checks if the session can access the object.
 *
 * @returns true if a decision has been made.
 * @returns false if the default access policy should be applied.
 *
 * @param   pObj        The object in question.
 * @param   pSession    The session wanting to access the object.
 * @param   pszObjName  The object name, can be NULL.
 * @param   prc         Where to store the result when returning true.
 */
bool VBOXCALL   supdrvOSObjCanAccess(PSUPDRVOBJ pObj, PSUPDRVSESSION pSession, const char *pszObjName, int *prc)
{
    NOREF(pObj);
    NOREF(pSession);
    NOREF(pszObjName);
    NOREF(prc);
    return false;
}


/**
 * Force async tsc mode (stub).
 */
bool VBOXCALL  supdrvOSGetForcedAsyncTscMode(PSUPDRVDEVEXT pDevExt)
{
    RT_NOREF1(pDevExt);
    return g_Options.fOptForceAsyncTsc != 0;
}


/**
 * Whether the host takes CPUs offline during a suspend/resume operation.
 */
bool VBOXCALL  supdrvOSAreCpusOfflinedOnSuspend(void)
{
    return false;
}


/**
 * Whether the hardware TSC has been synchronized by the OS.
 */
bool VBOXCALL  supdrvOSAreTscDeltasInSync(void)
{
    /* If IPRT didn't find KeIpiGenericCall we pretend windows(, the firmware,
       or whoever) always configures TSCs perfectly. */
    return !RTMpOnPairIsConcurrentExecSupported();
}


/**
 * Checks whether we're allowed by Hyper-V to modify CR4.
 */
int  VBOXCALL supdrvOSGetRawModeUsability(void)
{
    int rc = VINF_SUCCESS;

#ifdef RT_ARCH_AMD64
    /*
     * Broadwell running W10 17083.100:
     *        CR4: 0x170678
     *  Evil mask: 0x170638
     *      X86_CR4_SMEP        - evil
     *      X86_CR4_FSGSBASE    - evil
     *      X86_CR4_PCIDE       - evil
     *      X86_CR4_OSXSAVE     - evil
     *      X86_CR4_OSFXSR      - evil
     *      X86_CR4_OSXMMEEXCPT - evil
     *      X86_CR4_PSE         - evil
     *      X86_CR4_PAE         - evil
     *      X86_CR4_MCE         - okay
     *      X86_CR4_DE          - evil
     */
    if (ASMHasCpuId())
    {
        uint32_t cStd = ASMCpuId_EAX(0);
        if (ASMIsValidStdRange(cStd))
        {
            uint32_t uIgn         = 0;
            uint32_t fEdxFeatures = 0;
            uint32_t fEcxFeatures = 0;
            ASMCpuIdExSlow(1, 0, 0, 0, &uIgn, &uIgn, &fEcxFeatures, &fEdxFeatures);
            if (fEcxFeatures & X86_CPUID_FEATURE_ECX_HVP)
            {
                RTCCUINTREG  const fOldFlags    = ASMIntDisableFlags();
                RTCCUINTXREG const fCr4         = ASMGetCR4();

                RTCCUINTXREG const fSafeToClear = X86_CR4_TSD      | X86_CR4_DE     | X86_CR4_PGE  | X86_CR4_PCE
                                                | X86_CR4_FSGSBASE | X86_CR4_PCIDE  | X86_CR4_SMEP | X86_CR4_SMAP
                                                | X86_CR4_OSXSAVE  | X86_CR4_OSFXSR | X86_CR4_OSXMMEEXCPT;
                RTCCUINTXREG       fLoadCr4     = fCr4 & ~fSafeToClear;
                RTCCUINTXREG const fCleared     = fCr4 & fSafeToClear;
                if (!(fCleared & X86_CR4_TSD) && (fEdxFeatures & X86_CPUID_FEATURE_EDX_TSC))
                    fLoadCr4 |= X86_CR4_TSD;
                if (!(fCleared & X86_CR4_PGE) && (fEdxFeatures & X86_CPUID_FEATURE_EDX_PGE))
                    fLoadCr4 |= X86_CR4_PGE;
                __try
                {
                    ASMSetCR4(fLoadCr4);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rc = VERR_SUPDRV_NO_RAW_MODE_HYPER_V_ROOT;
                }
                if (RT_SUCCESS(rc))
                    ASMSetCR4(fCr4);
                ASMSetFlags(fOldFlags);
            }
        }
    }
#endif
    return rc;
}


#define MY_SystemLoadGdiDriverInSystemSpaceInformation  54
#define MY_SystemUnloadGdiDriverInformation             27

typedef struct MYSYSTEMGDIDRIVERINFO
{
    UNICODE_STRING  Name;                   /**< In:  image file name. */
    PVOID           ImageAddress;           /**< Out: the load address. */
    PVOID           SectionPointer;         /**< Out: section object. */
    PVOID           EntryPointer;           /**< Out: entry point address. */
    PVOID           ExportSectionPointer;   /**< Out: export directory/section. */
    ULONG           ImageLength;            /**< Out: SizeOfImage. */
} MYSYSTEMGDIDRIVERINFO;

extern "C" __declspec(dllimport) NTSTATUS NTAPI ZwSetSystemInformation(ULONG, PVOID, ULONG);

int  VBOXCALL   supdrvOSLdrOpen(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage, const char *pszFilename)
{
    pImage->pvNtSectionObj = NULL;
    pImage->hMemLock = NIL_RTR0MEMOBJ;

#ifdef VBOX_WITHOUT_NATIVE_R0_LOADER
# ifndef RT_ARCH_X86
#  error "VBOX_WITHOUT_NATIVE_R0_LOADER is only safe on x86."
# endif
    NOREF(pDevExt); NOREF(pszFilename); NOREF(pImage);
    return VERR_NOT_SUPPORTED;

#else
    /*
     * Convert the filename from DOS UTF-8 to NT UTF-16.
     */
    size_t cwcFilename;
    int rc = RTStrCalcUtf16LenEx(pszFilename, RTSTR_MAX, &cwcFilename);
    if (RT_FAILURE(rc))
        return rc;

    PRTUTF16 pwcsFilename = (PRTUTF16)RTMemTmpAlloc((4 + cwcFilename + 1) * sizeof(RTUTF16));
    if (!pwcsFilename)
        return VERR_NO_TMP_MEMORY;

    pwcsFilename[0] = '\\';
    pwcsFilename[1] = '?';
    pwcsFilename[2] = '?';
    pwcsFilename[3] = '\\';
    PRTUTF16 pwcsTmp = &pwcsFilename[4];
    rc = RTStrToUtf16Ex(pszFilename, RTSTR_MAX, &pwcsTmp, cwcFilename + 1, NULL);
    if (RT_SUCCESS(rc))
    {
        /*
         * Try load it.
         */
        MYSYSTEMGDIDRIVERINFO Info;
        RtlInitUnicodeString(&Info.Name, pwcsFilename);
        Info.ImageAddress           = NULL;
        Info.SectionPointer         = NULL;
        Info.EntryPointer           = NULL;
        Info.ExportSectionPointer   = NULL;
        Info.ImageLength            = 0;

        NTSTATUS rcNt = ZwSetSystemInformation(MY_SystemLoadGdiDriverInSystemSpaceInformation, &Info, sizeof(Info));
        if (NT_SUCCESS(rcNt))
        {
            pImage->pvImage = Info.ImageAddress;
            pImage->pvNtSectionObj = Info.SectionPointer;
            Log(("ImageAddress=%p SectionPointer=%p ImageLength=%#x cbImageBits=%#x rcNt=%#x '%ls'\n",
                 Info.ImageAddress, Info.SectionPointer, Info.ImageLength, pImage->cbImageBits, rcNt, Info.Name.Buffer));
# ifdef DEBUG_bird
            SUPR0Printf("ImageAddress=%p SectionPointer=%p ImageLength=%#x cbImageBits=%#x rcNt=%#x '%ws'\n",
                        Info.ImageAddress, Info.SectionPointer, Info.ImageLength, pImage->cbImageBits, rcNt, Info.Name.Buffer);
# endif
            if (pImage->cbImageBits == Info.ImageLength)
            {
                /*
                 * Lock down the entire image, just to be on the safe side.
                 */
                rc = RTR0MemObjLockKernel(&pImage->hMemLock, pImage->pvImage, pImage->cbImageBits, RTMEM_PROT_READ);
                if (RT_FAILURE(rc))
                {
                    pImage->hMemLock = NIL_RTR0MEMOBJ;
                    supdrvOSLdrUnload(pDevExt, pImage);
                }
            }
            else
            {
                supdrvOSLdrUnload(pDevExt, pImage);
                rc = VERR_LDR_MISMATCH_NATIVE;
            }
        }
        else
        {
            Log(("rcNt=%#x '%ls'\n", rcNt, pwcsFilename));
            SUPR0Printf("VBoxDrv: rcNt=%x '%ws'\n", rcNt, pwcsFilename);
            switch (rcNt)
            {
                case /* 0xc0000003 */ STATUS_INVALID_INFO_CLASS:
# ifdef RT_ARCH_AMD64
                    /* Unwind will crash and BSOD, so no fallback here! */
                    rc = VERR_NOT_IMPLEMENTED;
# else
                    /*
                     * Use the old way of loading the modules.
                     *
                     * Note! We do *NOT* try class 26 because it will probably
                     *       not work correctly on terminal servers and such.
                     */
                    rc = VERR_NOT_SUPPORTED;
# endif
                    break;
                case /* 0xc0000034 */ STATUS_OBJECT_NAME_NOT_FOUND:
                    rc = VERR_MODULE_NOT_FOUND;
                    break;
                case /* 0xC0000263 */ STATUS_DRIVER_ENTRYPOINT_NOT_FOUND:
                    rc = VERR_LDR_IMPORTED_SYMBOL_NOT_FOUND;
                    break;
                case /* 0xC0000428 */ STATUS_INVALID_IMAGE_HASH:
                    rc = VERR_LDR_IMAGE_HASH;
                    break;
                case /* 0xC000010E */ STATUS_IMAGE_ALREADY_LOADED:
                    Log(("WARNING: see @bugref{4853} for cause of this failure on Windows 7 x64\n"));
                    rc = VERR_ALREADY_LOADED;
                    break;
                default:
                    rc = VERR_LDR_GENERAL_FAILURE;
                    break;
            }

            pImage->pvNtSectionObj = NULL;
        }
    }

    RTMemTmpFree(pwcsFilename);
    NOREF(pDevExt);
    return rc;
#endif
}


void VBOXCALL   supdrvOSLdrNotifyOpened(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage, const char *pszFilename)
{
    NOREF(pDevExt); NOREF(pImage); NOREF(pszFilename);
}

void VBOXCALL   supdrvOSLdrNotifyUnloaded(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage)
{
    NOREF(pDevExt); NOREF(pImage);
}

int  VBOXCALL   supdrvOSLdrValidatePointer(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage, void *pv, const uint8_t *pbImageBits)
{
    NOREF(pDevExt); NOREF(pImage); NOREF(pv); NOREF(pbImageBits);
    return VINF_SUCCESS;
}


/**
 * memcmp + errormsg + log.
 *
 * @returns Same as memcmp.
 * @param   pImage          The image.
 * @param   pbImageBits     The image bits ring-3 uploads.
 * @param   uRva            The RVA to start comparing at.
 * @param   cb              The number of bytes to compare.
 * @param   pReq            The load request.
 */
static int supdrvNtCompare(PSUPDRVLDRIMAGE pImage, const uint8_t *pbImageBits, uint32_t uRva, uint32_t cb, PSUPLDRLOAD pReq)
{
    int iDiff = memcmp((uint8_t const *)pImage->pvImage + uRva, pbImageBits + uRva, cb);
    if (iDiff)
    {
        uint32_t        cbLeft = cb;
        const uint8_t  *pbNativeBits = (const uint8_t *)pImage->pvImage;
        for (size_t off = uRva; cbLeft > 0; off++, cbLeft--)
            if (pbNativeBits[off] != pbImageBits[off])
            {
                /* Note! We need to copy image bits into a temporary stack buffer here as we'd
                         otherwise risk overwriting them while formatting the error message. */
                uint8_t abBytes[64];
                memcpy(abBytes, &pbImageBits[off], RT_MIN(64, cbLeft));
                supdrvLdrLoadError(VERR_LDR_MISMATCH_NATIVE, pReq,
                                   "Mismatch at %#x (%p) of %s loaded at %p:\n"
                                   "ntld: %.*Rhxs\n"
                                   "iprt: %.*Rhxs",
                                   off, &pbNativeBits[off], pImage->szName, pImage->pvImage,
                                   RT_MIN(64, cbLeft), &pbNativeBits[off],
                                   RT_MIN(64, cbLeft), &abBytes[0]);
                SUPR0Printf("VBoxDrv: %s", pReq->u.Out.szError);
                break;
            }
    }
    return iDiff;
}


int  VBOXCALL   supdrvOSLdrLoad(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage, const uint8_t *pbImageBits, PSUPLDRLOAD pReq)
{
    NOREF(pDevExt);
    if (pImage->pvNtSectionObj)
    {
        /*
         * Usually, the entire image matches exactly.
         */
        if (!memcmp(pImage->pvImage, pbImageBits, pImage->cbImageBits))
            return VINF_SUCCESS;

        /*
         * On Windows 10 the ImageBase member of the optional header is sometimes
         * updated with the actual load address and sometimes not.  Try compare
         *
         */
        uint32_t const  offNtHdrs = *(uint16_t *)pbImageBits == IMAGE_DOS_SIGNATURE
                                  ? ((IMAGE_DOS_HEADER const *)pbImageBits)->e_lfanew
                                  : 0;
        AssertLogRelReturn(offNtHdrs + sizeof(IMAGE_NT_HEADERS) < pImage->cbImageBits, VERR_INTERNAL_ERROR_5);
        IMAGE_NT_HEADERS const *pNtHdrsIprt = (IMAGE_NT_HEADERS const *)(pbImageBits + offNtHdrs);
        IMAGE_NT_HEADERS const *pNtHdrsNtLd = (IMAGE_NT_HEADERS const *)((uintptr_t)pImage->pvImage + offNtHdrs);

        uint32_t const  offImageBase = offNtHdrs + RT_OFFSETOF(IMAGE_NT_HEADERS, OptionalHeader.ImageBase);
        uint32_t const  cbImageBase  = RT_SIZEOFMEMB(IMAGE_NT_HEADERS, OptionalHeader.ImageBase);
        if (   pNtHdrsNtLd->OptionalHeader.ImageBase != pNtHdrsIprt->OptionalHeader.ImageBase
            && (   pNtHdrsNtLd->OptionalHeader.ImageBase == (uintptr_t)pImage->pvImage
                || pNtHdrsIprt->OptionalHeader.ImageBase == (uintptr_t)pImage->pvImage)
            && pNtHdrsIprt->Signature == IMAGE_NT_SIGNATURE
            && pNtHdrsIprt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC
            && !memcmp(pImage->pvImage, pbImageBits, offImageBase)
            && !memcmp((uint8_t const *)pImage->pvImage + offImageBase + cbImageBase,
                       pbImageBits                      + offImageBase + cbImageBase,
                       pImage->cbImageBits              - offImageBase - cbImageBase))
            return VINF_SUCCESS;

        /*
         * On Windows Server 2003 (sp2 x86) both import thunk tables are fixed
         * up and we typically get a mismatch in the INIT section.
         *
         * So, lets see if everything matches when excluding the
         * OriginalFirstThunk tables and (maybe) the ImageBase member.
         * For simplicity the max number of exclusion regions is set to 16.
         */
        if (    pNtHdrsIprt->Signature == IMAGE_NT_SIGNATURE
            &&  pNtHdrsIprt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC
            &&  pNtHdrsIprt->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT
            &&  pNtHdrsIprt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size >= sizeof(IMAGE_IMPORT_DESCRIPTOR)
            &&  pNtHdrsIprt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress > sizeof(IMAGE_NT_HEADERS)
            &&  pNtHdrsIprt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress < pImage->cbImageBits
            )
        {
            struct MyRegion
            {
                uint32_t uRva;
                uint32_t cb;
            }           aExcludeRgns[16];
            unsigned    cExcludeRgns = 0;

            /* ImageBase: */
            if (   pNtHdrsNtLd->OptionalHeader.ImageBase != pNtHdrsIprt->OptionalHeader.ImageBase
                && (   pNtHdrsNtLd->OptionalHeader.ImageBase == (uintptr_t)pImage->pvImage
                    || pNtHdrsIprt->OptionalHeader.ImageBase == (uintptr_t)pImage->pvImage) )
            {
                aExcludeRgns[cExcludeRgns].uRva = offImageBase;
                aExcludeRgns[cExcludeRgns].cb   = cbImageBase;
                cExcludeRgns++;
            }

            /* Imports: */
            uint32_t    cImpsLeft    = pNtHdrsIprt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size
                                     / sizeof(IMAGE_IMPORT_DESCRIPTOR);
            uint32_t    offImps      = pNtHdrsIprt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
            AssertLogRelReturn(offImps + cImpsLeft * sizeof(IMAGE_IMPORT_DESCRIPTOR) <= pImage->cbImageBits, VERR_INTERNAL_ERROR_3);
            IMAGE_IMPORT_DESCRIPTOR const *pImp = (IMAGE_IMPORT_DESCRIPTOR const *)(pbImageBits + offImps);
            while (   cImpsLeft-- > 0
                   && cExcludeRgns < RT_ELEMENTS(aExcludeRgns))
            {
                uint32_t uRvaThunk = pImp->OriginalFirstThunk;
                if (    uRvaThunk >  sizeof(IMAGE_NT_HEADERS)
                    &&  uRvaThunk <= pImage->cbImageBits - sizeof(IMAGE_THUNK_DATA)
                    &&  uRvaThunk != pImp->FirstThunk)
                {
                    /* Find the size of the thunk table. */
                    IMAGE_THUNK_DATA const *paThunk    = (IMAGE_THUNK_DATA const *)(pbImageBits + uRvaThunk);
                    uint32_t                cMaxThunks = (pImage->cbImageBits - uRvaThunk) / sizeof(IMAGE_THUNK_DATA);
                    uint32_t                cThunks    = 0;
                    while (cThunks < cMaxThunks && paThunk[cThunks].u1.Function != 0)
                        cThunks++;

                    /* Ordered table insert. */
                    unsigned i = 0;
                    for (; i < cExcludeRgns; i++)
                        if (uRvaThunk < aExcludeRgns[i].uRva)
                            break;
                    if (i != cExcludeRgns)
                        memmove(&aExcludeRgns[i + 1], &aExcludeRgns[i], (cExcludeRgns - i) * sizeof(aExcludeRgns[0]));
                    aExcludeRgns[i].uRva = uRvaThunk;
                    aExcludeRgns[i].cb   = cThunks * sizeof(IMAGE_THUNK_DATA);
                    cExcludeRgns++;
                }

                /* advance */
                pImp++;
            }

            /*
             * Ok, do the comparison.
             */
            int         iDiff    = 0;
            uint32_t    uRvaNext = 0;
            for (unsigned i = 0; !iDiff && i < cExcludeRgns; i++)
            {
                if (uRvaNext < aExcludeRgns[i].uRva)
                    iDiff = supdrvNtCompare(pImage, pbImageBits, uRvaNext, aExcludeRgns[i].uRva - uRvaNext, pReq);
                uRvaNext = aExcludeRgns[i].uRva + aExcludeRgns[i].cb;
            }
            if (!iDiff && uRvaNext < pImage->cbImageBits)
                iDiff = supdrvNtCompare(pImage, pbImageBits, uRvaNext, pImage->cbImageBits - uRvaNext, pReq);
            if (!iDiff)
                return VINF_SUCCESS;
        }
        else
            supdrvNtCompare(pImage, pbImageBits, 0, pImage->cbImageBits, pReq);
        return VERR_LDR_MISMATCH_NATIVE;
    }
    return supdrvLdrLoadError(VERR_INTERNAL_ERROR_4, pReq, "No NT section object! Impossible!");
}


void VBOXCALL   supdrvOSLdrUnload(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage)
{
    if (pImage->pvNtSectionObj)
    {
        if (pImage->hMemLock != NIL_RTR0MEMOBJ)
        {
            RTR0MemObjFree(pImage->hMemLock, false /*fFreeMappings*/);
            pImage->hMemLock = NIL_RTR0MEMOBJ;
        }

        NTSTATUS rcNt = ZwSetSystemInformation(MY_SystemUnloadGdiDriverInformation,
                                               &pImage->pvNtSectionObj, sizeof(pImage->pvNtSectionObj));
        if (rcNt != STATUS_SUCCESS)
            SUPR0Printf("VBoxDrv: failed to unload '%s', rcNt=%#x\n", pImage->szName, rcNt);
        pImage->pvNtSectionObj = NULL;
    }
    NOREF(pDevExt);
}


#ifdef SUPDRV_WITH_MSR_PROBER

#if 1
/** @todo make this selectable. */
# define AMD_MSR_PASSCODE 0x9c5a203a
#else
# define ASMRdMsrEx(a, b, c) ASMRdMsr(a)
# define ASMWrMsrEx(a, b, c) ASMWrMsr(a,c)
#endif


/**
 * Argument package used by supdrvOSMsrProberRead and supdrvOSMsrProberWrite.
 */
typedef struct SUPDRVNTMSPROBERARGS
{
    uint32_t    uMsr;
    uint64_t    uValue;
    bool        fGp;
} SUPDRVNTMSPROBERARGS;

/** @callback_method_impl{FNRTMPWORKER, Worker for supdrvOSMsrProberRead.} */
static DECLCALLBACK(void) supdrvNtMsProberReadOnCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    /*
     * rdmsr and wrmsr faults can be caught even with interrupts disabled.
     * (At least on 32-bit XP.)
     */
    SUPDRVNTMSPROBERARGS   *pArgs = (SUPDRVNTMSPROBERARGS *)pvUser1; NOREF(idCpu); NOREF(pvUser2);
    RTCCUINTREG             fOldFlags = ASMIntDisableFlags();
    __try
    {
        pArgs->uValue = ASMRdMsrEx(pArgs->uMsr, AMD_MSR_PASSCODE);
        pArgs->fGp    = false;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pArgs->fGp    = true;
        pArgs->uValue = 0;
    }
    ASMSetFlags(fOldFlags);
}


int VBOXCALL    supdrvOSMsrProberRead(uint32_t uMsr, RTCPUID idCpu, uint64_t *puValue)
{
    SUPDRVNTMSPROBERARGS Args;
    Args.uMsr   = uMsr;
    Args.uValue = 0;
    Args.fGp    = true;

    if (idCpu == NIL_RTCPUID)
        supdrvNtMsProberReadOnCpu(idCpu, &Args, NULL);
    else
    {
        int rc = RTMpOnSpecific(idCpu, supdrvNtMsProberReadOnCpu, &Args, NULL);
        if (RT_FAILURE(rc))
            return rc;
    }

    if (Args.fGp)
        return VERR_ACCESS_DENIED;
    *puValue = Args.uValue;
    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTMPWORKER, Worker for supdrvOSMsrProberWrite.} */
static DECLCALLBACK(void) supdrvNtMsProberWriteOnCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    /*
     * rdmsr and wrmsr faults can be caught even with interrupts disabled.
     * (At least on 32-bit XP.)
     */
    SUPDRVNTMSPROBERARGS   *pArgs = (SUPDRVNTMSPROBERARGS *)pvUser1; NOREF(idCpu); NOREF(pvUser2);
    RTCCUINTREG             fOldFlags = ASMIntDisableFlags();
    __try
    {
        ASMWrMsrEx(pArgs->uMsr, AMD_MSR_PASSCODE, pArgs->uValue);
        pArgs->fGp = false;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pArgs->fGp = true;
    }
    ASMSetFlags(fOldFlags);
}

int VBOXCALL    supdrvOSMsrProberWrite(uint32_t uMsr, RTCPUID idCpu, uint64_t uValue)
{
    SUPDRVNTMSPROBERARGS Args;
    Args.uMsr   = uMsr;
    Args.uValue = uValue;
    Args.fGp    = true;

    if (idCpu == NIL_RTCPUID)
        supdrvNtMsProberWriteOnCpu(idCpu, &Args, NULL);
    else
    {
        int rc = RTMpOnSpecific(idCpu, supdrvNtMsProberWriteOnCpu, &Args, NULL);
        if (RT_FAILURE(rc))
            return rc;
    }

    if (Args.fGp)
        return VERR_ACCESS_DENIED;
    return VINF_SUCCESS;
}

/** @callback_method_impl{FNRTMPWORKER, Worker for supdrvOSMsrProberModify.} */
static DECLCALLBACK(void) supdrvNtMsProberModifyOnCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    PSUPMSRPROBER       pReq        = (PSUPMSRPROBER)pvUser1;
    register uint32_t   uMsr        = pReq->u.In.uMsr;
    bool const          fFaster     = pReq->u.In.enmOp == SUPMSRPROBEROP_MODIFY_FASTER;
    uint64_t            uBefore     = 0;
    uint64_t            uWritten    = 0;
    uint64_t            uAfter      = 0;
    bool                fBeforeGp   = true;
    bool                fModifyGp   = true;
    bool                fAfterGp    = true;
    bool                fRestoreGp  = true;
    RTCCUINTREG         fOldFlags;
    RT_NOREF2(idCpu, pvUser2);

    /*
     * Do the job.
     */
    fOldFlags = ASMIntDisableFlags();
    ASMCompilerBarrier(); /* paranoia */
    if (!fFaster)
        ASMWriteBackAndInvalidateCaches();

    __try
    {
        uBefore   = ASMRdMsrEx(uMsr, AMD_MSR_PASSCODE);
        fBeforeGp = false;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fBeforeGp = true;
    }
    if (!fBeforeGp)
    {
        register uint64_t uRestore = uBefore;

        /* Modify. */
        uWritten  = uRestore;
        uWritten &= pReq->u.In.uArgs.Modify.fAndMask;
        uWritten |= pReq->u.In.uArgs.Modify.fOrMask;
        __try
        {
            ASMWrMsrEx(uMsr, AMD_MSR_PASSCODE, uWritten);
            fModifyGp = false;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            fModifyGp = true;
        }

        /* Read modified value. */
        __try
        {
            uAfter   = ASMRdMsrEx(uMsr, AMD_MSR_PASSCODE);
            fAfterGp = false;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            fAfterGp = true;
        }

        /* Restore original value. */
        __try
        {
            ASMWrMsrEx(uMsr, AMD_MSR_PASSCODE, uRestore);
            fRestoreGp = false;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            fRestoreGp = true;
        }

        /* Invalid everything we can. */
        if (!fFaster)
        {
            ASMWriteBackAndInvalidateCaches();
            ASMReloadCR3();
            ASMNopPause();
        }
    }

    ASMCompilerBarrier(); /* paranoia */
    ASMSetFlags(fOldFlags);

    /*
     * Write out the results.
     */
    pReq->u.Out.uResults.Modify.uBefore    = uBefore;
    pReq->u.Out.uResults.Modify.uWritten   = uWritten;
    pReq->u.Out.uResults.Modify.uAfter     = uAfter;
    pReq->u.Out.uResults.Modify.fBeforeGp  = fBeforeGp;
    pReq->u.Out.uResults.Modify.fModifyGp  = fModifyGp;
    pReq->u.Out.uResults.Modify.fAfterGp   = fAfterGp;
    pReq->u.Out.uResults.Modify.fRestoreGp = fRestoreGp;
    RT_ZERO(pReq->u.Out.uResults.Modify.afReserved);
}


int VBOXCALL    supdrvOSMsrProberModify(RTCPUID idCpu, PSUPMSRPROBER pReq)
{
    if (idCpu == NIL_RTCPUID)
    {
        supdrvNtMsProberModifyOnCpu(idCpu, pReq, NULL);
        return VINF_SUCCESS;
    }
    return RTMpOnSpecific(idCpu, supdrvNtMsProberModifyOnCpu, pReq, NULL);
}

#endif /* SUPDRV_WITH_MSR_PROBER */


/**
 * Converts an IPRT error code to an nt status code.
 *
 * @returns corresponding nt status code.
 * @param   rc      IPRT error status code.
 */
static NTSTATUS     VBoxDrvNtErr2NtStatus(int rc)
{
    switch (rc)
    {
        case VINF_SUCCESS:                  return STATUS_SUCCESS;
        case VERR_GENERAL_FAILURE:          return STATUS_NOT_SUPPORTED;
        case VERR_INVALID_PARAMETER:        return STATUS_INVALID_PARAMETER;
        case VERR_INVALID_MAGIC:            return STATUS_UNKNOWN_REVISION;
        case VERR_INVALID_HANDLE:           return STATUS_INVALID_HANDLE;
        case VERR_INVALID_POINTER:          return STATUS_INVALID_ADDRESS;
        case VERR_LOCK_FAILED:              return STATUS_NOT_LOCKED;
        case VERR_ALREADY_LOADED:           return STATUS_IMAGE_ALREADY_LOADED;
        case VERR_PERMISSION_DENIED:        return STATUS_ACCESS_DENIED;
        case VERR_VERSION_MISMATCH:         return STATUS_REVISION_MISMATCH;
    }

    if (rc < 0)
    {
        if (((uint32_t)rc & UINT32_C(0xffff0000)) == UINT32_C(0xffff0000))
            return (NTSTATUS)( ((uint32_t)rc & UINT32_C(0xffff)) | SUP_NT_STATUS_BASE );
    }
    return STATUS_UNSUCCESSFUL;
}


/**
 * Alternative version of SUPR0Printf for Windows.
 *
 * @returns 0.
 * @param   pszFormat   The format string.
 */
SUPR0DECL(int) SUPR0Printf(const char *pszFormat, ...)
{
    va_list va;
    char    szMsg[384];

    va_start(va, pszFormat);
    size_t cch = RTStrPrintfV(szMsg, sizeof(szMsg) - 1, pszFormat, va);
    szMsg[sizeof(szMsg) - 1] = '\0';
    va_end(va);

    RTLogWriteDebugger(szMsg, cch);
    return 0;
}


SUPR0DECL(uint32_t) SUPR0GetKernelFeatures(void)
{
    return 0;
}


#ifdef VBOX_WITH_HARDENING

/** @name Identifying Special Processes: CSRSS.EXE
 * @{ */


/**
 * Checks if the process is a system32 process by the given name.
 *
 * @returns true / false.
 * @param   pProcess            The process to check.
 * @param   pszName             The lower case process name (no path!).
 */
static bool supdrvNtProtectIsSystem32ProcessMatch(PEPROCESS pProcess, const char *pszName)
{
    Assert(strlen(pszName) < 16); /* see buffer below */

    /*
     * This test works on XP+.
     */
    const char *pszImageFile = (const char *)PsGetProcessImageFileName(pProcess);
    if (!pszImageFile)
        return false;

    if (RTStrICmp(pszImageFile, pszName) != 0)
        return false;

    /*
     * This test requires a Vista+ API.
     */
    if (g_pfnPsReferenceProcessFilePointer)
    {
        PFILE_OBJECT pFile = NULL;
        NTSTATUS rcNt = g_pfnPsReferenceProcessFilePointer(pProcess, &pFile);
        if (!NT_SUCCESS(rcNt))
            return false;

        union
        {
            OBJECT_NAME_INFORMATION Info;
            uint8_t        abBuffer[sizeof(g_System32NtPath) + 16 * sizeof(WCHAR)];
        } Buf;
        ULONG cbIgn;
        rcNt = ObQueryNameString(pFile, &Buf.Info, sizeof(Buf) - sizeof(WCHAR), &cbIgn);
        ObDereferenceObject(pFile);
        if (!NT_SUCCESS(rcNt))
            return false;

        /* Terminate the name. */
        PRTUTF16 pwszName = Buf.Info.Name.Buffer;
        pwszName[Buf.Info.Name.Length / sizeof(RTUTF16)] = '\0';

        /* Match the name against the system32 directory path. */
        uint32_t cbSystem32 = g_System32NtPath.UniStr.Length;
        if (Buf.Info.Name.Length < cbSystem32)
            return false;
        if (memcmp(pwszName, g_System32NtPath.UniStr.Buffer, cbSystem32))
            return false;
        pwszName += cbSystem32 / sizeof(RTUTF16);
        if (*pwszName++ != '\\')
            return false;

        /* Compare the name. */
        const char *pszRight = pszName;
        for (;;)
        {
            WCHAR wchLeft = *pwszName++;
            char  chRight = *pszRight++;
            Assert(chRight == RT_C_TO_LOWER(chRight));

            if (   wchLeft != chRight
                && RT_C_TO_LOWER(wchLeft) != chRight)
                return false;
            if (!chRight)
                break;
        }
    }

    return true;
}


/**
 * Checks if the current process is likely to be CSRSS.
 *
 * @returns true/false.
 * @param   pProcess        The process.
 */
static bool supdrvNtProtectIsCsrssByProcess(PEPROCESS pProcess)
{
    /*
     * On Windows 8.1 CSRSS.EXE is a protected process.
     */
    if (g_pfnPsIsProtectedProcessLight)
    {
        if (!g_pfnPsIsProtectedProcessLight(pProcess))
            return false;
    }

    /*
     * The name tests.
     */
    if (!supdrvNtProtectIsSystem32ProcessMatch(pProcess, "csrss.exe"))
        return false;

    /** @todo Could extend the CSRSS.EXE check with that the TokenUser of the
     *        current process must be "NT AUTHORITY\SYSTEM" (S-1-5-18). */

    return true;
}


/**
 * Helper for supdrvNtProtectGetAlpcPortObjectType that tries out a name.
 *
 * @returns true if done, false if not.
 * @param   pwszPortNm          The port path.
 * @param   ppObjType           The object type return variable, updated when
 *                              returning true.
 */
static bool supdrvNtProtectGetAlpcPortObjectType2(PCRTUTF16 pwszPortNm, POBJECT_TYPE *ppObjType)
{
    bool fDone = false;

    UNICODE_STRING UniStrPortNm;
    UniStrPortNm.Buffer = (WCHAR *)pwszPortNm;
    UniStrPortNm.Length = (USHORT)(RTUtf16Len(pwszPortNm) * sizeof(WCHAR));
    UniStrPortNm.MaximumLength = UniStrPortNm.Length + sizeof(WCHAR);

    OBJECT_ATTRIBUTES ObjAttr;
    InitializeObjectAttributes(&ObjAttr, &UniStrPortNm, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE hPort;
    NTSTATUS rcNt = g_pfnZwAlpcCreatePort(&hPort, &ObjAttr, NULL /*pPortAttribs*/);
    if (NT_SUCCESS(rcNt))
    {
        PVOID pvObject;
        rcNt = ObReferenceObjectByHandle(hPort, 0 /*DesiredAccess*/, NULL /*pObjectType*/,
                                         KernelMode, &pvObject, NULL /*pHandleInfo*/);
        if (NT_SUCCESS(rcNt))
        {
            POBJECT_TYPE pObjType = g_pfnObGetObjectType(pvObject);
            if (pObjType)
            {
                SUPR0Printf("vboxdrv: ALPC Port Object Type %p (vs %p)\n", pObjType, *ppObjType);
                *ppObjType = pObjType;
                fDone = true;
            }
            ObDereferenceObject(pvObject);
        }
        NtClose(hPort);
    }
    return fDone;
}


/**
 * Attempts to retrieve the ALPC Port object type.
 *
 * We've had at least three reports that using LpcPortObjectType when trying to
 * get at the ApiPort object results in STATUS_OBJECT_TYPE_MISMATCH errors.
 * It's not known who has modified LpcPortObjectType or AlpcPortObjectType (not
 * exported) so that it differs from the actual ApiPort type, or maybe this
 * unknown entity is intercepting our attempt to reference the port and
 * tries to mislead us.  The paranoid explanataion is of course that some evil
 * root kit like software is messing with the OS, however, it's possible that
 * this is valid kernel behavior that 99.8% of our users and 100% of the
 * developers are not triggering for some reason.
 *
 * The code here creates an ALPC port object and gets it's type.  It will cache
 * the result in g_pAlpcPortObjectType2 on success.
 *
 * @returns Object type.
 * @param   uSessionId      The session id.
 * @param   pszSessionId    The session id formatted as a string.
 */
static POBJECT_TYPE supdrvNtProtectGetAlpcPortObjectType(uint32_t uSessionId, const char *pszSessionId)
{
    POBJECT_TYPE pObjType = *LpcPortObjectType;

    if (   g_pfnZwAlpcCreatePort
        && g_pfnObGetObjectType)
    {
        int     rc;
        ssize_t cchTmp; NOREF(cchTmp);
        char    szTmp[16];
        RTUTF16 wszPortNm[128];
        size_t  offRand;

        /*
         * First attempt is in the session directory.
         */
        rc  = RTUtf16CopyAscii(wszPortNm, RT_ELEMENTS(wszPortNm), "\\Sessions\\");
        rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), pszSessionId);
        rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), "\\VBoxDrv-");
        cchTmp = RTStrFormatU32(szTmp, sizeof(szTmp), (uint32_t)(uintptr_t)PsGetProcessId(PsGetCurrentProcess()), 16, 0, 0, 0);
        Assert(cchTmp > 0);
        rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), szTmp);
        rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), "-");
        offRand = RTUtf16Len(wszPortNm);
        cchTmp = RTStrFormatU32(szTmp, sizeof(szTmp), RTRandU32(), 16, 0, 0, 0);
        Assert(cchTmp > 0);
        rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), szTmp);
        AssertRCSuccess(rc);

        bool fDone = supdrvNtProtectGetAlpcPortObjectType2(wszPortNm, &pObjType);
        if (!fDone)
        {
            wszPortNm[offRand] = '\0';
            cchTmp = RTStrFormatU32(szTmp, sizeof(szTmp), RTRandU32(), 16, 0, 0, 0); Assert(cchTmp > 0);
            rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), szTmp);
            AssertRCSuccess(rc);

            fDone = supdrvNtProtectGetAlpcPortObjectType2(wszPortNm, &pObjType);
        }
        if (!fDone)
        {
            /*
             * Try base names.
             */
            if (uSessionId == 0)
                rc  = RTUtf16CopyAscii(wszPortNm, RT_ELEMENTS(wszPortNm), "\\BaseNamedObjects\\VBoxDrv-");
            else
            {
                rc  = RTUtf16CopyAscii(wszPortNm, RT_ELEMENTS(wszPortNm), "\\Sessions\\");
                rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), pszSessionId);
                rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), "\\BaseNamedObjects\\VBoxDrv-");
            }
            cchTmp = RTStrFormatU32(szTmp, sizeof(szTmp), (uint32_t)(uintptr_t)PsGetProcessId(PsGetCurrentProcess()), 16, 0, 0, 0);
            Assert(cchTmp > 0);
            rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), szTmp);
            rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), "-");
            offRand = RTUtf16Len(wszPortNm);
            cchTmp = RTStrFormatU32(szTmp, sizeof(szTmp), RTRandU32(), 16, 0, 0, 0);
            Assert(cchTmp > 0);
            rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), szTmp);
            AssertRCSuccess(rc);

            bool fDone = supdrvNtProtectGetAlpcPortObjectType2(wszPortNm, &pObjType);
            if (!fDone)
            {
                wszPortNm[offRand] = '\0';
                cchTmp = RTStrFormatU32(szTmp, sizeof(szTmp), RTRandU32(), 16, 0, 0, 0);
                Assert(cchTmp > 0);
                rc |= RTUtf16CatAscii(wszPortNm, RT_ELEMENTS(wszPortNm), szTmp);
                AssertRCSuccess(rc);

                fDone = supdrvNtProtectGetAlpcPortObjectType2(wszPortNm, &pObjType);
            }
        }

        /* Cache the result in g_pAlpcPortObjectType2. */
        if (   g_pAlpcPortObjectType2 == NULL
            && pObjType != g_pAlpcPortObjectType1
            && fDone)
            g_pAlpcPortObjectType2 = pObjType;

    }

    return pObjType;
}


/**
 * Called in the context of VBoxDrvNtCreate to determin the CSRSS for the
 * current process.
 *
 * The Client/Server Runtime Subsystem (CSRSS) process needs to be allowed some
 * additional access right so we need to make 101% sure we correctly identify
 * the CSRSS process a process is associated with.
 *
 * @returns IPRT status code.
 * @param   pNtProtect          The NT protected process structure. The
 *                              hCsrssPid member will be updated on success.
 */
static int supdrvNtProtectFindAssociatedCsrss(PSUPDRVNTPROTECT pNtProtect)
{
    Assert(pNtProtect->AvlCore.Key == PsGetCurrentProcessId());
    Assert(pNtProtect->pCsrssProcess == NULL);
    Assert(pNtProtect->hCsrssPid == NULL);

    /*
     * We'll try use the ApiPort LPC object for the session we're in to track
     * down the CSRSS process. So, we start by constructing a path to it.
     */
    int         rc;
    uint32_t    uSessionId = PsGetProcessSessionId(PsGetCurrentProcess());
    char        szSessionId[16];
    WCHAR       wszApiPort[48];
    if (uSessionId == 0)
    {
        szSessionId[0] = '0';
        szSessionId[1] = '\0';
        rc = RTUtf16CopyAscii(wszApiPort, RT_ELEMENTS(wszApiPort), "\\Windows\\ApiPort");
    }
    else
    {
        ssize_t cchTmp = RTStrFormatU32(szSessionId, sizeof(szSessionId), uSessionId, 10, 0, 0, 0);
        AssertReturn(cchTmp > 0, (int)cchTmp);
        rc = RTUtf16CopyAscii(wszApiPort, RT_ELEMENTS(wszApiPort), "\\Sessions\\");
        if (RT_SUCCESS(rc))
            rc = RTUtf16CatAscii(wszApiPort, RT_ELEMENTS(wszApiPort), szSessionId);
        if (RT_SUCCESS(rc))
            rc = RTUtf16CatAscii(wszApiPort, RT_ELEMENTS(wszApiPort), "\\Windows\\ApiPort");
    }
    AssertRCReturn(rc, rc);

    UNICODE_STRING ApiPortStr;
    ApiPortStr.Buffer = wszApiPort;
    ApiPortStr.Length = (USHORT)(RTUtf16Len(wszApiPort) * sizeof(RTUTF16));
    ApiPortStr.MaximumLength = ApiPortStr.Length + sizeof(RTUTF16);

    /*
     * The object cannot be opened, but we can reference it by name.
     */
    void *pvApiPortObj = NULL;
    NTSTATUS rcNt = ObReferenceObjectByName(&ApiPortStr,
                                            0,
                                            NULL /*pAccessState*/,
                                            STANDARD_RIGHTS_READ,
                                            g_pAlpcPortObjectType1,
                                            KernelMode,
                                            NULL /*pvParseContext*/,
                                            &pvApiPortObj);
    if (   rcNt == STATUS_OBJECT_TYPE_MISMATCH
        && g_pAlpcPortObjectType2 != NULL)
        rcNt = ObReferenceObjectByName(&ApiPortStr,
                                       0,
                                       NULL /*pAccessState*/,
                                       STANDARD_RIGHTS_READ,
                                       g_pAlpcPortObjectType2,
                                       KernelMode,
                                       NULL /*pvParseContext*/,
                                       &pvApiPortObj);
    if (   rcNt == STATUS_OBJECT_TYPE_MISMATCH
        && g_pfnObGetObjectType
        && g_pfnZwAlpcCreatePort)
        rcNt = ObReferenceObjectByName(&ApiPortStr,
                                       0,
                                       NULL /*pAccessState*/,
                                       STANDARD_RIGHTS_READ,
                                       supdrvNtProtectGetAlpcPortObjectType(uSessionId, szSessionId),
                                       KernelMode,
                                       NULL /*pvParseContext*/,
                                       &pvApiPortObj);
    if (!NT_SUCCESS(rcNt))
    {
        SUPR0Printf("vboxdrv: Error opening '%ls': %#x\n", wszApiPort, rcNt);
        return rcNt == STATUS_OBJECT_TYPE_MISMATCH ? VERR_SUPDRV_APIPORT_OPEN_ERROR_TYPE : VERR_SUPDRV_APIPORT_OPEN_ERROR;
    }

    /*
     * Query the processes in the system so we can locate CSRSS.EXE candidates.
     * Note! Attempts at using SystemSessionProcessInformation failed with
     *       STATUS_ACCESS_VIOLATION.
     * Note! The 32 bytes on the size of to counteract the allocation header
     *       that rtR0MemAllocEx slaps on everything.
     */
    ULONG       cbNeeded = _64K - 32;
    uint32_t    cbBuf;
    uint8_t    *pbBuf = NULL;
    do
    {
        cbBuf = RT_ALIGN(cbNeeded + _4K, _64K) - 32;
        pbBuf = (uint8_t *)RTMemAlloc(cbBuf);
        if (!pbBuf)
            break;

        cbNeeded = 0;
#if 0 /* doesn't work. */
        SYSTEM_SESSION_PROCESS_INFORMATION Req;
        Req.SessionId    = uSessionId;
        Req.BufferLength = cbBuf;
        Req.Buffer       = pbBuf;
        rcNt = NtQuerySystemInformation(SystemSessionProcessInformation, &Req, sizeof(Req), &cbNeeded);
#else
        rcNt = NtQuerySystemInformation(SystemProcessInformation, pbBuf, cbBuf, &cbNeeded);
#endif
        if (NT_SUCCESS(rcNt))
            break;

        RTMemFree(pbBuf);
        pbBuf = NULL;
    } while (   rcNt == STATUS_INFO_LENGTH_MISMATCH
             && cbNeeded > cbBuf
             && cbNeeded < 32U*_1M);

    if (   pbBuf
        && NT_SUCCESS(rcNt)
        && cbNeeded >= sizeof(SYSTEM_PROCESS_INFORMATION))
    {
        /*
         * Walk the returned data and look for the process associated with the
         * ApiPort object.  The ApiPort object keeps the EPROCESS address of
         * the owner process (i.e. CSRSS) relatively early in the structure. On
         * 64-bit windows 8.1 it's at offset 0x18.  So, obtain the EPROCESS
         * pointer to likely CSRSS processes and check for a match in the first
         * 0x40 bytes of the ApiPort object.
         */
        rc = VERR_SUPDRV_CSRSS_NOT_FOUND;
        for (uint32_t offBuf = 0; offBuf <= cbNeeded - sizeof(SYSTEM_PROCESS_INFORMATION);)
        {
            PRTNT_SYSTEM_PROCESS_INFORMATION pProcInfo = (PRTNT_SYSTEM_PROCESS_INFORMATION)&pbBuf[offBuf];
            if (   pProcInfo->ProcessName.Length == 9 * sizeof(WCHAR)
                && pProcInfo->NumberOfThreads > 2   /* Very low guess. */
                && pProcInfo->HandleCount     > 32  /* Very low guess, I hope. */
                && (uintptr_t)pProcInfo->ProcessName.Buffer - (uintptr_t)pbBuf < cbNeeded
                && RT_C_TO_LOWER(pProcInfo->ProcessName.Buffer[0]) == 'c'
                && RT_C_TO_LOWER(pProcInfo->ProcessName.Buffer[1]) == 's'
                && RT_C_TO_LOWER(pProcInfo->ProcessName.Buffer[2]) == 'r'
                && RT_C_TO_LOWER(pProcInfo->ProcessName.Buffer[3]) == 's'
                && RT_C_TO_LOWER(pProcInfo->ProcessName.Buffer[4]) == 's'
                &&               pProcInfo->ProcessName.Buffer[5]  == '.'
                && RT_C_TO_LOWER(pProcInfo->ProcessName.Buffer[6]) == 'e'
                && RT_C_TO_LOWER(pProcInfo->ProcessName.Buffer[7]) == 'x'
                && RT_C_TO_LOWER(pProcInfo->ProcessName.Buffer[8]) == 'e' )
            {

                /* Get the process structure and perform some more thorough
                   process checks. */
                PEPROCESS pProcess;
                rcNt = PsLookupProcessByProcessId(pProcInfo->UniqueProcessId, &pProcess);
                if (NT_SUCCESS(rcNt))
                {
                    if (supdrvNtProtectIsCsrssByProcess(pProcess))
                    {
                        if (PsGetProcessSessionId(pProcess) == uSessionId)
                        {
                            /* Final test, check the ApiPort.
                               Note! The old LPC (pre Vista) objects has the PID
                                     much earlier in the structure.  Might be
                                     worth looking for it instead. */
                            bool fThatsIt = false;
                            __try
                            {
                                PEPROCESS *ppPortProc = (PEPROCESS *)pvApiPortObj;
                                uint32_t   cTests = g_uNtVerCombined >= SUP_NT_VER_VISTA ? 16 : 38; /* ALPC since Vista. */
                                do
                                {
                                    fThatsIt = *ppPortProc == pProcess;
                                    ppPortProc++;
                                } while (!fThatsIt && --cTests > 0);
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                fThatsIt = false;
                            }
                            if (fThatsIt)
                            {
                                /* Ok, we found it!  Keep the process structure
                                   reference as well as the PID so we can
                                   safely identify it later on.  */
                                pNtProtect->hCsrssPid     = pProcInfo->UniqueProcessId;
                                pNtProtect->pCsrssProcess = pProcess;
                                rc = VINF_SUCCESS;
                                break;
                            }
                        }
                    }

                    ObDereferenceObject(pProcess);
                }
            }

            /* Advance. */
            if (!pProcInfo->NextEntryOffset)
                break;
            offBuf += pProcInfo->NextEntryOffset;
        }
    }
    else
        rc = VERR_SUPDRV_SESSION_PROCESS_ENUM_ERROR;
    RTMemFree(pbBuf);
    ObDereferenceObject(pvApiPortObj);
    return rc;
}


/**
 * Checks that the given process is the CSRSS process associated with protected
 * process.
 *
 * @returns true / false.
 * @param   pNtProtect          The NT protection structure.
 * @param   pCsrss              The process structure of the alleged CSRSS.EXE
 *                              process.
 */
static bool supdrvNtProtectIsAssociatedCsrss(PSUPDRVNTPROTECT pNtProtect, PEPROCESS pCsrss)
{
    if (pNtProtect->pCsrssProcess == pCsrss)
    {
        if (pNtProtect->hCsrssPid == PsGetProcessId(pCsrss))
        {
            return true;
        }
    }
    return false;
}


/**
 * Checks if the given process is the stupid themes service.
 *
 * The caller does some screening of access masks and what not. We do the rest.
 *
 * @returns true / false.
 * @param   pNtProtect          The NT protection structure.
 * @param   pAnnoyingProcess    The process structure of an process that might
 *                              happen to be the annoying themes process.
 */
static bool supdrvNtProtectIsFrigginThemesService(PSUPDRVNTPROTECT pNtProtect, PEPROCESS pAnnoyingProcess)
{
    RT_NOREF1(pNtProtect);

    /*
     * Check the process name.
     */
    if (!supdrvNtProtectIsSystem32ProcessMatch(pAnnoyingProcess, "svchost.exe"))
        return false;

    /** @todo Come up with more checks. */

    return true;
}


#ifdef VBOX_WITHOUT_DEBUGGER_CHECKS
/**
 * Checks if the given process is one of the whitelisted debuggers.
 *
 * @returns true / false.
 * @param   pProcess            The process to check.
 */
static bool supdrvNtProtectIsWhitelistedDebugger(PEPROCESS pProcess)
{
    const char *pszImageFile = (const char *)PsGetProcessImageFileName(pProcess);
    if (!pszImageFile)
        return false;

    if (pszImageFile[0] == 'w' || pszImageFile[0] == 'W')
    {
        if (RTStrICmp(pszImageFile, "windbg.exe") == 0)
            return true;
        if (RTStrICmp(pszImageFile, "werfault.exe") == 0)
            return true;
        if (RTStrICmp(pszImageFile, "werfaultsecure.exe") == 0)
            return true;
    }
    else if (pszImageFile[0] == 'd' || pszImageFile[0] == 'D')
    {
        if (RTStrICmp(pszImageFile, "drwtsn32.exe") == 0)
            return true;
        if (RTStrICmp(pszImageFile, "dwwin.exe") == 0)
            return true;
    }

    return false;
}
#endif /* VBOX_WITHOUT_DEBUGGER_CHECKS */


/** @} */


/** @name Process Creation Callbacks.
 * @{ */


/**
 * Cleans up VBoxDrv or VBoxDrvStub error info not collected by the dead process.
 *
 * @param   hProcessId          The ID of the dead process.
 */
static void supdrvNtErrorInfoCleanupProcess(HANDLE hProcessId)
{
    int rc = RTSemMutexRequestNoResume(g_hErrorInfoLock, RT_INDEFINITE_WAIT);
    if (RT_SUCCESS(rc))
    {
        PSUPDRVNTERRORINFO pCur, pNext;
        RTListForEachSafe(&g_ErrorInfoHead, pCur, pNext, SUPDRVNTERRORINFO, ListEntry)
        {
            if (pCur->hProcessId == hProcessId)
            {
                RTListNodeRemove(&pCur->ListEntry);
                RTMemFree(pCur);
            }
        }
        RTSemMutexRelease(g_hErrorInfoLock);
    }
}


/**
 * Common worker used by the process creation hooks as well as the process
 * handle creation hooks to check if a VM process is being created.
 *
 * @returns true if likely to be a VM process, false if not.
 * @param   pNtStub             The NT protection structure for the possible
 *                              stub process.
 * @param   hParentPid          The parent pid.
 * @param   hChildPid           The child pid.
 */
static bool supdrvNtProtectIsSpawningStubProcess(PSUPDRVNTPROTECT pNtStub, HANDLE hParentPid, HANDLE hChildPid)
{
    bool fRc = false;
    if (pNtStub->AvlCore.Key == hParentPid) /* paranoia */
    {
        if (pNtStub->enmProcessKind == kSupDrvNtProtectKind_StubSpawning)
        {
            /* Compare short names. */
            PEPROCESS pStubProcess;
            NTSTATUS rcNt = PsLookupProcessByProcessId(hParentPid, &pStubProcess);
            if (NT_SUCCESS(rcNt))
            {
                PEPROCESS pChildProcess;
                rcNt = PsLookupProcessByProcessId(hChildPid, &pChildProcess);
                if (NT_SUCCESS(rcNt))
                {
                    const char *pszStub  = (const char *)PsGetProcessImageFileName(pStubProcess);
                    const char *pszChild = (const char *)PsGetProcessImageFileName(pChildProcess);
                    fRc = pszStub != NULL
                       && pszChild != NULL
                       && strcmp(pszStub, pszChild) == 0;

                    /** @todo check that the full image names matches. */

                    ObDereferenceObject(pChildProcess);
                }
                ObDereferenceObject(pStubProcess);
            }
        }
    }
    return fRc;
}


/**
 * Common code used by the notifies to protect a child process.
 *
 * @returns VBox status code.
 * @param   pNtStub             The NT protect structure for the parent.
 * @param   hChildPid           The child pid.
 */
static int supdrvNtProtectProtectNewStubChild(PSUPDRVNTPROTECT pNtParent, HANDLE hChildPid)
{
    /*
     * Create a child protection struction.
     */
    PSUPDRVNTPROTECT pNtChild;
    int rc = supdrvNtProtectCreate(&pNtChild, hChildPid, kSupDrvNtProtectKind_VmProcessUnconfirmed, false /*fLink*/);
    if (RT_SUCCESS(rc))
    {
        pNtChild->fFirstProcessCreateHandle = true;
        pNtChild->fFirstThreadCreateHandle = true;
        pNtChild->fCsrssFirstProcessCreateHandle = true;
        pNtChild->cCsrssFirstProcessDuplicateHandle = ARCH_BITS == 32 ? 2 : 1;
        pNtChild->fThemesFirstProcessCreateHandle = true;
        pNtChild->hParentPid = pNtParent->AvlCore.Key;
        pNtChild->hCsrssPid = pNtParent->hCsrssPid;
        pNtChild->pCsrssProcess = pNtParent->pCsrssProcess;
        if (pNtChild->pCsrssProcess)
            ObReferenceObject(pNtChild->pCsrssProcess);

        /*
         * Take the spinlock, recheck parent conditions and link things.
         */
        RTSpinlockAcquire(g_hNtProtectLock);
        if (pNtParent->enmProcessKind == kSupDrvNtProtectKind_StubSpawning)
        {
            bool fSuccess = RTAvlPVInsert(&g_NtProtectTree, &pNtChild->AvlCore);
            if (fSuccess)
            {
                pNtChild->fInTree         = true;
                pNtParent->u.pChild       = pNtChild; /* Parent keeps the initial reference. */
                pNtParent->enmProcessKind = kSupDrvNtProtectKind_StubParent;
                pNtChild->u.pParent       = pNtParent;

                RTSpinlockRelease(g_hNtProtectLock);
                return VINF_SUCCESS;
            }

            rc = VERR_INTERNAL_ERROR_2;
        }
        else
            rc = VERR_WRONG_ORDER;
        pNtChild->enmProcessKind = kSupDrvNtProtectKind_VmProcessDead;
        RTSpinlockRelease(g_hNtProtectLock);

        supdrvNtProtectRelease(pNtChild);
    }
    return rc;
}


/**
 * Common process termination code.
 *
 * Transitions protected process to the dead states, protecting against handle
 * PID reuse (esp. with unconfirmed VM processes) and handle cleanup issues.
 *
 * @param   hDeadPid            The PID of the dead process.
 */
static void supdrvNtProtectUnprotectDeadProcess(HANDLE hDeadPid)
{
    PSUPDRVNTPROTECT pNtProtect = supdrvNtProtectLookup(hDeadPid);
    if (pNtProtect)
    {
        PSUPDRVNTPROTECT pNtChild = NULL;

        RTSpinlockAcquire(g_hNtProtectLock);

        /*
         * If this is an unconfirmed VM process, we must release the reference
         * the parent structure holds.
         */
        if (pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed)
        {
            PSUPDRVNTPROTECT pNtParent = pNtProtect->u.pParent;
            AssertRelease(pNtParent); AssertRelease(pNtParent->u.pChild == pNtProtect);
            pNtParent->u.pChild   = NULL;
            pNtProtect->u.pParent = NULL;
            pNtChild = pNtProtect;
        }
        /*
         * If this is a stub exitting before the VM process gets confirmed,
         * release the protection of the potential VM process as this is not
         * the prescribed behavior.
         */
        else if (   pNtProtect->enmProcessKind == kSupDrvNtProtectKind_StubParent
                 && pNtProtect->u.pChild)
        {
            pNtChild = pNtProtect->u.pChild;
            pNtProtect->u.pChild = NULL;
            pNtChild->u.pParent  = NULL;
            pNtChild->enmProcessKind = kSupDrvNtProtectKind_VmProcessDead;
        }

        /*
         * Transition it to the dead state to prevent it from opening the
         * support driver again or be posthumously abused as a vm process parent.
         */
        if (   pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed
            || pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessConfirmed)
            pNtProtect->enmProcessKind = kSupDrvNtProtectKind_VmProcessDead;
        else if (   pNtProtect->enmProcessKind == kSupDrvNtProtectKind_StubParent
                 || pNtProtect->enmProcessKind == kSupDrvNtProtectKind_StubSpawning
                 || pNtProtect->enmProcessKind == kSupDrvNtProtectKind_StubUnverified)
            pNtProtect->enmProcessKind = kSupDrvNtProtectKind_StubDead;

        RTSpinlockRelease(g_hNtProtectLock);

        supdrvNtProtectRelease(pNtProtect);
        supdrvNtProtectRelease(pNtChild);

        /*
         * Do session cleanups.
         */
        AssertReturnVoid((HANDLE)(uintptr_t)RTProcSelf() == hDeadPid);
        if (g_pDevObjSys)
        {
            PSUPDRVDEVEXT  pDevExt  = (PSUPDRVDEVEXT)g_pDevObjSys->DeviceExtension;
            PSUPDRVSESSION pSession = supdrvSessionHashTabLookup(pDevExt, (RTPROCESS)(uintptr_t)hDeadPid,
                                                                 RTR0ProcHandleSelf(), NULL);
            if (pSession)
            {
                supdrvSessionHashTabRemove(pDevExt, pSession, NULL);
                supdrvSessionRelease(pSession); /* Drops the reference from supdrvSessionHashTabLookup. */
            }
        }
    }
}


/**
 * Common worker for the process creation callback that verifies a new child
 * being created by the handle creation callback code.
 *
 * @param   pNtStub         The parent.
 * @param   pNtVm           The child.
 * @param   fCallerChecks   The result of any additional tests the caller made.
 *                          This is in order to avoid duplicating the failure
 *                          path code.
 */
static void supdrvNtProtectVerifyNewChildProtection(PSUPDRVNTPROTECT pNtStub, PSUPDRVNTPROTECT pNtVm, bool fCallerChecks)
{
    if (   fCallerChecks
        && pNtStub->enmProcessKind == kSupDrvNtProtectKind_StubParent
        && pNtVm->enmProcessKind   == kSupDrvNtProtectKind_VmProcessUnconfirmed
        && pNtVm->u.pParent        == pNtStub
        && pNtStub->u.pChild       == pNtVm)
    {
        /* Fine, reset the CSRSS hack (fixes ViRobot APT Shield 2.0 issue). */
        pNtVm->fFirstProcessCreateHandle = true;
        return;
    }

    LogRel(("vboxdrv: Misdetected vm stub; hParentPid=%p hChildPid=%p\n", pNtStub->AvlCore.Key, pNtVm->AvlCore.Key));
    if (pNtStub->enmProcessKind != kSupDrvNtProtectKind_VmProcessConfirmed)
        supdrvNtProtectUnprotectDeadProcess(pNtVm->AvlCore.Key);
}


/**
 * Old style callback (since forever).
 *
 * @param   hParentPid          The parent PID.
 * @param   hNewPid             The PID of the new child.
 * @param   fCreated            TRUE if it's a creation notification,
 *                              FALSE if termination.
 * @remarks ASSUMES this arrives before the handle creation callback.
 */
static VOID __stdcall
supdrvNtProtectCallback_ProcessCreateNotify(HANDLE hParentPid, HANDLE hNewPid, BOOLEAN fCreated)
{
    /*
     * Is it a new process that needs protection?
     */
    if (fCreated)
    {
        PSUPDRVNTPROTECT pNtStub = supdrvNtProtectLookup(hParentPid);
        if (pNtStub)
        {
            PSUPDRVNTPROTECT pNtVm = supdrvNtProtectLookup(hNewPid);
            if (!pNtVm)
            {
                if (supdrvNtProtectIsSpawningStubProcess(pNtStub, hParentPid, hNewPid))
                    supdrvNtProtectProtectNewStubChild(pNtStub, hNewPid);
            }
            else
            {
                supdrvNtProtectVerifyNewChildProtection(pNtStub, pNtVm, true);
                supdrvNtProtectRelease(pNtVm);
            }
            supdrvNtProtectRelease(pNtStub);
        }
    }
    /*
     * Process termination, do clean ups.
     */
    else
    {
        supdrvNtProtectUnprotectDeadProcess(hNewPid);
        supdrvNtErrorInfoCleanupProcess(hNewPid);
    }
}


/**
 * New style callback (Vista SP1+ / w2k8).
 *
 * @param   pNewProcess         The new process.
 * @param   hNewPid             The PID of the new process.
 * @param   pInfo               Process creation details. NULL if process
 *                              termination notification.
 * @remarks ASSUMES this arrives before the handle creation callback.
 */
static VOID __stdcall
supdrvNtProtectCallback_ProcessCreateNotifyEx(PEPROCESS pNewProcess, HANDLE hNewPid, PPS_CREATE_NOTIFY_INFO pInfo)
{
    RT_NOREF1(pNewProcess);

    /*
     * Is it a new process that needs protection?
     */
    if (pInfo)
    {
        PSUPDRVNTPROTECT pNtStub = supdrvNtProtectLookup(pInfo->CreatingThreadId.UniqueProcess);

        Log(("vboxdrv/NewProcessEx: ctx=%04zx/%p pid=%04zx ppid=%04zx ctor=%04zx/%04zx rcNt=%#x %.*ls\n",
             PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
             hNewPid, pInfo->ParentProcessId,
             pInfo->CreatingThreadId.UniqueProcess, pInfo->CreatingThreadId.UniqueThread, pInfo->CreationStatus,
             pInfo->FileOpenNameAvailable && pInfo->ImageFileName ? (size_t)pInfo->ImageFileName->Length / 2 : 0,
             pInfo->FileOpenNameAvailable && pInfo->ImageFileName ? pInfo->ImageFileName->Buffer : NULL));

        if (pNtStub)
        {
            PSUPDRVNTPROTECT pNtVm = supdrvNtProtectLookup(hNewPid);
            if (!pNtVm)
            {
                /* Parent must be creator. */
                if (pInfo->CreatingThreadId.UniqueProcess == pInfo->ParentProcessId)
                {
                    if (supdrvNtProtectIsSpawningStubProcess(pNtStub, pInfo->ParentProcessId, hNewPid))
                        supdrvNtProtectProtectNewStubChild(pNtStub, hNewPid);
                }
            }
            else
            {
                /* Parent must be creator (as above). */
                supdrvNtProtectVerifyNewChildProtection(pNtStub, pNtVm,
                                                        pInfo->CreatingThreadId.UniqueProcess == pInfo->ParentProcessId);
                supdrvNtProtectRelease(pNtVm);
            }
            supdrvNtProtectRelease(pNtStub);
        }
    }
    /*
     * Process termination, do clean ups.
     */
    else
    {
        supdrvNtProtectUnprotectDeadProcess(hNewPid);
        supdrvNtErrorInfoCleanupProcess(hNewPid);
    }
}

/** @} */


/** @name Process Handle Callbacks.
 * @{ */

/** Process rights that we allow for handles to stub and VM processes. */
# define SUPDRV_NT_ALLOW_PROCESS_RIGHTS  \
    (  PROCESS_TERMINATE \
     | PROCESS_VM_READ \
     | PROCESS_QUERY_INFORMATION \
     | PROCESS_QUERY_LIMITED_INFORMATION \
     | PROCESS_SUSPEND_RESUME \
     | DELETE \
     | READ_CONTROL \
     | SYNCHRONIZE)

/** Evil process rights. */
# define SUPDRV_NT_EVIL_PROCESS_RIGHTS  \
    (  PROCESS_CREATE_THREAD \
     | PROCESS_SET_SESSIONID /*?*/ \
     | PROCESS_VM_OPERATION \
     | PROCESS_VM_WRITE \
     | PROCESS_DUP_HANDLE \
     | PROCESS_CREATE_PROCESS /*?*/ \
     | PROCESS_SET_QUOTA /*?*/ \
     | PROCESS_SET_INFORMATION \
     | PROCESS_SET_LIMITED_INFORMATION /*?*/ \
     | 0)
AssertCompile((SUPDRV_NT_ALLOW_PROCESS_RIGHTS & SUPDRV_NT_EVIL_PROCESS_RIGHTS) == 0);


static OB_PREOP_CALLBACK_STATUS __stdcall
supdrvNtProtectCallback_ProcessHandlePre(PVOID pvUser, POB_PRE_OPERATION_INFORMATION pOpInfo)
{
    Assert(pvUser == NULL); RT_NOREF1(pvUser);
    Assert(pOpInfo->Operation == OB_OPERATION_HANDLE_CREATE || pOpInfo->Operation == OB_OPERATION_HANDLE_DUPLICATE);
    Assert(pOpInfo->ObjectType == *PsProcessType);

    /*
     * Protected?  Kludge required for NtOpenProcess calls comming in before
     * the create process hook triggers on Windows 8.1 (possibly others too).
     */
    HANDLE           hObjPid    = PsGetProcessId((PEPROCESS)pOpInfo->Object);
    PSUPDRVNTPROTECT pNtProtect = supdrvNtProtectLookup(hObjPid);
    if (!pNtProtect)
    {
        HANDLE           hParentPid = PsGetProcessInheritedFromUniqueProcessId((PEPROCESS)pOpInfo->Object);
        PSUPDRVNTPROTECT pNtStub = supdrvNtProtectLookup(hParentPid);
        if (pNtStub)
        {
            if (supdrvNtProtectIsSpawningStubProcess(pNtStub, hParentPid, hObjPid))
            {
                supdrvNtProtectProtectNewStubChild(pNtStub, hObjPid);
                pNtProtect = supdrvNtProtectLookup(hObjPid);
            }
            supdrvNtProtectRelease(pNtStub);
        }
    }
    pOpInfo->CallContext = pNtProtect; /* Just for reference. */
    if (pNtProtect)
    {
        /*
         * Ok, it's a protected process.  Strip rights as required or possible.
         */
        static ACCESS_MASK const s_fCsrssStupidDesires = 0x1fffff;
        ACCESS_MASK fAllowedRights = SUPDRV_NT_ALLOW_PROCESS_RIGHTS;

        if (pOpInfo->Operation == OB_OPERATION_HANDLE_CREATE)
        {
            /* Don't restrict the process accessing itself. */
            if ((PEPROCESS)pOpInfo->Object == PsGetCurrentProcess())
            {
                pOpInfo->CallContext = NULL; /* don't assert */
                pNtProtect->fFirstProcessCreateHandle = false;

                Log(("vboxdrv/ProcessHandlePre: %sctx=%04zx/%p wants %#x to %p in pid=%04zx [%d] %s\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->CreateHandleInformation.DesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind,
                     PsGetProcessImageFileName(PsGetCurrentProcess()) ));
            }
#ifdef VBOX_WITHOUT_DEBUGGER_CHECKS
            /* Allow debuggers full access. */
            else if (supdrvNtProtectIsWhitelistedDebugger(PsGetCurrentProcess()))
            {
                pOpInfo->CallContext = NULL; /* don't assert */
                pNtProtect->fFirstProcessCreateHandle = false;

                Log(("vboxdrv/ProcessHandlePre: %sctx=%04zx/%p wants %#x to %p in pid=%04zx [%d] %s [debugger]\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->CreateHandleInformation.DesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind,
                     PsGetProcessImageFileName(PsGetCurrentProcess()) ));
            }
#endif
            else
            {
                ACCESS_MASK const fDesiredAccess = pOpInfo->Parameters->CreateHandleInformation.DesiredAccess;

                /* Special case 1 on Vista, 7 & 8:
                   The CreateProcess code passes the handle over to CSRSS.EXE
                   and the code inBaseSrvCreateProcess will duplicate the
                   handle with 0x1fffff as access mask.  NtDuplicateObject will
                   fail this call before it ever gets down here.

                   Special case 2 on 8.1:
                   The CreateProcess code requires additional rights for
                   something, we'll drop these in the stub code. */
                if (   pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed
                    && pNtProtect->fFirstProcessCreateHandle
                    && pOpInfo->KernelHandle == 0
                    && pNtProtect->hParentPid == PsGetProcessId(PsGetCurrentProcess())
                    && ExGetPreviousMode() != KernelMode)
                {
                    if (   !pOpInfo->KernelHandle
                        && fDesiredAccess == s_fCsrssStupidDesires)
                    {
                        if (g_uNtVerCombined < SUP_MAKE_NT_VER_SIMPLE(6, 3))
                            fAllowedRights |= s_fCsrssStupidDesires;
                        else
                            fAllowedRights = fAllowedRights
                                           | PROCESS_VM_OPERATION
                                           | PROCESS_VM_WRITE
                                           | PROCESS_SET_INFORMATION
                                           | PROCESS_SET_LIMITED_INFORMATION
                                           | 0;
                        pOpInfo->CallContext = NULL; /* don't assert this. */
                    }
                    pNtProtect->fFirstProcessCreateHandle = false;
                }

                /* Special case 3 on 8.1:
                   The interaction between the CreateProcess code and CSRSS.EXE
                   has changed to the better with Windows 8.1.  CSRSS.EXE no
                   longer duplicates the process (thread too) handle, but opens
                   it, thus allowing us to do our job. */
                if (   g_uNtVerCombined >= SUP_MAKE_NT_VER_SIMPLE(6, 3)
                    && pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed
                    && pNtProtect->fCsrssFirstProcessCreateHandle
                    && pOpInfo->KernelHandle == 0
                    && ExGetPreviousMode() == UserMode
                    && supdrvNtProtectIsAssociatedCsrss(pNtProtect, PsGetCurrentProcess()) )
                {
                    pNtProtect->fCsrssFirstProcessCreateHandle = false;
                    if (fDesiredAccess == s_fCsrssStupidDesires)
                    {
                        /* Not needed: PROCESS_CREATE_THREAD, PROCESS_SET_SESSIONID,
                           PROCESS_CREATE_PROCESS */
                        fAllowedRights = fAllowedRights
                                       | PROCESS_VM_OPERATION
                                       | PROCESS_VM_WRITE
                                       | PROCESS_DUP_HANDLE /* Needed for CreateProcess/VBoxTestOGL. */
                                       | 0;
                        pOpInfo->CallContext = NULL; /* don't assert this. */
                    }
                }

                /* Special case 4, Windows 7, Vista, possibly 8, but not 8.1:
                   The Themes service requires PROCESS_DUP_HANDLE access to our
                   process or we won't get any menus and dialogs will be half
                   unreadable.  This is _very_ unfortunate and more work will
                   go into making this more secure.  */
                if (   g_uNtVerCombined >= SUP_MAKE_NT_VER_SIMPLE(6, 0)
                    && g_uNtVerCombined  < SUP_MAKE_NT_VER_SIMPLE(6, 2)
                    && fDesiredAccess == 0x1478 /* 6.1.7600.16385 (win7_rtm.090713-1255) */
                    && pNtProtect->fThemesFirstProcessCreateHandle
                    && pOpInfo->KernelHandle == 0
                    && ExGetPreviousMode() == UserMode
                    && supdrvNtProtectIsFrigginThemesService(pNtProtect, PsGetCurrentProcess()) )
                {
                    pNtProtect->fThemesFirstProcessCreateHandle = true; /* Only once! */
                    fAllowedRights |= PROCESS_DUP_HANDLE;
                    pOpInfo->CallContext = NULL; /* don't assert this. */
                }

                /* Special case 6a, Windows 10+: AudioDG.exe opens the process with the
                   PROCESS_SET_LIMITED_INFORMATION right.  It seems like it need it for
                   some myserious and weirdly placed cpu set management of our process.
                   I'd love to understand what that's all about...
                   Currently playing safe and only grand this right, however limited, to
                   audiodg.exe. */
                if (   g_uNtVerCombined >= SUP_MAKE_NT_VER_SIMPLE(10, 0)
                    && (   fDesiredAccess == PROCESS_SET_LIMITED_INFORMATION
                        || fDesiredAccess == (PROCESS_SET_LIMITED_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION) /* expected fix #1 */
                        || fDesiredAccess == (PROCESS_SET_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION)         /* expected fix #2 */
                        )
                    && pOpInfo->KernelHandle == 0
                    && ExGetPreviousMode() == UserMode
                    && supdrvNtProtectIsSystem32ProcessMatch(PsGetCurrentProcess(), "audiodg.exe") )
                {
                    fAllowedRights |= PROCESS_SET_LIMITED_INFORMATION;
                    pOpInfo->CallContext = NULL; /* don't assert this. */
                }

                Log(("vboxdrv/ProcessHandlePre: %sctx=%04zx/%p wants %#x to %p/pid=%04zx [%d], allow %#x => %#x; %s [prev=%#x]\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     fDesiredAccess, pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind,
                     fAllowedRights, fDesiredAccess & fAllowedRights,
                     PsGetProcessImageFileName(PsGetCurrentProcess()), ExGetPreviousMode() ));

                pOpInfo->Parameters->CreateHandleInformation.DesiredAccess &= fAllowedRights;
            }
        }
        else
        {
            /* Don't restrict the process accessing itself. */
            if (   (PEPROCESS)pOpInfo->Object == PsGetCurrentProcess()
                && pOpInfo->Parameters->DuplicateHandleInformation.TargetProcess == pOpInfo->Object)
            {
                Log(("vboxdrv/ProcessHandlePre: ctx=%04zx/%p[%p] dup from %04zx/%p with %#x to %p in pid=%04zx [%d] %s\n",
                     PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->DuplicateHandleInformation.TargetProcess,
                     PsGetProcessId((PEPROCESS)pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess),
                     pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess,
                     pOpInfo->Parameters->DuplicateHandleInformation.DesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind,
                     PsGetProcessImageFileName(PsGetCurrentProcess()) ));

                pOpInfo->CallContext = NULL; /* don't assert */
            }
            else
            {
                ACCESS_MASK const fDesiredAccess = pOpInfo->Parameters->DuplicateHandleInformation.DesiredAccess;

                /* Special case 5 on Vista, 7 & 8:
                   This is the CSRSS.EXE end of special case #1. */
                if (   g_uNtVerCombined < SUP_MAKE_NT_VER_SIMPLE(6, 3)
                    && pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed
                    && pNtProtect->cCsrssFirstProcessDuplicateHandle > 0
                    && pOpInfo->KernelHandle == 0
                    && fDesiredAccess == s_fCsrssStupidDesires
                    &&    pNtProtect->hParentPid
                       == PsGetProcessId((PEPROCESS)pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess)
                    && pOpInfo->Parameters->DuplicateHandleInformation.TargetProcess == PsGetCurrentProcess()
                    && ExGetPreviousMode() == UserMode
                    && supdrvNtProtectIsAssociatedCsrss(pNtProtect, PsGetCurrentProcess()))
                {
                    if (ASMAtomicDecS32(&pNtProtect->cCsrssFirstProcessDuplicateHandle) >= 0)
                    {
                        /* Not needed: PROCESS_CREATE_THREAD, PROCESS_SET_SESSIONID,
                           PROCESS_CREATE_PROCESS, PROCESS_DUP_HANDLE */
                        fAllowedRights = fAllowedRights
                                       | PROCESS_VM_OPERATION
                                       | PROCESS_VM_WRITE
                                       | PROCESS_DUP_HANDLE /* Needed for launching VBoxTestOGL. */
                                       | 0;
                        pOpInfo->CallContext = NULL; /* don't assert this. */
                    }
                }

                /* Special case 6b, Windows 10+: AudioDG.exe duplicates the handle it opened above. */
                if (   g_uNtVerCombined >= SUP_MAKE_NT_VER_SIMPLE(10, 0)
                    && (   fDesiredAccess == PROCESS_SET_LIMITED_INFORMATION
                        || fDesiredAccess == (PROCESS_SET_LIMITED_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION) /* expected fix #1 */
                        || fDesiredAccess == (PROCESS_SET_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION)         /* expected fix #2 */
                        )
                    && pOpInfo->KernelHandle == 0
                    && ExGetPreviousMode() == UserMode
                    && supdrvNtProtectIsSystem32ProcessMatch(PsGetCurrentProcess(), "audiodg.exe") )
                {
                    fAllowedRights |= PROCESS_SET_LIMITED_INFORMATION;
                    pOpInfo->CallContext = NULL; /* don't assert this. */
                }

                Log(("vboxdrv/ProcessHandlePre: %sctx=%04zx/%p[%p] dup from %04zx/%p with %#x to %p in pid=%04zx [%d] %s\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->DuplicateHandleInformation.TargetProcess,
                     PsGetProcessId((PEPROCESS)pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess),
                     pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess,
                     fDesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind,
                     PsGetProcessImageFileName(PsGetCurrentProcess()) ));

                pOpInfo->Parameters->DuplicateHandleInformation.DesiredAccess &= fAllowedRights;
            }
        }
        supdrvNtProtectRelease(pNtProtect);
    }

    return OB_PREOP_SUCCESS;
}


static VOID __stdcall
supdrvNtProtectCallback_ProcessHandlePost(PVOID pvUser, POB_POST_OPERATION_INFORMATION pOpInfo)
{
    Assert(pvUser == NULL); RT_NOREF1(pvUser);
    Assert(pOpInfo->Operation == OB_OPERATION_HANDLE_CREATE || pOpInfo->Operation == OB_OPERATION_HANDLE_DUPLICATE);
    Assert(pOpInfo->ObjectType == *PsProcessType);

    if (   pOpInfo->CallContext
        && NT_SUCCESS(pOpInfo->ReturnStatus))
    {
        ACCESS_MASK const fGrantedAccess = pOpInfo->Operation == OB_OPERATION_HANDLE_CREATE
                                         ? pOpInfo->Parameters->CreateHandleInformation.GrantedAccess
                                         : pOpInfo->Parameters->DuplicateHandleInformation.GrantedAccess;
        AssertReleaseMsg(   !(fGrantedAccess & ~(  SUPDRV_NT_ALLOW_PROCESS_RIGHTS
                                                 | WRITE_OWNER | WRITE_DAC /* these two might be forced upon us */
                                                 | PROCESS_UNKNOWN_4000 /* Seen set on win 8.1 */
                                                 /*| PROCESS_UNKNOWN_8000 */ ) )
                         || pOpInfo->KernelHandle,
                         ("GrantedAccess=%#x - we allow %#x - we did not allow %#x\n",
                          fGrantedAccess, SUPDRV_NT_ALLOW_PROCESS_RIGHTS, fGrantedAccess & ~SUPDRV_NT_ALLOW_PROCESS_RIGHTS));
    }
}

# undef SUPDRV_NT_ALLOW_PROCESS_RIGHTS

/** @} */


/** @name Thread Handle Callbacks
 * @{ */

/* From ntifs.h */
extern "C" NTKERNELAPI PEPROCESS __stdcall IoThreadToProcess(PETHREAD);

/** Thread rights that we allow for handles to stub and VM processes. */
# define SUPDRV_NT_ALLOWED_THREAD_RIGHTS  \
    (  THREAD_TERMINATE \
     | THREAD_GET_CONTEXT \
     | THREAD_QUERY_INFORMATION \
     | THREAD_QUERY_LIMITED_INFORMATION \
     | DELETE \
     | READ_CONTROL \
     | SYNCHRONIZE)
/** @todo consider THREAD_SET_LIMITED_INFORMATION & THREAD_RESUME */

/** Evil thread rights.
 * @remarks THREAD_RESUME is not included as it seems to be forced upon us by
 *          Windows 8.1, at least for some processes.  We dont' actively
 *          allow it though, just tollerate it when forced to. */
# define SUPDRV_NT_EVIL_THREAD_RIGHTS  \
    (  THREAD_SUSPEND_RESUME \
     | THREAD_SET_CONTEXT \
     | THREAD_SET_INFORMATION \
     | THREAD_SET_LIMITED_INFORMATION /*?*/ \
     | THREAD_SET_THREAD_TOKEN /*?*/ \
     | THREAD_IMPERSONATE /*?*/ \
     | THREAD_DIRECT_IMPERSONATION /*?*/ \
     /*| THREAD_RESUME - see remarks. */ \
     | 0)
AssertCompile((SUPDRV_NT_EVIL_THREAD_RIGHTS & SUPDRV_NT_ALLOWED_THREAD_RIGHTS) == 0);


static OB_PREOP_CALLBACK_STATUS __stdcall
supdrvNtProtectCallback_ThreadHandlePre(PVOID pvUser, POB_PRE_OPERATION_INFORMATION pOpInfo)
{
    Assert(pvUser == NULL); RT_NOREF1(pvUser);
    Assert(pOpInfo->Operation == OB_OPERATION_HANDLE_CREATE || pOpInfo->Operation == OB_OPERATION_HANDLE_DUPLICATE);
    Assert(pOpInfo->ObjectType == *PsThreadType);

    PEPROCESS pProcess = IoThreadToProcess((PETHREAD)pOpInfo->Object);
    PSUPDRVNTPROTECT pNtProtect = supdrvNtProtectLookup(PsGetProcessId(pProcess));
    pOpInfo->CallContext = pNtProtect; /* Just for reference. */
    if (pNtProtect)
    {
        static ACCESS_MASK const s_fCsrssStupidDesires = 0x1fffff;
        ACCESS_MASK fAllowedRights = SUPDRV_NT_ALLOWED_THREAD_RIGHTS;

        if (pOpInfo->Operation == OB_OPERATION_HANDLE_CREATE)
        {
            /* Don't restrict the process accessing its own threads. */
            if (pProcess == PsGetCurrentProcess())
            {
                Log(("vboxdrv/ThreadHandlePre: %sctx=%04zx/%p wants %#x to %p in pid=%04zx [%d] self\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->CreateHandleInformation.DesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind));
                pOpInfo->CallContext = NULL; /* don't assert */
                pNtProtect->fFirstThreadCreateHandle = false;
            }
#ifdef VBOX_WITHOUT_DEBUGGER_CHECKS
            /* Allow debuggers full access. */
            else if (supdrvNtProtectIsWhitelistedDebugger(PsGetCurrentProcess()))
            {
                Log(("vboxdrv/ThreadHandlePre: %sctx=%04zx/%p wants %#x to %p in pid=%04zx [%d] %s [debugger]\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->CreateHandleInformation.DesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind,
                     PsGetProcessImageFileName(PsGetCurrentProcess()) ));
                pOpInfo->CallContext = NULL; /* don't assert */
            }
#endif
            else
            {
                /* Special case 1 on Vista, 7, 8:
                   The CreateProcess code passes the handle over to CSRSS.EXE
                   and the code inBaseSrvCreateProcess will duplicate the
                   handle with 0x1fffff as access mask.  NtDuplicateObject will
                   fail this call before it ever gets down here.  */
                if (   g_uNtVerCombined < SUP_MAKE_NT_VER_SIMPLE(6, 3)
                    && pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed
                    && pNtProtect->fFirstThreadCreateHandle
                    && pOpInfo->KernelHandle == 0
                    && ExGetPreviousMode() == UserMode
                    && pNtProtect->hParentPid == PsGetProcessId(PsGetCurrentProcess()) )
                {
                    if (   !pOpInfo->KernelHandle
                        && pOpInfo->Parameters->CreateHandleInformation.DesiredAccess == s_fCsrssStupidDesires)
                    {
                        fAllowedRights |= s_fCsrssStupidDesires;
                        pOpInfo->CallContext = NULL; /* don't assert this. */
                    }
                    pNtProtect->fFirstThreadCreateHandle = false;
                }

                /* Special case 2 on 8.1, possibly also Vista, 7, 8:
                   When creating a process like VBoxTestOGL from the VM process,
                   CSRSS.EXE will try talk to the calling thread and, it
                   appears, impersonate it.  We unfortunately need to allow
                   this or there will be no 3D support.  Typical DbgPrint:
                        "SXS: BasepCreateActCtx() Calling csrss server failed. Status = 0xc00000a5" */
                SUPDRVNTPROTECTKIND enmProcessKind;
                if (   g_uNtVerCombined >= SUP_MAKE_NT_VER_COMBINED(6, 0, 0, 0, 0)
                    && (   (enmProcessKind = pNtProtect->enmProcessKind) == kSupDrvNtProtectKind_VmProcessConfirmed
                        || enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed)
                    && pOpInfo->KernelHandle == 0
                    && ExGetPreviousMode() == UserMode
                    && supdrvNtProtectIsAssociatedCsrss(pNtProtect, PsGetCurrentProcess()) )
                {
                    fAllowedRights |= THREAD_IMPERSONATE;
                    fAllowedRights |= THREAD_DIRECT_IMPERSONATION;
                    //fAllowedRights |= THREAD_SET_LIMITED_INFORMATION; - try without this one
                    pOpInfo->CallContext = NULL; /* don't assert this. */
                }

                Log(("vboxdrv/ThreadHandlePre: %sctx=%04zx/%p wants %#x to %p in pid=%04zx [%d], allow %#x => %#x; %s [prev=%#x]\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->CreateHandleInformation.DesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind, fAllowedRights,
                     pOpInfo->Parameters->CreateHandleInformation.DesiredAccess & fAllowedRights,
                     PsGetProcessImageFileName(PsGetCurrentProcess()), ExGetPreviousMode()));

                pOpInfo->Parameters->CreateHandleInformation.DesiredAccess &= fAllowedRights;
            }
        }
        else
        {
            /* Don't restrict the process accessing its own threads. */
            if (   pProcess == PsGetCurrentProcess()
                && (PEPROCESS)pOpInfo->Parameters->DuplicateHandleInformation.TargetProcess == pProcess)
            {
                Log(("vboxdrv/ThreadHandlePre: %sctx=%04zx/%p[%p] dup from %04zx/%p with %#x to %p in pid=%04zx [%d] self\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->DuplicateHandleInformation.TargetProcess,
                     PsGetProcessId((PEPROCESS)pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess),
                     pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess,
                     pOpInfo->Parameters->DuplicateHandleInformation.DesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind,
                     PsGetProcessImageFileName(PsGetCurrentProcess()) ));
                pOpInfo->CallContext = NULL; /* don't assert */
            }
            else
            {
                /* Special case 3 on Vista, 7, 8:
                   This is the follow up to special case 1. */
                SUPDRVNTPROTECTKIND enmProcessKind;
                if (   g_uNtVerCombined >= SUP_MAKE_NT_VER_COMBINED(6, 0, 0, 0, 0)
                    && (   (enmProcessKind = pNtProtect->enmProcessKind) == kSupDrvNtProtectKind_VmProcessConfirmed
                        || enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed)
                    && pOpInfo->Parameters->DuplicateHandleInformation.TargetProcess == PsGetCurrentProcess()
                    && pOpInfo->KernelHandle == 0
                    && ExGetPreviousMode() == UserMode
                    && supdrvNtProtectIsAssociatedCsrss(pNtProtect, PsGetCurrentProcess()) )
                {
                    fAllowedRights |= THREAD_IMPERSONATE;
                    fAllowedRights |= THREAD_DIRECT_IMPERSONATION;
                    //fAllowedRights |= THREAD_SET_LIMITED_INFORMATION; - try without this one
                    pOpInfo->CallContext = NULL; /* don't assert this. */
                }

                Log(("vboxdrv/ThreadHandlePre: %sctx=%04zx/%p[%p] dup from %04zx/%p with %#x to %p in pid=%04zx [%d], allow %#x => %#x; %s\n",
                     pOpInfo->KernelHandle ? "k" : "", PsGetProcessId(PsGetCurrentProcess()), PsGetCurrentProcess(),
                     pOpInfo->Parameters->DuplicateHandleInformation.TargetProcess,
                     PsGetProcessId((PEPROCESS)pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess),
                     pOpInfo->Parameters->DuplicateHandleInformation.SourceProcess,
                     pOpInfo->Parameters->DuplicateHandleInformation.DesiredAccess,
                     pOpInfo->Object, pNtProtect->AvlCore.Key, pNtProtect->enmProcessKind, fAllowedRights,
                     pOpInfo->Parameters->DuplicateHandleInformation.DesiredAccess & fAllowedRights,
                     PsGetProcessImageFileName(PsGetCurrentProcess()) ));

                pOpInfo->Parameters->DuplicateHandleInformation.DesiredAccess &= fAllowedRights;
            }
        }

        supdrvNtProtectRelease(pNtProtect);
    }

    return OB_PREOP_SUCCESS;
}


static VOID __stdcall
supdrvNtProtectCallback_ThreadHandlePost(PVOID pvUser, POB_POST_OPERATION_INFORMATION pOpInfo)
{
    Assert(pvUser == NULL); RT_NOREF1(pvUser);
    Assert(pOpInfo->Operation == OB_OPERATION_HANDLE_CREATE || pOpInfo->Operation == OB_OPERATION_HANDLE_DUPLICATE);
    Assert(pOpInfo->ObjectType == *PsThreadType);

    if (   pOpInfo->CallContext
        && NT_SUCCESS(pOpInfo->ReturnStatus))
    {
        ACCESS_MASK const fGrantedAccess = pOpInfo->Parameters->CreateHandleInformation.GrantedAccess;
        AssertReleaseMsg(   !(fGrantedAccess & ~(  SUPDRV_NT_ALLOWED_THREAD_RIGHTS
                                                 | WRITE_OWNER | WRITE_DAC /* these two might be forced upon us */
                                                 | THREAD_RESUME /* This seems to be force upon us too with 8.1. */
                                                 ) )
                         || pOpInfo->KernelHandle,
                         ("GrantedAccess=%#x - we allow %#x - we did not allow %#x\n",
                          fGrantedAccess, SUPDRV_NT_ALLOWED_THREAD_RIGHTS, fGrantedAccess & ~SUPDRV_NT_ALLOWED_THREAD_RIGHTS));
    }
}

# undef SUPDRV_NT_ALLOWED_THREAD_RIGHTS

/** @} */


/**
 * Creates a new process protection structure.
 *
 * @returns VBox status code.
 * @param   ppNtProtect         Where to return the pointer to the structure
 *                              on success.
 * @param   hPid                The process ID of the process to protect.
 * @param   enmProcessKind      The kind of process we're protecting.
 * @param   fLink               Whether to link the structure into the tree.
 */
static int supdrvNtProtectCreate(PSUPDRVNTPROTECT *ppNtProtect, HANDLE hPid, SUPDRVNTPROTECTKIND enmProcessKind, bool fLink)
{
    AssertReturn(g_hNtProtectLock != NIL_RTSPINLOCK, VERR_WRONG_ORDER);

    PSUPDRVNTPROTECT pNtProtect = (PSUPDRVNTPROTECT)RTMemAllocZ(sizeof(*pNtProtect));
    if (!pNtProtect)
        return VERR_NO_MEMORY;

    pNtProtect->AvlCore.Key                  = hPid;
    pNtProtect->u32Magic                     = SUPDRVNTPROTECT_MAGIC;
    pNtProtect->cRefs                        = 1;
    pNtProtect->enmProcessKind               = enmProcessKind;
    pNtProtect->hParentPid                   = NULL;
    pNtProtect->hOpenTid                     = NULL;
    pNtProtect->hCsrssPid                    = NULL;
    pNtProtect->pCsrssProcess                = NULL;

    if (fLink)
    {
        RTSpinlockAcquire(g_hNtProtectLock);
        bool fSuccess = RTAvlPVInsert(&g_NtProtectTree, &pNtProtect->AvlCore);
        pNtProtect->fInTree = fSuccess;
        RTSpinlockRelease(g_hNtProtectLock);

        if (!fSuccess)
        {
            /* Duplicate entry, fail. */
            pNtProtect->u32Magic = SUPDRVNTPROTECT_MAGIC_DEAD;
            LogRel(("supdrvNtProtectCreate: Duplicate (%#x).\n", pNtProtect->AvlCore.Key));
            RTMemFree(pNtProtect);
            return VERR_DUPLICATE;
        }
    }

    *ppNtProtect = pNtProtect;
    return VINF_SUCCESS;
}


/**
 * Releases a reference to a NT protection structure.
 *
 * @param   pNtProtect          The NT protection structure.
 */
static void supdrvNtProtectRelease(PSUPDRVNTPROTECT pNtProtect)
{
    if (!pNtProtect)
        return;
    AssertReturnVoid(pNtProtect->u32Magic == SUPDRVNTPROTECT_MAGIC);

    RTSpinlockAcquire(g_hNtProtectLock);
    uint32_t cRefs = ASMAtomicDecU32(&pNtProtect->cRefs);
    if (cRefs != 0)
        RTSpinlockRelease(g_hNtProtectLock);
    else
    {
        /*
         * That was the last reference.  Remove it from the tree, invalidate it
         * and free the resources associated with it.  Also, release any
         * child/parent references related to this protection structure.
         */
        ASMAtomicWriteU32(&pNtProtect->u32Magic, SUPDRVNTPROTECT_MAGIC_DEAD);
        if (pNtProtect->fInTree)
        {
            PSUPDRVNTPROTECT pRemoved = (PSUPDRVNTPROTECT)RTAvlPVRemove(&g_NtProtectTree, pNtProtect->AvlCore.Key);
            Assert(pRemoved == pNtProtect); RT_NOREF_PV(pRemoved);
            pNtProtect->fInTree = false;
        }

        PSUPDRVNTPROTECT pChild = NULL;
        if (pNtProtect->enmProcessKind == kSupDrvNtProtectKind_StubParent)
        {
            pChild = pNtProtect->u.pChild;
            if (pChild)
            {
                pNtProtect->u.pChild   = NULL;
                pChild->u.pParent      = NULL;
                pChild->enmProcessKind = kSupDrvNtProtectKind_VmProcessDead;
                uint32_t cChildRefs = ASMAtomicDecU32(&pChild->cRefs);
                if (!cChildRefs)
                {
                    Assert(pChild->fInTree);
                    if (pChild->fInTree)
                    {
                        PSUPDRVNTPROTECT pRemovedChild = (PSUPDRVNTPROTECT)RTAvlPVRemove(&g_NtProtectTree, pChild->AvlCore.Key);
                        Assert(pRemovedChild == pChild); RT_NOREF_PV(pRemovedChild);
                        pChild->fInTree = false;
                    }
                }
                else
                    pChild = NULL;
            }
        }
        else
            AssertRelease(pNtProtect->enmProcessKind != kSupDrvNtProtectKind_VmProcessUnconfirmed);

        RTSpinlockRelease(g_hNtProtectLock);

        if (pNtProtect->pCsrssProcess)
        {
            ObDereferenceObject(pNtProtect->pCsrssProcess);
            pNtProtect->pCsrssProcess = NULL;
        }

        RTMemFree(pNtProtect);
        if (pChild)
            RTMemFree(pChild);
    }
}


/**
 * Looks up a PID in the NT protect tree.
 *
 * @returns Pointer to a NT protection structure (with a referenced) on success,
 *          NULL if not found.
 * @param   hPid                The process ID.
 */
static PSUPDRVNTPROTECT supdrvNtProtectLookup(HANDLE hPid)
{
    RTSpinlockAcquire(g_hNtProtectLock);
    PSUPDRVNTPROTECT pFound = (PSUPDRVNTPROTECT)RTAvlPVGet(&g_NtProtectTree, hPid);
    if (pFound)
        ASMAtomicIncU32(&pFound->cRefs);
    RTSpinlockRelease(g_hNtProtectLock);
    return pFound;
}


/**
 * Validates a few facts about the stub process when the VM process opens
 * vboxdrv.
 *
 * This makes sure the stub process is still around and that it has neither
 * debugger nor extra threads in it.
 *
 * @returns VBox status code.
 * @param   pNtProtect          The unconfirmed VM process currently trying to
 *                              open vboxdrv.
 * @param   pErrInfo            Additional error information.
 */
static int supdrvNtProtectVerifyStubForVmProcess(PSUPDRVNTPROTECT pNtProtect, PRTERRINFO pErrInfo)
{
    /*
     * Grab a reference to the parent stub process.
     */
    SUPDRVNTPROTECTKIND enmStub = kSupDrvNtProtectKind_Invalid;
    PSUPDRVNTPROTECT    pNtStub = NULL;
    RTSpinlockAcquire(g_hNtProtectLock);
    if (pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed)
    {
        pNtStub = pNtProtect->u.pParent; /* weak reference. */
        if (pNtStub)
        {
            enmStub = pNtStub->enmProcessKind;
            if (enmStub == kSupDrvNtProtectKind_StubParent)
            {
                uint32_t cRefs = ASMAtomicIncU32(&pNtStub->cRefs);
                Assert(cRefs > 0 && cRefs < 1024); RT_NOREF_PV(cRefs);
            }
            else
                pNtStub = NULL;
        }
    }
    RTSpinlockRelease(g_hNtProtectLock);

    /*
     * We require the stub process to be present.
     */
    if (!pNtStub)
        return RTErrInfoSetF(pErrInfo, VERR_SUP_VP_STUB_NOT_FOUND, "Missing stub process (enmStub=%d).", enmStub);

    /*
     * Open the parent process and thread so we can check for debuggers and unwanted threads.
     */
    int rc;
    PEPROCESS pStubProcess;
    NTSTATUS rcNt = PsLookupProcessByProcessId(pNtStub->AvlCore.Key, &pStubProcess);
    if (NT_SUCCESS(rcNt))
    {
        HANDLE hStubProcess;
        rcNt = ObOpenObjectByPointer(pStubProcess, OBJ_KERNEL_HANDLE, NULL /*PassedAccessState*/,
                                     0 /*DesiredAccess*/, *PsProcessType, KernelMode, &hStubProcess);
        if (NT_SUCCESS(rcNt))
        {
            PETHREAD pStubThread;
            rcNt = PsLookupThreadByThreadId(pNtStub->hOpenTid, &pStubThread);
            if (NT_SUCCESS(rcNt))
            {
                HANDLE hStubThread;
                rcNt = ObOpenObjectByPointer(pStubThread, OBJ_KERNEL_HANDLE, NULL /*PassedAccessState*/,
                                             0 /*DesiredAccess*/, *PsThreadType, KernelMode, &hStubThread);
                if (NT_SUCCESS(rcNt))
                {
                    /*
                     * Do some simple sanity checking.
                     */
                    rc = supHardNtVpDebugger(hStubProcess, pErrInfo);
                    if (RT_SUCCESS(rc))
                        rc = supHardNtVpThread(hStubProcess, hStubThread, pErrInfo);

                    /* Clean up. */
                    rcNt = NtClose(hStubThread); AssertMsg(NT_SUCCESS(rcNt), ("%#x\n", rcNt));
                }
                else
                    rc = RTErrInfoSetF(pErrInfo, VERR_SUP_VP_STUB_THREAD_OPEN_ERROR,
                                       "Error opening stub thread %p (tid %p, pid %p): %#x",
                                       pStubThread, pNtStub->hOpenTid, pNtStub->AvlCore.Key, rcNt);
            }
            else
                rc = RTErrInfoSetF(pErrInfo, VERR_SUP_VP_STUB_THREAD_NOT_FOUND,
                                   "Failed to locate thread %p in %p: %#x", pNtStub->hOpenTid, pNtStub->AvlCore.Key, rcNt);
            rcNt = NtClose(hStubProcess); AssertMsg(NT_SUCCESS(rcNt), ("%#x\n", rcNt));
        }
        else
            rc = RTErrInfoSetF(pErrInfo, VERR_SUP_VP_STUB_OPEN_ERROR,
                               "Error opening stub process %p (pid %p): %#x", pStubProcess, pNtStub->AvlCore.Key, rcNt);
        ObDereferenceObject(pStubProcess);
    }
    else
        rc = RTErrInfoSetF(pErrInfo, VERR_SUP_VP_STUB_NOT_FOUND,
                           "Failed to locate stub process %p: %#x", pNtStub->AvlCore.Key, rcNt);

    supdrvNtProtectRelease(pNtStub);
    return rc;
}


/**
 * Worker for supdrvNtProtectVerifyProcess that verifies the handles to a VM
 * process and its thread.
 *
 * @returns VBox status code.
 * @param   pNtProtect          The NT protect structure for getting information
 *                              about special processes.
 * @param   pErrInfo            Where to return additional error details.
 */
static int supdrvNtProtectRestrictHandlesToProcessAndThread(PSUPDRVNTPROTECT pNtProtect, PRTERRINFO pErrInfo)
{
    /*
     * What to protect.
     */
    PEPROCESS   pProtectedProcess = PsGetCurrentProcess();
    HANDLE      hProtectedPid     = PsGetProcessId(pProtectedProcess);
    PETHREAD    pProtectedThread  = PsGetCurrentThread();
    AssertReturn(pNtProtect->AvlCore.Key == hProtectedPid, VERR_INTERNAL_ERROR_5);

    /*
     * Take a snapshot of all the handles in the system.
     * Note! The 32 bytes on the size of to counteract the allocation header
     *       that rtR0MemAllocEx slaps on everything.
     */
    uint32_t    cbBuf    = _256K - 32;
    uint8_t    *pbBuf    = (uint8_t *)RTMemAlloc(cbBuf);
    ULONG       cbNeeded = cbBuf;
    NTSTATUS rcNt = NtQuerySystemInformation(SystemExtendedHandleInformation, pbBuf, cbBuf, &cbNeeded);
    if (!NT_SUCCESS(rcNt))
    {
        while (   rcNt == STATUS_INFO_LENGTH_MISMATCH
               && cbNeeded > cbBuf
               && cbBuf <= 32U*_1M)
        {
            cbBuf = RT_ALIGN_32(cbNeeded + _4K, _64K) - 32;
            RTMemFree(pbBuf);
            pbBuf = (uint8_t *)RTMemAlloc(cbBuf);
            if (!pbBuf)
                return RTErrInfoSetF(pErrInfo, VERR_NO_MEMORY, "Error allocating %zu bytes for querying handles.", cbBuf);
            rcNt = NtQuerySystemInformation(SystemExtendedHandleInformation, pbBuf, cbBuf, &cbNeeded);
        }
        if (!NT_SUCCESS(rcNt))
        {
            RTMemFree(pbBuf);
            return RTErrInfoSetF(pErrInfo, RTErrConvertFromNtStatus(rcNt),
                                 "NtQuerySystemInformation/SystemExtendedHandleInformation failed: %#x\n", rcNt);
        }
    }

    /*
     * Walk the information and look for handles to the two objects we're protecting.
     */
    int rc = VINF_SUCCESS;
# ifdef VBOX_WITHOUT_DEBUGGER_CHECKS
    HANDLE   idLastDebugger = (HANDLE)~(uintptr_t)0;
# endif

    uint32_t cCsrssProcessHandles  = 0;
    uint32_t cSystemProcessHandles = 0;
    uint32_t cEvilProcessHandles   = 0;
    uint32_t cBenignProcessHandles = 0;

    uint32_t cCsrssThreadHandles   = 0;
    uint32_t cEvilThreadHandles    = 0;
    uint32_t cBenignThreadHandles  = 0;

    SYSTEM_HANDLE_INFORMATION_EX const *pInfo = (SYSTEM_HANDLE_INFORMATION_EX const *)pbBuf;
    ULONG_PTR i = pInfo->NumberOfHandles;
    AssertRelease(RT_OFFSETOF(SYSTEM_HANDLE_INFORMATION_EX, Handles[i]) == cbNeeded);
    while (i-- > 0)
    {
        const char *pszType;
        SYSTEM_HANDLE_ENTRY_INFO_EX const *pHandleInfo = &pInfo->Handles[i];
        if (pHandleInfo->Object == pProtectedProcess)
        {
            /* Handles within the protected process are fine. */
            if (   !(pHandleInfo->GrantedAccess & SUPDRV_NT_EVIL_PROCESS_RIGHTS)
                || pHandleInfo->UniqueProcessId == hProtectedPid)
            {
                cBenignProcessHandles++;
                continue;
            }

            /* CSRSS is allowed to have one evil process handle.
               See the special cases in the hook code. */
            if (   cCsrssProcessHandles < 1
                && pHandleInfo->UniqueProcessId == pNtProtect->hCsrssPid)
            {
                cCsrssProcessHandles++;
                continue;
            }

            /* The system process is allowed having two open process handle in
               Windows 8.1 and later, and one in earlier. This is probably a
               little overly paranoid as I think we can safely trust the
               system process... */
            if (   cSystemProcessHandles < (g_uNtVerCombined >= SUP_MAKE_NT_VER_SIMPLE(6, 3) ? UINT32_C(2) : UINT32_C(1))
                && pHandleInfo->UniqueProcessId == PsGetProcessId(PsInitialSystemProcess))
            {
                cSystemProcessHandles++;
                continue;
            }

            cEvilProcessHandles++;
            pszType = "process";
        }
        else if (pHandleInfo->Object == pProtectedThread)
        {
            /* Handles within the protected process is fine. */
            if (   !(pHandleInfo->GrantedAccess & SUPDRV_NT_EVIL_THREAD_RIGHTS)
                || pHandleInfo->UniqueProcessId == hProtectedPid)
            {
                cBenignThreadHandles++;
                continue;
            }

            /* CSRSS is allowed to have one evil handle to the primary thread
               for LPC purposes.  See the hook for special case. */
            if (   cCsrssThreadHandles < 1
                && pHandleInfo->UniqueProcessId == pNtProtect->hCsrssPid)
            {
                cCsrssThreadHandles++;
                continue;
            }

            cEvilThreadHandles++;
            pszType = "thread";
        }
        else
            continue;

# ifdef VBOX_WITHOUT_DEBUGGER_CHECKS
        /* Ignore whitelisted debuggers. */
        if (pHandleInfo->UniqueProcessId == idLastDebugger)
            continue;
        PEPROCESS pDbgProc;
        NTSTATUS rcNt = PsLookupProcessByProcessId(pHandleInfo->UniqueProcessId, &pDbgProc);
        if (NT_SUCCESS(rcNt))
        {
            bool fIsDebugger = supdrvNtProtectIsWhitelistedDebugger(pDbgProc);
            ObDereferenceObject(pDbgProc);
            if (fIsDebugger)
            {
                idLastDebugger = pHandleInfo->UniqueProcessId;
                continue;
            }
        }
# endif

        /* Found evil handle. Currently ignoring on pre-Vista. */
# ifndef VBOX_WITH_VISTA_NO_SP
        if (   g_uNtVerCombined >= SUP_NT_VER_VISTA
# else
        if (   g_uNtVerCombined >= SUP_MAKE_NT_VER_COMBINED(6, 0, 6001, 0, 0)
# endif
            || g_pfnObRegisterCallbacks)
        {
            LogRel(("vboxdrv: Found evil handle to budding VM process: pid=%p h=%p acc=%#x attr=%#x type=%s\n",
                    pHandleInfo->UniqueProcessId, pHandleInfo->HandleValue,
                    pHandleInfo->GrantedAccess, pHandleInfo->HandleAttributes, pszType));
            rc = RTErrInfoAddF(pErrInfo, VERR_SUPDRV_HARDENING_EVIL_HANDLE,
                               *pErrInfo->pszMsg
                               ? "\nFound evil handle to budding VM process: pid=%p h=%p acc=%#x attr=%#x type=%s"
                               : "Found evil handle to budding VM process: pid=%p h=%p acc=%#x attr=%#x type=%s",
                               pHandleInfo->UniqueProcessId, pHandleInfo->HandleValue,
                               pHandleInfo->GrantedAccess, pHandleInfo->HandleAttributes, pszType);

            /* Try add the process name. */
            PEPROCESS pOffendingProcess;
            rcNt = PsLookupProcessByProcessId(pHandleInfo->UniqueProcessId, &pOffendingProcess);
            if (NT_SUCCESS(rcNt))
            {
                const char *pszName = (const char *)PsGetProcessImageFileName(pOffendingProcess);
                if (pszName && *pszName)
                    rc = RTErrInfoAddF(pErrInfo, rc, " [%s]", pszName);

                ObDereferenceObject(pOffendingProcess);
            }
        }
    }

    RTMemFree(pbBuf);
    return rc;
}


/**
 * Checks if the current process checks out as a VM process stub.
 *
 * @returns VBox status code.
 * @param   pNtProtect          The NT protect structure.  This is upgraded to a
 *                              final protection kind (state) on success.
 */
static int supdrvNtProtectVerifyProcess(PSUPDRVNTPROTECT pNtProtect)
{
    AssertReturn(PsGetProcessId(PsGetCurrentProcess()) == pNtProtect->AvlCore.Key, VERR_INTERNAL_ERROR_3);

    /*
     * Do the verification.  The handle restriction checks are only preformed
     * on VM processes.
     */
    int rc = VINF_SUCCESS;
    PSUPDRVNTERRORINFO pErrorInfo = (PSUPDRVNTERRORINFO)RTMemAllocZ(sizeof(*pErrorInfo));
    if (RT_SUCCESS(rc))
    {
        pErrorInfo->hProcessId = PsGetCurrentProcessId();
        pErrorInfo->hThreadId  = PsGetCurrentThreadId();
        RTERRINFO ErrInfo;
        RTErrInfoInit(&ErrInfo, pErrorInfo->szErrorInfo, sizeof(pErrorInfo->szErrorInfo));

        if (pNtProtect->enmProcessKind >= kSupDrvNtProtectKind_VmProcessUnconfirmed)
            rc = supdrvNtProtectRestrictHandlesToProcessAndThread(pNtProtect, &ErrInfo);
        if (RT_SUCCESS(rc))
        {
            rc = supHardenedWinVerifyProcess(NtCurrentProcess(), NtCurrentThread(), SUPHARDNTVPKIND_VERIFY_ONLY, 0 /*fFlags*/,
                                             NULL /*pcFixes*/, &ErrInfo);
            if (RT_SUCCESS(rc) && pNtProtect->enmProcessKind >= kSupDrvNtProtectKind_VmProcessUnconfirmed)
                rc = supdrvNtProtectVerifyStubForVmProcess(pNtProtect, &ErrInfo);
        }
    }
    else
        rc = VERR_NO_MEMORY;

    /*
     * Upgrade and return.
     */
    HANDLE hOpenTid = PsGetCurrentThreadId();
    RTSpinlockAcquire(g_hNtProtectLock);

    /* Stub process verficiation is pretty much straight forward. */
    if (pNtProtect->enmProcessKind == kSupDrvNtProtectKind_StubUnverified)
    {
        pNtProtect->enmProcessKind = RT_SUCCESS(rc) ? kSupDrvNtProtectKind_StubSpawning : kSupDrvNtProtectKind_StubDead;
        pNtProtect->hOpenTid       = hOpenTid;
    }
    /* The VM process verification is a little bit more complicated
       because we need to drop the parent process reference as well. */
    else if (pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessUnconfirmed)
    {
        AssertRelease(pNtProtect->cRefs >= 2); /* Parent + Caller */
        PSUPDRVNTPROTECT pParent = pNtProtect->u.pParent;
        AssertRelease(pParent);
        AssertRelease(pParent->u.pParent == pNtProtect);
        AssertRelease(pParent->enmProcessKind == kSupDrvNtProtectKind_StubParent);
        pParent->u.pParent = NULL;

        pNtProtect->u.pParent = NULL;
        ASMAtomicDecU32(&pNtProtect->cRefs);

        if (RT_SUCCESS(rc))
        {
            pNtProtect->enmProcessKind = kSupDrvNtProtectKind_VmProcessConfirmed;
            pNtProtect->hOpenTid       = hOpenTid;
        }
        else
            pNtProtect->enmProcessKind = kSupDrvNtProtectKind_VmProcessDead;
    }

    /* Since the stub and VM processes are only supposed to have one thread,
       we're not supposed to be subject to any races from within the processes.

       There is a race between VM process verification and the stub process
       exiting, though.  We require the stub process to be alive until the new
       VM process has made it thru the validation.  So, when the stub
       terminates the notification handler will change the state of both stub
       and VM process to dead.

       Also, I'm not entirely certain where the process
       termination notification is triggered from, so that can theorically
       create a race in both cases.  */
    else
    {
        AssertReleaseMsg(   pNtProtect->enmProcessKind == kSupDrvNtProtectKind_StubDead
                         || pNtProtect->enmProcessKind == kSupDrvNtProtectKind_VmProcessDead,
                         ("enmProcessKind=%d rc=%Rrc\n", pNtProtect->enmProcessKind, rc));
        if (RT_SUCCESS(rc))
            rc = VERR_INVALID_STATE; /* There should be no races here. */
    }

    RTSpinlockRelease(g_hNtProtectLock);

    /*
     * Free error info on success, keep it on failure.
     */
    if (RT_SUCCESS(rc))
        RTMemFree(pErrorInfo);
    else if (pErrorInfo)
    {
        pErrorInfo->cchErrorInfo = (uint32_t)strlen(pErrorInfo->szErrorInfo);
        if (!pErrorInfo->cchErrorInfo)
            pErrorInfo->cchErrorInfo = (uint32_t)RTStrPrintf(pErrorInfo->szErrorInfo, sizeof(pErrorInfo->szErrorInfo),
                                                             "supdrvNtProtectVerifyProcess: rc=%d", rc);
        RTLogWriteDebugger(pErrorInfo->szErrorInfo, pErrorInfo->cchErrorInfo);

        int rc2 = RTSemMutexRequest(g_hErrorInfoLock, RT_INDEFINITE_WAIT);
        if (RT_SUCCESS(rc2))
        {
            pErrorInfo->uCreatedMsTs = RTTimeMilliTS();

            /* Free old entries. */
            PSUPDRVNTERRORINFO pCur;
            while (   (pCur = RTListGetFirst(&g_ErrorInfoHead, SUPDRVNTERRORINFO, ListEntry)) != NULL
                   && (int64_t)(pErrorInfo->uCreatedMsTs - pCur->uCreatedMsTs) > 60000 /*60sec*/)
            {
                RTListNodeRemove(&pCur->ListEntry);
                RTMemFree(pCur);
            }

            /* Insert our new entry. */
            RTListAppend(&g_ErrorInfoHead, &pErrorInfo->ListEntry);

            RTSemMutexRelease(g_hErrorInfoLock);
        }
        else
            RTMemFree(pErrorInfo);
    }

    return rc;
}


# ifndef VBOX_WITHOUT_DEBUGGER_CHECKS

/**
 * Checks if the current process is being debugged.
 * @return @c true if debugged, @c false if not.
 */
static bool supdrvNtIsDebuggerAttached(void)
{
    return PsIsProcessBeingDebugged(PsGetCurrentProcess()) != FALSE;
}

# endif /* !VBOX_WITHOUT_DEBUGGER_CHECKS */


/**
 * Terminates the hardening bits.
 */
static void supdrvNtProtectTerm(void)
{
    /*
     * Stop intercepting process and thread handle creation calls.
     */
    if (g_pvObCallbacksCookie)
    {
        g_pfnObUnRegisterCallbacks(g_pvObCallbacksCookie);
        g_pvObCallbacksCookie = NULL;
    }

    /*
     * Stop intercepting process creation and termination notifications.
     */
    NTSTATUS rcNt;
    if (g_pfnPsSetCreateProcessNotifyRoutineEx)
        rcNt = g_pfnPsSetCreateProcessNotifyRoutineEx(supdrvNtProtectCallback_ProcessCreateNotifyEx, TRUE /*fRemove*/);
    else
        rcNt = PsSetCreateProcessNotifyRoutine(supdrvNtProtectCallback_ProcessCreateNotify, TRUE /*fRemove*/);
    AssertMsg(NT_SUCCESS(rcNt), ("rcNt=%#x\n", rcNt));

    Assert(g_NtProtectTree == NULL);

    /*
     * Clean up globals.
     */
    RTSpinlockDestroy(g_hNtProtectLock);
    g_NtProtectTree = NIL_RTSPINLOCK;

    RTSemMutexDestroy(g_hErrorInfoLock);
    g_hErrorInfoLock = NIL_RTSEMMUTEX;

    PSUPDRVNTERRORINFO pCur;
    while ((pCur = RTListGetFirst(&g_ErrorInfoHead, SUPDRVNTERRORINFO, ListEntry)) != NULL)
    {
        RTListNodeRemove(&pCur->ListEntry);
        RTMemFree(pCur);
    }

    supHardenedWinTermImageVerifier();
}

# ifdef RT_ARCH_X86
DECLASM(void) supdrvNtQueryVirtualMemory_0xAF(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB0(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB1(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB2(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB3(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB4(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB5(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB6(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB7(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB8(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xB9(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xBA(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xBB(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xBC(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xBD(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0xBE(void);
# elif defined(RT_ARCH_AMD64)
DECLASM(void) supdrvNtQueryVirtualMemory_0x1F(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0x20(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0x21(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0x22(void);
DECLASM(void) supdrvNtQueryVirtualMemory_0x23(void);
extern "C" NTSYSAPI NTSTATUS NTAPI ZwRequestWaitReplyPort(HANDLE, PVOID, PVOID);
# endif


/**
 * Initalizes the hardening bits.
 *
 * @returns NT status code.
 */
static NTSTATUS supdrvNtProtectInit(void)
{
    /*
     * Initialize the globals.
     */

    /* The NT version. */
    ULONG uMajor, uMinor, uBuild;
    PsGetVersion(&uMajor, &uMinor, &uBuild, NULL);
    g_uNtVerCombined = SUP_MAKE_NT_VER_COMBINED(uMajor, uMinor, uBuild, 0, 0);

    /* Resolve methods we want but isn't available everywhere. */
    UNICODE_STRING RoutineName;

    RtlInitUnicodeString(&RoutineName, L"ObGetObjectType");
    g_pfnObGetObjectType = (PFNOBGETOBJECTTYPE)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"ObRegisterCallbacks");
    g_pfnObRegisterCallbacks   = (PFNOBREGISTERCALLBACKS)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"ObUnRegisterCallbacks");
    g_pfnObUnRegisterCallbacks = (PFNOBUNREGISTERCALLBACKS)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"PsSetCreateProcessNotifyRoutineEx");
    g_pfnPsSetCreateProcessNotifyRoutineEx = (PFNPSSETCREATEPROCESSNOTIFYROUTINEEX)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"PsReferenceProcessFilePointer");
    g_pfnPsReferenceProcessFilePointer = (PFNPSREFERENCEPROCESSFILEPOINTER)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"PsIsProtectedProcessLight");
    g_pfnPsIsProtectedProcessLight = (PFNPSISPROTECTEDPROCESSLIGHT)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"ZwAlpcCreatePort");
    g_pfnZwAlpcCreatePort = (PFNZWALPCCREATEPORT)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"ZwQueryVirtualMemory"); /* Yes, using Zw version here. */
    g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)MmGetSystemRoutineAddress(&RoutineName);
    if (!g_pfnNtQueryVirtualMemory && g_uNtVerCombined < SUP_NT_VER_VISTA)
    {
        /* XP & W2K3 doesn't have this function exported, so we've cooked up a
           few alternative in the assembly helper file that uses the code in
           ZwReadFile with a different eax value.  We figure the syscall number
           by inspecting ZwQueryVolumeInformationFile as it's the next number. */
# ifdef RT_ARCH_X86
        uint8_t const *pbCode = (uint8_t const *)(uintptr_t)ZwQueryVolumeInformationFile;
        if (*pbCode == 0xb8) /* mov eax, dword */
            switch (*(uint32_t const *)&pbCode[1])
            {
                case 0xb0: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xAF; break; /* just in case */
                case 0xb1: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB0; break; /* just in case */
                case 0xb2: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB1; break; /* just in case */
                case 0xb3: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB2; break; /* XP SP3 */
                case 0xb4: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB2; break; /* just in case */
                case 0xb5: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB3; break; /* just in case */
                case 0xb6: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB4; break; /* just in case */
                case 0xb7: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB5; break; /* just in case */
                case 0xb8: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB6; break; /* just in case */
                case 0xb9: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB7; break; /* just in case */
                case 0xba: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xB8; break; /* just in case */
                case 0xbb: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xBA; break; /* W2K3 R2 SP2 */
                case 0xbc: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xBB; break; /* just in case */
                case 0xbd: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xBC; break; /* just in case */
                case 0xbe: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xBD; break; /* just in case */
                case 0xbf: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0xBE; break; /* just in case */
            }
# elif defined(RT_ARCH_AMD64)
        uint8_t const *pbCode = (uint8_t const *)(uintptr_t)ZwRequestWaitReplyPort;
        if (   pbCode[ 0] == 0x48   /* mov rax, rsp */
            && pbCode[ 1] == 0x8b
            && pbCode[ 2] == 0xc4
            && pbCode[ 3] == 0xfa   /* cli */
            && pbCode[ 4] == 0x48   /* sub rsp, 10h */
            && pbCode[ 5] == 0x83
            && pbCode[ 6] == 0xec
            && pbCode[ 7] == 0x10
            && pbCode[ 8] == 0x50   /* push rax */
            && pbCode[ 9] == 0x9c   /* pushfq */
            && pbCode[10] == 0x6a   /* push 10 */
            && pbCode[11] == 0x10
            && pbCode[12] == 0x48   /* lea rax, [nt!KiServiceLinkage] */
            && pbCode[13] == 0x8d
            && pbCode[14] == 0x05
            && pbCode[19] == 0x50   /* push rax */
            && pbCode[20] == 0xb8   /* mov eax,1fh <- the syscall no. */
            /*&& pbCode[21] == 0x1f*/
            && pbCode[22] == 0x00
            && pbCode[23] == 0x00
            && pbCode[24] == 0x00
            && pbCode[25] == 0xe9   /* jmp KiServiceInternal */
           )
        {
            uint8_t const *pbKiServiceInternal = &pbCode[30] + *(int32_t const *)&pbCode[26];
            uint8_t const *pbKiServiceLinkage  = &pbCode[19] + *(int32_t const *)&pbCode[15];
            if (*pbKiServiceLinkage == 0xc3)
            {
                g_pfnKiServiceInternal = (PFNRT)pbKiServiceInternal;
                g_pfnKiServiceLinkage  = (PFNRT)pbKiServiceLinkage;
                switch (pbCode[21])
                {
                    case 0x1e: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0x1F; break;
                    case 0x1f: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0x20; break;
                    case 0x20: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0x21; break;
                    case 0x21: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0x22; break;
                    case 0x22: g_pfnNtQueryVirtualMemory = (PFNNTQUERYVIRTUALMEMORY)supdrvNtQueryVirtualMemory_0x23; break;
                }
            }
        }
# endif
    }
    if (!g_pfnNtQueryVirtualMemory)
    {
        LogRel(("vboxdrv: Cannot locate ZwQueryVirtualMemory in ntoskrnl, nor were we able to cook up a replacement.\n"));
        return STATUS_PROCEDURE_NOT_FOUND;
    }

# ifdef VBOX_STRICT
    if (   g_uNtVerCombined >= SUP_NT_VER_W70
        && (   g_pfnObGetObjectType == NULL
            || g_pfnZwAlpcCreatePort == NULL) )
    {
        LogRel(("vboxdrv: g_pfnObGetObjectType=%p g_pfnZwAlpcCreatePort=%p.\n", g_pfnObGetObjectType, g_pfnZwAlpcCreatePort));
        return STATUS_PROCEDURE_NOT_FOUND;
    }
# endif

    /* LPC object type. */
    g_pAlpcPortObjectType1 = *LpcPortObjectType;

    /* The spinlock protecting our structures. */
    int rc = RTSpinlockCreate(&g_hNtProtectLock, RTSPINLOCK_FLAGS_INTERRUPT_UNSAFE, "NtProtectLock");
    if (RT_FAILURE(rc))
        return VBoxDrvNtErr2NtStatus(rc);
    g_NtProtectTree = NULL;

    NTSTATUS rcNt;

    /* The mutex protecting the error information. */
    RTListInit(&g_ErrorInfoHead);
    rc = RTSemMutexCreate(&g_hErrorInfoLock);
    if (RT_SUCCESS(rc))
    {
        /* Image stuff + certificates. */
        rc = supHardenedWinInitImageVerifier(NULL);
        if (RT_SUCCESS(rc))
        {
            /*
             * Intercept process creation and termination.
             */
            if (g_pfnPsSetCreateProcessNotifyRoutineEx)
                rcNt = g_pfnPsSetCreateProcessNotifyRoutineEx(supdrvNtProtectCallback_ProcessCreateNotifyEx, FALSE /*fRemove*/);
            else
                rcNt = PsSetCreateProcessNotifyRoutine(supdrvNtProtectCallback_ProcessCreateNotify, FALSE /*fRemove*/);
            if (NT_SUCCESS(rcNt))
            {
                /*
                 * Intercept process and thread handle creation calls.
                 * The preferred method is only available on Vista SP1+.
                 */
                if (g_pfnObRegisterCallbacks && g_pfnObUnRegisterCallbacks)
                {
                    static OB_OPERATION_REGISTRATION s_aObOperations[] =
                    {
                        {
                            0, /* PsProcessType - imported, need runtime init, better do it explicitly. */
                            OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
                            supdrvNtProtectCallback_ProcessHandlePre,
                            supdrvNtProtectCallback_ProcessHandlePost,
                        },
                        {
                            0, /* PsThreadType - imported, need runtime init, better do it explicitly. */
                            OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
                            supdrvNtProtectCallback_ThreadHandlePre,
                            supdrvNtProtectCallback_ThreadHandlePost,
                        },
                    };
                    s_aObOperations[0].ObjectType = PsProcessType;
                    s_aObOperations[1].ObjectType = PsThreadType;

                    static OB_CALLBACK_REGISTRATION s_ObCallbackReg =
                    {
                        /* .Version                     = */ OB_FLT_REGISTRATION_VERSION,
                        /* .OperationRegistrationCount  = */ RT_ELEMENTS(s_aObOperations),
                        /* .Altitude.Length             = */ 0,
                        /* .Altitude.MaximumLength      = */ 0,
                        /* .Altitude.Buffer             = */ NULL,
                        /* .RegistrationContext         = */ NULL,
                        /* .OperationRegistration       = */ &s_aObOperations[0]
                    };
                    static WCHAR const *s_apwszAltitudes[] = /** @todo get a valid number */
                    {
                         L"48596.98940",  L"46935.19485",  L"49739.39704",  L"40334.74976",
                         L"66667.98940",  L"69888.19485",  L"69889.39704",  L"60364.74976",
                         L"85780.98940",  L"88978.19485",  L"89939.39704",  L"80320.74976",
                         L"329879.98940", L"326787.19485", L"328915.39704", L"320314.74976",
                    };

                    rcNt = STATUS_FLT_INSTANCE_ALTITUDE_COLLISION;
                    for (uint32_t i = 0; i < RT_ELEMENTS(s_apwszAltitudes) && rcNt == STATUS_FLT_INSTANCE_ALTITUDE_COLLISION; i++)
                    {
                        s_ObCallbackReg.Altitude.Buffer = (WCHAR *)s_apwszAltitudes[i];
                        s_ObCallbackReg.Altitude.Length = (uint16_t)RTUtf16Len(s_apwszAltitudes[i]) * sizeof(WCHAR);
                        s_ObCallbackReg.Altitude.MaximumLength = s_ObCallbackReg.Altitude.Length + sizeof(WCHAR);

                        rcNt = g_pfnObRegisterCallbacks(&s_ObCallbackReg, &g_pvObCallbacksCookie);
                        if (NT_SUCCESS(rcNt))
                        {
                            /*
                             * Happy ending.
                             */
                            return STATUS_SUCCESS;
                        }
                    }
                    LogRel(("vboxdrv: ObRegisterCallbacks failed with rcNt=%#x\n", rcNt));
                    g_pvObCallbacksCookie = NULL;
                }
                else
                {
                    /*
                     * For the time being, we do not implement extra process
                     * protection on pre-Vista-SP1 systems as they are lacking
                     * necessary KPIs.  XP is end of life, we do not wish to
                     * spend more time on it, so we don't put up a fuss there.
                     * Vista users without SP1 can install SP1 (or later), darn it,
                     * so refuse to load.
                     */
                    /** @todo Hack up an XP solution - will require hooking kernel APIs or doing bad
                     *        stuff to a couple of object types. */
# ifndef VBOX_WITH_VISTA_NO_SP
                    if (g_uNtVerCombined >= SUP_NT_VER_VISTA)
# else
                    if (g_uNtVerCombined >= SUP_MAKE_NT_VER_COMBINED(6, 0, 6001, 0, 0))
# endif
                    {
                        DbgPrint("vboxdrv: ObRegisterCallbacks was not found. Please make sure you got the latest updates and service packs installed\n");
                        rcNt = STATUS_SXS_VERSION_CONFLICT;
                    }
                    else
                    {
                        Log(("vboxdrv: ObRegisterCallbacks was not found; ignored pre-Vista\n"));
                        return rcNt = STATUS_SUCCESS;
                    }
                    g_pvObCallbacksCookie = NULL;
                }

                /*
                 * Drop process create/term notifications.
                 */
                if (g_pfnPsSetCreateProcessNotifyRoutineEx)
                    g_pfnPsSetCreateProcessNotifyRoutineEx(supdrvNtProtectCallback_ProcessCreateNotifyEx, TRUE /*fRemove*/);
                else
                    PsSetCreateProcessNotifyRoutine(supdrvNtProtectCallback_ProcessCreateNotify, TRUE /*fRemove*/);
            }
            else
                LogRel(("vboxdrv: PsSetCreateProcessNotifyRoutine%s failed with rcNt=%#x\n",
                        g_pfnPsSetCreateProcessNotifyRoutineEx ? "Ex" : "", rcNt));
            supHardenedWinTermImageVerifier();
        }
        else
            rcNt = VBoxDrvNtErr2NtStatus(rc);

        RTSemMutexDestroy(g_hErrorInfoLock);
        g_hErrorInfoLock = NIL_RTSEMMUTEX;
    }
    else
        rcNt = VBoxDrvNtErr2NtStatus(rc);

    RTSpinlockDestroy(g_hNtProtectLock);
    g_NtProtectTree = NIL_RTSPINLOCK;
    return rcNt;
}

#endif /* VBOX_WITH_HARDENING */

