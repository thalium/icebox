/* $Id: VMMDev.cpp $ */
/** @file
 * VMMDev - Guest <-> VMM/Host communication device.
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
 */

/** @page pg_vmmdev   The VMM Device.
 *
 * The VMM device is a custom hardware device emulation for communicating with
 * the guest additions.
 *
 * Whenever host wants to inform guest about something an IRQ notification will
 * be raised.
 *
 * VMMDev PDM interface will contain the guest notification method.
 *
 * There is a 32 bit event mask which will be read by guest on an interrupt.  A
 * non zero bit in the mask means that the specific event occurred and requires
 * processing on guest side.
 *
 * After reading the event mask guest must issue a generic request
 * AcknowlegdeEvents.
 *
 * IRQ line is set to 1 (request) if there are unprocessed events, that is the
 * event mask is not zero.
 *
 * After receiving an interrupt and checking event mask, the guest must process
 * events using the event specific mechanism.
 *
 * That is if mouse capabilities were changed, guest will use
 * VMMDev_GetMouseStatus generic request.
 *
 * Event mask is only a set of flags indicating that guest must proceed with a
 * procedure.
 *
 * Unsupported events are therefore ignored. The guest additions must inform
 * host which events they want to receive, to avoid unnecessary IRQ processing.
 * By default no events are signalled to guest.
 *
 * This seems to be fast method. It requires only one context switch for an
 * event processing.
 *
 *
 * @section sec_vmmdev_heartbeat    Heartbeat
 *
 * The heartbeat is a feature to monitor whether the guest OS is hung or not.
 *
 * The main kernel component of the guest additions, VBoxGuest, sets up a timer
 * at a frequency returned by VMMDevReq_HeartbeatConfigure
 * (VMMDevReqHeartbeat::cNsInterval, VMMDEV::cNsHeartbeatInterval) and performs
 * a VMMDevReq_GuestHeartbeat request every time the timer ticks.
 *
 * The host side (VMMDev) arms a timer with a more distant deadline
 * (VMMDEV::cNsHeartbeatTimeout), twice cNsHeartbeatInterval by default.  Each
 * time a VMMDevReq_GuestHeartbeat request comes in, the timer is rearmed with
 * the same relative deadline.  So, as long as VMMDevReq_GuestHeartbeat comes
 * when they should, the host timer will never fire.
 *
 * When the timer fires, we consider the guest as hung / flatlined / dead.
 * Currently we only LogRel that, but it's easy to extend this with an event in
 * Main API.
 *
 * Should the guest reawaken at some later point, we LogRel that event and
 * continue as normal.  Again something which would merit an API event.
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
/* Enable dev_vmm Log3 statements to get IRQ-related logging. */
#define LOG_GROUP LOG_GROUP_DEV_VMM
#include <VBoxVideo.h>  /* For VBVA definitions. */
#include <VBox/VMMDev.h>
#include <VBox/vmm/mm.h>
#include <VBox/log.h>
#include <VBox/param.h>
#include <iprt/path.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <VBox/vmm/pgm.h>
#include <VBox/err.h>
#include <VBox/vmm/vm.h> /* for VM_IS_EMT */
#include <VBox/dbg.h>
#include <VBox/version.h>

#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>
#include <iprt/buildconfig.h>
#include <iprt/string.h>
#include <iprt/time.h>
#ifndef IN_RC
# include <iprt/mem.h>
#endif
#ifdef IN_RING3
# include <iprt/uuid.h>
#endif

#include "VMMDevState.h"
#ifdef VBOX_WITH_HGCM
# include "VMMDevHGCM.h"
#endif
#ifndef VBOX_WITHOUT_TESTING_FEATURES
# include "VMMDevTesting.h"
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define VMMDEV_INTERFACE_VERSION_IS_1_03(s) \
    (   RT_HIWORD((s)->guestInfo.interfaceVersion) == 1 \
     && RT_LOWORD((s)->guestInfo.interfaceVersion) == 3 )

#define VMMDEV_INTERFACE_VERSION_IS_OK(additionsVersion) \
      (   RT_HIWORD(additionsVersion) == RT_HIWORD(VMMDEV_VERSION) \
       && RT_LOWORD(additionsVersion) <= RT_LOWORD(VMMDEV_VERSION) )

#define VMMDEV_INTERFACE_VERSION_IS_OLD(additionsVersion) \
      (   (RT_HIWORD(additionsVersion) < RT_HIWORD(VMMDEV_VERSION) \
       || (   RT_HIWORD(additionsVersion) == RT_HIWORD(VMMDEV_VERSION) \
           && RT_LOWORD(additionsVersion) <= RT_LOWORD(VMMDEV_VERSION) ) )

#define VMMDEV_INTERFACE_VERSION_IS_TOO_OLD(additionsVersion) \
      ( RT_HIWORD(additionsVersion) < RT_HIWORD(VMMDEV_VERSION) )

#define VMMDEV_INTERFACE_VERSION_IS_NEW(additionsVersion) \
      (   RT_HIWORD(additionsVersion) > RT_HIWORD(VMMDEV_VERSION) \
       || (   RT_HIWORD(additionsVersion) == RT_HIWORD(VMMDEV_VERSION) \
           && RT_LOWORD(additionsVersion) >  RT_LOWORD(VMMDEV_VERSION) ) )

/** Default interval in nanoseconds between guest heartbeats.
 *  Used when no HeartbeatInterval is set in CFGM and for setting
 *  HB check timer if the guest's heartbeat frequency is less than 1Hz. */
#define VMMDEV_HEARTBEAT_DEFAULT_INTERVAL                       (2U*RT_NS_1SEC_64)


#ifndef VBOX_DEVICE_STRUCT_TESTCASE

/* -=-=-=-=- Misc Helpers -=-=-=-=- */

/**
 * Log information about the Guest Additions.
 *
 * @param   pGuestInfo  The information we've got from the Guest Additions driver.
 */
static void vmmdevLogGuestOsInfo(VBoxGuestInfo *pGuestInfo)
{
    const char *pszOs;
    switch (pGuestInfo->osType & ~VBOXOSTYPE_x64)
    {
        case VBOXOSTYPE_DOS:                              pszOs = "DOS";            break;
        case VBOXOSTYPE_Win31:                            pszOs = "Windows 3.1";    break;
        case VBOXOSTYPE_Win9x:                            pszOs = "Windows 9x";     break;
        case VBOXOSTYPE_Win95:                            pszOs = "Windows 95";     break;
        case VBOXOSTYPE_Win98:                            pszOs = "Windows 98";     break;
        case VBOXOSTYPE_WinMe:                            pszOs = "Windows Me";     break;
        case VBOXOSTYPE_WinNT:                            pszOs = "Windows NT";     break;
        case VBOXOSTYPE_WinNT4:                           pszOs = "Windows NT4";    break;
        case VBOXOSTYPE_Win2k:                            pszOs = "Windows 2k";     break;
        case VBOXOSTYPE_WinXP:                            pszOs = "Windows XP";     break;
        case VBOXOSTYPE_Win2k3:                           pszOs = "Windows 2k3";    break;
        case VBOXOSTYPE_WinVista:                         pszOs = "Windows Vista";  break;
        case VBOXOSTYPE_Win2k8:                           pszOs = "Windows 2k8";    break;
        case VBOXOSTYPE_Win7:                             pszOs = "Windows 7";      break;
        case VBOXOSTYPE_Win8:                             pszOs = "Windows 8";      break;
        case VBOXOSTYPE_Win2k12_x64 & ~VBOXOSTYPE_x64:    pszOs = "Windows 2k12";   break;
        case VBOXOSTYPE_Win81:                            pszOs = "Windows 8.1";    break;
        case VBOXOSTYPE_Win10:                            pszOs = "Windows 10";     break;
        case VBOXOSTYPE_Win2k16_x64 & ~VBOXOSTYPE_x64:    pszOs = "Windows 2k16";   break;
        case VBOXOSTYPE_OS2:                              pszOs = "OS/2";           break;
        case VBOXOSTYPE_OS2Warp3:                         pszOs = "OS/2 Warp 3";    break;
        case VBOXOSTYPE_OS2Warp4:                         pszOs = "OS/2 Warp 4";    break;
        case VBOXOSTYPE_OS2Warp45:                        pszOs = "OS/2 Warp 4.5";  break;
        case VBOXOSTYPE_ECS:                              pszOs = "OS/2 ECS";       break;
        case VBOXOSTYPE_OS21x:                            pszOs = "OS/2 2.1x";      break;
        case VBOXOSTYPE_Linux:                            pszOs = "Linux";          break;
        case VBOXOSTYPE_Linux22:                          pszOs = "Linux 2.2";      break;
        case VBOXOSTYPE_Linux24:                          pszOs = "Linux 2.4";      break;
        case VBOXOSTYPE_Linux26:                          pszOs = "Linux >= 2.6";   break;
        case VBOXOSTYPE_ArchLinux:                        pszOs = "ArchLinux";      break;
        case VBOXOSTYPE_Debian:                           pszOs = "Debian";         break;
        case VBOXOSTYPE_OpenSUSE:                         pszOs = "openSUSE";       break;
        case VBOXOSTYPE_FedoraCore:                       pszOs = "Fedora";         break;
        case VBOXOSTYPE_Gentoo:                           pszOs = "Gentoo";         break;
        case VBOXOSTYPE_Mandriva:                         pszOs = "Mandriva";       break;
        case VBOXOSTYPE_RedHat:                           pszOs = "RedHat";         break;
        case VBOXOSTYPE_Turbolinux:                       pszOs = "TurboLinux";     break;
        case VBOXOSTYPE_Ubuntu:                           pszOs = "Ubuntu";         break;
        case VBOXOSTYPE_Xandros:                          pszOs = "Xandros";        break;
        case VBOXOSTYPE_Oracle:                           pszOs = "Oracle Linux";   break;
        case VBOXOSTYPE_FreeBSD:                          pszOs = "FreeBSD";        break;
        case VBOXOSTYPE_OpenBSD:                          pszOs = "OpenBSD";        break;
        case VBOXOSTYPE_NetBSD:                           pszOs = "NetBSD";         break;
        case VBOXOSTYPE_Netware:                          pszOs = "Netware";        break;
        case VBOXOSTYPE_Solaris:                          pszOs = "Solaris";        break;
        case VBOXOSTYPE_OpenSolaris:                      pszOs = "OpenSolaris";    break;
        case VBOXOSTYPE_Solaris11_x64 & ~VBOXOSTYPE_x64:  pszOs = "Solaris 11";     break;
        case VBOXOSTYPE_MacOS:                            pszOs = "Mac OS X";       break;
        case VBOXOSTYPE_MacOS106:                         pszOs = "Mac OS X 10.6";  break;
        case VBOXOSTYPE_MacOS107_x64 & ~VBOXOSTYPE_x64:   pszOs = "Mac OS X 10.7";  break;
        case VBOXOSTYPE_MacOS108_x64 & ~VBOXOSTYPE_x64:   pszOs = "Mac OS X 10.8";  break;
        case VBOXOSTYPE_MacOS109_x64 & ~VBOXOSTYPE_x64:   pszOs = "Mac OS X 10.9";  break;
        case VBOXOSTYPE_MacOS1010_x64 & ~VBOXOSTYPE_x64:  pszOs = "Mac OS X 10.10"; break;
        case VBOXOSTYPE_MacOS1011_x64 & ~VBOXOSTYPE_x64:  pszOs = "Mac OS X 10.11"; break;
        case VBOXOSTYPE_MacOS1012_x64 & ~VBOXOSTYPE_x64:  pszOs = "macOS 10.12";    break;
        case VBOXOSTYPE_MacOS1013_x64 & ~VBOXOSTYPE_x64:  pszOs = "macOS 10.13";    break;
        case VBOXOSTYPE_Haiku:                            pszOs = "Haiku";          break;
        default:                                          pszOs = "unknown";        break;
    }
    LogRel(("VMMDev: Guest Additions information report: Interface = 0x%08X osType = 0x%08X (%s, %u-bit)\n",
            pGuestInfo->interfaceVersion, pGuestInfo->osType, pszOs,
            pGuestInfo->osType & VBOXOSTYPE_x64 ? 64 : 32));
}

/**
 * Sets the IRQ (raise it or lower it) for 1.03 additions.
 *
 * @param   pThis       The VMMDev state.
 * @thread  Any.
 * @remarks Must be called owning the critical section.
 */
static void vmmdevSetIRQ_Legacy(PVMMDEV pThis)
{
    if (pThis->fu32AdditionsOk)
    {
        /* Filter unsupported events */
        uint32_t fEvents = pThis->u32HostEventFlags & pThis->pVMMDevRAMR3->V.V1_03.u32GuestEventMask;

        Log(("vmmdevSetIRQ: fEvents=%#010x, u32HostEventFlags=%#010x, u32GuestEventMask=%#010x.\n",
             fEvents, pThis->u32HostEventFlags, pThis->pVMMDevRAMR3->V.V1_03.u32GuestEventMask));

        /* Move event flags to VMMDev RAM */
        pThis->pVMMDevRAMR3->V.V1_03.u32HostEvents = fEvents;

        uint32_t uIRQLevel = 0;
        if (fEvents)
        {
            /* Clear host flags which will be delivered to guest. */
            pThis->u32HostEventFlags &= ~fEvents;
            Log(("vmmdevSetIRQ: u32HostEventFlags=%#010x\n", pThis->u32HostEventFlags));
            uIRQLevel = 1;
        }

        /* Set IRQ level for pin 0 (see NoWait comment in vmmdevMaybeSetIRQ). */
        /** @todo make IRQ pin configurable, at least a symbolic constant */
        PDMDevHlpPCISetIrqNoWait(pThis->pDevIns, 0, uIRQLevel);
        Log(("vmmdevSetIRQ: IRQ set %d\n", uIRQLevel));
    }
    else
        Log(("vmmdevSetIRQ: IRQ is not generated, guest has not yet reported to us.\n"));
}

/**
 * Sets the IRQ if there are events to be delivered.
 *
 * @param   pThis       The VMMDev state.
 * @thread  Any.
 * @remarks Must be called owning the critical section.
 */
static void vmmdevMaybeSetIRQ(PVMMDEV pThis)
{
    Log3(("vmmdevMaybeSetIRQ: u32HostEventFlags=%#010x, u32GuestFilterMask=%#010x.\n",
          pThis->u32HostEventFlags, pThis->u32GuestFilterMask));

    if (pThis->u32HostEventFlags & pThis->u32GuestFilterMask)
    {
        /*
         * Note! No need to wait for the IRQs to be set (if we're not luck
         *       with the locks, etc).  It is a notification about something,
         *       which has already happened.
         */
        pThis->pVMMDevRAMR3->V.V1_04.fHaveEvents = true;
        PDMDevHlpPCISetIrqNoWait(pThis->pDevIns, 0, 1);
        Log3(("vmmdevMaybeSetIRQ: IRQ set.\n"));
    }
}

/**
 * Notifies the guest about new events (@a fAddEvents).
 *
 * @param   pThis           The VMMDev state.
 * @param   fAddEvents      New events to add.
 * @thread  Any.
 * @remarks Must be called owning the critical section.
 */
static void vmmdevNotifyGuestWorker(PVMMDEV pThis, uint32_t fAddEvents)
{
    Log3(("vmmdevNotifyGuestWorker: fAddEvents=%#010x.\n", fAddEvents));
    Assert(PDMCritSectIsOwner(&pThis->CritSect));

    if (!VMMDEV_INTERFACE_VERSION_IS_1_03(pThis))
    {
        Log3(("vmmdevNotifyGuestWorker: New additions detected.\n"));

        if (pThis->fu32AdditionsOk)
        {
            const bool fHadEvents = (pThis->u32HostEventFlags & pThis->u32GuestFilterMask) != 0;

            Log3(("vmmdevNotifyGuestWorker: fHadEvents=%d, u32HostEventFlags=%#010x, u32GuestFilterMask=%#010x.\n",
                  fHadEvents, pThis->u32HostEventFlags, pThis->u32GuestFilterMask));

            pThis->u32HostEventFlags |= fAddEvents;

            if (!fHadEvents)
                vmmdevMaybeSetIRQ(pThis);
        }
        else
        {
            pThis->u32HostEventFlags |= fAddEvents;
            Log(("vmmdevNotifyGuestWorker: IRQ is not generated, guest has not yet reported to us.\n"));
        }
    }
    else
    {
        Log3(("vmmdevNotifyGuestWorker: Old additions detected.\n"));

        pThis->u32HostEventFlags |= fAddEvents;
        vmmdevSetIRQ_Legacy(pThis);
    }
}



/* -=-=-=-=- Interfaces shared with VMMDevHGCM.cpp  -=-=-=-=- */

/**
 * Notifies the guest about new events (@a fAddEvents).
 *
 * This is used by VMMDev.cpp as well as VMMDevHGCM.cpp.
 *
 * @param   pThis           The VMMDev state.
 * @param   fAddEvents      New events to add.
 * @thread  Any.
 */
void VMMDevNotifyGuest(PVMMDEV pThis, uint32_t fAddEvents)
{
    Log3(("VMMDevNotifyGuest: fAddEvents=%#010x\n", fAddEvents));

    /*
     * Only notify the VM when it's running.
     */
    VMSTATE enmVMState = PDMDevHlpVMState(pThis->pDevIns);
/** @todo r=bird: Shouldn't there be more states here?  Wouldn't we drop
 *        notifications now when we're in the process of suspending or
 *        similar? */
    if (   enmVMState == VMSTATE_RUNNING
        || enmVMState == VMSTATE_RUNNING_LS)
    {
        PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);
        vmmdevNotifyGuestWorker(pThis, fAddEvents);
        PDMCritSectLeave(&pThis->CritSect);
    }
}

/**
 * Code shared by VMMDevReq_CtlGuestFilterMask and HGCM for controlling the
 * events the guest are interested in.
 *
 * @param   pThis           The VMMDev state.
 * @param   fOrMask         Events to add (VMMDEV_EVENT_XXX). Pass 0 for no
 *                          change.
 * @param   fNotMask        Events to remove (VMMDEV_EVENT_XXX). Pass 0 for no
 *                          change.
 *
 * @remarks When HGCM will automatically enable VMMDEV_EVENT_HGCM when the guest
 *          starts submitting HGCM requests.  Otherwise, the events are
 *          controlled by the guest.
 */
void VMMDevCtlSetGuestFilterMask(PVMMDEV pThis, uint32_t fOrMask, uint32_t fNotMask)
{
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    const bool fHadEvents = (pThis->u32HostEventFlags & pThis->u32GuestFilterMask) != 0;

    Log(("VMMDevCtlSetGuestFilterMask: fOrMask=%#010x, u32NotMask=%#010x, fHadEvents=%d.\n", fOrMask, fNotMask, fHadEvents));
    if (fHadEvents)
    {
        if (!pThis->fNewGuestFilterMask)
            pThis->u32NewGuestFilterMask = pThis->u32GuestFilterMask;

        pThis->u32NewGuestFilterMask |= fOrMask;
        pThis->u32NewGuestFilterMask &= ~fNotMask;
        pThis->fNewGuestFilterMask = true;
    }
    else
    {
        pThis->u32GuestFilterMask |= fOrMask;
        pThis->u32GuestFilterMask &= ~fNotMask;
        vmmdevMaybeSetIRQ(pThis);
    }

    PDMCritSectLeave(&pThis->CritSect);
}



/* -=-=-=-=- Request processing functions. -=-=-=-=- */

/**
 * Handles VMMDevReq_ReportGuestInfo.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pRequestHeader  The header of the request to handle.
 */
