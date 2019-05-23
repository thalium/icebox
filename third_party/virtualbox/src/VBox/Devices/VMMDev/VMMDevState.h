/* $Id: VMMDevState.h $ */
/** @file
 * VMMDev - Guest <-> VMM/Host communication device, internal header.
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

#ifndef ___VMMDev_VMMDevState_h
#define ___VMMDev_VMMDevState_h

#include <VBox/VMMDev.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmifs.h>
#ifndef VBOX_WITHOUT_TESTING_FEATURES
# include <iprt/test.h>
# include <VBox/VMMDevTesting.h>
#endif

#include <iprt/list.h>

#define VMMDEV_WITH_ALT_TIMESYNC

typedef struct DISPLAYCHANGEINFO
{
    uint32_t xres;
    uint32_t yres;
    uint32_t bpp;
    uint32_t display;
    int32_t xOrigin;
    int32_t yOrigin;
    bool fEnabled;
    bool fChangeOrigin;
} DISPLAYCHANGEINFO;

typedef struct DISPLAYCHANGEREQUEST
{
    bool fPending;
    bool afAlignment[3];
    DISPLAYCHANGEINFO displayChangeRequest;
    DISPLAYCHANGEINFO lastReadDisplayChangeRequest;
} DISPLAYCHANGEREQUEST;

typedef struct DISPLAYCHANGEDATA
{
    /* Which monitor is being reported to the guest. */
    int iCurrentMonitor;

    /** true if the guest responded to VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST at least once */
    bool fGuestSentChangeEventAck;
    bool afAlignment[3];

    DISPLAYCHANGEREQUEST aRequests[64]; /// @todo maxMonitors
} DISPLAYCHANGEDATA;


/**
 * Credentials for automatic guest logon and host configured logon (?).
 *
 * This is not stored in the same block as the instance data in order to make it
 * harder to access.
 */
typedef struct VMMDEVCREDS
{
    /** credentials for guest logon purposes */
    struct
    {
        char szUserName[VMMDEV_CREDENTIALS_SZ_SIZE];
        char szPassword[VMMDEV_CREDENTIALS_SZ_SIZE];
        char szDomain[VMMDEV_CREDENTIALS_SZ_SIZE];
        bool fAllowInteractiveLogon;
    } Logon;

    /** credentials for verification by guest */
    struct
    {
        char szUserName[VMMDEV_CREDENTIALS_SZ_SIZE];
        char szPassword[VMMDEV_CREDENTIALS_SZ_SIZE];
        char szDomain[VMMDEV_CREDENTIALS_SZ_SIZE];
    } Judge;
} VMMDEVCREDS;


/**
 * Facility status entry.
 */
typedef struct VMMDEVFACILITYSTATUSENTRY
{
    /** The facility (may contain values other than the defined ones). */
    VBoxGuestFacilityType       enmFacility;
    /** The status (may contain values other than the defined ones). */
    VBoxGuestFacilityStatus     enmStatus;
    /** Whether this entry is fixed and cannot be reused when inactive. */
    bool                        fFixed;
    /** Explicit alignment padding / reserved for future use. MBZ. */
    bool                        afPadding[3];
    /** The facility flags (yet to be defined). */
    uint32_t                    fFlags;
    /** Last update timestamp. */
    RTTIMESPEC                  TimeSpecTS;
} VMMDEVFACILITYSTATUSENTRY;
/** Pointer to a facility status entry. */
typedef VMMDEVFACILITYSTATUSENTRY *PVMMDEVFACILITYSTATUSENTRY;