static int vmmdevReqHandler_ReportGuestInfo(PVMMDEV pThis, VMMDevRequestHeader *pRequestHeader)
{
    AssertMsgReturn(pRequestHeader->size == sizeof(VMMDevReportGuestInfo), ("%u\n", pRequestHeader->size), VERR_INVALID_PARAMETER);
    VBoxGuestInfo const *pInfo = &((VMMDevReportGuestInfo *)pRequestHeader)->guestInfo;

    if (memcmp(&pThis->guestInfo, pInfo, sizeof(*pInfo)) != 0)
    {
        /* Make a copy of supplied information. */
        pThis->guestInfo = *pInfo;

        /* Check additions interface version. */
        pThis->fu32AdditionsOk = VMMDEV_INTERFACE_VERSION_IS_OK(pThis->guestInfo.interfaceVersion);

        vmmdevLogGuestOsInfo(&pThis->guestInfo);

        if (pThis->pDrv && pThis->pDrv->pfnUpdateGuestInfo)
            pThis->pDrv->pfnUpdateGuestInfo(pThis->pDrv, &pThis->guestInfo);
    }

    if (!pThis->fu32AdditionsOk)
        return VERR_VERSION_MISMATCH;

    /* Clear our IRQ in case it was high for whatever reason. */
    PDMDevHlpPCISetIrqNoWait(pThis->pDevIns, 0, 0);

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GuestHeartbeat.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis    The VMMDev instance data.
 */
static int vmmDevReqHandler_GuestHeartbeat(PVMMDEV pThis)
{
    int rc;
    if (pThis->fHeartbeatActive)
    {
        uint64_t const nsNowTS = TMTimerGetNano(pThis->pFlatlinedTimer);
        if (!pThis->fFlatlined)
        { /* likely */ }
        else
        {
            LogRel(("VMMDev: GuestHeartBeat: Guest is alive (gone %'llu ns)\n", nsNowTS - pThis->nsLastHeartbeatTS));
            ASMAtomicWriteBool(&pThis->fFlatlined, false);
        }
        ASMAtomicWriteU64(&pThis->nsLastHeartbeatTS, nsNowTS);

        /* Postpone (or restart if we missed a beat) the timeout timer. */
        rc = TMTimerSetNano(pThis->pFlatlinedTimer, pThis->cNsHeartbeatTimeout);
    }
    else
        rc = VINF_SUCCESS;
    return rc;
}


/**
 * Timer that fires when where have been no heartbeats for a given time.
 *
 * @remarks Does not take the VMMDev critsect.
 */
static DECLCALLBACK(void) vmmDevHeartbeatFlatlinedTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF1(pDevIns);
    PVMMDEV pThis = (PVMMDEV)pvUser;
    if (pThis->fHeartbeatActive)
    {
        uint64_t cNsElapsed = TMTimerGetNano(pTimer) - pThis->nsLastHeartbeatTS;
        if (   !pThis->fFlatlined
            && cNsElapsed >= pThis->cNsHeartbeatInterval)
        {
            LogRel(("VMMDev: vmmDevHeartbeatFlatlinedTimer: Guest seems to be unresponsive. Last heartbeat received %RU64 seconds ago\n",
                    cNsElapsed / RT_NS_1SEC));
            ASMAtomicWriteBool(&pThis->fFlatlined, true);
        }
    }
}


/**
 * Handles VMMDevReq_HeartbeatConfigure.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis     The VMMDev instance data.
 * @param   pReqHdr   The header of the request to handle.
 */
static int vmmDevReqHandler_HeartbeatConfigure(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    AssertMsgReturn(pReqHdr->size == sizeof(VMMDevReqHeartbeat), ("%u\n", pReqHdr->size), VERR_INVALID_PARAMETER);
    VMMDevReqHeartbeat *pReq = (VMMDevReqHeartbeat *)pReqHdr;
    int rc;

    pReq->cNsInterval = pThis->cNsHeartbeatInterval;

    if (pReq->fEnabled != pThis->fHeartbeatActive)
    {
        ASMAtomicWriteBool(&pThis->fHeartbeatActive, pReq->fEnabled);
        if (pReq->fEnabled)
        {
            /*
             * Activate the heartbeat monitor.
             */
            pThis->nsLastHeartbeatTS = TMTimerGetNano(pThis->pFlatlinedTimer);
            rc = TMTimerSetNano(pThis->pFlatlinedTimer, pThis->cNsHeartbeatTimeout);
            if (RT_SUCCESS(rc))
                LogRel(("VMMDev: Heartbeat flatline timer set to trigger after %'RU64 ns\n", pThis->cNsHeartbeatTimeout));
            else
                LogRel(("VMMDev: Error starting flatline timer (heartbeat): %Rrc\n", rc));
        }
        else
        {
            /*
             * Deactivate the heartbeat monitor.
             */
            rc = TMTimerStop(pThis->pFlatlinedTimer);
            LogRel(("VMMDev: Heartbeat checking timer has been stopped (rc=%Rrc)\n", rc));
        }
    }
    else
    {
        LogRel(("VMMDev: vmmDevReqHandler_HeartbeatConfigure: No change (fHeartbeatActive=%RTbool).\n", pThis->fHeartbeatActive));
        rc = VINF_SUCCESS;
    }

    return rc;
}


/**
 * Validates a publisher tag.
 *
 * @returns true / false.
 * @param   pszTag              Tag to validate.
 */
static bool vmmdevReqIsValidPublisherTag(const char *pszTag)
{
    /* Note! This character set is also found in Config.kmk. */
    static char const s_szValidChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz()[]{}+-.,";

    while (*pszTag != '\0')
    {
        if (!strchr(s_szValidChars, *pszTag))
            return false;
        pszTag++;
    }
    return true;
}


/**
 * Validates a build tag.
 *
 * @returns true / false.
 * @param   pszTag              Tag to validate.
 */
static bool vmmdevReqIsValidBuildTag(const char *pszTag)
{
    int cchPrefix;
    if (!strncmp(pszTag, "RC", 2))
        cchPrefix = 2;
    else if (!strncmp(pszTag, "BETA", 4))
        cchPrefix = 4;
    else if (!strncmp(pszTag, "ALPHA", 5))
        cchPrefix = 5;
    else
        return false;

    if (pszTag[cchPrefix] == '\0')
        return true;

    uint8_t u8;
    int rc = RTStrToUInt8Full(&pszTag[cchPrefix], 10, &u8);
    return rc == VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_ReportGuestInfo2.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_ReportGuestInfo2(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    AssertMsgReturn(pReqHdr->size == sizeof(VMMDevReportGuestInfo2), ("%u\n", pReqHdr->size), VERR_INVALID_PARAMETER);
    VBoxGuestInfo2 const *pInfo2 = &((VMMDevReportGuestInfo2 *)pReqHdr)->guestInfo;

    LogRel(("VMMDev: Guest Additions information report: Version %d.%d.%d r%d '%.*s'\n",
            pInfo2->additionsMajor, pInfo2->additionsMinor, pInfo2->additionsBuild,
            pInfo2->additionsRevision, sizeof(pInfo2->szName), pInfo2->szName));

    /* The interface was introduced in 3.2 and will definitely not be
       backported beyond 3.0 (bird). */
    AssertMsgReturn(pInfo2->additionsMajor >= 3,
                    ("%u.%u.%u\n", pInfo2->additionsMajor, pInfo2->additionsMinor, pInfo2->additionsBuild),
                    VERR_INVALID_PARAMETER);

    /* The version must fit in a full version compression. */
    uint32_t uFullVersion = VBOX_FULL_VERSION_MAKE(pInfo2->additionsMajor, pInfo2->additionsMinor, pInfo2->additionsBuild);
    AssertMsgReturn(   VBOX_FULL_VERSION_GET_MAJOR(uFullVersion) == pInfo2->additionsMajor
                    && VBOX_FULL_VERSION_GET_MINOR(uFullVersion) == pInfo2->additionsMinor
                    && VBOX_FULL_VERSION_GET_BUILD(uFullVersion) == pInfo2->additionsBuild,
                    ("%u.%u.%u\n", pInfo2->additionsMajor, pInfo2->additionsMinor, pInfo2->additionsBuild),
                    VERR_OUT_OF_RANGE);

    /*
     * Validate the name.
     * Be less strict towards older additions (< v4.1.50).
     */
    AssertCompile(sizeof(pThis->guestInfo2.szName) == sizeof(pInfo2->szName));
    AssertReturn(RTStrEnd(pInfo2->szName, sizeof(pInfo2->szName)) != NULL, VERR_INVALID_PARAMETER);
    const char *pszName = pInfo2->szName;

    /* The version number which shouldn't be there. */
    char        szTmp[sizeof(pInfo2->szName)];
    size_t      cchStart = RTStrPrintf(szTmp, sizeof(szTmp), "%u.%u.%u", pInfo2->additionsMajor, pInfo2->additionsMinor, pInfo2->additionsBuild);
    AssertMsgReturn(!strncmp(pszName, szTmp, cchStart), ("%s != %s\n", pszName, szTmp), VERR_INVALID_PARAMETER);
    pszName += cchStart;

    /* Now we can either have nothing or a build tag or/and a publisher tag. */
    if (*pszName != '\0')
    {
        const char *pszRelaxedName = "";
        bool const fStrict = pInfo2->additionsMajor > 4
                          || (pInfo2->additionsMajor == 4 && pInfo2->additionsMinor > 1)
                          || (pInfo2->additionsMajor == 4 && pInfo2->additionsMinor == 1 && pInfo2->additionsBuild >= 50);
        bool fOk = false;
        if (*pszName == '_')
        {
            pszName++;
            strcpy(szTmp, pszName);
            char *pszTag2 = strchr(szTmp, '_');
            if (!pszTag2)
            {
                fOk = vmmdevReqIsValidBuildTag(szTmp)
                   || vmmdevReqIsValidPublisherTag(szTmp);
            }
            else
            {
                *pszTag2++ = '\0';
                fOk = vmmdevReqIsValidBuildTag(szTmp);
                if (fOk)
                {
                    fOk = vmmdevReqIsValidPublisherTag(pszTag2);
                    if (!fOk)
                        pszRelaxedName = szTmp;
                }
            }
        }

        if (!fOk)
        {
            AssertLogRelMsgReturn(!fStrict, ("%s", pszName), VERR_INVALID_PARAMETER);

            /* non-strict mode, just zap the extra stuff. */
            LogRel(("VMMDev: ReportGuestInfo2: Ignoring unparsable version name bits: '%s' -> '%s'.\n", pszName, pszRelaxedName));
            pszName = pszRelaxedName;
        }
    }

    /*
     * Save the info and tell Main or whoever is listening.
     */
    pThis->guestInfo2.uFullVersion  = uFullVersion;
    pThis->guestInfo2.uRevision     = pInfo2->additionsRevision;
    pThis->guestInfo2.fFeatures     = pInfo2->additionsFeatures;
    strcpy(pThis->guestInfo2.szName, pszName);

    if (pThis->pDrv && pThis->pDrv->pfnUpdateGuestInfo2)
        pThis->pDrv->pfnUpdateGuestInfo2(pThis->pDrv, uFullVersion, pszName, pInfo2->additionsRevision, pInfo2->additionsFeatures);

    /* Clear our IRQ in case it was high for whatever reason. */
    PDMDevHlpPCISetIrqNoWait(pThis->pDevIns, 0, 0);

    return VINF_SUCCESS;
}


/**
 * Allocates a new facility status entry, initializing it to inactive.
 *
 * @returns Pointer to a facility status entry on success, NULL on failure
 *          (table full).
 * @param   pThis           The VMMDev instance data.
 * @param   enmFacility     The facility type code.
 * @param   fFixed          This is set when allocating the standard entries
 *                          from the constructor.
 * @param   pTimeSpecNow    Optionally giving the entry timestamp to use (ctor).
 */
static PVMMDEVFACILITYSTATUSENTRY
vmmdevAllocFacilityStatusEntry(PVMMDEV pThis, VBoxGuestFacilityType enmFacility, bool fFixed, PCRTTIMESPEC pTimeSpecNow)
{
    /* If full, expunge one inactive entry. */
    if (pThis->cFacilityStatuses == RT_ELEMENTS(pThis->aFacilityStatuses))
    {
        uint32_t i = pThis->cFacilityStatuses;
        while (i-- > 0)
        {
            if (   pThis->aFacilityStatuses[i].enmStatus == VBoxGuestFacilityStatus_Inactive
                && !pThis->aFacilityStatuses[i].fFixed)
            {
                pThis->cFacilityStatuses--;
                int cToMove = pThis->cFacilityStatuses - i;
                if (cToMove)
                    memmove(&pThis->aFacilityStatuses[i], &pThis->aFacilityStatuses[i + 1],
                            cToMove * sizeof(pThis->aFacilityStatuses[i]));
                RT_ZERO(pThis->aFacilityStatuses[pThis->cFacilityStatuses]);
                break;
            }
        }

        if (pThis->cFacilityStatuses == RT_ELEMENTS(pThis->aFacilityStatuses))
            return NULL;
    }

    /* Find location in array (it's sorted). */
    uint32_t i = pThis->cFacilityStatuses;
    while (i-- > 0)
        if ((uint32_t)pThis->aFacilityStatuses[i].enmFacility < (uint32_t)enmFacility)
            break;
    i++;

    /* Move. */
    int cToMove = pThis->cFacilityStatuses - i;
    if (cToMove > 0)
        memmove(&pThis->aFacilityStatuses[i + 1], &pThis->aFacilityStatuses[i],
                cToMove * sizeof(pThis->aFacilityStatuses[i]));
    pThis->cFacilityStatuses++;

    /* Initialize. */
    pThis->aFacilityStatuses[i].enmFacility  = enmFacility;
    pThis->aFacilityStatuses[i].enmStatus    = VBoxGuestFacilityStatus_Inactive;
    pThis->aFacilityStatuses[i].fFixed       = fFixed;
    pThis->aFacilityStatuses[i].afPadding[0] = 0;
    pThis->aFacilityStatuses[i].afPadding[1] = 0;
    pThis->aFacilityStatuses[i].afPadding[2] = 0;
    pThis->aFacilityStatuses[i].fFlags       = 0;
    if (pTimeSpecNow)
        pThis->aFacilityStatuses[i].TimeSpecTS = *pTimeSpecNow;
    else
        RTTimeSpecSetNano(&pThis->aFacilityStatuses[i].TimeSpecTS, 0);

    return &pThis->aFacilityStatuses[i];
}


/**
 * Gets a facility status entry, allocating a new one if not already present.
 *
 * @returns Pointer to a facility status entry on success, NULL on failure
 *          (table full).
 * @param   pThis           The VMMDev instance data.
 * @param   enmFacility     The facility type code.
 */
static PVMMDEVFACILITYSTATUSENTRY vmmdevGetFacilityStatusEntry(PVMMDEV pThis, VBoxGuestFacilityType enmFacility)
{
    /** @todo change to binary search. */
    uint32_t i = pThis->cFacilityStatuses;
    while (i-- > 0)
    {
        if (pThis->aFacilityStatuses[i].enmFacility == enmFacility)
            return &pThis->aFacilityStatuses[i];
        if ((uint32_t)pThis->aFacilityStatuses[i].enmFacility < (uint32_t)enmFacility)
            break;
    }
    return vmmdevAllocFacilityStatusEntry(pThis, enmFacility, false /*fFixed*/, NULL);
}


/**
 * Handles VMMDevReq_ReportGuestStatus.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_ReportGuestStatus(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    /*
     * Validate input.
     */
    AssertMsgReturn(pReqHdr->size == sizeof(VMMDevReportGuestStatus), ("%u\n", pReqHdr->size), VERR_INVALID_PARAMETER);
    VBoxGuestStatus *pStatus = &((VMMDevReportGuestStatus *)pReqHdr)->guestStatus;
    AssertMsgReturn(   pStatus->facility > VBoxGuestFacilityType_Unknown
                    && pStatus->facility <= VBoxGuestFacilityType_All,
                    ("%d\n", pStatus->facility),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(pStatus->status == (VBoxGuestFacilityStatus)(uint16_t)pStatus->status,
                    ("%#x (%u)\n", pStatus->status, pStatus->status),
                    VERR_OUT_OF_RANGE);

    /*
     * Do the update.
     */
    RTTIMESPEC Now;
    RTTimeNow(&Now);
    if (pStatus->facility == VBoxGuestFacilityType_All)
    {
        uint32_t i = pThis->cFacilityStatuses;
        while (i-- > 0)
        {
            pThis->aFacilityStatuses[i].TimeSpecTS = Now;
            pThis->aFacilityStatuses[i].enmStatus  = pStatus->status;
            pThis->aFacilityStatuses[i].fFlags     = pStatus->flags;
        }
    }
    else
    {
        PVMMDEVFACILITYSTATUSENTRY pEntry = vmmdevGetFacilityStatusEntry(pThis, pStatus->facility);
        if (!pEntry)
        {
            LogRelMax(10, ("VMMDev: Facility table is full - facility=%u status=%u\n", pStatus->facility, pStatus->status));
            return VERR_OUT_OF_RESOURCES;
        }

        pEntry->TimeSpecTS = Now;
        pEntry->enmStatus  = pStatus->status;
        pEntry->fFlags     = pStatus->flags;
    }

    if (pThis->pDrv && pThis->pDrv->pfnUpdateGuestStatus)
        pThis->pDrv->pfnUpdateGuestStatus(pThis->pDrv, pStatus->facility, pStatus->status, pStatus->flags, &Now);

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_ReportGuestUserState.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_ReportGuestUserState(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    /*
     * Validate input.
     */
    VMMDevReportGuestUserState *pReq = (VMMDevReportGuestUserState *)pReqHdr;
    AssertMsgReturn(pReq->header.size >= sizeof(*pReq), ("%u\n", pReqHdr->size), VERR_INVALID_PARAMETER);

    if (   pThis->pDrv
        && pThis->pDrv->pfnUpdateGuestUserState)
    {
        /* Play safe. */
        AssertReturn(pReq->header.size      <= _2K, VERR_TOO_MUCH_DATA);
        AssertReturn(pReq->status.cbUser    <= 256, VERR_TOO_MUCH_DATA);
        AssertReturn(pReq->status.cbDomain  <= 256, VERR_TOO_MUCH_DATA);
        AssertReturn(pReq->status.cbDetails <= _1K, VERR_TOO_MUCH_DATA);

        /* pbDynamic marks the beginning of the struct's dynamically
         * allocated data area. */
        uint8_t *pbDynamic = (uint8_t *)&pReq->status.szUser;
        uint32_t cbLeft    = pReqHdr->size - RT_OFFSETOF(VMMDevReportGuestUserState, status.szUser);

        /* The user. */
        AssertReturn(pReq->status.cbUser > 0, VERR_INVALID_PARAMETER); /* User name is required. */
        AssertReturn(pReq->status.cbUser <= cbLeft, VERR_INVALID_PARAMETER);
        const char *pszUser = (const char *)pbDynamic;
        AssertReturn(RTStrEnd(pszUser, pReq->status.cbUser), VERR_INVALID_PARAMETER);
        int rc = RTStrValidateEncoding(pszUser);
        AssertRCReturn(rc, rc);

        /* Advance to the next field. */
        pbDynamic += pReq->status.cbUser;
        cbLeft    -= pReq->status.cbUser;

        /* pszDomain can be NULL. */
        AssertReturn(pReq->status.cbDomain <= cbLeft, VERR_INVALID_PARAMETER);
        const char *pszDomain = NULL;
        if (pReq->status.cbDomain)
        {
            pszDomain = (const char *)pbDynamic;
            AssertReturn(RTStrEnd(pszDomain, pReq->status.cbDomain), VERR_INVALID_PARAMETER);
            rc = RTStrValidateEncoding(pszDomain);
            AssertRCReturn(rc, rc);

            /* Advance to the next field. */
            pbDynamic += pReq->status.cbDomain;
            cbLeft    -= pReq->status.cbDomain;
        }

        /* pbDetails can be NULL. */
        const uint8_t *pbDetails = NULL;
        AssertReturn(pReq->status.cbDetails <= cbLeft, VERR_INVALID_PARAMETER);
        if (pReq->status.cbDetails > 0)
            pbDetails = pbDynamic;

        pThis->pDrv->pfnUpdateGuestUserState(pThis->pDrv, pszUser, pszDomain, (uint32_t)pReq->status.state,
                                             pbDetails, pReq->status.cbDetails);
    }

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_ReportGuestCapabilities.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_ReportGuestCapabilities(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqGuestCapabilities *pReq = (VMMDevReqGuestCapabilities *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* Enable VMMDEV_GUEST_SUPPORTS_GRAPHICS automatically for guests using the old
     * request to report their capabilities.
     */
    const uint32_t fu32Caps = pReq->caps | VMMDEV_GUEST_SUPPORTS_GRAPHICS;

    if (pThis->guestCaps != fu32Caps)
    {
        /* make a copy of supplied information */
        pThis->guestCaps = fu32Caps;

        LogRel(("VMMDev: Guest Additions capability report (legacy): (0x%x) seamless: %s, hostWindowMapping: %s, graphics: yes\n",
                fu32Caps,
                fu32Caps & VMMDEV_GUEST_SUPPORTS_SEAMLESS ? "yes" : "no",
                fu32Caps & VMMDEV_GUEST_SUPPORTS_GUEST_HOST_WINDOW_MAPPING ? "yes" : "no"));

        if (pThis->pDrv && pThis->pDrv->pfnUpdateGuestCapabilities)
            pThis->pDrv->pfnUpdateGuestCapabilities(pThis->pDrv, fu32Caps);
    }
    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_SetGuestCapabilities.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_SetGuestCapabilities(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqGuestCapabilities2 *pReq = (VMMDevReqGuestCapabilities2 *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    uint32_t fu32Caps = pThis->guestCaps;
    fu32Caps |= pReq->u32OrMask;
    fu32Caps &= ~pReq->u32NotMask;

    LogRel(("VMMDev: Guest Additions capability report: (%#x -> %#x) seamless: %s, hostWindowMapping: %s, graphics: %s\n",
            pThis->guestCaps, fu32Caps,
            fu32Caps & VMMDEV_GUEST_SUPPORTS_SEAMLESS ? "yes" : "no",
            fu32Caps & VMMDEV_GUEST_SUPPORTS_GUEST_HOST_WINDOW_MAPPING ? "yes" : "no",
            fu32Caps & VMMDEV_GUEST_SUPPORTS_GRAPHICS ? "yes" : "no"));

    pThis->guestCaps = fu32Caps;

    if (pThis->pDrv && pThis->pDrv->pfnUpdateGuestCapabilities)
        pThis->pDrv->pfnUpdateGuestCapabilities(pThis->pDrv, fu32Caps);

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GetMouseStatus.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetMouseStatus(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqMouseStatus *pReq = (VMMDevReqMouseStatus *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    pReq->mouseFeatures = pThis->mouseCapabilities
                        & VMMDEV_MOUSE_MASK;
    pReq->pointerXPos   = pThis->mouseXAbs;
    pReq->pointerYPos   = pThis->mouseYAbs;
    LogRel2(("VMMDev: vmmdevReqHandler_GetMouseStatus: mouseFeatures=%#x, xAbs=%d, yAbs=%d\n",
             pReq->mouseFeatures, pReq->pointerXPos, pReq->pointerYPos));
    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_SetMouseStatus.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_SetMouseStatus(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqMouseStatus *pReq = (VMMDevReqMouseStatus *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    LogRelFlow(("VMMDev: vmmdevReqHandler_SetMouseStatus: mouseFeatures=%#x\n", pReq->mouseFeatures));

    bool fNotify = false;
    if (   (pReq->mouseFeatures & VMMDEV_MOUSE_NOTIFY_HOST_MASK)
        != (  pThis->mouseCapabilities
            & VMMDEV_MOUSE_NOTIFY_HOST_MASK))
        fNotify = true;

    pThis->mouseCapabilities &= ~VMMDEV_MOUSE_GUEST_MASK;
    pThis->mouseCapabilities |= (pReq->mouseFeatures & VMMDEV_MOUSE_GUEST_MASK);

    LogRelFlow(("VMMDev: vmmdevReqHandler_SetMouseStatus: New host capabilities: %#x\n", pThis->mouseCapabilities));

    /*
     * Notify connector if something changed.
     */
    if (fNotify)
    {
        LogRelFlow(("VMMDev: vmmdevReqHandler_SetMouseStatus: Notifying connector\n"));
        pThis->pDrv->pfnUpdateMouseCapabilities(pThis->pDrv, pThis->mouseCapabilities);
    }

    return VINF_SUCCESS;
}

static int vmmdevVerifyPointerShape(VMMDevReqMousePointer *pReq)
{
    /* Should be enough for most mouse pointers. */
    if (pReq->width > 8192 || pReq->height > 8192)
        return VERR_INVALID_PARAMETER;

    uint32_t cbShape = (pReq->width + 7) / 8 * pReq->height; /* size of the AND mask */
    cbShape = ((cbShape + 3) & ~3) + pReq->width * 4 * pReq->height; /* + gap + size of the XOR mask */
    if (RT_UOFFSETOF(VMMDevReqMousePointer, pointerData) + cbShape > pReq->header.size)
        return VERR_INVALID_PARAMETER;

    return VINF_SUCCESS;
}

/**
 * Handles VMMDevReq_SetPointerShape.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_SetPointerShape(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqMousePointer *pReq = (VMMDevReqMousePointer *)pReqHdr;
    if (pReq->header.size < sizeof(*pReq))
    {
        AssertMsg(pReq->header.size == 0x10028 && pReq->header.version == 10000,  /* don't complain about legacy!!! */
                  ("VMMDev mouse shape structure has invalid size %d (%#x) version=%d!\n",
                   pReq->header.size, pReq->header.size, pReq->header.version));
        return VERR_INVALID_PARAMETER;
    }

    bool fVisible = RT_BOOL(pReq->fFlags & VBOX_MOUSE_POINTER_VISIBLE);
    bool fAlpha   = RT_BOOL(pReq->fFlags & VBOX_MOUSE_POINTER_ALPHA);
    bool fShape   = RT_BOOL(pReq->fFlags & VBOX_MOUSE_POINTER_SHAPE);

    Log(("VMMDevReq_SetPointerShape: visible: %d, alpha: %d, shape = %d, width: %d, height: %d\n",
         fVisible, fAlpha, fShape, pReq->width, pReq->height));

    if (pReq->header.size == sizeof(VMMDevReqMousePointer))
    {
        /* The guest did not provide the shape actually. */
        fShape = false;
    }

    /* forward call to driver */
    if (fShape)
    {
        int rc = vmmdevVerifyPointerShape(pReq);
        if (RT_FAILURE(rc))
            return rc;

        pThis->pDrv->pfnUpdatePointerShape(pThis->pDrv,
                                           fVisible,
                                           fAlpha,
                                           pReq->xHot, pReq->yHot,
                                           pReq->width, pReq->height,
                                           pReq->pointerData);
    }
    else
    {
        pThis->pDrv->pfnUpdatePointerShape(pThis->pDrv,
                                           fVisible,
                                           0,
                                           0, 0,
                                           0, 0,
                                           NULL);
    }

    pThis->fHostCursorRequested = fVisible;
    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GetHostTime.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetHostTime(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqHostTime *pReq = (VMMDevReqHostTime *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    if (RT_LIKELY(!pThis->fGetHostTimeDisabled))
    {
        RTTIMESPEC now;
        pReq->time = RTTimeSpecGetMilli(PDMDevHlpTMUtcNow(pThis->pDevIns, &now));
        return VINF_SUCCESS;
    }
    return VERR_NOT_SUPPORTED;
}


/**
 * Handles VMMDevReq_GetHypervisorInfo.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetHypervisorInfo(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqHypervisorInfo *pReq = (VMMDevReqHypervisorInfo *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    return PGMR3MappingsSize(PDMDevHlpGetVM(pThis->pDevIns), &pReq->hypervisorSize);
}


/**
 * Handles VMMDevReq_SetHypervisorInfo.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_SetHypervisorInfo(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqHypervisorInfo *pReq = (VMMDevReqHypervisorInfo *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    int rc;
    PVM pVM = PDMDevHlpGetVM(pThis->pDevIns);
    if (pReq->hypervisorStart == 0)
        rc = PGMR3MappingsUnfix(pVM);
    else
    {
        /* only if the client has queried the size before! */
        uint32_t cbMappings;
        rc = PGMR3MappingsSize(pVM, &cbMappings);
        if (RT_SUCCESS(rc) && pReq->hypervisorSize == cbMappings)
        {
            /* new reservation */
            rc = PGMR3MappingsFix(pVM, pReq->hypervisorStart, pReq->hypervisorSize);
            LogRel(("VMMDev: Guest reported fixed hypervisor window at 0%010x LB %#x (rc=%Rrc)\n",
                    pReq->hypervisorStart, pReq->hypervisorSize, rc));
        }
        else if (RT_FAILURE(rc))
            rc = VERR_TRY_AGAIN;
    }
    return rc;
}


/**
 * Handles VMMDevReq_RegisterPatchMemory.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_RegisterPatchMemory(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqPatchMemory *pReq = (VMMDevReqPatchMemory *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    return VMMR3RegisterPatchMemory(PDMDevHlpGetVM(pThis->pDevIns), pReq->pPatchMem, pReq->cbPatchMem);
}


/**
 * Handles VMMDevReq_DeregisterPatchMemory.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_DeregisterPatchMemory(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqPatchMemory *pReq = (VMMDevReqPatchMemory *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    return VMMR3DeregisterPatchMemory(PDMDevHlpGetVM(pThis->pDevIns), pReq->pPatchMem, pReq->cbPatchMem);
}


/**
 * Handles VMMDevReq_SetPowerStatus.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_SetPowerStatus(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevPowerStateRequest *pReq = (VMMDevPowerStateRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    switch (pReq->powerState)
    {
        case VMMDevPowerState_Pause:
        {
            LogRel(("VMMDev: Guest requests the VM to be suspended (paused)\n"));
            return PDMDevHlpVMSuspend(pThis->pDevIns);
        }

        case VMMDevPowerState_PowerOff:
        {
            LogRel(("VMMDev: Guest requests the VM to be turned off\n"));
            return PDMDevHlpVMPowerOff(pThis->pDevIns);
        }

        case VMMDevPowerState_SaveState:
        {
            if (true /*pThis->fAllowGuestToSaveState*/)
            {
                LogRel(("VMMDev: Guest requests the VM to be saved and powered off\n"));
                return PDMDevHlpVMSuspendSaveAndPowerOff(pThis->pDevIns);
            }
            LogRel(("VMMDev: Guest requests the VM to be saved and powered off, declined\n"));
            return VERR_ACCESS_DENIED;
        }

        default:
            AssertMsgFailed(("VMMDev: Invalid power state request: %d\n", pReq->powerState));
            return VERR_INVALID_PARAMETER;
    }
}


/**
 * Handles VMMDevReq_GetDisplayChangeRequest
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 * @remarks Deprecated.
 */
static int vmmdevReqHandler_GetDisplayChangeRequest(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevDisplayChangeRequest *pReq = (VMMDevDisplayChangeRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

/**
 * @todo It looks like a multi-monitor guest which only uses
 *        @c VMMDevReq_GetDisplayChangeRequest (not the *2 version) will get
 *        into a @c VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST event loop if it tries
 *        to acknowlege host requests for additional monitors.  Should the loop
 *        which checks for those requests be removed?
 */

    DISPLAYCHANGEREQUEST *pDispRequest = &pThis->displayChangeData.aRequests[0];

    if (pReq->eventAck == VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST)
    {
        /* Current request has been read at least once. */
        pDispRequest->fPending = false;

        /* Check if there are more pending requests. */
        for (unsigned i = 1; i < RT_ELEMENTS(pThis->displayChangeData.aRequests); i++)
        {
            if (pThis->displayChangeData.aRequests[i].fPending)
            {
                VMMDevNotifyGuest(pThis, VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST);
                break;
            }
        }

        /* Remember which resolution the client has queried, subsequent reads
         * will return the same values. */
        pDispRequest->lastReadDisplayChangeRequest = pDispRequest->displayChangeRequest;
        pThis->displayChangeData.fGuestSentChangeEventAck = true;
    }

    if (pThis->displayChangeData.fGuestSentChangeEventAck)
    {
        pReq->xres = pDispRequest->lastReadDisplayChangeRequest.xres;
        pReq->yres = pDispRequest->lastReadDisplayChangeRequest.yres;
        pReq->bpp  = pDispRequest->lastReadDisplayChangeRequest.bpp;
    }
    else
    {
        /* This is not a response to a VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST, just
         * read the last valid video mode hint. This happens when the guest X server
         * determines the initial mode. */
        pReq->xres = pDispRequest->displayChangeRequest.xres;
        pReq->yres = pDispRequest->displayChangeRequest.yres;
        pReq->bpp  = pDispRequest->displayChangeRequest.bpp;
    }
    Log(("VMMDev: returning display change request xres = %d, yres = %d, bpp = %d\n", pReq->xres, pReq->yres, pReq->bpp));

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GetDisplayChangeRequest2.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetDisplayChangeRequest2(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevDisplayChangeRequest2 *pReq = (VMMDevDisplayChangeRequest2 *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    DISPLAYCHANGEREQUEST *pDispRequest = NULL;

    if (pReq->eventAck == VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST)
    {
        /* Select a pending request to report. */
        unsigned i;
        for (i = 0; i < RT_ELEMENTS(pThis->displayChangeData.aRequests); i++)
        {
            if (pThis->displayChangeData.aRequests[i].fPending)
            {
                pDispRequest = &pThis->displayChangeData.aRequests[i];
                /* Remember which request should be reported. */
                pThis->displayChangeData.iCurrentMonitor = i;
                Log3(("VMMDev: will report pending request for %u\n", i));
                break;
            }
        }

        /* Check if there are more pending requests. */
        i++;
        for (; i < RT_ELEMENTS(pThis->displayChangeData.aRequests); i++)
        {
            if (pThis->displayChangeData.aRequests[i].fPending)
            {
                VMMDevNotifyGuest(pThis, VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST);
                Log3(("VMMDev: another pending at %u\n", i));
                break;
            }
        }

        if (pDispRequest)
        {
            /* Current request has been read at least once. */
            pDispRequest->fPending = false;

            /* Remember which resolution the client has queried, subsequent reads
             * will return the same values. */
            pDispRequest->lastReadDisplayChangeRequest = pDispRequest->displayChangeRequest;
            pThis->displayChangeData.fGuestSentChangeEventAck = true;
        }
        else
        {
             Log3(("VMMDev: no pending request!!!\n"));
        }
    }

    if (!pDispRequest)
    {
        Log3(("VMMDev: default to %d\n", pThis->displayChangeData.iCurrentMonitor));
        pDispRequest = &pThis->displayChangeData.aRequests[pThis->displayChangeData.iCurrentMonitor];
    }

    if (pThis->displayChangeData.fGuestSentChangeEventAck)
    {
        pReq->xres    = pDispRequest->lastReadDisplayChangeRequest.xres;
        pReq->yres    = pDispRequest->lastReadDisplayChangeRequest.yres;
        pReq->bpp     = pDispRequest->lastReadDisplayChangeRequest.bpp;
        pReq->display = pDispRequest->lastReadDisplayChangeRequest.display;
    }
    else
    {
        /* This is not a response to a VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST, just
         * read the last valid video mode hint. This happens when the guest X server
         * determines the initial video mode. */
        pReq->xres    = pDispRequest->displayChangeRequest.xres;
        pReq->yres    = pDispRequest->displayChangeRequest.yres;
        pReq->bpp     = pDispRequest->displayChangeRequest.bpp;
        pReq->display = pDispRequest->displayChangeRequest.display;
    }
    Log(("VMMDev: returning display change request xres = %d, yres = %d, bpp = %d at %d\n",
         pReq->xres, pReq->yres, pReq->bpp, pReq->display));

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GetDisplayChangeRequestEx.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetDisplayChangeRequestEx(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevDisplayChangeRequestEx *pReq = (VMMDevDisplayChangeRequestEx *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    DISPLAYCHANGEREQUEST *pDispRequest = NULL;

    if (pReq->eventAck == VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST)
    {
        /* Select a pending request to report. */
        unsigned i;
        for (i = 0; i < RT_ELEMENTS(pThis->displayChangeData.aRequests); i++)
        {
            if (pThis->displayChangeData.aRequests[i].fPending)
            {
                pDispRequest = &pThis->displayChangeData.aRequests[i];
                /* Remember which request should be reported. */
                pThis->displayChangeData.iCurrentMonitor = i;
                Log3(("VMMDev: will report pending request for %d\n",
                      i));
                break;
            }
        }

        /* Check if there are more pending requests. */
        i++;
        for (; i < RT_ELEMENTS(pThis->displayChangeData.aRequests); i++)
        {
            if (pThis->displayChangeData.aRequests[i].fPending)
            {
                VMMDevNotifyGuest(pThis, VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST);
                Log3(("VMMDev: another pending at %d\n",
                      i));
                break;
            }
        }

        if (pDispRequest)
        {
            /* Current request has been read at least once. */
            pDispRequest->fPending = false;

            /* Remember which resolution the client has queried, subsequent reads
             * will return the same values. */
            pDispRequest->lastReadDisplayChangeRequest = pDispRequest->displayChangeRequest;
            pThis->displayChangeData.fGuestSentChangeEventAck = true;
        }
        else
        {
             Log3(("VMMDev: no pending request!!!\n"));
        }
    }

    if (!pDispRequest)
    {
        Log3(("VMMDev: default to %d\n",
              pThis->displayChangeData.iCurrentMonitor));
        pDispRequest = &pThis->displayChangeData.aRequests[pThis->displayChangeData.iCurrentMonitor];
    }

    if (pThis->displayChangeData.fGuestSentChangeEventAck)
    {
        pReq->xres          = pDispRequest->lastReadDisplayChangeRequest.xres;
        pReq->yres          = pDispRequest->lastReadDisplayChangeRequest.yres;
        pReq->bpp           = pDispRequest->lastReadDisplayChangeRequest.bpp;
        pReq->display       = pDispRequest->lastReadDisplayChangeRequest.display;
        pReq->cxOrigin      = pDispRequest->lastReadDisplayChangeRequest.xOrigin;
        pReq->cyOrigin      = pDispRequest->lastReadDisplayChangeRequest.yOrigin;
        pReq->fEnabled      = pDispRequest->lastReadDisplayChangeRequest.fEnabled;
        pReq->fChangeOrigin = pDispRequest->lastReadDisplayChangeRequest.fChangeOrigin;
    }
    else
    {
        /* This is not a response to a VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST, just
         * read the last valid video mode hint. This happens when the guest X server
         * determines the initial video mode. */
        pReq->xres          = pDispRequest->displayChangeRequest.xres;
        pReq->yres          = pDispRequest->displayChangeRequest.yres;
        pReq->bpp           = pDispRequest->displayChangeRequest.bpp;
        pReq->display       = pDispRequest->displayChangeRequest.display;
        pReq->cxOrigin      = pDispRequest->displayChangeRequest.xOrigin;
        pReq->cyOrigin      = pDispRequest->displayChangeRequest.yOrigin;
        pReq->fEnabled      = pDispRequest->displayChangeRequest.fEnabled;
        pReq->fChangeOrigin = pDispRequest->displayChangeRequest.fChangeOrigin;

    }
    Log(("VMMDevEx: returning display change request xres = %d, yres = %d, bpp = %d id %d xPos = %d, yPos = %d & Enabled=%d\n",
         pReq->xres, pReq->yres, pReq->bpp, pReq->display, pReq->cxOrigin, pReq->cyOrigin, pReq->fEnabled));

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_VideoModeSupported.
 *
 * Query whether the given video mode is supported.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_VideoModeSupported(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevVideoModeSupportedRequest *pReq = (VMMDevVideoModeSupportedRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* forward the call */
    return pThis->pDrv->pfnVideoModeSupported(pThis->pDrv,
                                              0, /* primary screen. */
                                              pReq->width,
                                              pReq->height,
                                              pReq->bpp,
                                              &pReq->fSupported);
}


/**
 * Handles VMMDevReq_VideoModeSupported2.
 *
 * Query whether the given video mode is supported for a specific display
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_VideoModeSupported2(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevVideoModeSupportedRequest2 *pReq = (VMMDevVideoModeSupportedRequest2 *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* forward the call */
    return pThis->pDrv->pfnVideoModeSupported(pThis->pDrv,
                                              pReq->display,
                                              pReq->width,
                                              pReq->height,
                                              pReq->bpp,
                                              &pReq->fSupported);
}



/**
 * Handles VMMDevReq_GetHeightReduction.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetHeightReduction(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevGetHeightReductionRequest *pReq = (VMMDevGetHeightReductionRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* forward the call */
    return pThis->pDrv->pfnGetHeightReduction(pThis->pDrv, &pReq->heightReduction);
}


/**
 * Handles VMMDevReq_AcknowledgeEvents.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_AcknowledgeEvents(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevEvents *pReq = (VMMDevEvents *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    if (!VMMDEV_INTERFACE_VERSION_IS_1_03(pThis))
    {
        if (pThis->fNewGuestFilterMask)
        {
            pThis->fNewGuestFilterMask = false;
            pThis->u32GuestFilterMask = pThis->u32NewGuestFilterMask;
        }

        pReq->events = pThis->u32HostEventFlags & pThis->u32GuestFilterMask;

        pThis->u32HostEventFlags &= ~pThis->u32GuestFilterMask;
        pThis->pVMMDevRAMR3->V.V1_04.fHaveEvents = false;
        PDMDevHlpPCISetIrqNoWait(pThis->pDevIns, 0, 0);
    }
    else
        vmmdevSetIRQ_Legacy(pThis);
    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_CtlGuestFilterMask.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_CtlGuestFilterMask(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevCtlGuestFilterMask *pReq = (VMMDevCtlGuestFilterMask *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    LogRelFlow(("VMMDev: vmmdevReqHandler_CtlGuestFilterMask: OR mask: %#x, NOT mask: %#x\n", pReq->u32OrMask, pReq->u32NotMask));

    /* HGCM event notification is enabled by the VMMDev device
     * automatically when any HGCM command is issued.  The guest
     * cannot disable these notifications. */
    VMMDevCtlSetGuestFilterMask(pThis, pReq->u32OrMask, pReq->u32NotMask & ~VMMDEV_EVENT_HGCM);
    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_HGCM

/**
 * Handles VMMDevReq_HGCMConnect.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 * @param   GCPhysReqHdr    The guest physical address of the request header.
 */
static int vmmdevReqHandler_HGCMConnect(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr, RTGCPHYS GCPhysReqHdr)
{
    VMMDevHGCMConnect *pReq = (VMMDevHGCMConnect *)pReqHdr;
    AssertMsgReturn(pReq->header.header.size >= sizeof(*pReq), ("%u\n", pReq->header.header.size), VERR_INVALID_PARAMETER); /** @todo Not sure why this is >= ... */

    if (pThis->pHGCMDrv)
    {
        Log(("VMMDevReq_HGCMConnect\n"));
        return vmmdevHGCMConnect(pThis, pReq, GCPhysReqHdr);
    }

    Log(("VMMDevReq_HGCMConnect: HGCM Connector is NULL!\n"));
    return VERR_NOT_SUPPORTED;
}


/**
 * Handles VMMDevReq_HGCMDisconnect.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 * @param   GCPhysReqHdr    The guest physical address of the request header.
 */
static int vmmdevReqHandler_HGCMDisconnect(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr, RTGCPHYS GCPhysReqHdr)
{
    VMMDevHGCMDisconnect *pReq = (VMMDevHGCMDisconnect *)pReqHdr;
    AssertMsgReturn(pReq->header.header.size >= sizeof(*pReq), ("%u\n", pReq->header.header.size), VERR_INVALID_PARAMETER);  /** @todo Not sure why this >= ... */

    if (pThis->pHGCMDrv)
    {
        Log(("VMMDevReq_VMMDevHGCMDisconnect\n"));
        return vmmdevHGCMDisconnect(pThis, pReq, GCPhysReqHdr);
    }

    Log(("VMMDevReq_VMMDevHGCMDisconnect: HGCM Connector is NULL!\n"));
    return VERR_NOT_SUPPORTED;
}


/**
 * Handles VMMDevReq_HGCMCall.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 * @param   GCPhysReqHdr    The guest physical address of the request header.
 */
static int vmmdevReqHandler_HGCMCall(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr, RTGCPHYS GCPhysReqHdr)
{
    VMMDevHGCMCall *pReq = (VMMDevHGCMCall *)pReqHdr;
    AssertMsgReturn(pReq->header.header.size >= sizeof(*pReq), ("%u\n", pReq->header.header.size), VERR_INVALID_PARAMETER);

    if (pThis->pHGCMDrv)
    {
        Log2(("VMMDevReq_HGCMCall: sizeof(VMMDevHGCMRequest) = %04X\n", sizeof(VMMDevHGCMCall)));
        Log2(("%.*Rhxd\n", pReq->header.header.size, pReq));

        return vmmdevHGCMCall(pThis, pReq, pReq->header.header.size, GCPhysReqHdr, pReq->header.header.requestType);
    }

    Log(("VMMDevReq_HGCMCall: HGCM Connector is NULL!\n"));
    return VERR_NOT_SUPPORTED;
}

/**
 * Handles VMMDevReq_HGCMCancel.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 * @param   GCPhysReqHdr    The guest physical address of the request header.
 */
static int vmmdevReqHandler_HGCMCancel(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr, RTGCPHYS GCPhysReqHdr)
{
    VMMDevHGCMCancel *pReq = (VMMDevHGCMCancel *)pReqHdr;
    AssertMsgReturn(pReq->header.header.size >= sizeof(*pReq), ("%u\n", pReq->header.header.size), VERR_INVALID_PARAMETER);  /** @todo Not sure why this >= ... */

    if (pThis->pHGCMDrv)
    {
        Log(("VMMDevReq_VMMDevHGCMCancel\n"));
        return vmmdevHGCMCancel(pThis, pReq, GCPhysReqHdr);
    }

    Log(("VMMDevReq_VMMDevHGCMCancel: HGCM Connector is NULL!\n"));
    return VERR_NOT_SUPPORTED;
}


/**
 * Handles VMMDevReq_HGCMCancel2.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_HGCMCancel2(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevHGCMCancel2 *pReq = (VMMDevHGCMCancel2 *)pReqHdr;
    AssertMsgReturn(pReq->header.size >= sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);  /** @todo Not sure why this >= ... */

    if (pThis->pHGCMDrv)
    {
        Log(("VMMDevReq_HGCMCancel2\n"));
        return vmmdevHGCMCancel2(pThis, pReq->physReqToCancel);
    }

    Log(("VMMDevReq_HGCMConnect2: HGCM Connector is NULL!\n"));
    return VERR_NOT_SUPPORTED;
}

#endif /* VBOX_WITH_HGCM */


/**
 * Handles VMMDevReq_VideoAccelEnable.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_VideoAccelEnable(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevVideoAccelEnable *pReq = (VMMDevVideoAccelEnable *)pReqHdr;
    AssertMsgReturn(pReq->header.size >= sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);  /** @todo Not sure why this >= ... */

    if (!pThis->pDrv)
    {
        Log(("VMMDevReq_VideoAccelEnable Connector is NULL!!\n"));
        return VERR_NOT_SUPPORTED;
    }

    if (pReq->cbRingBuffer != VMMDEV_VBVA_RING_BUFFER_SIZE)
    {
        /* The guest driver seems compiled with different headers. */
        LogRelMax(16,("VMMDevReq_VideoAccelEnable guest ring buffer size %#x, should be %#x!!\n", pReq->cbRingBuffer, VMMDEV_VBVA_RING_BUFFER_SIZE));
        return VERR_INVALID_PARAMETER;
    }

    /* The request is correct. */
    pReq->fu32Status |= VBVA_F_STATUS_ACCEPTED;

    LogFlow(("VMMDevReq_VideoAccelEnable pReq->u32Enable = %d\n", pReq->u32Enable));

    int rc = pReq->u32Enable
           ? pThis->pDrv->pfnVideoAccelEnable(pThis->pDrv, true, &pThis->pVMMDevRAMR3->vbvaMemory)
           : pThis->pDrv->pfnVideoAccelEnable(pThis->pDrv, false, NULL);

    if (   pReq->u32Enable
        && RT_SUCCESS(rc))
    {
        pReq->fu32Status |= VBVA_F_STATUS_ENABLED;

        /* Remember that guest successfully enabled acceleration.
         * We need to reestablish it on restoring the VM from saved state.
         */
        pThis->u32VideoAccelEnabled = 1;
    }
    else
    {
        /* The acceleration was not enabled. Remember that. */
        pThis->u32VideoAccelEnabled = 0;
    }
    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_VideoAccelFlush.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_VideoAccelFlush(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevVideoAccelFlush *pReq = (VMMDevVideoAccelFlush *)pReqHdr;
    AssertMsgReturn(pReq->header.size >= sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);  /** @todo Not sure why this >= ... */

    if (!pThis->pDrv)
    {
        Log(("VMMDevReq_VideoAccelFlush: Connector is NULL!!!\n"));
        return VERR_NOT_SUPPORTED;
    }

    pThis->pDrv->pfnVideoAccelFlush(pThis->pDrv);
    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_VideoSetVisibleRegion.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_VideoSetVisibleRegion(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevVideoSetVisibleRegion *pReq = (VMMDevVideoSetVisibleRegion *)pReqHdr;
    AssertMsgReturn(pReq->header.size + sizeof(RTRECT) >= sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    if (!pThis->pDrv)
    {
        Log(("VMMDevReq_VideoSetVisibleRegion: Connector is NULL!!!\n"));
        return VERR_NOT_SUPPORTED;
    }

    if (   pReq->cRect > _1M /* restrict to sane range */
        || pReq->header.size != sizeof(VMMDevVideoSetVisibleRegion) + pReq->cRect * sizeof(RTRECT) - sizeof(RTRECT))
    {
        Log(("VMMDevReq_VideoSetVisibleRegion: cRects=%#x doesn't match size=%#x or is out of bounds\n",
             pReq->cRect, pReq->header.size));
        return VERR_INVALID_PARAMETER;
    }

    Log(("VMMDevReq_VideoSetVisibleRegion %d rectangles\n", pReq->cRect));
    /* forward the call */
    return pThis->pDrv->pfnSetVisibleRegion(pThis->pDrv, pReq->cRect, &pReq->Rect);
}


/**
 * Handles VMMDevReq_GetSeamlessChangeRequest.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetSeamlessChangeRequest(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevSeamlessChangeRequest *pReq = (VMMDevSeamlessChangeRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* just pass on the information */
    Log(("VMMDev: returning seamless change request mode=%d\n", pThis->fSeamlessEnabled));
    if (pThis->fSeamlessEnabled)
        pReq->mode = VMMDev_Seamless_Visible_Region;
    else
        pReq->mode = VMMDev_Seamless_Disabled;

    if (pReq->eventAck == VMMDEV_EVENT_SEAMLESS_MODE_CHANGE_REQUEST)
    {
        /* Remember which mode the client has queried. */
        pThis->fLastSeamlessEnabled = pThis->fSeamlessEnabled;
    }

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GetVRDPChangeRequest.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetVRDPChangeRequest(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevVRDPChangeRequest *pReq = (VMMDevVRDPChangeRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* just pass on the information */
    Log(("VMMDev: returning VRDP status %d level %d\n", pThis->fVRDPEnabled, pThis->uVRDPExperienceLevel));

    pReq->u8VRDPActive = pThis->fVRDPEnabled;
    pReq->u32VRDPExperienceLevel = pThis->uVRDPExperienceLevel;

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GetMemBalloonChangeRequest.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetMemBalloonChangeRequest(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevGetMemBalloonChangeRequest *pReq = (VMMDevGetMemBalloonChangeRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* just pass on the information */
    Log(("VMMDev: returning memory balloon size =%d\n", pThis->cMbMemoryBalloon));
    pReq->cBalloonChunks = pThis->cMbMemoryBalloon;
    pReq->cPhysMemChunks = pThis->cbGuestRAM / (uint64_t)_1M;

    if (pReq->eventAck == VMMDEV_EVENT_BALLOON_CHANGE_REQUEST)
    {
        /* Remember which mode the client has queried. */
        pThis->cMbMemoryBalloonLast = pThis->cMbMemoryBalloon;
    }

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_ChangeMemBalloon.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_ChangeMemBalloon(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevChangeMemBalloon *pReq = (VMMDevChangeMemBalloon *)pReqHdr;
    AssertMsgReturn(pReq->header.size >= sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);
    AssertMsgReturn(pReq->cPages      == VMMDEV_MEMORY_BALLOON_CHUNK_PAGES, ("%u\n", pReq->cPages), VERR_INVALID_PARAMETER);
    AssertMsgReturn(pReq->header.size == (uint32_t)RT_OFFSETOF(VMMDevChangeMemBalloon, aPhysPage[pReq->cPages]),
                    ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    Log(("VMMDevReq_ChangeMemBalloon\n"));
    int rc = PGMR3PhysChangeMemBalloon(PDMDevHlpGetVM(pThis->pDevIns), !!pReq->fInflate, pReq->cPages, pReq->aPhysPage);
    if (pReq->fInflate)
        STAM_REL_U32_INC(&pThis->StatMemBalloonChunks);
    else
        STAM_REL_U32_DEC(&pThis->StatMemBalloonChunks);
    return rc;
}


/**
 * Handles VMMDevReq_GetStatisticsChangeRequest.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetStatisticsChangeRequest(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevGetStatisticsChangeRequest *pReq = (VMMDevGetStatisticsChangeRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    Log(("VMMDevReq_GetStatisticsChangeRequest\n"));
    /* just pass on the information */
    Log(("VMMDev: returning statistics interval %d seconds\n", pThis->u32StatIntervalSize));
    pReq->u32StatInterval = pThis->u32StatIntervalSize;

    if (pReq->eventAck == VMMDEV_EVENT_STATISTICS_INTERVAL_CHANGE_REQUEST)
    {
        /* Remember which mode the client has queried. */
        pThis->u32LastStatIntervalSize= pThis->u32StatIntervalSize;
    }

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_ReportGuestStats.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_ReportGuestStats(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReportGuestStats *pReq = (VMMDevReportGuestStats *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    Log(("VMMDevReq_ReportGuestStats\n"));
#ifdef LOG_ENABLED
    VBoxGuestStatistics *pGuestStats = &pReq->guestStats;

    Log(("Current statistics:\n"));
    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_CPU_LOAD_IDLE)
        Log(("CPU%u: CPU Load Idle          %-3d%%\n", pGuestStats->u32CpuId, pGuestStats->u32CpuLoad_Idle));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_CPU_LOAD_KERNEL)
        Log(("CPU%u: CPU Load Kernel        %-3d%%\n", pGuestStats->u32CpuId, pGuestStats->u32CpuLoad_Kernel));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_CPU_LOAD_USER)
        Log(("CPU%u: CPU Load User          %-3d%%\n", pGuestStats->u32CpuId, pGuestStats->u32CpuLoad_User));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_THREADS)
        Log(("CPU%u: Thread                 %d\n", pGuestStats->u32CpuId, pGuestStats->u32Threads));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_PROCESSES)
        Log(("CPU%u: Processes              %d\n", pGuestStats->u32CpuId, pGuestStats->u32Processes));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_HANDLES)
        Log(("CPU%u: Handles                %d\n", pGuestStats->u32CpuId, pGuestStats->u32Handles));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_MEMORY_LOAD)
        Log(("CPU%u: Memory Load            %d%%\n", pGuestStats->u32CpuId, pGuestStats->u32MemoryLoad));

    /* Note that reported values are in pages; upper layers expect them in megabytes */
    Log(("CPU%u: Page size              %-4d bytes\n", pGuestStats->u32CpuId, pGuestStats->u32PageSize));
    Assert(pGuestStats->u32PageSize == 4096);

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_PHYS_MEM_TOTAL)
        Log(("CPU%u: Total physical memory  %-4d MB\n", pGuestStats->u32CpuId, (pGuestStats->u32PhysMemTotal + (_1M/_4K)-1) / (_1M/_4K)));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_PHYS_MEM_AVAIL)
        Log(("CPU%u: Free physical memory   %-4d MB\n", pGuestStats->u32CpuId, pGuestStats->u32PhysMemAvail / (_1M/_4K)));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_PHYS_MEM_BALLOON)
        Log(("CPU%u: Memory balloon size    %-4d MB\n", pGuestStats->u32CpuId, pGuestStats->u32PhysMemBalloon / (_1M/_4K)));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_MEM_COMMIT_TOTAL)
        Log(("CPU%u: Committed memory       %-4d MB\n", pGuestStats->u32CpuId, pGuestStats->u32MemCommitTotal / (_1M/_4K)));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_MEM_KERNEL_TOTAL)
        Log(("CPU%u: Total kernel memory    %-4d MB\n", pGuestStats->u32CpuId, pGuestStats->u32MemKernelTotal / (_1M/_4K)));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_MEM_KERNEL_PAGED)
        Log(("CPU%u: Paged kernel memory    %-4d MB\n", pGuestStats->u32CpuId, pGuestStats->u32MemKernelPaged / (_1M/_4K)));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_MEM_KERNEL_NONPAGED)
        Log(("CPU%u: Nonpaged kernel memory %-4d MB\n", pGuestStats->u32CpuId, pGuestStats->u32MemKernelNonPaged / (_1M/_4K)));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_MEM_SYSTEM_CACHE)
        Log(("CPU%u: System cache size      %-4d MB\n", pGuestStats->u32CpuId, pGuestStats->u32MemSystemCache / (_1M/_4K)));

    if (pGuestStats->u32StatCaps & VBOX_GUEST_STAT_PAGE_FILE_SIZE)
        Log(("CPU%u: Page file size         %-4d MB\n", pGuestStats->u32CpuId, pGuestStats->u32PageFileSize / (_1M/_4K)));
    Log(("Statistics end *******************\n"));
#endif /* LOG_ENABLED */

    /* forward the call */
    return pThis->pDrv->pfnReportStatistics(pThis->pDrv, &pReq->guestStats);
}