/** device structure containing all state information */
typedef struct VMMDevState
{
    /** The PCI device structure. */
    PDMPCIDEV           PciDev;
    /** The critical section for this device.
     * @remarks We use this rather than the default one, it's simpler with all
     *          the driver interfaces where we have to waste time digging out the
     *          PDMDEVINS structure. */
    PDMCRITSECT         CritSect;

    /** hypervisor address space size */
    uint32_t hypervisorSize;

    /** mouse capabilities of host and guest */
    uint32_t mouseCapabilities;
    /** absolute mouse position in pixels */
    int32_t mouseXAbs;
    int32_t mouseYAbs;
    /** Does the guest currently want the host pointer to be shown? */
    uint32_t fHostCursorRequested;

    /** Alignment padding. */
    uint32_t u32Alignment0;

    /** Pointer to device instance. */
    PPDMDEVINSR3 pDevIns;
    /** LUN\#0 + Status: VMMDev port base interface. */
    PDMIBASE IBase;
    /** LUN\#0: VMMDev port interface. */
    PDMIVMMDEVPORT IPort;
#ifdef VBOX_WITH_HGCM
    /** LUN\#0: HGCM port interface. */
    PDMIHGCMPORT IHGCMPort;
#endif
    /** Pointer to base interface of the driver. */
    R3PTRTYPE(PPDMIBASE) pDrvBase;
    /** VMMDev connector interface */
    R3PTRTYPE(PPDMIVMMDEVCONNECTOR) pDrv;
#ifdef VBOX_WITH_HGCM
    /** HGCM connector interface */
    R3PTRTYPE(PPDMIHGCMCONNECTOR) pHGCMDrv;
#endif
    /** Alignment padding. */
    RTR3PTR PtrR3Alignment1;
    /** message buffer for backdoor logging. */
    char szMsg[512];
    /** message buffer index. */
    uint32_t iMsg;
    /** Alignment padding. */
    uint32_t u32Alignment2;

    /** IRQ number assigned to the device */
    uint32_t irq;
    /** Current host side event flags */
    uint32_t u32HostEventFlags;
    /** Mask of events guest is interested in.
     * @note The HGCM events are enabled automatically by the VMMDev device when
     *       guest issues HGCM commands. */
    uint32_t u32GuestFilterMask;
    /** Delayed mask of guest events */
    uint32_t u32NewGuestFilterMask;
    /** Flag whether u32NewGuestFilterMask is valid */
    bool fNewGuestFilterMask;
    /** Alignment padding. */
    bool afAlignment3[3];

    /** GC physical address of VMMDev RAM area */
    RTGCPHYS32 GCPhysVMMDevRAM;
    /** R3 pointer to VMMDev RAM area */
    R3PTRTYPE(VMMDevMemory *) pVMMDevRAMR3;

    /** R3 pointer to VMMDev Heap RAM area
     */
    R3PTRTYPE(VMMDevMemory *) pVMMDevHeapR3;
    /** GC physical address of VMMDev Heap RAM area */
    RTGCPHYS32 GCPhysVMMDevHeap;

    /** Information reported by guest via VMMDevReportGuestInfo generic request.
     * Until this information is reported the VMMDev refuses any other requests.
     */
    VBoxGuestInfo guestInfo;
    /** Information report \#2, chewed a litte. */
    struct
    {
        uint32_t uFullVersion; /**< non-zero if info is present. */
        uint32_t uRevision;
        uint32_t fFeatures;
        char     szName[128];
    } guestInfo2;

    /** Array of guest facility statuses. */
    VMMDEVFACILITYSTATUSENTRY   aFacilityStatuses[32];
    /** The number of valid entries in the facility status array. */
    uint32_t                    cFacilityStatuses;

    /** Information reported by guest via VMMDevReportGuestCapabilities. */
    uint32_t      guestCaps;

    /** "Additions are Ok" indicator, set to true after processing VMMDevReportGuestInfo,
     * if additions version is compatible. This flag is here to avoid repeated comparing
     * of the version in guestInfo.
     */
    uint32_t fu32AdditionsOk;

    /** Video acceleration status set by guest. */
    uint32_t u32VideoAccelEnabled;

    DISPLAYCHANGEDATA displayChangeData;

    /** Pointer to the credentials. */
    R3PTRTYPE(VMMDEVCREDS *) pCredentials;

    bool afAlignment4[HC_ARCH_BITS == 32 ? 3 : 7];

    /* memory balloon change request */
    uint32_t    cMbMemoryBalloon;
    /** The last balloon size queried by the guest additions. */
    uint32_t    cMbMemoryBalloonLast;

    /* guest ram size */
    uint64_t    cbGuestRAM;

    /* unique session id; the id will be different after each start, reset or restore of the VM. */
    uint64_t    idSession;

    /* statistics interval change request */
    uint32_t    u32StatIntervalSize, u32LastStatIntervalSize;

    /* seamless mode change request */
    bool fLastSeamlessEnabled, fSeamlessEnabled;
    bool afAlignment5[1];

    bool fVRDPEnabled;
    uint32_t uVRDPExperienceLevel;

#ifdef VMMDEV_WITH_ALT_TIMESYNC
    uint64_t hostTime;
    bool fTimesyncBackdoorLo;
    bool afAlignment6[3];
#endif
    /** Set if GetHostTime should fail.
     * Loaded from the GetHostTimeDisabled configuration value. */
    bool fGetHostTimeDisabled;

    /** Set if backdoor logging should be disabled (output will be ignored then) */
    bool fBackdoorLogDisabled;

    /** Don't clear credentials */
    bool fKeepCredentials;

    /** Heap enabled. */
    bool fHeapEnabled;

    /** Guest Core Dumping enabled. */
    bool fGuestCoreDumpEnabled;

    /** Guest Core Dump location. */
    char szGuestCoreDumpDir[RTPATH_MAX];

    /** Number of additional cores to keep around. */
    uint32_t cGuestCoreDumps;

    bool afAlignment7[1];

#ifdef VBOX_WITH_HGCM
    /** List of pending HGCM requests (VBOXHGCMCMD). */
    RTLISTANCHORR3 listHGCMCmd;
    /** Critical section to protect the list. */
    RTCRITSECT critsectHGCMCmdList;
    /** Whether the HGCM events are already automatically enabled. */
    uint32_t u32HGCMEnabled;
    /** Saved state version of restored commands. */
    uint32_t u32SSMVersion;
#if HC_ARCH_BITS == 32
    /** Alignment padding. */
    uint32_t u32Alignment7;
#endif
#endif /* VBOX_WITH_HGCM */

    /** Status LUN: Shared folders LED */
    struct
    {
        /** The LED. */
        PDMLED                              Led;
        /** The LED ports. */
        PDMILEDPORTS                        ILeds;
        /** Partner of ILeds. */
        R3PTRTYPE(PPDMILEDCONNECTORS)       pLedsConnector;
    } SharedFolders;

    /** FLag whether CPU hotplug events are monitored */
    bool                fCpuHotPlugEventsEnabled;
    /** Alignment padding. */
    bool                afPadding8[3];
    /** CPU hotplug event */
    VMMDevCpuEventType  enmCpuHotPlugEvent;
    /** Core id of the CPU to change */
    uint32_t            idCpuCore;
    /** Package id of the CPU to change */
    uint32_t            idCpuPackage;

    uint32_t            StatMemBalloonChunks;

    /** Set if RC/R0 is enabled. */
    bool                fRZEnabled;
    /** Set if testing is enabled. */
    bool                fTestingEnabled;
    /** Set if testing the MMIO testing range is enabled. */
    bool                fTestingMMIO;
    /** Alignment padding. */
    bool                afPadding9[HC_ARCH_BITS == 32 ? 1 : 5];
#ifndef VBOX_WITHOUT_TESTING_FEATURES
    /** The high timestamp value. */
    uint32_t            u32TestingHighTimestamp;
    /** The current testing command (VMMDEV_TESTING_CMD_XXX). */
    uint32_t            u32TestingCmd;
    /** The testing data offset (command specific). */
    uint32_t            offTestingData;
    /** For buffering the what comes in over the testing data port. */
    union
    {
        char            padding[1024];

        /** VMMDEV_TESTING_CMD_INIT, VMMDEV_TESTING_CMD_SUB_NEW,
         *  VMMDEV_TESTING_CMD_FAILED. */
        struct
        {
            char        sz[1024];
        } String, Init, SubNew, Failed;

        /** VMMDEV_TESTING_CMD_TERM, VMMDEV_TESTING_CMD_SUB_DONE. */
        struct
        {
            uint32_t    c;
        } Error, Term, SubDone;

        /** VMMDEV_TESTING_CMD_VALUE. */
        struct
        {
            RTUINT64U   u64Value;
            uint32_t    u32Unit;
            char        szName[1024 - 8 - 4];
        } Value;

        /** The read back register (VMMDEV_TESTING_MMIO_OFF_READBACK,
         *  VMMDEV_TESTING_MMIO_OFF_READBACK_R3). */
        uint8_t         abReadBack[VMMDEV_TESTING_READBACK_SIZE];
    } TestingData;
    /** The XML output file name (can be a named pipe, doesn't matter to us). */
    R3PTRTYPE(char *)       pszTestingXmlOutput;
    /** Testing instance for dealing with the output. */
    RTTEST                  hTestingTest;
#endif /* !VBOX_WITHOUT_TESTING_FEATURES */

    /** @name Heartbeat
     * @{ */
    /** Timestamp of the last heartbeat from guest in nanosec. */
    uint64_t volatile   nsLastHeartbeatTS;
    /** Indicates whether we missed HB from guest on last check. */
    bool volatile       fFlatlined;
    /** Indicates whether heartbeat check is active. */
    bool volatile       fHeartbeatActive;
    /** Alignment padding. */
    bool                afAlignment8[6];
    /** Guest heartbeat interval in nanoseconds.
     * This is the interval the guest is told to produce heartbeats at. */
    uint64_t            cNsHeartbeatInterval;
    /** The amount of time without a heartbeat (nanoseconds) before we
     * conclude the guest is doing a Dixie Flatline (Neuromancer) impression. */
    uint64_t            cNsHeartbeatTimeout;
    /** Timer for signalling a flatlined guest. */
    PTMTIMERR3          pFlatlinedTimer;
    /** @} */
} VMMDevState;
typedef VMMDevState VMMDEV;
/** Pointer to the VMM device state. */
typedef VMMDEV *PVMMDEV;
AssertCompileMemberAlignment(VMMDEV, CritSect, 8);
AssertCompileMemberAlignment(VMMDEV, cbGuestRAM, 8);
AssertCompileMemberAlignment(VMMDEV, enmCpuHotPlugEvent, 4);
AssertCompileMemberAlignment(VMMDEV, aFacilityStatuses, 8);
#ifndef VBOX_WITHOUT_TESTING_FEATURES
AssertCompileMemberAlignment(VMMDEV, TestingData.Value.u64Value, 8);
#endif