/**
 * Handles VMMDevReq_QueryCredentials.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_QueryCredentials(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevCredentials *pReq = (VMMDevCredentials *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* let's start by nulling out the data */
    memset(pReq->szUserName, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
    memset(pReq->szPassword, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
    memset(pReq->szDomain, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);

    /* should we return whether we got credentials for a logon? */
    if (pReq->u32Flags & VMMDEV_CREDENTIALS_QUERYPRESENCE)
    {
        if (   pThis->pCredentials->Logon.szUserName[0]
            || pThis->pCredentials->Logon.szPassword[0]
            || pThis->pCredentials->Logon.szDomain[0])
            pReq->u32Flags |= VMMDEV_CREDENTIALS_PRESENT;
        else
            pReq->u32Flags &= ~VMMDEV_CREDENTIALS_PRESENT;
    }

    /* does the guest want to read logon credentials? */
    if (pReq->u32Flags & VMMDEV_CREDENTIALS_READ)
    {
        if (pThis->pCredentials->Logon.szUserName[0])
            strcpy(pReq->szUserName, pThis->pCredentials->Logon.szUserName);
        if (pThis->pCredentials->Logon.szPassword[0])
            strcpy(pReq->szPassword, pThis->pCredentials->Logon.szPassword);
        if (pThis->pCredentials->Logon.szDomain[0])
            strcpy(pReq->szDomain, pThis->pCredentials->Logon.szDomain);
        if (!pThis->pCredentials->Logon.fAllowInteractiveLogon)
            pReq->u32Flags |= VMMDEV_CREDENTIALS_NOLOCALLOGON;
        else
            pReq->u32Flags &= ~VMMDEV_CREDENTIALS_NOLOCALLOGON;
    }

    if (!pThis->fKeepCredentials)
    {
        /* does the caller want us to destroy the logon credentials? */
        if (pReq->u32Flags & VMMDEV_CREDENTIALS_CLEAR)
        {
            memset(pThis->pCredentials->Logon.szUserName, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
            memset(pThis->pCredentials->Logon.szPassword, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
            memset(pThis->pCredentials->Logon.szDomain, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
        }
    }

    /* does the guest want to read credentials for verification? */
    if (pReq->u32Flags & VMMDEV_CREDENTIALS_READJUDGE)
    {
        if (pThis->pCredentials->Judge.szUserName[0])
            strcpy(pReq->szUserName, pThis->pCredentials->Judge.szUserName);
        if (pThis->pCredentials->Judge.szPassword[0])
            strcpy(pReq->szPassword, pThis->pCredentials->Judge.szPassword);
        if (pThis->pCredentials->Judge.szDomain[0])
            strcpy(pReq->szDomain, pThis->pCredentials->Judge.szDomain);
    }

    /* does the caller want us to destroy the judgement credentials? */
    if (pReq->u32Flags & VMMDEV_CREDENTIALS_CLEARJUDGE)
    {
        memset(pThis->pCredentials->Judge.szUserName, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
        memset(pThis->pCredentials->Judge.szPassword, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
        memset(pThis->pCredentials->Judge.szDomain, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
    }

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_ReportCredentialsJudgement.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_ReportCredentialsJudgement(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevCredentials *pReq = (VMMDevCredentials *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /* what does the guest think about the credentials? (note: the order is important here!) */
    if (pReq->u32Flags & VMMDEV_CREDENTIALS_JUDGE_DENY)
        pThis->pDrv->pfnSetCredentialsJudgementResult(pThis->pDrv, VMMDEV_CREDENTIALS_JUDGE_DENY);
    else if (pReq->u32Flags & VMMDEV_CREDENTIALS_JUDGE_NOJUDGEMENT)
        pThis->pDrv->pfnSetCredentialsJudgementResult(pThis->pDrv, VMMDEV_CREDENTIALS_JUDGE_NOJUDGEMENT);
    else if (pReq->u32Flags & VMMDEV_CREDENTIALS_JUDGE_OK)
        pThis->pDrv->pfnSetCredentialsJudgementResult(pThis->pDrv, VMMDEV_CREDENTIALS_JUDGE_OK);
    else
    {
        Log(("VMMDevReq_ReportCredentialsJudgement: invalid flags: %d!!!\n", pReq->u32Flags));
        /** @todo why don't we return VERR_INVALID_PARAMETER to the guest? */
    }

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GetHostVersion.
 *
 * @returns VBox status code that the guest should see.
 * @param   pReqHdr         The header of the request to handle.
 * @since   3.1.0
 * @note    The ring-0 VBoxGuestLib uses this to check whether
 *          VMMDevHGCMParmType_PageList is supported.
 */
static int vmmdevReqHandler_GetHostVersion(VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqHostVersion *pReq = (VMMDevReqHostVersion *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    pReq->major     = RTBldCfgVersionMajor();
    pReq->minor     = RTBldCfgVersionMinor();
    pReq->build     = RTBldCfgVersionBuild();
    pReq->revision  = RTBldCfgRevision();
    pReq->features  = VMMDEV_HVF_HGCM_PHYS_PAGE_LIST;
    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_GetCpuHotPlugRequest.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetCpuHotPlugRequest(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevGetCpuHotPlugRequest *pReq = (VMMDevGetCpuHotPlugRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    pReq->enmEventType  = pThis->enmCpuHotPlugEvent;
    pReq->idCpuCore     = pThis->idCpuCore;
    pReq->idCpuPackage  = pThis->idCpuPackage;

    /* Clear the event */
    pThis->enmCpuHotPlugEvent = VMMDevCpuEventType_None;
    pThis->idCpuCore          = UINT32_MAX;
    pThis->idCpuPackage       = UINT32_MAX;

    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_SetCpuHotPlugStatus.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_SetCpuHotPlugStatus(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevCpuHotPlugStatusRequest *pReq = (VMMDevCpuHotPlugStatusRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    if (pReq->enmStatusType == VMMDevCpuStatusType_Disable)
        pThis->fCpuHotPlugEventsEnabled = false;
    else if (pReq->enmStatusType == VMMDevCpuStatusType_Enable)
        pThis->fCpuHotPlugEventsEnabled = true;
    else
        return VERR_INVALID_PARAMETER;
    return VINF_SUCCESS;
}


#ifdef DEBUG
/**
 * Handles VMMDevReq_LogString.
 *
 * @returns VBox status code that the guest should see.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_LogString(VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqLogString *pReq = (VMMDevReqLogString *)pReqHdr;
    AssertMsgReturn(pReq->header.size >= sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);
    AssertMsgReturn(pReq->szString[pReq->header.size - RT_OFFSETOF(VMMDevReqLogString, szString) - 1] == '\0',
                    ("not null terminated\n"), VERR_INVALID_PARAMETER);

    LogIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP_DEV_VMM_BACKDOOR, ("DEBUG LOG: %s", pReq->szString));
    return VINF_SUCCESS;
}
#endif /* DEBUG */

/**
 * Handles VMMDevReq_GetSessionId.
 *
 * Get a unique "session" ID for this VM, where the ID will be different after each
 * start, reset or restore of the VM.  This can be used for restore detection
 * inside the guest.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetSessionId(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqSessionId *pReq = (VMMDevReqSessionId *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(*pReq), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    pReq->idSession = pThis->idSession;
    return VINF_SUCCESS;
}


#ifdef VBOX_WITH_PAGE_SHARING

/**
 * Handles VMMDevReq_RegisterSharedModule.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_RegisterSharedModule(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    /*
     * Basic input validation (more done by GMM).
     */
    VMMDevSharedModuleRegistrationRequest *pReq = (VMMDevSharedModuleRegistrationRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size >= sizeof(VMMDevSharedModuleRegistrationRequest),
                    ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);
    AssertMsgReturn(pReq->header.size == RT_UOFFSETOF(VMMDevSharedModuleRegistrationRequest, aRegions[pReq->cRegions]),
                    ("%u cRegions=%u\n", pReq->header.size, pReq->cRegions), VERR_INVALID_PARAMETER);

    AssertReturn(RTStrEnd(pReq->szName, sizeof(pReq->szName)), VERR_INVALID_PARAMETER);
    AssertReturn(RTStrEnd(pReq->szVersion, sizeof(pReq->szVersion)), VERR_INVALID_PARAMETER);
    int rc = RTStrValidateEncoding(pReq->szName);
    AssertRCReturn(rc, rc);
    rc = RTStrValidateEncoding(pReq->szVersion);
    AssertRCReturn(rc, rc);

    /*
     * Forward the request to the VMM.
     */
    return PGMR3SharedModuleRegister(PDMDevHlpGetVM(pThis->pDevIns), pReq->enmGuestOS, pReq->szName, pReq->szVersion,
                                     pReq->GCBaseAddr, pReq->cbModule, pReq->cRegions, pReq->aRegions);
}

/**
 * Handles VMMDevReq_UnregisterSharedModule.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_UnregisterSharedModule(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    /*
     * Basic input validation.
     */
    VMMDevSharedModuleUnregistrationRequest *pReq = (VMMDevSharedModuleUnregistrationRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(VMMDevSharedModuleUnregistrationRequest),
                    ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    AssertReturn(RTStrEnd(pReq->szName, sizeof(pReq->szName)), VERR_INVALID_PARAMETER);
    AssertReturn(RTStrEnd(pReq->szVersion, sizeof(pReq->szVersion)), VERR_INVALID_PARAMETER);
    int rc = RTStrValidateEncoding(pReq->szName);
    AssertRCReturn(rc, rc);
    rc = RTStrValidateEncoding(pReq->szVersion);
    AssertRCReturn(rc, rc);

    /*
     * Forward the request to the VMM.
     */
    return PGMR3SharedModuleUnregister(PDMDevHlpGetVM(pThis->pDevIns), pReq->szName, pReq->szVersion,
                                       pReq->GCBaseAddr, pReq->cbModule);
}

/**
 * Handles VMMDevReq_CheckSharedModules.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_CheckSharedModules(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevSharedModuleCheckRequest *pReq = (VMMDevSharedModuleCheckRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(VMMDevSharedModuleCheckRequest),
                    ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);
    return PGMR3SharedModuleCheckAll(PDMDevHlpGetVM(pThis->pDevIns));
}

/**
 * Handles VMMDevReq_GetPageSharingStatus.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_GetPageSharingStatus(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevPageSharingStatusRequest *pReq = (VMMDevPageSharingStatusRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(VMMDevPageSharingStatusRequest),
                    ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    pReq->fEnabled = false;
    int rc = pThis->pDrv->pfnIsPageFusionEnabled(pThis->pDrv, &pReq->fEnabled);
    if (RT_FAILURE(rc))
        pReq->fEnabled = false;
    return VINF_SUCCESS;
}


/**
 * Handles VMMDevReq_DebugIsPageShared.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         The header of the request to handle.
 */
static int vmmdevReqHandler_DebugIsPageShared(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevPageIsSharedRequest *pReq = (VMMDevPageIsSharedRequest *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(VMMDevPageIsSharedRequest),
                    ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

# ifdef DEBUG
    return PGMR3SharedModuleGetPageState(PDMDevHlpGetVM(pThis->pDevIns), pReq->GCPtrPage, &pReq->fShared, &pReq->uPageFlags);
# else
    RT_NOREF1(pThis);
    return VERR_NOT_IMPLEMENTED;
# endif
}

#endif /* VBOX_WITH_PAGE_SHARING */


/**
 * Handles VMMDevReq_WriteCoreDumpe
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pReqHdr         Pointer to the request header.
 */
static int vmmdevReqHandler_WriteCoreDump(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr)
{
    VMMDevReqWriteCoreDump *pReq = (VMMDevReqWriteCoreDump *)pReqHdr;
    AssertMsgReturn(pReq->header.size == sizeof(VMMDevReqWriteCoreDump), ("%u\n", pReq->header.size), VERR_INVALID_PARAMETER);

    /*
     * Only available if explicitly enabled by the user.
     */
    if (!pThis->fGuestCoreDumpEnabled)
        return VERR_ACCESS_DENIED;

    /*
     * User makes sure the directory exists before composing the path.
     */
    if (!RTDirExists(pThis->szGuestCoreDumpDir))
        return VERR_PATH_NOT_FOUND;

    char szCorePath[RTPATH_MAX];
    RTStrCopy(szCorePath, sizeof(szCorePath), pThis->szGuestCoreDumpDir);
    RTPathAppend(szCorePath, sizeof(szCorePath), "VBox.core");

    /*
     * Rotate existing cores based on number of additional cores to keep around.
     */
    if (pThis->cGuestCoreDumps > 0)
        for (int64_t i = pThis->cGuestCoreDumps - 1; i >= 0; i--)
        {
            char szFilePathOld[RTPATH_MAX];
            if (i == 0)
                RTStrCopy(szFilePathOld, sizeof(szFilePathOld), szCorePath);
            else
                RTStrPrintf(szFilePathOld, sizeof(szFilePathOld), "%s.%lld", szCorePath, i);

            char szFilePathNew[RTPATH_MAX];
            RTStrPrintf(szFilePathNew, sizeof(szFilePathNew), "%s.%lld", szCorePath, i + 1);
            int vrc = RTFileMove(szFilePathOld, szFilePathNew, RTFILEMOVE_FLAGS_REPLACE);
            if (vrc == VERR_FILE_NOT_FOUND)
                RTFileDelete(szFilePathNew);
        }

    /*
     * Write the core file.
     */
    PUVM pUVM = PDMDevHlpGetUVM(pThis->pDevIns);
    return DBGFR3CoreWrite(pUVM, szCorePath, true /*fReplaceFile*/);
}


/**
 * Dispatch the request to the appropriate handler function.
 *
 * @returns Port I/O handler exit code.
 * @param   pThis           The VMM device instance data.
 * @param   pReqHdr         The request header (cached in host memory).
 * @param   GCPhysReqHdr    The guest physical address of the request (for
 *                          HGCM).
 * @param   pfDelayedUnlock Where to indicate whether the critical section exit
 *                          needs to be delayed till after the request has been
 *                          written back. This is a HGCM kludge, see critsect
 *                          work in hgcmCompletedWorker for more details.
 */
static int vmmdevReqDispatcher(PVMMDEV pThis, VMMDevRequestHeader *pReqHdr, RTGCPHYS GCPhysReqHdr, bool *pfDelayedUnlock)
{
    int rcRet = VINF_SUCCESS;
    *pfDelayedUnlock = false;

    switch (pReqHdr->requestType)
    {
        case VMMDevReq_ReportGuestInfo:
            pReqHdr->rc = vmmdevReqHandler_ReportGuestInfo(pThis, pReqHdr);
            break;

        case VMMDevReq_ReportGuestInfo2:
            pReqHdr->rc = vmmdevReqHandler_ReportGuestInfo2(pThis, pReqHdr);
            break;

        case VMMDevReq_ReportGuestStatus:
            pReqHdr->rc = vmmdevReqHandler_ReportGuestStatus(pThis, pReqHdr);
            break;

        case VMMDevReq_ReportGuestUserState:
            pReqHdr->rc = vmmdevReqHandler_ReportGuestUserState(pThis, pReqHdr);
            break;

        case VMMDevReq_ReportGuestCapabilities:
            pReqHdr->rc = vmmdevReqHandler_ReportGuestCapabilities(pThis, pReqHdr);
            break;

        case VMMDevReq_SetGuestCapabilities:
            pReqHdr->rc = vmmdevReqHandler_SetGuestCapabilities(pThis, pReqHdr);
            break;

        case VMMDevReq_WriteCoreDump:
            pReqHdr->rc = vmmdevReqHandler_WriteCoreDump(pThis, pReqHdr);
            break;

        case VMMDevReq_GetMouseStatus:
            pReqHdr->rc = vmmdevReqHandler_GetMouseStatus(pThis, pReqHdr);
            break;

        case VMMDevReq_SetMouseStatus:
            pReqHdr->rc = vmmdevReqHandler_SetMouseStatus(pThis, pReqHdr);
            break;

        case VMMDevReq_SetPointerShape:
            pReqHdr->rc = vmmdevReqHandler_SetPointerShape(pThis, pReqHdr);
            break;

        case VMMDevReq_GetHostTime:
            pReqHdr->rc = vmmdevReqHandler_GetHostTime(pThis, pReqHdr);
            break;

        case VMMDevReq_GetHypervisorInfo:
            pReqHdr->rc = vmmdevReqHandler_GetHypervisorInfo(pThis, pReqHdr);
            break;

        case VMMDevReq_SetHypervisorInfo:
            pReqHdr->rc = vmmdevReqHandler_SetHypervisorInfo(pThis, pReqHdr);
            break;

        case VMMDevReq_RegisterPatchMemory:
            pReqHdr->rc = vmmdevReqHandler_RegisterPatchMemory(pThis, pReqHdr);
            break;

        case VMMDevReq_DeregisterPatchMemory:
            pReqHdr->rc = vmmdevReqHandler_DeregisterPatchMemory(pThis, pReqHdr);
            break;

        case VMMDevReq_SetPowerStatus:
        {
            int rc = pReqHdr->rc = vmmdevReqHandler_SetPowerStatus(pThis, pReqHdr);
            if (rc != VINF_SUCCESS && RT_SUCCESS(rc))
                rcRet = rc;
            break;
        }

        case VMMDevReq_GetDisplayChangeRequest:
            pReqHdr->rc = vmmdevReqHandler_GetDisplayChangeRequest(pThis, pReqHdr);
            break;

        case VMMDevReq_GetDisplayChangeRequest2:
            pReqHdr->rc = vmmdevReqHandler_GetDisplayChangeRequest2(pThis, pReqHdr);
            break;

        case VMMDevReq_GetDisplayChangeRequestEx:
            pReqHdr->rc = vmmdevReqHandler_GetDisplayChangeRequestEx(pThis, pReqHdr);
            break;

        case VMMDevReq_VideoModeSupported:
            pReqHdr->rc = vmmdevReqHandler_VideoModeSupported(pThis, pReqHdr);
            break;

        case VMMDevReq_VideoModeSupported2:
            pReqHdr->rc = vmmdevReqHandler_VideoModeSupported2(pThis, pReqHdr);
            break;

        case VMMDevReq_GetHeightReduction:
            pReqHdr->rc = vmmdevReqHandler_GetHeightReduction(pThis, pReqHdr);
            break;

        case VMMDevReq_AcknowledgeEvents:
            pReqHdr->rc = vmmdevReqHandler_AcknowledgeEvents(pThis, pReqHdr);
            break;

        case VMMDevReq_CtlGuestFilterMask:
            pReqHdr->rc = vmmdevReqHandler_CtlGuestFilterMask(pThis, pReqHdr);
            break;

#ifdef VBOX_WITH_HGCM
        case VMMDevReq_HGCMConnect:
            pReqHdr->rc = vmmdevReqHandler_HGCMConnect(pThis, pReqHdr, GCPhysReqHdr);
            *pfDelayedUnlock = true;
            break;

        case VMMDevReq_HGCMDisconnect:
            pReqHdr->rc = vmmdevReqHandler_HGCMDisconnect(pThis, pReqHdr, GCPhysReqHdr);
            *pfDelayedUnlock = true;
            break;

# ifdef VBOX_WITH_64_BITS_GUESTS
        case VMMDevReq_HGCMCall32:
        case VMMDevReq_HGCMCall64:
# else
        case VMMDevReq_HGCMCall:
# endif /* VBOX_WITH_64_BITS_GUESTS */
            pReqHdr->rc = vmmdevReqHandler_HGCMCall(pThis, pReqHdr, GCPhysReqHdr);
            *pfDelayedUnlock = true;
            break;

        case VMMDevReq_HGCMCancel:
            pReqHdr->rc = vmmdevReqHandler_HGCMCancel(pThis, pReqHdr, GCPhysReqHdr);
            *pfDelayedUnlock = true;
            break;

        case VMMDevReq_HGCMCancel2:
            pReqHdr->rc = vmmdevReqHandler_HGCMCancel2(pThis, pReqHdr);
            break;
#endif /* VBOX_WITH_HGCM */

        case VMMDevReq_VideoAccelEnable:
            pReqHdr->rc = vmmdevReqHandler_VideoAccelEnable(pThis, pReqHdr);
            break;

        case VMMDevReq_VideoAccelFlush:
            pReqHdr->rc = vmmdevReqHandler_VideoAccelFlush(pThis, pReqHdr);
            break;

        case VMMDevReq_VideoSetVisibleRegion:
            pReqHdr->rc = vmmdevReqHandler_VideoSetVisibleRegion(pThis, pReqHdr);
            break;

        case VMMDevReq_GetSeamlessChangeRequest:
            pReqHdr->rc = vmmdevReqHandler_GetSeamlessChangeRequest(pThis, pReqHdr);
            break;

        case VMMDevReq_GetVRDPChangeRequest:
            pReqHdr->rc = vmmdevReqHandler_GetVRDPChangeRequest(pThis, pReqHdr);
            break;

        case VMMDevReq_GetMemBalloonChangeRequest:
            pReqHdr->rc = vmmdevReqHandler_GetMemBalloonChangeRequest(pThis, pReqHdr);
            break;

        case VMMDevReq_ChangeMemBalloon:
            pReqHdr->rc = vmmdevReqHandler_ChangeMemBalloon(pThis, pReqHdr);
            break;

        case VMMDevReq_GetStatisticsChangeRequest:
            pReqHdr->rc = vmmdevReqHandler_GetStatisticsChangeRequest(pThis, pReqHdr);
            break;

        case VMMDevReq_ReportGuestStats:
            pReqHdr->rc = vmmdevReqHandler_ReportGuestStats(pThis, pReqHdr);
            break;

        case VMMDevReq_QueryCredentials:
            pReqHdr->rc = vmmdevReqHandler_QueryCredentials(pThis, pReqHdr);
            break;

        case VMMDevReq_ReportCredentialsJudgement:
            pReqHdr->rc = vmmdevReqHandler_ReportCredentialsJudgement(pThis, pReqHdr);
            break;

        case VMMDevReq_GetHostVersion:
            pReqHdr->rc = vmmdevReqHandler_GetHostVersion(pReqHdr);
            break;

        case VMMDevReq_GetCpuHotPlugRequest:
            pReqHdr->rc = vmmdevReqHandler_GetCpuHotPlugRequest(pThis, pReqHdr);
            break;

        case VMMDevReq_SetCpuHotPlugStatus:
            pReqHdr->rc = vmmdevReqHandler_SetCpuHotPlugStatus(pThis, pReqHdr);
            break;

#ifdef VBOX_WITH_PAGE_SHARING
        case VMMDevReq_RegisterSharedModule:
            pReqHdr->rc = vmmdevReqHandler_RegisterSharedModule(pThis, pReqHdr);
            break;

        case VMMDevReq_UnregisterSharedModule:
            pReqHdr->rc = vmmdevReqHandler_UnregisterSharedModule(pThis, pReqHdr);
            break;

        case VMMDevReq_CheckSharedModules:
            pReqHdr->rc = vmmdevReqHandler_CheckSharedModules(pThis, pReqHdr);
            break;

        case VMMDevReq_GetPageSharingStatus:
            pReqHdr->rc = vmmdevReqHandler_GetPageSharingStatus(pThis, pReqHdr);
            break;

        case VMMDevReq_DebugIsPageShared:
            pReqHdr->rc = vmmdevReqHandler_DebugIsPageShared(pThis, pReqHdr);
            break;

#endif /* VBOX_WITH_PAGE_SHARING */

#ifdef DEBUG
        case VMMDevReq_LogString:
            pReqHdr->rc = vmmdevReqHandler_LogString(pReqHdr);
            break;
#endif

        case VMMDevReq_GetSessionId:
            pReqHdr->rc = vmmdevReqHandler_GetSessionId(pThis, pReqHdr);
            break;

        /*
         * Guest wants to give up a timeslice.
         * Note! This was only ever used by experimental GAs!
         */
        /** @todo maybe we could just remove this? */
        case VMMDevReq_Idle:
        {
            /* just return to EMT telling it that we want to halt */
            rcRet = VINF_EM_HALT;
            break;
        }

        case VMMDevReq_GuestHeartbeat:
            pReqHdr->rc = vmmDevReqHandler_GuestHeartbeat(pThis);
            break;

        case VMMDevReq_HeartbeatConfigure:
            pReqHdr->rc = vmmDevReqHandler_HeartbeatConfigure(pThis, pReqHdr);
            break;

        default:
        {
            pReqHdr->rc = VERR_NOT_IMPLEMENTED;
            Log(("VMMDev unknown request type %d\n", pReqHdr->requestType));
            break;
        }
    }
    return rcRet;
}


/**
 * @callback_method_impl{FNIOMIOPORTOUT, Port I/O Handler for the generic
 *                      request interface.}
 */
static DECLCALLBACK(int) vmmdevRequestHandler(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    RT_NOREF2(Port, cb);
    PVMMDEV pThis = (VMMDevState*)pvUser;

    /*
     * The caller has passed the guest context physical address of the request
     * structure. We'll copy all of it into a heap buffer eventually, but we
     * will have to start off with the header.
     */
    VMMDevRequestHeader requestHeader;
    RT_ZERO(requestHeader);
    PDMDevHlpPhysRead(pDevIns, (RTGCPHYS)u32, &requestHeader, sizeof(requestHeader));

    /* The structure size must be greater or equal to the header size. */
    if (requestHeader.size < sizeof(VMMDevRequestHeader))
    {
        Log(("VMMDev request header size too small! size = %d\n", requestHeader.size));
        return VINF_SUCCESS;
    }

    /* Check the version of the header structure. */
    if (requestHeader.version != VMMDEV_REQUEST_HEADER_VERSION)
    {
        Log(("VMMDev: guest header version (0x%08X) differs from ours (0x%08X)\n", requestHeader.version, VMMDEV_REQUEST_HEADER_VERSION));
        return VINF_SUCCESS;
    }

    Log2(("VMMDev request issued: %d\n", requestHeader.requestType));

    int                  rcRet          = VINF_SUCCESS;
    bool                 fDelayedUnlock = false;
    VMMDevRequestHeader *pRequestHeader = NULL;

    /* Check that is doesn't exceed the max packet size. */
    if (requestHeader.size <= VMMDEV_MAX_VMMDEVREQ_SIZE)
    {
        /*
         * We require the GAs to report it's information before we let it have
         * access to all the functions.  The VMMDevReq_ReportGuestInfo request
         * is the one which unlocks the access.  Newer additions will first
         * issue VMMDevReq_ReportGuestInfo2, older ones doesn't know this one.
         * Two exceptions: VMMDevReq_GetHostVersion and VMMDevReq_WriteCoreDump.
         */
        if (   pThis->fu32AdditionsOk
            || requestHeader.requestType == VMMDevReq_ReportGuestInfo2
            || requestHeader.requestType == VMMDevReq_ReportGuestInfo
            || requestHeader.requestType == VMMDevReq_WriteCoreDump
            || requestHeader.requestType == VMMDevReq_GetHostVersion
           )
        {
            /*
             * The request looks fine. Allocate a heap block for it, read the
             * entire package from guest memory and feed it to the dispatcher.
             */
            pRequestHeader = (VMMDevRequestHeader *)RTMemAlloc(requestHeader.size);
            if (pRequestHeader)
            {
                memcpy(pRequestHeader, &requestHeader, sizeof(VMMDevRequestHeader));
                size_t cbLeft = requestHeader.size - sizeof(VMMDevRequestHeader);
                if (cbLeft)
                    PDMDevHlpPhysRead(pDevIns,
                                      (RTGCPHYS)u32             + sizeof(VMMDevRequestHeader),
                                      (uint8_t *)pRequestHeader + sizeof(VMMDevRequestHeader),
                                      cbLeft);

                PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);
                rcRet = vmmdevReqDispatcher(pThis, pRequestHeader, u32, &fDelayedUnlock);
                if (!fDelayedUnlock)
                    PDMCritSectLeave(&pThis->CritSect);
            }
            else
            {
                Log(("VMMDev: RTMemAlloc failed!\n"));
                requestHeader.rc = VERR_NO_MEMORY;
            }
        }
        else
        {
            LogRelMax(10, ("VMMDev: Guest has not yet reported to us -- refusing operation of request #%d\n",
                           requestHeader.requestType));
            requestHeader.rc = VERR_NOT_SUPPORTED;
        }
    }
    else
    {
        LogRelMax(50, ("VMMDev: Request packet too big (%x), refusing operation\n", requestHeader.size));
        requestHeader.rc = VERR_NOT_SUPPORTED;
    }

    /*
     * Write the result back to guest memory
     */
    if (pRequestHeader)
    {
        PDMDevHlpPhysWrite(pDevIns, u32, pRequestHeader, pRequestHeader->size);
        if (fDelayedUnlock)
            PDMCritSectLeave(&pThis->CritSect);
        RTMemFree(pRequestHeader);
    }
    else
    {
        /* early error case; write back header only */
        PDMDevHlpPhysWrite(pDevIns, u32, &requestHeader, sizeof(requestHeader));
        Assert(!fDelayedUnlock);
    }

    return rcRet;
}


/* -=-=-=-=-=- PCI Device -=-=-=-=-=- */


/**
 * @callback_method_impl{FNPCIIOREGIONMAP,MMIO/MMIO2 regions}
 */
static DECLCALLBACK(int) vmmdevIORAMRegionMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                              RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    RT_NOREF1(cb);
    LogFlow(("vmmdevR3IORAMRegionMap: iRegion=%d GCPhysAddress=%RGp cb=%RGp enmType=%d\n", iRegion, GCPhysAddress, cb, enmType));
    PVMMDEV pThis = RT_FROM_MEMBER(pPciDev, VMMDEV, PciDev);
    int rc;

    if (iRegion == 1)
    {
        AssertReturn(enmType == PCI_ADDRESS_SPACE_MEM, VERR_INTERNAL_ERROR);
        Assert(pThis->pVMMDevRAMR3 != NULL);
        if (GCPhysAddress != NIL_RTGCPHYS)
        {
            /*
             * Map the MMIO2 memory.
             */
            pThis->GCPhysVMMDevRAM = GCPhysAddress;
            Assert(pThis->GCPhysVMMDevRAM == GCPhysAddress);
            rc = PDMDevHlpMMIOExMap(pDevIns, pPciDev, iRegion, GCPhysAddress);
        }
        else
        {
            /*
             * It is about to be unmapped, just clean up.
             */
            pThis->GCPhysVMMDevRAM = NIL_RTGCPHYS32;
            rc = VINF_SUCCESS;
        }
    }
    else if (iRegion == 2)
    {
        AssertReturn(enmType == PCI_ADDRESS_SPACE_MEM_PREFETCH, VERR_INTERNAL_ERROR);
        Assert(pThis->pVMMDevHeapR3 != NULL);
        if (GCPhysAddress != NIL_RTGCPHYS)
        {
            /*
             * Map the MMIO2 memory.
             */
            pThis->GCPhysVMMDevHeap = GCPhysAddress;
            Assert(pThis->GCPhysVMMDevHeap == GCPhysAddress);
            rc = PDMDevHlpMMIOExMap(pDevIns, pPciDev, iRegion, GCPhysAddress);
            if (RT_SUCCESS(rc))
                rc = PDMDevHlpRegisterVMMDevHeap(pDevIns, GCPhysAddress, pThis->pVMMDevHeapR3, VMMDEV_HEAP_SIZE);
        }
        else
        {
            /*
             * It is about to be unmapped, just clean up.
             */
            PDMDevHlpRegisterVMMDevHeap(pDevIns, NIL_RTGCPHYS, pThis->pVMMDevHeapR3, VMMDEV_HEAP_SIZE);
            pThis->GCPhysVMMDevHeap = NIL_RTGCPHYS32;
            rc = VINF_SUCCESS;
        }
    }
    else
    {
        AssertMsgFailed(("%d\n", iRegion));
        rc = VERR_INVALID_PARAMETER;
    }

    return rc;
}


/**
 * @callback_method_impl{FNPCIIOREGIONMAP,I/O Port Region}
 */
static DECLCALLBACK(int) vmmdevIOPortRegionMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                               RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    RT_NOREF3(iRegion, cb, enmType);
    PVMMDEV pThis = RT_FROM_MEMBER(pPciDev, VMMDEV, PciDev);

    Assert(enmType == PCI_ADDRESS_SPACE_IO);
    Assert(iRegion == 0);
    AssertMsg(RT_ALIGN(GCPhysAddress, 8) == GCPhysAddress, ("Expected 8 byte alignment. GCPhysAddress=%#x\n", GCPhysAddress));

    /*
     * Register our port IO handlers.
     */
    int rc = PDMDevHlpIOPortRegister(pDevIns, (RTIOPORT)GCPhysAddress + VMMDEV_PORT_OFF_REQUEST, 1,
                                     pThis, vmmdevRequestHandler, NULL, NULL, NULL, "VMMDev Request Handler");
    AssertRC(rc);
    return rc;
}



/* -=-=-=-=-=- Backdoor Logging and Time Sync. -=-=-=-=-=- */

/**
 * @callback_method_impl{FNIOMIOPORTOUT, Backdoor Logging.}
 */
static DECLCALLBACK(int) vmmdevBackdoorLog(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    RT_NOREF1(pvUser);
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, VMMDevState *);

    if (!pThis->fBackdoorLogDisabled && cb == 1 && Port == RTLOG_DEBUG_PORT)
    {

        /* The raw version. */
        switch (u32)
        {
            case '\r': LogIt(RTLOGGRPFLAGS_LEVEL_2, LOG_GROUP_DEV_VMM_BACKDOOR, ("vmmdev: <return>\n")); break;
            case '\n': LogIt(RTLOGGRPFLAGS_LEVEL_2, LOG_GROUP_DEV_VMM_BACKDOOR, ("vmmdev: <newline>\n")); break;
            case '\t': LogIt(RTLOGGRPFLAGS_LEVEL_2, LOG_GROUP_DEV_VMM_BACKDOOR, ("vmmdev: <tab>\n")); break;
            default:   LogIt(RTLOGGRPFLAGS_LEVEL_2, LOG_GROUP_DEV_VMM_BACKDOOR, ("vmmdev: %c (%02x)\n", u32, u32)); break;
        }

        /* The readable, buffered version. */
        if (u32 == '\n' || u32 == '\r')
        {
            pThis->szMsg[pThis->iMsg] = '\0';
            if (pThis->iMsg)
                LogRelIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP_DEV_VMM_BACKDOOR, ("VMMDev: Guest Log: %s\n", pThis->szMsg));
            pThis->iMsg = 0;
        }
        else
        {
            if (pThis->iMsg >= sizeof(pThis->szMsg)-1)
            {
                pThis->szMsg[pThis->iMsg] = '\0';
                LogRelIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP_DEV_VMM_BACKDOOR, ("VMMDev: Guest Log: %s\n", pThis->szMsg));
                pThis->iMsg = 0;
            }
            pThis->szMsg[pThis->iMsg] = (char )u32;
            pThis->szMsg[++pThis->iMsg] = '\0';
        }
    }
    return VINF_SUCCESS;
}

#ifdef VMMDEV_WITH_ALT_TIMESYNC

/**
 * @callback_method_impl{FNIOMIOPORTOUT, Alternative time synchronization.}
 */
static DECLCALLBACK(int) vmmdevAltTimeSyncWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    RT_NOREF2(pvUser, Port);
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, VMMDevState *);
    if (cb == 4)
    {
        /* Selects high (0) or low (1) DWORD. The high has to be read first. */
        switch (u32)
        {
            case 0:
                pThis->fTimesyncBackdoorLo = false;
                break;
            case 1:
                pThis->fTimesyncBackdoorLo = true;
                break;
            default:
                Log(("vmmdevAltTimeSyncWrite: Invalid access cb=%#x u32=%#x\n", cb, u32));
                break;
        }
    }
    else
        Log(("vmmdevAltTimeSyncWrite: Invalid access cb=%#x u32=%#x\n", cb, u32));
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNIOMIOPORTOUT, Alternative time synchronization.}
 */
static DECLCALLBACK(int) vmmdevAltTimeSyncRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    RT_NOREF2(pvUser, Port);
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, VMMDevState *);
    int     rc;
    if (cb == 4)
    {
        if (pThis->fTimesyncBackdoorLo)
            *pu32 = (uint32_t)pThis->hostTime;
        else
        {
            /* Reading the high dword gets and saves the current time. */
            RTTIMESPEC Now;
            pThis->hostTime = RTTimeSpecGetMilli(PDMDevHlpTMUtcNow(pDevIns, &Now));
            *pu32 = (uint32_t)(pThis->hostTime >> 32);
        }
        rc = VINF_SUCCESS;
    }
    else
    {
        Log(("vmmdevAltTimeSyncRead: Invalid access cb=%#x\n", cb));
        rc = VERR_IOM_IOPORT_UNUSED;
    }
    return rc;
}

#endif /* VMMDEV_WITH_ALT_TIMESYNC */


/* -=-=-=-=-=- IBase -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) vmmdevPortQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IBase);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIVMMDEVPORT, &pThis->IPort);
#ifdef VBOX_WITH_HGCM
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHGCMPORT, &pThis->IHGCMPort);
#endif
    /* Currently only for shared folders. */
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDPORTS, &pThis->SharedFolders.ILeds);
    return NULL;
}


/* -=-=-=-=-=- ILeds -=-=-=-=-=- */

/**
 * Gets the pointer to the status LED of a unit.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   iLUN            The unit which status LED we desire.
 * @param   ppLed           Where to store the LED pointer.
 */
static DECLCALLBACK(int) vmmdevQueryStatusLed(PPDMILEDPORTS pInterface, unsigned iLUN, PPDMLED *ppLed)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, SharedFolders.ILeds);
    if (iLUN == 0) /* LUN 0 is shared folders */
    {
        *ppLed = &pThis->SharedFolders.Led;
        return VINF_SUCCESS;
    }
    return VERR_PDM_LUN_NOT_FOUND;
}


/* -=-=-=-=-=- PDMIVMMDEVPORT (VMMDEV::IPort) -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnQueryAbsoluteMouse}
 */
static DECLCALLBACK(int) vmmdevIPort_QueryAbsoluteMouse(PPDMIVMMDEVPORT pInterface, int32_t *pxAbs, int32_t *pyAbs)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);

    /** @todo at the first sign of trouble in this area, just enter the critsect.
     * As indicated by the comment below, the atomic reads serves no real purpose
     * here since we can assume cache coherency protocoles and int32_t alignment
     * rules making sure we won't see a halfwritten value. */
    if (pxAbs)
        *pxAbs = ASMAtomicReadS32(&pThis->mouseXAbs); /* why the atomic read? */
    if (pyAbs)
        *pyAbs = ASMAtomicReadS32(&pThis->mouseYAbs);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnSetAbsoluteMouse}
 */
static DECLCALLBACK(int) vmmdevIPort_SetAbsoluteMouse(PPDMIVMMDEVPORT pInterface, int32_t xAbs, int32_t yAbs)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    if (   pThis->mouseXAbs != xAbs
        || pThis->mouseYAbs != yAbs)
    {
        Log2(("vmmdevIPort_SetAbsoluteMouse : settings absolute position to x = %d, y = %d\n", xAbs, yAbs));
        pThis->mouseXAbs = xAbs;
        pThis->mouseYAbs = yAbs;
        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_MOUSE_POSITION_CHANGED);
    }

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnQueryMouseCapabilities}
 */
static DECLCALLBACK(int) vmmdevIPort_QueryMouseCapabilities(PPDMIVMMDEVPORT pInterface, uint32_t *pfCapabilities)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    AssertPtrReturn(pfCapabilities, VERR_INVALID_PARAMETER);

    *pfCapabilities = pThis->mouseCapabilities;
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnUpdateMouseCapabilities}
 */
static DECLCALLBACK(int)
vmmdevIPort_UpdateMouseCapabilities(PPDMIVMMDEVPORT pInterface, uint32_t fCapsAdded, uint32_t fCapsRemoved)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    uint32_t fOldCaps = pThis->mouseCapabilities;
    pThis->mouseCapabilities &= ~(fCapsRemoved & VMMDEV_MOUSE_HOST_MASK);
    pThis->mouseCapabilities |= (fCapsAdded & VMMDEV_MOUSE_HOST_MASK)
                              | VMMDEV_MOUSE_HOST_RECHECKS_NEEDS_HOST_CURSOR;
    bool fNotify = fOldCaps != pThis->mouseCapabilities;

    LogRelFlow(("VMMDev: vmmdevIPort_UpdateMouseCapabilities: fCapsAdded=0x%x, fCapsRemoved=0x%x, fNotify=%RTbool\n", fCapsAdded,
                fCapsRemoved, fNotify));

    if (fNotify)
        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_MOUSE_CAPABILITIES_CHANGED);

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnRequestDisplayChange}
 */