void VMMDevNotifyGuest(VMMDEV *pVMMDevState, uint32_t u32EventMask);
void VMMDevCtlSetGuestFilterMask(VMMDEV *pVMMDevState, uint32_t u32OrMask, uint32_t u32NotMask);

/** The saved state version. */
#define VMMDEV_SAVED_STATE_VERSION                              VMMDEV_SAVED_STATE_VERSION_HGCM_PARAMS
/** Updated HGCM commands. */
#define VMMDEV_SAVED_STATE_VERSION_HGCM_PARAMS                  17
/** The saved state version with heartbeat state. */
#define VMMDEV_SAVED_STATE_VERSION_HEARTBEAT                    16
/** The saved state version without heartbeat state. */
#define VMMDEV_SAVED_STATE_VERSION_NO_HEARTBEAT                 15
/** The saved state version which is missing the guest facility statuses. */
#define VMMDEV_SAVED_STATE_VERSION_MISSING_FACILITY_STATUSES    14
/** The saved state version which is missing the guestInfo2 bits. */
#define VMMDEV_SAVED_STATE_VERSION_MISSING_GUEST_INFO_2         13
/** The saved state version used by VirtualBox 3.0.
 *  This doesn't have the config part. */
#define VMMDEV_SAVED_STATE_VERSION_VBOX_30                      11

#endif /* !___VMMDev_VMMDevState_h */