static DECLCALLBACK(int)
vmmdevIPort_RequestDisplayChange(PPDMIVMMDEVPORT pInterface, uint32_t cx, uint32_t cy, uint32_t cBits, uint32_t idxDisplay,
                                 int32_t xOrigin, int32_t yOrigin, bool fEnabled, bool fChangeOrigin)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);

    if (idxDisplay >= RT_ELEMENTS(pThis->displayChangeData.aRequests))
        return VERR_INVALID_PARAMETER;

    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    DISPLAYCHANGEREQUEST *pRequest = &pThis->displayChangeData.aRequests[idxDisplay];

    /* Verify that the new resolution is different and that guest does not yet know about it. */
    bool fSameResolution = (!cx    || pRequest->lastReadDisplayChangeRequest.xres == cx)
                        && (!cy    || pRequest->lastReadDisplayChangeRequest.yres == cy)
                        && (!cBits || pRequest->lastReadDisplayChangeRequest.bpp  == cBits)
                        && pRequest->lastReadDisplayChangeRequest.xOrigin  == xOrigin
                        && pRequest->lastReadDisplayChangeRequest.yOrigin  == yOrigin
                        && pRequest->lastReadDisplayChangeRequest.fEnabled == fEnabled
                        && pRequest->lastReadDisplayChangeRequest.display  == idxDisplay;

    if (!cx && !cy && !cBits)
    {
        /* Special case of reset video mode. */
        fSameResolution = false;
    }

    Log3(("vmmdevIPort_RequestDisplayChange: same=%d. new: cx=%d, cy=%d, cBits=%d, idxDisplay=%d.\
          old: cx=%d, cy=%d, cBits=%d, idxDisplay=%d. \n \
          ,OriginX = %d , OriginY=%d, Enabled=%d, ChangeOrigin=%d\n",
          fSameResolution, cx, cy, cBits, idxDisplay, pRequest->lastReadDisplayChangeRequest.xres,
          pRequest->lastReadDisplayChangeRequest.yres, pRequest->lastReadDisplayChangeRequest.bpp,
          pRequest->lastReadDisplayChangeRequest.display,
          xOrigin, yOrigin, fEnabled, fChangeOrigin));

    /* we could validate the information here but hey, the guest can do that as well! */
    pRequest->displayChangeRequest.xres          = cx;
    pRequest->displayChangeRequest.yres          = cy;
    pRequest->displayChangeRequest.bpp           = cBits;
    pRequest->displayChangeRequest.display       = idxDisplay;
    pRequest->displayChangeRequest.xOrigin       = xOrigin;
    pRequest->displayChangeRequest.yOrigin       = yOrigin;
    pRequest->displayChangeRequest.fEnabled      = fEnabled;
    pRequest->displayChangeRequest.fChangeOrigin = fChangeOrigin;

    pRequest->fPending = !fSameResolution;

    if (!fSameResolution)
    {
        LogRel(("VMMDev: SetVideoModeHint: Got a video mode hint (%dx%dx%d)@(%dx%d),(%d;%d) at %d\n",
                cx, cy, cBits, xOrigin, yOrigin, fEnabled, fChangeOrigin, idxDisplay));

        /* IRQ so the guest knows what's going on */
        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST);
    }

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnRequestSeamlessChange}
 */
static DECLCALLBACK(int) vmmdevIPort_RequestSeamlessChange(PPDMIVMMDEVPORT pInterface, bool fEnabled)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    /* Verify that the new resolution is different and that guest does not yet know about it. */
    bool fSameMode = (pThis->fLastSeamlessEnabled == fEnabled);

    Log(("vmmdevIPort_RequestSeamlessChange: same=%d. new=%d\n", fSameMode, fEnabled));

    if (!fSameMode)
    {
        /* we could validate the information here but hey, the guest can do that as well! */
        pThis->fSeamlessEnabled = fEnabled;

        /* IRQ so the guest knows what's going on */
        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_SEAMLESS_MODE_CHANGE_REQUEST);
    }

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnSetMemoryBalloon}
 */
static DECLCALLBACK(int) vmmdevIPort_SetMemoryBalloon(PPDMIVMMDEVPORT pInterface, uint32_t cMbBalloon)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    /* Verify that the new resolution is different and that guest does not yet know about it. */
    Log(("vmmdevIPort_SetMemoryBalloon: old=%u new=%u\n", pThis->cMbMemoryBalloonLast, cMbBalloon));
    if (pThis->cMbMemoryBalloonLast != cMbBalloon)
    {
        /* we could validate the information here but hey, the guest can do that as well! */
        pThis->cMbMemoryBalloon = cMbBalloon;

        /* IRQ so the guest knows what's going on */
        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_BALLOON_CHANGE_REQUEST);
    }

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnVRDPChange}
 */
static DECLCALLBACK(int) vmmdevIPort_VRDPChange(PPDMIVMMDEVPORT pInterface, bool fVRDPEnabled, uint32_t uVRDPExperienceLevel)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    bool fSame = (pThis->fVRDPEnabled == fVRDPEnabled);

    Log(("vmmdevIPort_VRDPChange: old=%d. new=%d\n", pThis->fVRDPEnabled, fVRDPEnabled));

    if (!fSame)
    {
        pThis->fVRDPEnabled = fVRDPEnabled;
        pThis->uVRDPExperienceLevel = uVRDPExperienceLevel;

        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_VRDP);
    }

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnSetStatisticsInterval}
 */
static DECLCALLBACK(int) vmmdevIPort_SetStatisticsInterval(PPDMIVMMDEVPORT pInterface, uint32_t cSecsStatInterval)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    /* Verify that the new resolution is different and that guest does not yet know about it. */
    bool fSame = (pThis->u32LastStatIntervalSize == cSecsStatInterval);

    Log(("vmmdevIPort_SetStatisticsInterval: old=%d. new=%d\n", pThis->u32LastStatIntervalSize, cSecsStatInterval));

    if (!fSame)
    {
        /* we could validate the information here but hey, the guest can do that as well! */
        pThis->u32StatIntervalSize = cSecsStatInterval;

        /* IRQ so the guest knows what's going on */
        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_STATISTICS_INTERVAL_CHANGE_REQUEST);
    }

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnSetCredentials}
 */
static DECLCALLBACK(int) vmmdevIPort_SetCredentials(PPDMIVMMDEVPORT pInterface, const char *pszUsername,
                                                    const char *pszPassword, const char *pszDomain, uint32_t fFlags)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    AssertReturn(fFlags & (VMMDEV_SETCREDENTIALS_GUESTLOGON | VMMDEV_SETCREDENTIALS_JUDGE), VERR_INVALID_PARAMETER);
    size_t const cchUsername = strlen(pszUsername);
    AssertReturn(cchUsername < VMMDEV_CREDENTIALS_SZ_SIZE, VERR_BUFFER_OVERFLOW);
    size_t const cchPassword = strlen(pszPassword);
    AssertReturn(cchPassword < VMMDEV_CREDENTIALS_SZ_SIZE, VERR_BUFFER_OVERFLOW);
    size_t const cchDomain   = strlen(pszDomain);
    AssertReturn(cchDomain < VMMDEV_CREDENTIALS_SZ_SIZE, VERR_BUFFER_OVERFLOW);

    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    /*
     * Logon mode
     */
    if (fFlags & VMMDEV_SETCREDENTIALS_GUESTLOGON)
    {
        /* memorize the data */
        memcpy(pThis->pCredentials->Logon.szUserName, pszUsername, cchUsername);
        pThis->pCredentials->Logon.szUserName[cchUsername] = '\0';
        memcpy(pThis->pCredentials->Logon.szPassword, pszPassword, cchPassword);
        pThis->pCredentials->Logon.szPassword[cchPassword] = '\0';
        memcpy(pThis->pCredentials->Logon.szDomain,   pszDomain, cchDomain);
        pThis->pCredentials->Logon.szDomain[cchDomain]     = '\0';
        pThis->pCredentials->Logon.fAllowInteractiveLogon = !(fFlags & VMMDEV_SETCREDENTIALS_NOLOCALLOGON);
    }
    /*
     * Credentials verification mode?
     */
    else
    {
        /* memorize the data */
        memcpy(pThis->pCredentials->Judge.szUserName, pszUsername, cchUsername);
        pThis->pCredentials->Judge.szUserName[cchUsername] = '\0';
        memcpy(pThis->pCredentials->Judge.szPassword, pszPassword, cchPassword);
        pThis->pCredentials->Judge.szPassword[cchPassword] = '\0';
        memcpy(pThis->pCredentials->Judge.szDomain,   pszDomain,   cchDomain);
        pThis->pCredentials->Judge.szDomain[cchDomain]     = '\0';

        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_JUDGE_CREDENTIALS);
    }

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnVBVAChange}
 *
 * Notification from the Display.  Especially useful when acceleration is
 * disabled after a video mode change.
 */
static DECLCALLBACK(void) vmmdevIPort_VBVAChange(PPDMIVMMDEVPORT pInterface, bool fEnabled)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    Log(("vmmdevIPort_VBVAChange: fEnabled = %d\n", fEnabled));

    /* Only used by saved state, which I guess is why we don't bother with locking here. */
    pThis->u32VideoAccelEnabled = fEnabled;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnCpuHotUnplug}
 */
static DECLCALLBACK(int) vmmdevIPort_CpuHotUnplug(PPDMIVMMDEVPORT pInterface, uint32_t idCpuCore, uint32_t idCpuPackage)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    int     rc    = VINF_SUCCESS;

    Log(("vmmdevIPort_CpuHotUnplug: idCpuCore=%u idCpuPackage=%u\n", idCpuCore, idCpuPackage));

    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    if (pThis->fCpuHotPlugEventsEnabled)
    {
        pThis->enmCpuHotPlugEvent = VMMDevCpuEventType_Unplug;
        pThis->idCpuCore          = idCpuCore;
        pThis->idCpuPackage       = idCpuPackage;
        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_CPU_HOTPLUG);
    }
    else
        rc = VERR_VMMDEV_CPU_HOTPLUG_NOT_MONITORED_BY_GUEST;

    PDMCritSectLeave(&pThis->CritSect);
    return rc;
}

/**
 * @interface_method_impl{PDMIVMMDEVPORT,pfnCpuHotPlug}
 */
static DECLCALLBACK(int) vmmdevIPort_CpuHotPlug(PPDMIVMMDEVPORT pInterface, uint32_t idCpuCore, uint32_t idCpuPackage)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDEV, IPort);
    int     rc    = VINF_SUCCESS;

    Log(("vmmdevCpuPlug: idCpuCore=%u idCpuPackage=%u\n", idCpuCore, idCpuPackage));

    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    if (pThis->fCpuHotPlugEventsEnabled)
    {
        pThis->enmCpuHotPlugEvent = VMMDevCpuEventType_Plug;
        pThis->idCpuCore          = idCpuCore;
        pThis->idCpuPackage       = idCpuPackage;
        VMMDevNotifyGuest(pThis, VMMDEV_EVENT_CPU_HOTPLUG);
    }
    else
        rc = VERR_VMMDEV_CPU_HOTPLUG_NOT_MONITORED_BY_GUEST;

    PDMCritSectLeave(&pThis->CritSect);
    return rc;
}


/* -=-=-=-=-=- Saved State -=-=-=-=-=- */

/**
 * @callback_method_impl{FNSSMDEVLIVEEXEC}
 */
static DECLCALLBACK(int) vmmdevLiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    RT_NOREF1(uPass);
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, PVMMDEV);

    SSMR3PutBool(pSSM, pThis->fGetHostTimeDisabled);
    SSMR3PutBool(pSSM, pThis->fBackdoorLogDisabled);
    SSMR3PutBool(pSSM, pThis->fKeepCredentials);
    SSMR3PutBool(pSSM, pThis->fHeapEnabled);

    return VINF_SSM_DONT_CALL_AGAIN;
}


/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) vmmdevSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, PVMMDEV);
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    vmmdevLiveExec(pDevIns, pSSM, SSM_PASS_FINAL);

    SSMR3PutU32(pSSM, pThis->hypervisorSize);
    SSMR3PutU32(pSSM, pThis->mouseCapabilities);
    SSMR3PutS32(pSSM, pThis->mouseXAbs);
    SSMR3PutS32(pSSM, pThis->mouseYAbs);

    SSMR3PutBool(pSSM, pThis->fNewGuestFilterMask);
    SSMR3PutU32(pSSM, pThis->u32NewGuestFilterMask);
    SSMR3PutU32(pSSM, pThis->u32GuestFilterMask);
    SSMR3PutU32(pSSM, pThis->u32HostEventFlags);
    /* The following is not strictly necessary as PGM restores MMIO2, keeping it for historical reasons. */
    SSMR3PutMem(pSSM, &pThis->pVMMDevRAMR3->V, sizeof(pThis->pVMMDevRAMR3->V));

    SSMR3PutMem(pSSM, &pThis->guestInfo, sizeof(pThis->guestInfo));
    SSMR3PutU32(pSSM, pThis->fu32AdditionsOk);
    SSMR3PutU32(pSSM, pThis->u32VideoAccelEnabled);
    SSMR3PutBool(pSSM, pThis->displayChangeData.fGuestSentChangeEventAck);

    SSMR3PutU32(pSSM, pThis->guestCaps);

#ifdef VBOX_WITH_HGCM
    vmmdevHGCMSaveState(pThis, pSSM);
#endif /* VBOX_WITH_HGCM */

    SSMR3PutU32(pSSM, pThis->fHostCursorRequested);

    SSMR3PutU32(pSSM, pThis->guestInfo2.uFullVersion);
    SSMR3PutU32(pSSM, pThis->guestInfo2.uRevision);
    SSMR3PutU32(pSSM, pThis->guestInfo2.fFeatures);
    SSMR3PutStrZ(pSSM, pThis->guestInfo2.szName);
    SSMR3PutU32(pSSM, pThis->cFacilityStatuses);
    for (uint32_t i = 0; i < pThis->cFacilityStatuses; i++)
    {
        SSMR3PutU32(pSSM, pThis->aFacilityStatuses[i].enmFacility);
        SSMR3PutU32(pSSM, pThis->aFacilityStatuses[i].fFlags);
        SSMR3PutU16(pSSM, (uint16_t)pThis->aFacilityStatuses[i].enmStatus);
        SSMR3PutS64(pSSM, RTTimeSpecGetNano(&pThis->aFacilityStatuses[i].TimeSpecTS));
    }

    /* Heartbeat: */
    SSMR3PutBool(pSSM, pThis->fHeartbeatActive);
    SSMR3PutBool(pSSM, pThis->fFlatlined);
    SSMR3PutU64(pSSM, pThis->nsLastHeartbeatTS);
    TMR3TimerSave(pThis->pFlatlinedTimer, pSSM);

    PDMCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) vmmdevLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    /** @todo The code load code is assuming we're always loaded into a freshly
     *        constructed VM. */
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, PVMMDEV);
    int     rc;

    if (   uVersion > VMMDEV_SAVED_STATE_VERSION
        || uVersion < 6)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;

    /* config */
    if (uVersion > VMMDEV_SAVED_STATE_VERSION_VBOX_30)
    {
        bool f;
        rc = SSMR3GetBool(pSSM, &f); AssertRCReturn(rc, rc);
        if (pThis->fGetHostTimeDisabled != f)
            LogRel(("VMMDev: Config mismatch - fGetHostTimeDisabled: config=%RTbool saved=%RTbool\n", pThis->fGetHostTimeDisabled, f));

        rc = SSMR3GetBool(pSSM, &f); AssertRCReturn(rc, rc);
        if (pThis->fBackdoorLogDisabled != f)
            LogRel(("VMMDev: Config mismatch - fBackdoorLogDisabled: config=%RTbool saved=%RTbool\n", pThis->fBackdoorLogDisabled, f));

        rc = SSMR3GetBool(pSSM, &f); AssertRCReturn(rc, rc);
        if (pThis->fKeepCredentials != f)
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Config mismatch - fKeepCredentials: config=%RTbool saved=%RTbool"),
                                     pThis->fKeepCredentials, f);
        rc = SSMR3GetBool(pSSM, &f); AssertRCReturn(rc, rc);
        if (pThis->fHeapEnabled != f)
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Config mismatch - fHeapEnabled: config=%RTbool saved=%RTbool"),
                                    pThis->fHeapEnabled, f);
    }

    if (uPass != SSM_PASS_FINAL)
        return VINF_SUCCESS;

    /* state */
    SSMR3GetU32(pSSM, &pThis->hypervisorSize);
    SSMR3GetU32(pSSM, &pThis->mouseCapabilities);
    SSMR3GetS32(pSSM, &pThis->mouseXAbs);
    SSMR3GetS32(pSSM, &pThis->mouseYAbs);

    SSMR3GetBool(pSSM, &pThis->fNewGuestFilterMask);
    SSMR3GetU32(pSSM, &pThis->u32NewGuestFilterMask);
    SSMR3GetU32(pSSM, &pThis->u32GuestFilterMask);
    SSMR3GetU32(pSSM, &pThis->u32HostEventFlags);

    //SSMR3GetBool(pSSM, &pThis->pVMMDevRAMR3->fHaveEvents);
    // here be dragons (probably)
    SSMR3GetMem(pSSM, &pThis->pVMMDevRAMR3->V, sizeof (pThis->pVMMDevRAMR3->V));

    SSMR3GetMem(pSSM, &pThis->guestInfo, sizeof (pThis->guestInfo));
    SSMR3GetU32(pSSM, &pThis->fu32AdditionsOk);
    SSMR3GetU32(pSSM, &pThis->u32VideoAccelEnabled);
    if (uVersion > 10)
        SSMR3GetBool(pSSM, &pThis->displayChangeData.fGuestSentChangeEventAck);

    rc = SSMR3GetU32(pSSM, &pThis->guestCaps);

    /* Attributes which were temporarily introduced in r30072 */
    if (uVersion == 7)
    {
        uint32_t temp;
        SSMR3GetU32(pSSM, &temp);
        rc = SSMR3GetU32(pSSM, &temp);
    }
    AssertRCReturn(rc, rc);

#ifdef VBOX_WITH_HGCM
    rc = vmmdevHGCMLoadState(pThis, pSSM, uVersion);
    AssertRCReturn(rc, rc);
#endif /* VBOX_WITH_HGCM */

    if (uVersion >= 10)
        rc = SSMR3GetU32(pSSM, &pThis->fHostCursorRequested);
    AssertRCReturn(rc, rc);

    if (uVersion > VMMDEV_SAVED_STATE_VERSION_MISSING_GUEST_INFO_2)
    {
        SSMR3GetU32(pSSM, &pThis->guestInfo2.uFullVersion);
        SSMR3GetU32(pSSM, &pThis->guestInfo2.uRevision);
        SSMR3GetU32(pSSM, &pThis->guestInfo2.fFeatures);
        rc = SSMR3GetStrZ(pSSM, &pThis->guestInfo2.szName[0], sizeof(pThis->guestInfo2.szName));
        AssertRCReturn(rc, rc);
    }

    if (uVersion > VMMDEV_SAVED_STATE_VERSION_MISSING_FACILITY_STATUSES)
    {
        uint32_t cFacilityStatuses;
        rc = SSMR3GetU32(pSSM, &cFacilityStatuses);
        AssertRCReturn(rc, rc);

        for (uint32_t i = 0; i < cFacilityStatuses; i++)
        {
            uint32_t uFacility, fFlags;
            uint16_t uStatus;
            int64_t  iTimeStampNano;

            SSMR3GetU32(pSSM, &uFacility);
            SSMR3GetU32(pSSM, &fFlags);
            SSMR3GetU16(pSSM, &uStatus);
            rc = SSMR3GetS64(pSSM, &iTimeStampNano);
            AssertRCReturn(rc, rc);

            PVMMDEVFACILITYSTATUSENTRY pEntry = vmmdevGetFacilityStatusEntry(pThis, (VBoxGuestFacilityType)uFacility);
            AssertLogRelMsgReturn(pEntry,
                                  ("VMMDev: Ran out of entries restoring the guest facility statuses. Saved state has %u.\n", cFacilityStatuses),
                                  VERR_OUT_OF_RESOURCES);
            pEntry->enmStatus = (VBoxGuestFacilityStatus)uStatus;
            pEntry->fFlags    = fFlags;
            RTTimeSpecSetNano(&pEntry->TimeSpecTS, iTimeStampNano);
        }
    }

    /*
     * Heartbeat.
     */
    if (uVersion >= VMMDEV_SAVED_STATE_VERSION_HEARTBEAT)
    {
        SSMR3GetBool(pSSM, (bool *)&pThis->fHeartbeatActive);
        SSMR3GetBool(pSSM, (bool *)&pThis->fFlatlined);
        SSMR3GetU64(pSSM, (uint64_t *)&pThis->nsLastHeartbeatTS);
        rc = TMR3TimerLoad(pThis->pFlatlinedTimer, pSSM);
        AssertRCReturn(rc, rc);
        if (pThis->fFlatlined)
            LogRel(("vmmdevLoadState: Guest has flatlined. Last heartbeat %'RU64 ns before state was saved.\n",
                    TMTimerGetNano(pThis->pFlatlinedTimer) - pThis->nsLastHeartbeatTS));
    }

    /*
     * On a resume, we send the capabilities changed message so
     * that listeners can sync their state again
     */
    Log(("vmmdevLoadState: capabilities changed (%x), informing connector\n", pThis->mouseCapabilities));
    if (pThis->pDrv)
    {
        pThis->pDrv->pfnUpdateMouseCapabilities(pThis->pDrv, pThis->mouseCapabilities);
        if (uVersion >= 10)
            pThis->pDrv->pfnUpdatePointerShape(pThis->pDrv,
                                               /*fVisible=*/!!pThis->fHostCursorRequested,
                                               /*fAlpha=*/false,
                                               /*xHot=*/0, /*yHot=*/0,
                                               /*cx=*/0, /*cy=*/0,
                                               /*pvShape=*/NULL);
    }

    if (pThis->fu32AdditionsOk)
    {
        vmmdevLogGuestOsInfo(&pThis->guestInfo);
        if (pThis->pDrv)
        {
            if (pThis->guestInfo2.uFullVersion && pThis->pDrv->pfnUpdateGuestInfo2)
                pThis->pDrv->pfnUpdateGuestInfo2(pThis->pDrv, pThis->guestInfo2.uFullVersion, pThis->guestInfo2.szName,
                                                 pThis->guestInfo2.uRevision, pThis->guestInfo2.fFeatures);
            if (pThis->pDrv->pfnUpdateGuestInfo)
                pThis->pDrv->pfnUpdateGuestInfo(pThis->pDrv, &pThis->guestInfo);

            if (pThis->pDrv->pfnUpdateGuestStatus)
            {
                for (uint32_t i = 0; i < pThis->cFacilityStatuses; i++) /* ascending order! */
                    if (   pThis->aFacilityStatuses[i].enmStatus != VBoxGuestFacilityStatus_Inactive
                        || !pThis->aFacilityStatuses[i].fFixed)
                        pThis->pDrv->pfnUpdateGuestStatus(pThis->pDrv,
                                                          pThis->aFacilityStatuses[i].enmFacility,
                                                          (uint16_t)pThis->aFacilityStatuses[i].enmStatus,
                                                          pThis->aFacilityStatuses[i].fFlags,
                                                          &pThis->aFacilityStatuses[i].TimeSpecTS);
            }
        }
    }
    if (pThis->pDrv && pThis->pDrv->pfnUpdateGuestCapabilities)
        pThis->pDrv->pfnUpdateGuestCapabilities(pThis->pDrv, pThis->guestCaps);

    return VINF_SUCCESS;
}

/**
 * Load state done callback. Notify guest of restore event.
 *
 * @returns VBox status code.
 * @param   pDevIns    The device instance.
 * @param   pSSM The handle to the saved state.
 */
static DECLCALLBACK(int) vmmdevLoadStateDone(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF1(pSSM);
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, PVMMDEV);

#ifdef VBOX_WITH_HGCM
    int rc = vmmdevHGCMLoadStateDone(pThis);
    AssertLogRelRCReturn(rc, rc);
#endif /* VBOX_WITH_HGCM */

    /* Reestablish the acceleration status. */
    if (    pThis->u32VideoAccelEnabled
        &&  pThis->pDrv)
    {
        pThis->pDrv->pfnVideoAccelEnable(pThis->pDrv, !!pThis->u32VideoAccelEnabled, &pThis->pVMMDevRAMR3->vbvaMemory);
    }

    VMMDevNotifyGuest(pThis, VMMDEV_EVENT_RESTORED);

    return VINF_SUCCESS;
}


/* -=-=-=-=- PDMDEVREG -=-=-=-=- */

/**
 * (Re-)initializes the MMIO2 data.
 *
 * @param   pThis           Pointer to the VMMDev instance data.
 */
static void vmmdevInitRam(PVMMDEV pThis)
{
    memset(pThis->pVMMDevRAMR3, 0, sizeof(VMMDevMemory));
    pThis->pVMMDevRAMR3->u32Size = sizeof(VMMDevMemory);
    pThis->pVMMDevRAMR3->u32Version = VMMDEV_MEMORY_VERSION;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) vmmdevReset(PPDMDEVINS pDevIns)
{
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, PVMMDEV);
    PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);

    /*
     * Reset the mouse integration feature bits
     */
    if (pThis->mouseCapabilities & VMMDEV_MOUSE_GUEST_MASK)
    {
        pThis->mouseCapabilities &= ~VMMDEV_MOUSE_GUEST_MASK;
        /* notify the connector */
        Log(("vmmdevReset: capabilities changed (%x), informing connector\n", pThis->mouseCapabilities));
        pThis->pDrv->pfnUpdateMouseCapabilities(pThis->pDrv, pThis->mouseCapabilities);
    }
    pThis->fHostCursorRequested = false;

    pThis->hypervisorSize = 0;

    /* re-initialize the VMMDev memory */
    if (pThis->pVMMDevRAMR3)
        vmmdevInitRam(pThis);

    /* credentials have to go away (by default) */
    if (!pThis->fKeepCredentials)
    {
        memset(pThis->pCredentials->Logon.szUserName, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
        memset(pThis->pCredentials->Logon.szPassword, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
        memset(pThis->pCredentials->Logon.szDomain, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
    }
    memset(pThis->pCredentials->Judge.szUserName, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
    memset(pThis->pCredentials->Judge.szPassword, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);
    memset(pThis->pCredentials->Judge.szDomain, '\0', VMMDEV_CREDENTIALS_SZ_SIZE);

    /* Reset means that additions will report again. */
    const bool fVersionChanged = pThis->fu32AdditionsOk
                              || pThis->guestInfo.interfaceVersion
                              || pThis->guestInfo.osType != VBOXOSTYPE_Unknown;
    if (fVersionChanged)
        Log(("vmmdevReset: fu32AdditionsOk=%d additionsVersion=%x osType=%#x\n",
             pThis->fu32AdditionsOk, pThis->guestInfo.interfaceVersion, pThis->guestInfo.osType));
    pThis->fu32AdditionsOk = false;
    memset (&pThis->guestInfo, 0, sizeof (pThis->guestInfo));
    RT_ZERO(pThis->guestInfo2);
    const bool fCapsChanged = pThis->guestCaps != 0; /* Report transition to 0. */
    pThis->guestCaps = 0;

    /* Clear facilities. No need to tell Main as it will get a
       pfnUpdateGuestInfo callback. */
    RTTIMESPEC TimeStampNow;
    RTTimeNow(&TimeStampNow);
    uint32_t iFacility = pThis->cFacilityStatuses;
    while (iFacility-- > 0)
    {
        pThis->aFacilityStatuses[iFacility].enmStatus  = VBoxGuestFacilityStatus_Inactive;
        pThis->aFacilityStatuses[iFacility].TimeSpecTS = TimeStampNow;
    }

    /* clear pending display change request. */
    for (unsigned i = 0; i < RT_ELEMENTS(pThis->displayChangeData.aRequests); i++)
    {
        DISPLAYCHANGEREQUEST *pRequest = &pThis->displayChangeData.aRequests[i];
        memset (&pRequest->lastReadDisplayChangeRequest, 0, sizeof (pRequest->lastReadDisplayChangeRequest));
    }
    pThis->displayChangeData.iCurrentMonitor = 0;
    pThis->displayChangeData.fGuestSentChangeEventAck = false;

    /* disable seamless mode */
    pThis->fLastSeamlessEnabled = false;

    /* disabled memory ballooning */
    pThis->cMbMemoryBalloonLast = 0;

    /* disabled statistics updating */
    pThis->u32LastStatIntervalSize = 0;

#ifdef VBOX_WITH_HGCM
    /* Clear the "HGCM event enabled" flag so the event can be automatically reenabled.  */
    pThis->u32HGCMEnabled = 0;
#endif

    /*
     * Deactive heartbeat.
     */
    if (pThis->fHeartbeatActive)
    {
        TMTimerStop(pThis->pFlatlinedTimer);
        pThis->fFlatlined       = false;
        pThis->fHeartbeatActive = true;
    }

    /*
     * Clear the event variables.
     *
     * XXX By design we should NOT clear pThis->u32HostEventFlags because it is designed
     *     that way so host events do not depend on guest resets. However, the pending
     *     event flags actually _were_ cleared since ages so we mask out events from
     *     clearing which we really need to survive the reset. See xtracker 5767.
     */
    pThis->u32HostEventFlags    &= VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST;
    pThis->u32GuestFilterMask    = 0;
    pThis->u32NewGuestFilterMask = 0;
    pThis->fNewGuestFilterMask   = 0;

    /*
     * Call the update functions as required.
     */
    if (fVersionChanged && pThis->pDrv && pThis->pDrv->pfnUpdateGuestInfo)
        pThis->pDrv->pfnUpdateGuestInfo(pThis->pDrv, &pThis->guestInfo);
    if (fCapsChanged && pThis->pDrv && pThis->pDrv->pfnUpdateGuestCapabilities)
        pThis->pDrv->pfnUpdateGuestCapabilities(pThis->pDrv, pThis->guestCaps);

    /*
     * Generate a unique session id for this VM; it will be changed for each start, reset or restore.
     * This can be used for restore detection inside the guest.
     */
    pThis->idSession = ASMReadTSC();

    PDMCritSectLeave(&pThis->CritSect);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnRelocate}
 */
static DECLCALLBACK(void) vmmdevRelocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    NOREF(pDevIns);
    NOREF(offDelta);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) vmmdevDestruct(PPDMDEVINS pDevIns)
{
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, PVMMDEV);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Wipe and free the credentials.
     */
    if (pThis->pCredentials)
    {
        RTMemWipeThoroughly(pThis->pCredentials, sizeof(*pThis->pCredentials), 10);
        RTMemFree(pThis->pCredentials);
        pThis->pCredentials = NULL;
    }

#ifdef VBOX_WITH_HGCM
    vmmdevHGCMDestroy(pThis);
    RTCritSectDelete(&pThis->critsectHGCMCmdList);
#endif

#ifndef VBOX_WITHOUT_TESTING_FEATURES
    /*
     * Clean up the testing device.
     */
    vmmdevTestingTerminate(pDevIns);
#endif

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) vmmdevConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PVMMDEV pThis = PDMINS_2_DATA(pDevIns, PVMMDEV);
    int rc;

    Assert(iInstance == 0);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Initialize data (most of it anyway).
     */
    /* Save PDM device instance data for future reference. */
    pThis->pDevIns = pDevIns;

    /* PCI vendor, just a free bogus value */
    PCIDevSetVendorId(&pThis->PciDev, 0x80ee);
    /* device ID */
    PCIDevSetDeviceId(&pThis->PciDev, 0xcafe);
    /* class sub code (other type of system peripheral) */
    PCIDevSetClassSub(&pThis->PciDev, 0x80);
    /* class base code (base system peripheral) */
    PCIDevSetClassBase(&pThis->PciDev, 0x08);
    /* header type */
    PCIDevSetHeaderType(&pThis->PciDev, 0x00);
    /* interrupt on pin 0 */
    PCIDevSetInterruptPin(&pThis->PciDev, 0x01);

    RTTIMESPEC TimeStampNow;
    RTTimeNow(&TimeStampNow);
    vmmdevAllocFacilityStatusEntry(pThis, VBoxGuestFacilityType_VBoxGuestDriver, true /*fFixed*/, &TimeStampNow);
    vmmdevAllocFacilityStatusEntry(pThis, VBoxGuestFacilityType_VBoxService,     true /*fFixed*/, &TimeStampNow);
    vmmdevAllocFacilityStatusEntry(pThis, VBoxGuestFacilityType_VBoxTrayClient,  true /*fFixed*/, &TimeStampNow);
    vmmdevAllocFacilityStatusEntry(pThis, VBoxGuestFacilityType_Seamless,        true /*fFixed*/, &TimeStampNow);
    vmmdevAllocFacilityStatusEntry(pThis, VBoxGuestFacilityType_Graphics,        true /*fFixed*/, &TimeStampNow);
    Assert(pThis->cFacilityStatuses == 5);

    /*
     * Interfaces
     */
    /* IBase */
    pThis->IBase.pfnQueryInterface          = vmmdevPortQueryInterface;

    /* VMMDev port */
    pThis->IPort.pfnQueryAbsoluteMouse      = vmmdevIPort_QueryAbsoluteMouse;
    pThis->IPort.pfnSetAbsoluteMouse        = vmmdevIPort_SetAbsoluteMouse ;
    pThis->IPort.pfnQueryMouseCapabilities  = vmmdevIPort_QueryMouseCapabilities;
    pThis->IPort.pfnUpdateMouseCapabilities = vmmdevIPort_UpdateMouseCapabilities;
    pThis->IPort.pfnRequestDisplayChange    = vmmdevIPort_RequestDisplayChange;
    pThis->IPort.pfnSetCredentials          = vmmdevIPort_SetCredentials;
    pThis->IPort.pfnVBVAChange              = vmmdevIPort_VBVAChange;
    pThis->IPort.pfnRequestSeamlessChange   = vmmdevIPort_RequestSeamlessChange;
    pThis->IPort.pfnSetMemoryBalloon        = vmmdevIPort_SetMemoryBalloon;
    pThis->IPort.pfnSetStatisticsInterval   = vmmdevIPort_SetStatisticsInterval;
    pThis->IPort.pfnVRDPChange              = vmmdevIPort_VRDPChange;
    pThis->IPort.pfnCpuHotUnplug            = vmmdevIPort_CpuHotUnplug;
    pThis->IPort.pfnCpuHotPlug              = vmmdevIPort_CpuHotPlug;

    /* Shared folder LED */
    pThis->SharedFolders.Led.u32Magic       = PDMLED_MAGIC;
    pThis->SharedFolders.ILeds.pfnQueryStatusLed = vmmdevQueryStatusLed;

#ifdef VBOX_WITH_HGCM
    /* HGCM port */
    pThis->IHGCMPort.pfnCompleted           = hgcmCompleted;
#endif

    pThis->pCredentials = (VMMDEVCREDS *)RTMemAllocZ(sizeof(*pThis->pCredentials));
    if (!pThis->pCredentials)
        return VERR_NO_MEMORY;


    /*
     * Validate and read the configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns,
                                  "GetHostTimeDisabled|"
                                  "BackdoorLogDisabled|"
                                  "KeepCredentials|"
                                  "HeapEnabled|"
                                  "RZEnabled|"
                                  "GuestCoreDumpEnabled|"
                                  "GuestCoreDumpDir|"
                                  "GuestCoreDumpCount|"
                                  "HeartbeatInterval|"
                                  "HeartbeatTimeout|"
                                  "TestingEnabled|"
                                  "TestingMMIO|"
                                  "TestintXmlOutputFile"
                                  ,
                                  "");

    rc = CFGMR3QueryBoolDef(pCfg, "GetHostTimeDisabled", &pThis->fGetHostTimeDisabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"GetHostTimeDisabled\" as a boolean"));

    rc = CFGMR3QueryBoolDef(pCfg, "BackdoorLogDisabled", &pThis->fBackdoorLogDisabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"BackdoorLogDisabled\" as a boolean"));

    rc = CFGMR3QueryBoolDef(pCfg, "KeepCredentials", &pThis->fKeepCredentials, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"KeepCredentials\" as a boolean"));

    rc = CFGMR3QueryBoolDef(pCfg, "HeapEnabled", &pThis->fHeapEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"HeapEnabled\" as a boolean"));

    rc = CFGMR3QueryBoolDef(pCfg, "RZEnabled", &pThis->fRZEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"RZEnabled\" as a boolean"));

    rc = CFGMR3QueryBoolDef(pCfg, "GuestCoreDumpEnabled", &pThis->fGuestCoreDumpEnabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"GuestCoreDumpEnabled\" as a boolean"));

    char *pszGuestCoreDumpDir = NULL;
    rc = CFGMR3QueryStringAllocDef(pCfg, "GuestCoreDumpDir", &pszGuestCoreDumpDir, "");
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"GuestCoreDumpDir\" as a string"));

    RTStrCopy(pThis->szGuestCoreDumpDir, sizeof(pThis->szGuestCoreDumpDir), pszGuestCoreDumpDir);
    MMR3HeapFree(pszGuestCoreDumpDir);

    rc = CFGMR3QueryU32Def(pCfg, "GuestCoreDumpCount", &pThis->cGuestCoreDumps, 3);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"GuestCoreDumpCount\" as a 32-bit unsigned integer"));

    rc = CFGMR3QueryU64Def(pCfg, "HeartbeatInterval", &pThis->cNsHeartbeatInterval, VMMDEV_HEARTBEAT_DEFAULT_INTERVAL);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"HeartbeatInterval\" as a 64-bit unsigned integer"));
    if (pThis->cNsHeartbeatInterval < RT_NS_100MS / 2)
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Heartbeat interval \"HeartbeatInterval\" too small"));

    rc = CFGMR3QueryU64Def(pCfg, "HeartbeatTimeout", &pThis->cNsHeartbeatTimeout, pThis->cNsHeartbeatInterval * 2);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"HeartbeatTimeout\" as a 64-bit unsigned integer"));
    if (pThis->cNsHeartbeatTimeout < RT_NS_100MS)
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Heartbeat timeout \"HeartbeatTimeout\" too small"));
    if (pThis->cNsHeartbeatTimeout <= pThis->cNsHeartbeatInterval + RT_NS_10MS)
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("Configuration error: Heartbeat timeout \"HeartbeatTimeout\" value (%'ull ns) is too close to the interval (%'ull ns)"),
                                   pThis->cNsHeartbeatTimeout, pThis->cNsHeartbeatInterval);

#ifndef VBOX_WITHOUT_TESTING_FEATURES
    rc = CFGMR3QueryBoolDef(pCfg, "TestingEnabled", &pThis->fTestingEnabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"TestingEnabled\" as a boolean"));
    rc = CFGMR3QueryBoolDef(pCfg, "TestingMMIO", &pThis->fTestingMMIO, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed querying \"TestingMMIO\" as a boolean"));
    rc = CFGMR3QueryStringAllocDef(pCfg, "TestintXmlOutputFile", &pThis->pszTestingXmlOutput, NULL);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Failed querying \"TestintXmlOutputFile\" as a string"));

    /** @todo image-to-load-filename? */
#endif

    pThis->cbGuestRAM = MMR3PhysGetRamSize(PDMDevHlpGetVM(pDevIns));

    /*
     * We do our own locking entirely. So, install NOP critsect for the device
     * and create our own critsect for use where it really matters (++).
     */
    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);
    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->CritSect, RT_SRC_POS, "VMMDev#%u", iInstance);
    AssertRCReturn(rc, rc);

    /*
     * Register the backdoor logging port
     */
    rc = PDMDevHlpIOPortRegister(pDevIns, RTLOG_DEBUG_PORT, 1, NULL, vmmdevBackdoorLog,
                                 NULL, NULL, NULL, "VMMDev backdoor logging");
    AssertRCReturn(rc, rc);

#ifdef VMMDEV_WITH_ALT_TIMESYNC
    /*
     * Alternative timesync source.
     *
     * This was orignally added for creating a simple time sync service in an
     * OpenBSD guest without requiring VBoxGuest and VBoxService to be ported
     * first.  We keep it in case it comes in handy.
     */
    rc = PDMDevHlpIOPortRegister(pDevIns, 0x505, 1, NULL,
                                 vmmdevAltTimeSyncWrite, vmmdevAltTimeSyncRead,
                                 NULL, NULL, "VMMDev timesync backdoor");
    AssertRCReturn(rc, rc);
#endif

    /*
     * Register the PCI device.
     */
    rc = PDMDevHlpPCIRegister(pDevIns, &pThis->PciDev);
    if (RT_FAILURE(rc))
        return rc;
    if (pThis->PciDev.uDevFn != 32 || iInstance != 0)
        Log(("!!WARNING!!: pThis->PciDev.uDevFn=%d (ignore if testcase or no started by Main)\n", pThis->PciDev.uDevFn));
    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 0, 0x20, PCI_ADDRESS_SPACE_IO, vmmdevIOPortRegionMap);
    if (RT_FAILURE(rc))
        return rc;
    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 1, VMMDEV_RAM_SIZE, PCI_ADDRESS_SPACE_MEM, vmmdevIORAMRegionMap);
    if (RT_FAILURE(rc))
        return rc;
    if (pThis->fHeapEnabled)
    {
        rc = PDMDevHlpPCIIORegionRegister(pDevIns, 2, VMMDEV_HEAP_SIZE, PCI_ADDRESS_SPACE_MEM_PREFETCH, vmmdevIORAMRegionMap);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Allocate and initialize the MMIO2 memory.
     */
    rc = PDMDevHlpMMIO2Register(pDevIns, &pThis->PciDev, 1 /*iRegion*/, VMMDEV_RAM_SIZE, 0 /*fFlags*/,
                                (void **)&pThis->pVMMDevRAMR3, "VMMDev");
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("Failed to allocate %u bytes of memory for the VMM device"), VMMDEV_RAM_SIZE);
    vmmdevInitRam(pThis);

    if (pThis->fHeapEnabled)
    {
        rc = PDMDevHlpMMIO2Register(pDevIns, &pThis->PciDev, 2 /*iRegion*/, VMMDEV_HEAP_SIZE, 0 /*fFlags*/,
                                    (void **)&pThis->pVMMDevHeapR3, "VMMDev Heap");
        if (RT_FAILURE(rc))
            return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                       N_("Failed to allocate %u bytes of memory for the VMM device heap"), PAGE_SIZE);

        /* Register the memory area with PDM so HM can access it before it's mapped. */
        rc = PDMDevHlpRegisterVMMDevHeap(pDevIns, NIL_RTGCPHYS, pThis->pVMMDevHeapR3, VMMDEV_HEAP_SIZE);
        AssertLogRelRCReturn(rc, rc);
    }

#ifndef VBOX_WITHOUT_TESTING_FEATURES
    /*
     * Initialize testing.
     */
    rc = vmmdevTestingInitialize(pDevIns);
    if (RT_FAILURE(rc))
        return rc;
#endif

    /*
     * Get the corresponding connector interface
     */
    rc = PDMDevHlpDriverAttach(pDevIns, 0, &pThis->IBase, &pThis->pDrvBase, "VMM Driver Port");
    if (RT_SUCCESS(rc))
    {
        pThis->pDrv = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIVMMDEVCONNECTOR);
        AssertMsgReturn(pThis->pDrv, ("LUN #0 doesn't have a VMMDev connector interface!\n"), VERR_PDM_MISSING_INTERFACE);
#ifdef VBOX_WITH_HGCM
        pThis->pHGCMDrv = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIHGCMCONNECTOR);
        if (!pThis->pHGCMDrv)
        {
            Log(("LUN #0 doesn't have a HGCM connector interface, HGCM is not supported. rc=%Rrc\n", rc));
            /* this is not actually an error, just means that there is no support for HGCM */
        }
#endif
        /* Query the initial balloon size. */
        AssertPtr(pThis->pDrv->pfnQueryBalloonSize);
        rc = pThis->pDrv->pfnQueryBalloonSize(pThis->pDrv, &pThis->cMbMemoryBalloon);
        AssertRC(rc);

        Log(("Initial balloon size %x\n", pThis->cMbMemoryBalloon));
    }
    else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
    {
        Log(("%s/%d: warning: no driver attached to LUN #0!\n", pDevIns->pReg->szName, pDevIns->iInstance));
        rc = VINF_SUCCESS;
    }
    else
        AssertMsgFailedReturn(("Failed to attach LUN #0! rc=%Rrc\n", rc), rc);

    /*
     * Attach status driver for shared folders (optional).
     */
    PPDMIBASE pBase;
    rc = PDMDevHlpDriverAttach(pDevIns, PDM_STATUS_LUN, &pThis->IBase, &pBase, "Status Port");
    if (RT_SUCCESS(rc))
        pThis->SharedFolders.pLedsConnector = PDMIBASE_QUERY_INTERFACE(pBase, PDMILEDCONNECTORS);
    else if (rc != VERR_PDM_NO_ATTACHED_DRIVER)
    {
        AssertMsgFailed(("Failed to attach to status driver. rc=%Rrc\n", rc));
        return rc;
    }

    /*
     * Register saved state and init the HGCM CmdList critsect.
     */
    rc = PDMDevHlpSSMRegisterEx(pDevIns, VMMDEV_SAVED_STATE_VERSION, sizeof(*pThis), NULL,
                                NULL, vmmdevLiveExec, NULL,
                                NULL, vmmdevSaveExec, NULL,
                                NULL, vmmdevLoadExec, vmmdevLoadStateDone);
    AssertRCReturn(rc, rc);

    /*
     * Create heartbeat checking timer.
     */
    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, vmmDevHeartbeatFlatlinedTimer, pThis,
                                TMTIMER_FLAGS_NO_CRIT_SECT, "Heartbeat flatlined", &pThis->pFlatlinedTimer);
    AssertRCReturn(rc, rc);

#ifdef VBOX_WITH_HGCM
    RTListInit(&pThis->listHGCMCmd);
    rc = RTCritSectInit(&pThis->critsectHGCMCmdList);
    AssertRCReturn(rc, rc);
    pThis->u32HGCMEnabled = 0;
#endif /* VBOX_WITH_HGCM */

    /*
     * In this version of VirtualBox the GUI checks whether "needs host cursor"
     * changes.
     */
    pThis->mouseCapabilities |= VMMDEV_MOUSE_HOST_RECHECKS_NEEDS_HOST_CURSOR;

    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatMemBalloonChunks, STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Memory balloon size", "/Devices/VMMDev/BalloonChunks");

    /*
     * Generate a unique session id for this VM; it will be changed for each
     * start, reset or restore. This can be used for restore detection inside
     * the guest.
     */
    pThis->idSession = ASMReadTSC();
    return rc;
}

/**
 * The device registration structure.
 */
extern "C" const PDMDEVREG g_DeviceVMMDev =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "VMMDev",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "VirtualBox VMM Device\n",
    /* fFlags */
    PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT | PDM_DEVREG_FLAGS_GUEST_BITS_DEFAULT | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_VMM_DEV,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(VMMDevState),
    /* pfnConstruct */
    vmmdevConstruct,
    /* pfnDestruct */
    vmmdevDestruct,
    /* pfnRelocate */
    vmmdevRelocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    vmmdevReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */

