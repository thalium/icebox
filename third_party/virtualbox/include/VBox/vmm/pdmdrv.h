/** @file
 * PDM - Pluggable Device Manager, Drivers.
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

#ifndef ___VBox_vmm_pdmdrv_h
#define ___VBox_vmm_pdmdrv_h

#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmthread.h>
#include <VBox/vmm/pdmifs.h>
#include <VBox/vmm/pdmins.h>
#include <VBox/vmm/pdmcommon.h>
#include <VBox/vmm/pdmasynccompletion.h>
#ifdef VBOX_WITH_NETSHAPER
#include <VBox/vmm/pdmnetshaper.h>
#endif /* VBOX_WITH_NETSHAPER */
#include <VBox/vmm/pdmblkcache.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/ftm.h>
#include <VBox/err.h>
#include <iprt/stdarg.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_driver    The PDM Drivers API
 * @ingroup grp_pdm
 * @{
 */

/** Pointer const PDM Driver API, ring-3. */
typedef R3PTRTYPE(struct PDMDRVHLPR3 const *) PCPDMDRVHLPR3;
/** Pointer const PDM Driver API, ring-0. */
typedef R0PTRTYPE(struct PDMDRVHLPR0 const *) PCPDMDRVHLPR0;
/** Pointer const PDM Driver API, raw-mode context. */
typedef RCPTRTYPE(struct PDMDRVHLPRC const *) PCPDMDRVHLPRC;


/**
 * Construct a driver instance for a VM.
 *
 * @returns VBox status.
 * @param   pDrvIns     The driver instance data. If the registration structure
 *                      is needed, it can be accessed thru pDrvIns->pReg.
 * @param   pCfg        Configuration node handle for the driver.  This is
 *                      expected to be in high demand in the constructor and is
 *                      therefore passed as an argument.  When using it at other
 *                      times, it can be accessed via pDrvIns->pCfg.
 * @param   fFlags      Flags, combination of the PDM_TACH_FLAGS_* \#defines.
 */
typedef DECLCALLBACK(int)   FNPDMDRVCONSTRUCT(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
/** Pointer to a FNPDMDRVCONSTRUCT() function. */
typedef FNPDMDRVCONSTRUCT *PFNPDMDRVCONSTRUCT;

/**
 * Destruct a driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that
 * any non-VM resources can be freed correctly.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACK(void)   FNPDMDRVDESTRUCT(PPDMDRVINS pDrvIns);
/** Pointer to a FNPDMDRVDESTRUCT() function. */
typedef FNPDMDRVDESTRUCT *PFNPDMDRVDESTRUCT;

/**
 * Driver relocation callback.
 *
 * This is called when the instance data has been relocated in raw-mode context
 * (RC).  It is also called when the RC hypervisor selects changes.  The driver
 * must fixup all necessary pointers and re-query all interfaces to other RC
 * devices and drivers.
 *
 * Before the RC code is executed the first time, this function will be called
 * with a 0 delta so RC pointer calculations can be one in one place.
 *
 * @param   pDrvIns     Pointer to the driver instance.
 * @param   offDelta    The relocation delta relative to the old location.
 *
 * @remark  A relocation CANNOT fail.
 */
typedef DECLCALLBACK(void) FNPDMDRVRELOCATE(PPDMDRVINS pDrvIns, RTGCINTPTR offDelta);
/** Pointer to a FNPDMDRVRELOCATE() function. */
typedef FNPDMDRVRELOCATE *PFNPDMDRVRELOCATE;

/**
 * Driver I/O Control interface.
 *
 * This is used by external components, such as the COM interface, to
 * communicate with a driver using a driver specific interface. Generally,
 * the driver interfaces are used for this task.
 *
 * @returns VBox status code.
 * @param   pDrvIns     Pointer to the driver instance.
 * @param   uFunction   Function to perform.
 * @param   pvIn        Pointer to input data.
 * @param   cbIn        Size of input data.
 * @param   pvOut       Pointer to output data.
 * @param   cbOut       Size of output data.
 * @param   pcbOut      Where to store the actual size of the output data.
 */
typedef DECLCALLBACK(int) FNPDMDRVIOCTL(PPDMDRVINS pDrvIns, uint32_t uFunction,
                                        void *pvIn, uint32_t cbIn,
                                        void *pvOut, uint32_t cbOut, uint32_t *pcbOut);
/** Pointer to a FNPDMDRVIOCTL() function. */
typedef FNPDMDRVIOCTL *PFNPDMDRVIOCTL;

/**
 * Power On notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACK(void)   FNPDMDRVPOWERON(PPDMDRVINS pDrvIns);
/** Pointer to a FNPDMDRVPOWERON() function. */
typedef FNPDMDRVPOWERON *PFNPDMDRVPOWERON;

/**
 * Reset notification.
 *
 * @returns VBox status.
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACK(void)  FNPDMDRVRESET(PPDMDRVINS pDrvIns);
/** Pointer to a FNPDMDRVRESET() function. */
typedef FNPDMDRVRESET *PFNPDMDRVRESET;

/**
 * Suspend notification.
 *
 * @returns VBox status.
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACK(void)  FNPDMDRVSUSPEND(PPDMDRVINS pDrvIns);
/** Pointer to a FNPDMDRVSUSPEND() function. */
typedef FNPDMDRVSUSPEND *PFNPDMDRVSUSPEND;

/**
 * Resume notification.
 *
 * @returns VBox status.
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACK(void)  FNPDMDRVRESUME(PPDMDRVINS pDrvIns);
/** Pointer to a FNPDMDRVRESUME() function. */
typedef FNPDMDRVRESUME *PFNPDMDRVRESUME;

/**
 * Power Off notification.
 *
 * This is always called when VMR3PowerOff is called.
 * There will be no callback when hot plugging devices or when replumbing the driver
 * stack.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACK(void)   FNPDMDRVPOWEROFF(PPDMDRVINS pDrvIns);
/** Pointer to a FNPDMDRVPOWEROFF() function. */
typedef FNPDMDRVPOWEROFF *PFNPDMDRVPOWEROFF;

/**
 * Attach command.
 *
 * This is called to let the driver attach to a driver at runtime.  This is not
 * called during VM construction, the driver constructor have to do this by
 * calling PDMDrvHlpAttach.
 *
 * This is like plugging in the keyboard or mouse after turning on the PC.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The driver instance.
 * @param   fFlags      Flags, combination of the PDM_TACH_FLAGS_* \#defines.
 */
typedef DECLCALLBACK(int)  FNPDMDRVATTACH(PPDMDRVINS pDrvIns, uint32_t fFlags);
/** Pointer to a FNPDMDRVATTACH() function. */
typedef FNPDMDRVATTACH *PFNPDMDRVATTACH;

/**
 * Detach notification.
 *
 * This is called when a driver below it in the chain is detaching itself
 * from it. The driver should adjust it's state to reflect this.
 *
 * This is like ejecting a cdrom or floppy.
 *
 * @param   pDrvIns     The driver instance.
 * @param   fFlags      PDM_TACH_FLAGS_NOT_HOT_PLUG or 0.
 */
typedef DECLCALLBACK(void)  FNPDMDRVDETACH(PPDMDRVINS pDrvIns, uint32_t fFlags);
/** Pointer to a FNPDMDRVDETACH() function. */
typedef FNPDMDRVDETACH *PFNPDMDRVDETACH;



/**
 * PDM Driver Registration Structure.
 *
 * This structure is used when registering a driver from VBoxInitDrivers() (in
 * host ring-3 context).  PDM will continue use till the VM is terminated.
 */
typedef struct PDMDRVREG
{
    /** Structure version. PDM_DRVREG_VERSION defines the current version. */
    uint32_t            u32Version;
    /** Driver name. */
    char                szName[32];
    /** Name of the raw-mode context module (no path).
     * Only evalutated if PDM_DRVREG_FLAGS_RC is set. */
    char                szRCMod[32];
    /** Name of the ring-0 module (no path).
     * Only evalutated if PDM_DRVREG_FLAGS_R0 is set. */
    char                szR0Mod[32];
    /** The description of the driver. The UTF-8 string pointed to shall, like this structure,
     * remain unchanged from registration till VM destruction. */
    const char         *pszDescription;

    /** Flags, combination of the PDM_DRVREG_FLAGS_* \#defines. */
    uint32_t            fFlags;
    /** Driver class(es), combination of the PDM_DRVREG_CLASS_* \#defines. */
    uint32_t            fClass;
    /** Maximum number of instances (per VM). */
    uint32_t            cMaxInstances;
    /** Size of the instance data. */
    uint32_t            cbInstance;

    /** Construct instance - required. */
    PFNPDMDRVCONSTRUCT  pfnConstruct;
    /** Destruct instance - optional. */
    PFNPDMDRVDESTRUCT   pfnDestruct;
    /** Relocation command - optional. */
    PFNPDMDRVRELOCATE   pfnRelocate;
    /** I/O control - optional. */
    PFNPDMDRVIOCTL      pfnIOCtl;
    /** Power on notification - optional. */
    PFNPDMDRVPOWERON    pfnPowerOn;
    /** Reset notification - optional. */
    PFNPDMDRVRESET      pfnReset;
    /** Suspend notification  - optional. */
    PFNPDMDRVSUSPEND    pfnSuspend;
    /** Resume notification - optional. */
    PFNPDMDRVRESUME     pfnResume;
    /** Attach command - optional. */
    PFNPDMDRVATTACH     pfnAttach;
    /** Detach notification - optional. */
    PFNPDMDRVDETACH     pfnDetach;
    /** Power off notification - optional. */
    PFNPDMDRVPOWEROFF   pfnPowerOff;
    /** @todo */
    PFNRT               pfnSoftReset;
    /** Initialization safty marker. */
    uint32_t            u32VersionEnd;
} PDMDRVREG;
/** Pointer to a PDM Driver Structure. */
typedef PDMDRVREG *PPDMDRVREG;
/** Const pointer to a PDM Driver Structure. */
typedef PDMDRVREG const *PCPDMDRVREG;

/** Current DRVREG version number. */
#define PDM_DRVREG_VERSION                      PDM_VERSION_MAKE(0xf0ff, 1, 0)

/** PDM Driver Flags.
 * @{ */
/** @def PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT
 * The bit count for the current host. */
#if HC_ARCH_BITS == 32
# define PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT     UINT32_C(0x00000001)
#elif HC_ARCH_BITS == 64
# define PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT     UINT32_C(0x00000002)
#else
# error Unsupported HC_ARCH_BITS value.
#endif
/** The host bit count mask. */
#define PDM_DRVREG_FLAGS_HOST_BITS_MASK         UINT32_C(0x00000003)
/** This flag is used to indicate that the driver has a RC component. */
#define PDM_DRVREG_FLAGS_RC                     UINT32_C(0x00000010)
/** This flag is used to indicate that the driver has a R0 component. */
#define PDM_DRVREG_FLAGS_R0                     UINT32_C(0x00000020)

/** @} */


/** PDM Driver Classes.
 * @{ */
/** Mouse input driver. */
#define PDM_DRVREG_CLASS_MOUSE          RT_BIT(0)
/** Keyboard input driver. */
#define PDM_DRVREG_CLASS_KEYBOARD       RT_BIT(1)
/** Display driver. */
#define PDM_DRVREG_CLASS_DISPLAY        RT_BIT(2)
/** Network transport driver. */
#define PDM_DRVREG_CLASS_NETWORK        RT_BIT(3)
/** Block driver. */
#define PDM_DRVREG_CLASS_BLOCK          RT_BIT(4)
/** Media driver. */
#define PDM_DRVREG_CLASS_MEDIA          RT_BIT(5)
/** Mountable driver. */
#define PDM_DRVREG_CLASS_MOUNTABLE      RT_BIT(6)
/** Audio driver. */
#define PDM_DRVREG_CLASS_AUDIO          RT_BIT(7)
/** VMMDev driver. */
#define PDM_DRVREG_CLASS_VMMDEV         RT_BIT(8)
/** Status driver. */
#define PDM_DRVREG_CLASS_STATUS         RT_BIT(9)
/** ACPI driver. */
#define PDM_DRVREG_CLASS_ACPI           RT_BIT(10)
/** USB related driver. */
#define PDM_DRVREG_CLASS_USB            RT_BIT(11)
/** ISCSI Transport related driver. */
#define PDM_DRVREG_CLASS_ISCSITRANSPORT RT_BIT(12)
/** Char driver. */
#define PDM_DRVREG_CLASS_CHAR           RT_BIT(13)
/** Stream driver. */
#define PDM_DRVREG_CLASS_STREAM         RT_BIT(14)
/** SCSI driver. */
#define PDM_DRVREG_CLASS_SCSI           RT_BIT(15)
/** Generic raw PCI device driver. */
#define PDM_DRVREG_CLASS_PCIRAW         RT_BIT(16)
/** @} */


/**
 * PDM Driver Instance.
 *
 * @implements  PDMIBASE
 */
typedef struct PDMDRVINS
{
    /** Structure version. PDM_DRVINS_VERSION defines the current version. */
    uint32_t                    u32Version;
    /** Driver instance number. */
    uint32_t                    iInstance;

    /** Pointer the PDM Driver API. */
    RCPTRTYPE(PCPDMDRVHLPRC)    pHlpRC;
    /** Pointer to driver instance data. */
    RCPTRTYPE(void *)           pvInstanceDataRC;

    /** Pointer the PDM Driver API. */
    R0PTRTYPE(PCPDMDRVHLPR0)    pHlpR0;
    /** Pointer to driver instance data. */
    R0PTRTYPE(void *)           pvInstanceDataR0;

    /** Pointer the PDM Driver API. */
    R3PTRTYPE(PCPDMDRVHLPR3)    pHlpR3;
    /** Pointer to driver instance data. */
    R3PTRTYPE(void *)           pvInstanceDataR3;

    /** Pointer to driver registration structure.  */
    R3PTRTYPE(PCPDMDRVREG)      pReg;
    /** Configuration handle. */
    R3PTRTYPE(PCFGMNODE)        pCfg;

    /** Pointer to the base interface of the device/driver instance above. */
    R3PTRTYPE(PPDMIBASE)        pUpBase;
    /** Pointer to the base interface of the driver instance below. */
    R3PTRTYPE(PPDMIBASE)        pDownBase;

    /** The base interface of the driver.
     * The driver constructor initializes this. */
    PDMIBASE                    IBase;

    /** Tracing indicator. */
    uint32_t                    fTracing;
    /** The tracing ID of this device.  */
    uint32_t                    idTracing;
#if HC_ARCH_BITS == 32
    /** Align the internal data more naturally. */
    uint32_t                    au32Padding[HC_ARCH_BITS == 32 ? 7 : 0];
#endif

    /** Internal data. */
    union
    {
#ifdef PDMDRVINSINT_DECLARED
        PDMDRVINSINT            s;
#endif
        uint8_t                 padding[HC_ARCH_BITS == 32 ? 40 + 32 : 72 + 24];
    } Internal;

    /** Driver instance data. The size of this area is defined
     * in the PDMDRVREG::cbInstanceData field. */
    char                        achInstanceData[4];
} PDMDRVINS;

/** Current DRVREG version number. */
#define PDM_DRVINS_VERSION                      PDM_VERSION_MAKE(0xf0fe, 2, 0)

/** Converts a pointer to the PDMDRVINS::IBase to a pointer to PDMDRVINS. */
#define PDMIBASE_2_PDMDRV(pInterface)   ( (PPDMDRVINS)((char *)(pInterface) - RT_UOFFSETOF(PDMDRVINS, IBase)) )

/** @def PDMDRVINS_2_RCPTR
 * Converts a PDM Driver instance pointer a RC PDM Driver instance pointer.
 */
#define PDMDRVINS_2_RCPTR(pDrvIns)      ( (RCPTRTYPE(PPDMDRVINS))((RTRCUINTPTR)(pDrvIns)->pvInstanceDataRC - (RTRCUINTPTR)RT_UOFFSETOF(PDMDRVINS, achInstanceData)) )

/** @def PDMDRVINS_2_R3PTR
 * Converts a PDM Driver instance pointer a R3 PDM Driver instance pointer.
 */
#define PDMDRVINS_2_R3PTR(pDrvIns)      ( (R3PTRTYPE(PPDMDRVINS))((RTHCUINTPTR)(pDrvIns)->pvInstanceDataR3 - RT_UOFFSETOF(PDMDRVINS, achInstanceData)) )

/** @def PDMDRVINS_2_R0PTR
 * Converts a PDM Driver instance pointer a R0 PDM Driver instance pointer.
 */
#define PDMDRVINS_2_R0PTR(pDrvIns)      ( (R0PTRTYPE(PPDMDRVINS))((RTR0UINTPTR)(pDrvIns)->pvInstanceDataR0 - RT_UOFFSETOF(PDMDRVINS, achInstanceData)) )



/**
 * Checks the structure versions of the drive instance and driver helpers,
 * returning if they are incompatible.
 *
 * Intended for the constructor.
 *
 * @param   pDrvIns             Pointer to the PDM driver instance.
 */
#define PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns) \
    do \
    { \
        PPDMDRVINS pDrvInsTypeCheck = (pDrvIns); NOREF(pDrvInsTypeCheck); \
        AssertLogRelMsgReturn(PDM_VERSION_ARE_COMPATIBLE((pDrvIns)->u32Version, PDM_DRVINS_VERSION), \
                              ("DrvIns=%#x  mine=%#x\n", (pDrvIns)->u32Version, PDM_DRVINS_VERSION), \
                              VERR_PDM_DRVINS_VERSION_MISMATCH); \
        AssertLogRelMsgReturn(PDM_VERSION_ARE_COMPATIBLE((pDrvIns)->pHlpR3->u32Version, PDM_DRVHLPR3_VERSION), \
                              ("DrvHlp=%#x  mine=%#x\n", (pDrvIns)->pHlpR3->u32Version, PDM_DRVHLPR3_VERSION), \
                              VERR_PDM_DRVHLPR3_VERSION_MISMATCH); \
    } while (0)

/**
 * Quietly checks the structure versions of the drive instance and driver
 * helpers, returning if they are incompatible.
 *
 * Intended for the destructor.
 *
 * @param   pDrvIns             Pointer to the PDM driver instance.
 */
#define PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns) \
    do \
    { \
        PPDMDRVINS pDrvInsTypeCheck = (pDrvIns); NOREF(pDrvInsTypeCheck); \
        if (RT_LIKELY(   PDM_VERSION_ARE_COMPATIBLE((pDrvIns)->u32Version, PDM_DRVINS_VERSION) \
                      && PDM_VERSION_ARE_COMPATIBLE((pDrvIns)->pHlpR3->u32Version, PDM_DRVHLPR3_VERSION)) ) \
        { /* likely */ } else return; \
    } while (0)

/**
 * Wrapper around CFGMR3ValidateConfig for the root config for use in the
 * constructor - returns on failure.
 *
 * This should be invoked after having initialized the instance data
 * sufficiently for the correct operation of the destructor.  The destructor is
 * always called!
 *
 * @param   pDrvIns             Pointer to the PDM driver instance.
 * @param   pszValidValues      Patterns describing the valid value names.  See
 *                              RTStrSimplePatternMultiMatch for details on the
 *                              pattern syntax.
 * @param   pszValidNodes       Patterns describing the valid node (key) names.
 *                              Pass empty string if no valid nodess.
 */
#define PDMDRV_VALIDATE_CONFIG_RETURN(pDrvIns, pszValidValues, pszValidNodes) \
    do \
    { \
        int rcValCfg = CFGMR3ValidateConfig((pDrvIns)->pCfg, "/", pszValidValues, pszValidNodes, \
                                            (pDrvIns)->pReg->szName, (pDrvIns)->iInstance); \
        if (RT_SUCCESS(rcValCfg)) \
        { /* likely */ } else return rcValCfg; \
    } while (0)



/**
 * USB hub registration structure.
 */
typedef struct PDMUSBHUBREG
{
    /** Structure version number. PDM_USBHUBREG_VERSION defines the current version. */
    uint32_t            u32Version;

    /**
     * Request the hub to attach of the specified device.
     *
     * @returns VBox status code.
     * @param   pDrvIns            The hub instance.
     * @param   pUsbIns            The device to attach.
     * @param   pszCaptureFilename Path to the file for USB traffic capturing, optional.
     * @param   piPort             Where to store the port number the device was attached to.
     * @thread EMT.
     */
    DECLR3CALLBACKMEMBER(int, pfnAttachDevice,(PPDMDRVINS pDrvIns, PPDMUSBINS pUsbIns, const char *pszCaptureFilename, uint32_t *piPort));

    /**
     * Request the hub to detach of the specified device.
     *
     * The device has previously been attached to the hub with the
     * pfnAttachDevice call. This call is not currently expected to
     * fail.
     *
     * @returns VBox status code.
     * @param   pDrvIns     The hub instance.
     * @param   pUsbIns     The device to detach.
     * @param   iPort       The port number returned by the attach call.
     * @thread EMT.
     */
    DECLR3CALLBACKMEMBER(int, pfnDetachDevice,(PPDMDRVINS pDrvIns, PPDMUSBINS pUsbIns, uint32_t iPort));

    /** Counterpart to u32Version, same value. */
    uint32_t            u32TheEnd;
} PDMUSBHUBREG;
/** Pointer to a const USB hub registration structure. */
typedef const PDMUSBHUBREG *PCPDMUSBHUBREG;

/** Current PDMUSBHUBREG version number. */
#define PDM_USBHUBREG_VERSION                   PDM_VERSION_MAKE(0xf0fd, 2, 0)


/**
 * USB hub helpers.
 * This is currently just a place holder.
 */
typedef struct PDMUSBHUBHLP
{
    /** Structure version. PDM_USBHUBHLP_VERSION defines the current version. */
    uint32_t                    u32Version;

    /** Just a safety precaution. */
    uint32_t                    u32TheEnd;
} PDMUSBHUBHLP;
/** Pointer to PCI helpers. */
typedef PDMUSBHUBHLP *PPDMUSBHUBHLP;
/** Pointer to const PCI helpers. */
typedef const PDMUSBHUBHLP *PCPDMUSBHUBHLP;
/** Pointer to const PCI helpers pointer. */
typedef PCPDMUSBHUBHLP *PPCPDMUSBHUBHLP;

/** Current PDMUSBHUBHLP version number. */
#define PDM_USBHUBHLP_VERSION                   PDM_VERSION_MAKE(0xf0fc, 1, 0)


/**
 * PDM Driver API - raw-mode context variant.
 */
typedef struct PDMDRVHLPRC
{
    /** Structure version. PDM_DRVHLPRC_VERSION defines the current version. */
    uint32_t                    u32Version;

    /**
     * Set the VM error message
     *
     * @returns rc.
     * @param   pDrvIns         Driver instance.
     * @param   rc              VBox status code.
     * @param   SRC_POS         Use RT_SRC_POS.
     * @param   pszFormat       Error message format string.
     * @param   ...             Error message arguments.
     */
    DECLRCCALLBACKMEMBER(int, pfnVMSetError,(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL,
                                             const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(6, 7));

    /**
     * Set the VM error message
     *
     * @returns rc.
     * @param   pDrvIns         Driver instance.
     * @param   rc              VBox status code.
     * @param   SRC_POS         Use RT_SRC_POS.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLRCCALLBACKMEMBER(int, pfnVMSetErrorV,(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL,
                                              const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(6, 0));

    /**
     * Set the VM runtime error message
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
     * @param   pszErrorId      Error ID string.
     * @param   pszFormat       Error message format string.
     * @param   ...             Error message arguments.
     */
    DECLRCCALLBACKMEMBER(int, pfnVMSetRuntimeError,(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                    const char *pszFormat, ...)  RT_IPRT_FORMAT_ATTR(4, 5));

    /**
     * Set the VM runtime error message
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
     * @param   pszErrorId      Error ID string.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLRCCALLBACKMEMBER(int, pfnVMSetRuntimeErrorV,(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                     const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(4, 0));

    /**
     * Assert that the current thread is the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLRCCALLBACKMEMBER(bool, pfnAssertEMT,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Assert that the current thread is NOT the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLRCCALLBACKMEMBER(bool, pfnAssertOther,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Notify FTM about a checkpoint occurrence
     *
     * @param   pDrvIns             The driver instance.
     * @param   enmType             Checkpoint type
     * @thread  Any
     */
    DECLRCCALLBACKMEMBER(int, pfnFTSetCheckpoint,(PPDMDRVINS pDrvIns, FTMCHECKPOINTTYPE enmType));

    /** Just a safety precaution. */
    uint32_t                        u32TheEnd;
} PDMDRVHLPRC;
/** Current PDMDRVHLPRC version number. */
#define PDM_DRVHLPRC_VERSION                    PDM_VERSION_MAKE(0xf0f9, 2, 0)


/**
 * PDM Driver API, ring-0 context.
 */
typedef struct PDMDRVHLPR0
{
    /** Structure version. PDM_DRVHLPR0_VERSION defines the current version. */
    uint32_t                    u32Version;

    /**
     * Set the VM error message
     *
     * @returns rc.
     * @param   pDrvIns         Driver instance.
     * @param   rc              VBox status code.
     * @param   SRC_POS         Use RT_SRC_POS.
     * @param   pszFormat       Error message format string.
     * @param   ...             Error message arguments.
     */
    DECLR0CALLBACKMEMBER(int, pfnVMSetError,(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL,
                                             const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(6, 7));

    /**
     * Set the VM error message
     *
     * @returns rc.
     * @param   pDrvIns         Driver instance.
     * @param   rc              VBox status code.
     * @param   SRC_POS         Use RT_SRC_POS.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLR0CALLBACKMEMBER(int, pfnVMSetErrorV,(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL,
                                              const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(6, 0));

    /**
     * Set the VM runtime error message
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
     * @param   pszErrorId      Error ID string.
     * @param   pszFormat       Error message format string.
     * @param   ...             Error message arguments.
     */
    DECLR0CALLBACKMEMBER(int, pfnVMSetRuntimeError,(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                    const char *pszFormat, ...)  RT_IPRT_FORMAT_ATTR(4, 5));

    /**
     * Set the VM runtime error message
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
     * @param   pszErrorId      Error ID string.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLR0CALLBACKMEMBER(int, pfnVMSetRuntimeErrorV,(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                     const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(4, 0));

    /**
     * Assert that the current thread is the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR0CALLBACKMEMBER(bool, pfnAssertEMT,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Assert that the current thread is NOT the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR0CALLBACKMEMBER(bool, pfnAssertOther,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Notify FTM about a checkpoint occurrence
     *
     * @param   pDrvIns             The driver instance.
     * @param   enmType             Checkpoint type
     * @thread  Any
     */
    DECLR0CALLBACKMEMBER(int, pfnFTSetCheckpoint,(PPDMDRVINS pDrvIns, FTMCHECKPOINTTYPE enmType));

    /** Just a safety precaution. */
    uint32_t                        u32TheEnd;
} PDMDRVHLPR0;
/** Current DRVHLP version number. */
#define PDM_DRVHLPR0_VERSION                    PDM_VERSION_MAKE(0xf0f8, 2, 0)


#ifdef IN_RING3

/**
 * PDM Driver API.
 */
typedef struct PDMDRVHLPR3
{
    /** Structure version. PDM_DRVHLPR3_VERSION defines the current version. */
    uint32_t                    u32Version;

    /**
     * Attaches a driver (chain) to the driver.
     *
     * @returns VBox status code.
     * @param   pDrvIns             Driver instance.
     * @param   fFlags              PDM_TACH_FLAGS_NOT_HOT_PLUG or 0.
     * @param   ppBaseInterface     Where to store the pointer to the base interface.
     */
    DECLR3CALLBACKMEMBER(int, pfnAttach,(PPDMDRVINS pDrvIns, uint32_t fFlags, PPDMIBASE *ppBaseInterface));

    /**
     * Detach the driver the drivers below us.
     *
     * @returns VBox status code.
     * @param   pDrvIns             Driver instance.
     * @param   fFlags              PDM_TACH_FLAGS_NOT_HOT_PLUG or 0.
     */
    DECLR3CALLBACKMEMBER(int, pfnDetach,(PPDMDRVINS pDrvIns, uint32_t fFlags));

    /**
     * Detach the driver from the driver above it and destroy this
     * driver and all drivers below it.
     *
     * @returns VBox status code.
     * @param   pDrvIns             Driver instance.
     * @param   fFlags              PDM_TACH_FLAGS_NOT_HOT_PLUG or 0.
     */
    DECLR3CALLBACKMEMBER(int, pfnDetachSelf,(PPDMDRVINS pDrvIns, uint32_t fFlags));

    /**
     * Prepare a media mount.
     *
     * The driver must not have anything attached to itself
     * when calling this function as the purpose is to set up the configuration
     * of an future attachment.
     *
     * @returns VBox status code
     * @param   pDrvIns             Driver instance.
     * @param   pszFilename     Pointer to filename. If this is NULL it assumed that the caller have
     *                          constructed a configuration which can be attached to the bottom driver.
     * @param   pszCoreDriver   Core driver name. NULL will cause autodetection. Ignored if pszFilanem is NULL.
     */
    DECLR3CALLBACKMEMBER(int, pfnMountPrepare,(PPDMDRVINS pDrvIns, const char *pszFilename, const char *pszCoreDriver));

    /**
     * Assert that the current thread is the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAssertEMT,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Assert that the current thread is NOT the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAssertOther,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Set the VM error message
     *
     * @returns rc.
     * @param   pDrvIns         Driver instance.
     * @param   rc              VBox status code.
     * @param   SRC_POS         Use RT_SRC_POS.
     * @param   pszFormat       Error message format string.
     * @param   ...             Error message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMSetError,(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL,
                                             const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(6, 7));

    /**
     * Set the VM error message
     *
     * @returns rc.
     * @param   pDrvIns         Driver instance.
     * @param   rc              VBox status code.
     * @param   SRC_POS         Use RT_SRC_POS.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMSetErrorV,(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL,
                                              const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(6, 0));

    /**
     * Set the VM runtime error message
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
     * @param   pszErrorId      Error ID string.
     * @param   pszFormat       Error message format string.
     * @param   ...             Error message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMSetRuntimeError,(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                    const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(4, 5));

    /**
     * Set the VM runtime error message
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
     * @param   pszErrorId      Error ID string.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMSetRuntimeErrorV,(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                     const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(4, 0));

    /**
     * Gets the VM state.
     *
     * @returns VM state.
     * @param   pDrvIns         The driver instance.
     * @thread  Any thread (just keep in mind that it's volatile info).
     */
    DECLR3CALLBACKMEMBER(VMSTATE, pfnVMState, (PPDMDRVINS pDrvIns));

    /**
     * Checks if the VM was teleported and hasn't been fully resumed yet.
     *
     * @returns true / false.
     * @param   pDrvIns         The driver instance.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(bool, pfnVMTeleportedAndNotFullyResumedYet,(PPDMDRVINS pDrvIns));

    /**
     * Gets the support driver session.
     *
     * This is intended for working using the semaphore API.
     *
     * @returns Support driver session handle.
     * @param   pDrvIns         The driver instance.
     */
    DECLR3CALLBACKMEMBER(PSUPDRVSESSION, pfnGetSupDrvSession,(PPDMDRVINS pDrvIns));

    /**
     * Create a queue.
     *
     * @returns VBox status code.
     * @param   pDrvIns             Driver instance.
     * @param   cbItem              Size a queue item.
     * @param   cItems              Number of items in the queue.
     * @param   cMilliesInterval    Number of milliseconds between polling the queue.
     *                              If 0 then the emulation thread will be notified whenever an item arrives.
     * @param   pfnCallback         The consumer function.
     * @param   pszName             The queue base name. The instance number will be
     *                              appended automatically.
     * @param   ppQueue             Where to store the queue handle on success.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueueCreate,(PPDMDRVINS pDrvIns, uint32_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                              PFNPDMQUEUEDRV pfnCallback, const char *pszName, PPDMQUEUE *ppQueue));

    /**
     * Query the virtual timer frequency.
     *
     * @returns Frequency in Hz.
     * @param   pDrvIns             Driver instance.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMGetVirtualFreq,(PPDMDRVINS pDrvIns));

    /**
     * Query the virtual time.
     *
     * @returns The current virtual time.
     * @param   pDrvIns             Driver instance.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMGetVirtualTime,(PPDMDRVINS pDrvIns));

    /**
     * Creates a timer.
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   enmClock        The clock to use on this timer.
     * @param   pfnCallback     Callback function.
     * @param   pvUser          The user argument to the callback.
     * @param   fFlags          Timer creation flags, see grp_tm_timer_flags.
     * @param   pszDesc         Pointer to description string which must stay around
     *                          until the timer is fully destroyed (i.e. a bit after TMTimerDestroy()).
     * @param   ppTimer         Where to store the timer on success.
     * @thread  EMT
     */
    DECLR3CALLBACKMEMBER(int, pfnTMTimerCreate,(PPDMDRVINS pDrvIns, TMCLOCK enmClock, PFNTMTIMERDRV pfnCallback, void *pvUser, uint32_t fFlags, const char *pszDesc, PPTMTIMERR3 ppTimer));

    /**
     * Register a save state data unit.
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   uVersion        Data layout version number.
     * @param   cbGuess         The approximate amount of data in the unit.
     *                          Only for progress indicators.
     *
     * @param   pfnLivePrep     Prepare live save callback, optional.
     * @param   pfnLiveExec     Execute live save callback, optional.
     * @param   pfnLiveVote     Vote live save callback, optional.
     *
     * @param   pfnSavePrep     Prepare save callback, optional.
     * @param   pfnSaveExec     Execute save callback, optional.
     * @param   pfnSaveDone     Done save callback, optional.
     *
     * @param   pfnLoadPrep     Prepare load callback, optional.
     * @param   pfnLoadExec     Execute load callback, optional.
     * @param   pfnLoadDone     Done load callback, optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnSSMRegister,(PPDMDRVINS pDrvIns, uint32_t uVersion, size_t cbGuess,
                                              PFNSSMDRVLIVEPREP pfnLivePrep, PFNSSMDRVLIVEEXEC pfnLiveExec, PFNSSMDRVLIVEVOTE pfnLiveVote,
                                              PFNSSMDRVSAVEPREP pfnSavePrep, PFNSSMDRVSAVEEXEC pfnSaveExec, PFNSSMDRVSAVEDONE pfnSaveDone,
                                              PFNSSMDRVLOADPREP pfnLoadPrep, PFNSSMDRVLOADEXEC pfnLoadExec, PFNSSMDRVLOADDONE pfnLoadDone));

    /**
     * Deregister a save state data unit.
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   pszName         Data unit name.
     * @param   uInstance       The instance identifier of the data unit.
     *                          This must together with the name be unique.
     */
    DECLR3CALLBACKMEMBER(int, pfnSSMDeregister,(PPDMDRVINS pDrvIns, const char *pszName, uint32_t uInstance));

    /**
     * Register an info handler with DBGF.
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   pszName         Data unit name.
     * @param   pszDesc         The description of the info and any arguments
     *                          the handler may take.
     * @param   pfnHandler      The handler function to be called to display the
     *                          info.
     */
    DECLR3CALLBACKMEMBER(int, pfnDBGFInfoRegister,(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDRV pfnHandler));

    /**
     * Deregister an info handler from DBGF.
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   pszName         Data unit name.
     */
    DECLR3CALLBACKMEMBER(int, pfnDBGFInfoDeregister,(PPDMDRVINS pDrvIns, const char *pszName));

    /**
     * Registers a statistics sample if statistics are enabled.
     *
     * @param   pDrvIns     Driver instance.
     * @param   pvSample    Pointer to the sample.
     * @param   enmType     Sample type. This indicates what pvSample is pointing at.
     * @param   pszName     Sample name. The name is on this form "/<component>/<sample>".
     *                      Further nesting is possible.
     * @param   enmUnit     Sample unit.
     * @param   pszDesc     Sample description.
     */
    DECLR3CALLBACKMEMBER(void, pfnSTAMRegister,(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, const char *pszName,
                                                STAMUNIT enmUnit, const char *pszDesc));

    /**
     * Same as pfnSTAMRegister except that the name is specified in a
     * RTStrPrintf like fashion.
     *
     * @param   pDrvIns     Driver instance.
     * @param   pvSample    Pointer to the sample.
     * @param   enmType     Sample type. This indicates what pvSample is pointing at.
     * @param   enmVisibility  Visibility type specifying whether unused statistics should be visible or not.
     * @param   enmUnit     Sample unit.
     * @param   pszDesc     Sample description.
     * @param   pszName     The sample name format string.
     * @param   ...         Arguments to the format string.
     */
    DECLR3CALLBACKMEMBER(void, pfnSTAMRegisterF,(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                 STAMUNIT enmUnit, const char *pszDesc,
                                                 const char *pszName, ...) RT_IPRT_FORMAT_ATTR(7, 8));

    /**
     * Same as pfnSTAMRegister except that the name is specified in a
     * RTStrPrintfV like fashion.
     *
     * @param   pDrvIns         Driver instance.
     * @param   pvSample        Pointer to the sample.
     * @param   enmType         Sample type. This indicates what pvSample is pointing at.
     * @param   enmVisibility   Visibility type specifying whether unused statistics should be visible or not.
     * @param   enmUnit         Sample unit.
     * @param   pszDesc         Sample description.
     * @param   pszName         The sample name format string.
     * @param   args            Arguments to the format string.
     */
    DECLR3CALLBACKMEMBER(void, pfnSTAMRegisterV,(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                 STAMUNIT enmUnit, const char *pszDesc,
                                                 const char *pszName, va_list args) RT_IPRT_FORMAT_ATTR(7, 0));

    /**
     * Deregister a statistic item previously registered with pfnSTAMRegister,
     * pfnSTAMRegisterF or pfnSTAMRegisterV
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   pvSample        Pointer to the sample.
     */
    DECLR3CALLBACKMEMBER(int, pfnSTAMDeregister,(PPDMDRVINS pDrvIns, void *pvSample));

    /**
     * Calls the HC R0 VMM entry point, in a safer but slower manner than
     * SUPR3CallVMMR0.
     *
     * When entering using this call the R0 components can call into the host kernel
     * (i.e. use the SUPR0 and RT APIs).
     *
     * See VMMR0Entry() for more details.
     *
     * @returns error code specific to uFunction.
     * @param   pDrvIns     The driver instance.
     * @param   uOperation  Operation to execute.
     *                      This is limited to services.
     * @param   pvArg       Pointer to argument structure or if cbArg is 0 just an value.
     * @param   cbArg       The size of the argument. This is used to copy whatever the argument
     *                      points at into a kernel buffer to avoid problems like the user page
     *                      being invalidated while we're executing the call.
     */
    DECLR3CALLBACKMEMBER(int, pfnSUPCallVMMR0Ex,(PPDMDRVINS pDrvIns, unsigned uOperation, void *pvArg, unsigned cbArg));

    /**
     * Registers a USB HUB.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   fVersions       Indicates the kinds of USB devices that can be attached to this HUB.
     * @param   cPorts          The number of ports.
     * @param   pUsbHubReg      The hub callback structure that PDMUsb uses to interact with it.
     * @param   ppUsbHubHlp     The helper callback structure that the hub uses to talk to PDMUsb.
     *
     * @thread  EMT.
     */
    DECLR3CALLBACKMEMBER(int, pfnUSBRegisterHub,(PPDMDRVINS pDrvIns, uint32_t fVersions, uint32_t cPorts, PCPDMUSBHUBREG pUsbHubReg, PPCPDMUSBHUBHLP ppUsbHubHlp));

    /**
     * Set up asynchronous handling of a suspend, reset or power off notification.
     *
     * This shall only be called when getting the notification.  It must be called
     * for each one.
     *
     * @returns VBox status code.
     * @param   pDrvIns             The driver instance.
     * @param   pfnAsyncNotify      The callback.
     * @thread  EMT(0)
     */
    DECLR3CALLBACKMEMBER(int, pfnSetAsyncNotification, (PPDMDRVINS pDrvIns, PFNPDMDRVASYNCNOTIFY pfnAsyncNotify));

    /**
     * Notify EMT(0) that the driver has completed the asynchronous notification
     * handling.
     *
     * This can be called at any time, spurious calls will simply be ignored.
     *
     * @param   pDrvIns             The driver instance.
     * @thread  Any
     */
    DECLR3CALLBACKMEMBER(void, pfnAsyncNotificationCompleted, (PPDMDRVINS pDrvIns));

    /**
     * Creates a PDM thread.
     *
     * This differs from the RTThreadCreate() API in that PDM takes care of suspending,
     * resuming, and destroying the thread as the VM state changes.
     *
     * @returns VBox status code.
     * @param   pDrvIns     The driver instance.
     * @param   ppThread    Where to store the thread 'handle'.
     * @param   pvUser      The user argument to the thread function.
     * @param   pfnThread   The thread function.
     * @param   pfnWakeup   The wakup callback. This is called on the EMT thread when
     *                      a state change is pending.
     * @param   cbStack     See RTThreadCreate.
     * @param   enmType     See RTThreadCreate.
     * @param   pszName     See RTThreadCreate.
     */
    DECLR3CALLBACKMEMBER(int, pfnThreadCreate,(PPDMDRVINS pDrvIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADDRV pfnThread,
                                               PFNPDMTHREADWAKEUPDRV pfnWakeup, size_t cbStack, RTTHREADTYPE enmType, const char *pszName));

    /**
     * Creates an async completion template for a driver instance.
     *
     * The template is used when creating new completion tasks.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   ppTemplate      Where to store the template pointer on success.
     * @param   pfnCompleted    The completion callback routine.
     * @param   pvTemplateUser  Template user argument.
     * @param   pszDesc         Description.
     */
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionTemplateCreate,(PPDMDRVINS pDrvIns, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate,
                                                                PFNPDMASYNCCOMPLETEDRV pfnCompleted, void *pvTemplateUser,
                                                                const char *pszDesc));

#ifdef VBOX_WITH_NETSHAPER
    /**
     * Attaches network filter driver to a bandwidth group.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   pcszBwGroup     Name of the bandwidth group to attach to.
     * @param   pFilter         Pointer to the filter we attach.
     */
    DECLR3CALLBACKMEMBER(int, pfnNetShaperAttach,(PPDMDRVINS pDrvIns, const char *pszBwGroup,
                                                  PPDMNSFILTER pFilter));


    /**
     * Detaches network filter driver to a bandwidth group.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   pFilter         Pointer to the filter we attach.
     */
    DECLR3CALLBACKMEMBER(int, pfnNetShaperDetach,(PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter));
#endif /* VBOX_WITH_NETSHAPER */


    /**
     * Resolves the symbol for a raw-mode context interface.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   pvInterface     The interface structure.
     * @param   cbInterface     The size of the interface structure.
     * @param   pszSymPrefix    What to prefix the symbols in the list with before
     *                          resolving them.  This must start with 'drv' and
     *                          contain the driver name.
     * @param   pszSymList      List of symbols corresponding to the interface.
     *                          There is generally a there is generally a define
     *                          holding this list associated with the interface
     *                          definition (INTERFACE_SYM_LIST).  For more details
     *                          see PDMR3LdrGetInterfaceSymbols.
     * @thread  EMT
     */
    DECLR3CALLBACKMEMBER(int, pfnLdrGetRCInterfaceSymbols,(PPDMDRVINS pDrvIns, void *pvInterface, size_t cbInterface,
                                                           const char *pszSymPrefix, const char *pszSymList));

    /**
     * Resolves the symbol for a ring-0 context interface.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   pvInterface     The interface structure.
     * @param   cbInterface     The size of the interface structure.
     * @param   pszSymPrefix    What to prefix the symbols in the list with before
     *                          resolving them.  This must start with 'drv' and
     *                          contain the driver name.
     * @param   pszSymList      List of symbols corresponding to the interface.
     *                          There is generally a there is generally a define
     *                          holding this list associated with the interface
     *                          definition (INTERFACE_SYM_LIST).  For more details
     *                          see PDMR3LdrGetInterfaceSymbols.
     * @thread  EMT
     */
    DECLR3CALLBACKMEMBER(int, pfnLdrGetR0InterfaceSymbols,(PPDMDRVINS pDrvIns, void *pvInterface, size_t cbInterface,
                                                           const char *pszSymPrefix, const char *pszSymList));
    /**
     * Initializes a PDM critical section.
     *
     * The PDM critical sections are derived from the IPRT critical sections, but
     * works in both RC and R0 as well as R3.
     *
     * @returns VBox status code.
     * @param   pDrvIns             The driver instance.
     * @param   pCritSect           Pointer to the critical section.
     * @param   SRC_POS             Use RT_SRC_POS.
     * @param   pszName             The base name of the critical section.  Will be
     *                              mangeled with the instance number.  For
     *                              statistics and lock validation.
     * @thread  EMT
     */
    DECLR3CALLBACKMEMBER(int, pfnCritSectInit,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL, const char *pszName));

    /**
     * Call the ring-0 request handler routine of the driver.
     *
     * For this to work, the driver must be ring-0 enabled and export a request
     * handler function.  The name of the function must be the driver name in the
     * PDMDRVREG struct prefixed with 'drvR0' and suffixed with 'ReqHandler'.
     * The driver name will be capitalized.  It shall take the exact same
     * arguments as this function and be declared using PDMBOTHCBDECL.  See
     * FNPDMDRVREQHANDLERR0.
     *
     * @returns VBox status code.
     * @retval  VERR_SYMBOL_NOT_FOUND if the driver doesn't export the required
     *          handler function.
     * @retval  VERR_ACCESS_DENIED if the driver isn't ring-0 capable.
     *
     * @param   pDrvIns             The driver instance.
     * @param   uOperation          The operation to perform.
     * @param   u64Arg              64-bit integer argument.
     * @thread  Any
     */
    DECLR3CALLBACKMEMBER(int, pfnCallR0,(PPDMDRVINS pDrvIns, uint32_t uOperation, uint64_t u64Arg));

    /**
     * Notify FTM about a checkpoint occurrence
     *
     * @param   pDrvIns             The driver instance.
     * @param   enmType             Checkpoint type
     * @thread  Any
     */
    DECLR3CALLBACKMEMBER(int, pfnFTSetCheckpoint,(PPDMDRVINS pDrvIns, FTMCHECKPOINTTYPE enmType));

    /**
     * Creates a block cache for a driver driver instance.
     *
     * @returns VBox status code.
     * @param   pDrvIns             The driver instance.
     * @param   ppBlkCache          Where to store the handle to the block cache.
     * @param   pfnXferComplete     The I/O transfer complete callback.
     * @param   pfnXferEnqueue      The I/O request enqueue callback.
     * @param   pfnXferEnqueueDiscard   The discard request enqueue callback.
     * @param   pcszId              Unique ID used to identify the user.
     */
    DECLR3CALLBACKMEMBER(int, pfnBlkCacheRetain, (PPDMDRVINS pDrvIns, PPPDMBLKCACHE ppBlkCache,
                                                  PFNPDMBLKCACHEXFERCOMPLETEDRV pfnXferComplete,
                                                  PFNPDMBLKCACHEXFERENQUEUEDRV pfnXferEnqueue,
                                                  PFNPDMBLKCACHEXFERENQUEUEDISCARDDRV pfnXferEnqueueDiscard,
                                                  const char *pcszId));
    /**
     * Gets the reason for the most recent VM suspend.
     *
     * @returns The suspend reason. VMSUSPENDREASON_INVALID is returned if no
     *          suspend has been made or if the pDrvIns is invalid.
     * @param   pDrvIns             The driver instance.
     */
    DECLR3CALLBACKMEMBER(VMSUSPENDREASON, pfnVMGetSuspendReason,(PPDMDRVINS pDrvIns));

    /**
     * Gets the reason for the most recent VM resume.
     *
     * @returns The resume reason. VMRESUMEREASON_INVALID is returned if no
     *          resume has been made or if the pDrvIns is invalid.
     * @param   pDrvIns             The driver instance.
     */
    DECLR3CALLBACKMEMBER(VMRESUMEREASON, pfnVMGetResumeReason,(PPDMDRVINS pDrvIns));

    /** @name Space reserved for minor interface changes.
     * @{ */
    DECLR3CALLBACKMEMBER(void, pfnReserved0,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved1,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved2,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved3,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved4,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved5,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved6,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved7,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved8,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved9,(PPDMDRVINS pDrvIns));
    /** @}  */

    /** Just a safety precaution. */
    uint32_t                        u32TheEnd;
} PDMDRVHLPR3;
/** Current DRVHLP version number. */
#define PDM_DRVHLPR3_VERSION                    PDM_VERSION_MAKE(0xf0fb, 3, 0)

#endif /* IN_RING3 */


/**
 * @copydoc PDMDRVHLPR3::pfnVMSetError
 */
DECLINLINE(int)  RT_IPRT_FORMAT_ATTR(6, 7) PDMDrvHlpVMSetError(PPDMDRVINS pDrvIns, const int rc, RT_SRC_POS_DECL,
                                                               const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    pDrvIns->CTX_SUFF(pHlp)->pfnVMSetErrorV(pDrvIns, rc, RT_SRC_POS_ARGS, pszFormat, va);
    va_end(va);
    return rc;
}

/** @def PDMDRV_SET_ERROR
 * Set the VM error. See PDMDrvHlpVMSetError() for printf like message formatting.
 */
#define PDMDRV_SET_ERROR(pDrvIns, rc, pszError)  \
    PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS, "%s", pszError)

/**
 * @copydoc PDMDRVHLPR3::pfnVMSetErrorV
 */
DECLINLINE(int)  RT_IPRT_FORMAT_ATTR(6, 0) PDMDrvHlpVMSetErrorV(PPDMDRVINS pDrvIns, const int rc, RT_SRC_POS_DECL,
                                                                const char *pszFormat, va_list va)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnVMSetErrorV(pDrvIns, rc, RT_SRC_POS_ARGS, pszFormat, va);
}


/**
 * @copydoc PDMDRVHLPR3::pfnVMSetRuntimeError
 */
DECLINLINE(int)  RT_IPRT_FORMAT_ATTR(4, 5) PDMDrvHlpVMSetRuntimeError(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                                      const char *pszFormat, ...)
{
    va_list va;
    int rc;
    va_start(va, pszFormat);
    rc = pDrvIns->CTX_SUFF(pHlp)->pfnVMSetRuntimeErrorV(pDrvIns, fFlags, pszErrorId, pszFormat, va);
    va_end(va);
    return rc;
}

/** @def PDMDRV_SET_RUNTIME_ERROR
 * Set the VM runtime error. See PDMDrvHlpVMSetRuntimeError() for printf like message formatting.
 */
#define PDMDRV_SET_RUNTIME_ERROR(pDrvIns, fFlags, pszErrorId, pszError)  \
    PDMDrvHlpVMSetRuntimeError(pDrvIns, fFlags, pszErrorId, "%s", pszError)

/**
 * @copydoc PDMDRVHLPR3::pfnVMSetRuntimeErrorV
 */
DECLINLINE(int)  RT_IPRT_FORMAT_ATTR(4, 0) PDMDrvHlpVMSetRuntimeErrorV(PPDMDRVINS pDrvIns, uint32_t fFlags,
                                                                       const char *pszErrorId,  const char *pszFormat, va_list va)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnVMSetRuntimeErrorV(pDrvIns, fFlags, pszErrorId, pszFormat, va);
}



/** @def PDMDRV_ASSERT_EMT
 * Assert that the current thread is the emulation thread.
 */
#ifdef VBOX_STRICT
# define PDMDRV_ASSERT_EMT(pDrvIns)  pDrvIns->CTX_SUFF(pHlp)->pfnAssertEMT(pDrvIns, __FILE__, __LINE__, __FUNCTION__)
#else
# define PDMDRV_ASSERT_EMT(pDrvIns)  do { } while (0)
#endif

/** @def PDMDRV_ASSERT_OTHER
 * Assert that the current thread is NOT the emulation thread.
 */
#ifdef VBOX_STRICT
# define PDMDRV_ASSERT_OTHER(pDrvIns)  pDrvIns->CTX_SUFF(pHlp)->pfnAssertOther(pDrvIns, __FILE__, __LINE__, __FUNCTION__)
#else
# define PDMDRV_ASSERT_OTHER(pDrvIns)  do { } while (0)
#endif

/**
 * @copydoc PDMDRVHLPR3::pfnFTSetCheckpoint
 */
DECLINLINE(int) PDMDrvHlpFTSetCheckpoint(PPDMDRVINS pDrvIns, FTMCHECKPOINTTYPE enmType)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnFTSetCheckpoint(pDrvIns, enmType);
}


#ifdef IN_RING3

/**
 * @copydoc PDMDRVHLPR3::pfnAttach
 */
DECLINLINE(int) PDMDrvHlpAttach(PPDMDRVINS pDrvIns, uint32_t fFlags, PPDMIBASE *ppBaseInterface)
{
    return pDrvIns->pHlpR3->pfnAttach(pDrvIns, fFlags, ppBaseInterface);
}

/**
 * Check that there is no driver below the us that we should attach to.
 *
 * @returns VERR_PDM_NO_ATTACHED_DRIVER if there is no driver.
 * @param   pDrvIns     The driver instance.
 */
DECLINLINE(int) PDMDrvHlpNoAttach(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnAttach(pDrvIns, 0, NULL);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDetach
 */
DECLINLINE(int) PDMDrvHlpDetach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    return pDrvIns->pHlpR3->pfnDetach(pDrvIns, fFlags);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDetachSelf
 */
DECLINLINE(int) PDMDrvHlpDetachSelf(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    return pDrvIns->pHlpR3->pfnDetachSelf(pDrvIns, fFlags);
}

/**
 * @copydoc PDMDRVHLPR3::pfnMountPrepare
 */
DECLINLINE(int) PDMDrvHlpMountPrepare(PPDMDRVINS pDrvIns, const char *pszFilename, const char *pszCoreDriver)
{
    return pDrvIns->pHlpR3->pfnMountPrepare(pDrvIns, pszFilename, pszCoreDriver);
}

/**
 * @copydoc PDMDRVHLPR3::pfnVMState
 */
DECLINLINE(VMSTATE) PDMDrvHlpVMState(PPDMDRVINS pDrvIns)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnVMState(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnVMTeleportedAndNotFullyResumedYet
 */
DECLINLINE(bool) PDMDrvHlpVMTeleportedAndNotFullyResumedYet(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnVMTeleportedAndNotFullyResumedYet(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnGetSupDrvSession
 */
DECLINLINE(PSUPDRVSESSION) PDMDrvHlpGetSupDrvSession(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnGetSupDrvSession(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnQueueCreate
 */
DECLINLINE(int) PDMDrvHlpQueueCreate(PPDMDRVINS pDrvIns, uint32_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                        PFNPDMQUEUEDRV pfnCallback, const char *pszName, PPDMQUEUE *ppQueue)
{
    return pDrvIns->pHlpR3->pfnQueueCreate(pDrvIns, cbItem, cItems, cMilliesInterval, pfnCallback, pszName, ppQueue);
}

/**
 * @copydoc PDMDRVHLPR3::pfnTMGetVirtualFreq
 */
DECLINLINE(uint64_t) PDMDrvHlpTMGetVirtualFreq(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnTMGetVirtualFreq(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnTMGetVirtualTime
 */
DECLINLINE(uint64_t) PDMDrvHlpTMGetVirtualTime(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnTMGetVirtualTime(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnTMTimerCreate
 */
DECLINLINE(int) PDMDrvHlpTMTimerCreate(PPDMDRVINS pDrvIns, TMCLOCK enmClock, PFNTMTIMERDRV pfnCallback, void *pvUser, uint32_t fFlags, const char *pszDesc, PPTMTIMERR3 ppTimer)
{
    return pDrvIns->pHlpR3->pfnTMTimerCreate(pDrvIns, enmClock, pfnCallback, pvUser, fFlags, pszDesc, ppTimer);
}

/**
 * Register a save state data unit.
 *
 * @returns VBox status.
 * @param   pDrvIns         Driver instance.
 * @param   uVersion        Data layout version number.
 * @param   cbGuess         The approximate amount of data in the unit.
 *                          Only for progress indicators.
 * @param   pfnSaveExec     Execute save callback, optional.
 * @param   pfnLoadExec     Execute load callback, optional.
 */
DECLINLINE(int) PDMDrvHlpSSMRegister(PPDMDRVINS pDrvIns, uint32_t uVersion, size_t cbGuess,
                                     PFNSSMDRVSAVEEXEC pfnSaveExec, PFNSSMDRVLOADEXEC pfnLoadExec)
{
    return pDrvIns->pHlpR3->pfnSSMRegister(pDrvIns, uVersion, cbGuess,
                                              NULL /*pfnLivePrep*/, NULL /*pfnLiveExec*/, NULL /*pfnLiveVote*/,
                                              NULL /*pfnSavePrep*/, pfnSaveExec,          NULL /*pfnSaveDone*/,
                                              NULL /*pfnLoadPrep*/, pfnLoadExec,          NULL /*pfnLoadDone*/);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSSMRegister
 */
DECLINLINE(int) PDMDrvHlpSSMRegisterEx(PPDMDRVINS pDrvIns, uint32_t uVersion, size_t cbGuess,
                                       PFNSSMDRVLIVEPREP pfnLivePrep, PFNSSMDRVLIVEEXEC pfnLiveExec, PFNSSMDRVLIVEVOTE pfnLiveVote,
                                       PFNSSMDRVSAVEPREP pfnSavePrep, PFNSSMDRVSAVEEXEC pfnSaveExec, PFNSSMDRVSAVEDONE pfnSaveDone,
                                       PFNSSMDRVLOADPREP pfnLoadPrep, PFNSSMDRVLOADEXEC pfnLoadExec, PFNSSMDRVLOADDONE pfnLoadDone)
{
    return pDrvIns->pHlpR3->pfnSSMRegister(pDrvIns, uVersion, cbGuess,
                                              pfnLivePrep, pfnLiveExec, pfnLiveVote,
                                              pfnSavePrep, pfnSaveExec, pfnSaveDone,
                                              pfnLoadPrep, pfnLoadExec, pfnLoadDone);
}

/**
 * Register a load done callback.
 *
 * @returns VBox status.
 * @param   pDrvIns         Driver instance.
 * @param   pfnLoadDone         Done load callback, optional.
 */
DECLINLINE(int) PDMDrvHlpSSMRegisterLoadDone(PPDMDRVINS pDrvIns, PFNSSMDRVLOADDONE pfnLoadDone)
{
    return pDrvIns->pHlpR3->pfnSSMRegister(pDrvIns, 0 /*uVersion*/, 0 /*cbGuess*/,
                                              NULL /*pfnLivePrep*/, NULL /*pfnLiveExec*/, NULL /*pfnLiveVote*/,
                                              NULL /*pfnSavePrep*/, NULL /*pfnSaveExec*/, NULL /*pfnSaveDone*/,
                                              NULL /*pfnLoadPrep*/, NULL /*pfnLoadExec*/, pfnLoadDone);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDBGFInfoRegister
 */
DECLINLINE(int) PDMDrvHlpDBGFInfoRegister(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDRV pfnHandler)
{
    return pDrvIns->pHlpR3->pfnDBGFInfoRegister(pDrvIns, pszName, pszDesc, pfnHandler);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDBGFInfoRegister
 */
DECLINLINE(int) PDMDrvHlpDBGFInfoDeregister(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDRV pfnHandler)
{
    return pDrvIns->pHlpR3->pfnDBGFInfoRegister(pDrvIns, pszName, pszDesc, pfnHandler);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSTAMRegister
 */
DECLINLINE(void) PDMDrvHlpSTAMRegister(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    pDrvIns->pHlpR3->pfnSTAMRegister(pDrvIns, pvSample, enmType, pszName, enmUnit, pszDesc);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSTAMRegisterF
 */
DECLINLINE(void)  RT_IPRT_FORMAT_ATTR(7, 8) PDMDrvHlpSTAMRegisterF(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType,
                                                                   STAMVISIBILITY enmVisibility, STAMUNIT enmUnit,
                                                                   const char *pszDesc, const char *pszName, ...)
{
    va_list va;
    va_start(va, pszName);
    pDrvIns->pHlpR3->pfnSTAMRegisterV(pDrvIns, pvSample, enmType, enmVisibility, enmUnit, pszDesc, pszName, va);
    va_end(va);
}

/**
 * Convenience wrapper that registers counter which is always visible.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pCounter            Pointer to the counter variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   enmUnit             The unit.
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegCounterEx(PPDMDRVINS pDrvIns, PSTAMCOUNTER pCounter, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    pDrvIns->pHlpR3->pfnSTAMRegisterF(pDrvIns, pCounter, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, enmUnit, pszDesc,
                                      "/Drivers/%s-%u/%s", pDrvIns->pReg->szName, pDrvIns->iInstance, pszName);
}

/**
 * Convenience wrapper that registers counter which is always visible and has
 * the STAMUNIT_COUNT unit.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pCounter            Pointer to the counter variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegCounter(PPDMDRVINS pDrvIns, PSTAMCOUNTER pCounter, const char *pszName, const char *pszDesc)
{
    PDMDrvHlpSTAMRegCounterEx(pDrvIns, pCounter, pszName, STAMUNIT_COUNT, pszDesc);
}

/**
 * Convenience wrapper that registers profiling sample which is always visible.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pProfile            Pointer to the profiling variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   enmUnit             The unit.
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegProfileEx(PPDMDRVINS pDrvIns, PSTAMPROFILE pProfile, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    pDrvIns->pHlpR3->pfnSTAMRegisterF(pDrvIns, pProfile, STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, enmUnit, pszDesc,
                                      "/Drivers/%s-%u/%s", pDrvIns->pReg->szName, pDrvIns->iInstance, pszName);
}

/**
 * Convenience wrapper that registers profiling sample which is always visible
 * hand counts ticks per call (STAMUNIT_TICKS_PER_CALL).
 *
 * @param   pDrvIns             The driver instance.
 * @param   pProfile            Pointer to the profiling variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegProfile(PPDMDRVINS pDrvIns, PSTAMPROFILE pProfile, const char *pszName, const char *pszDesc)
{
    PDMDrvHlpSTAMRegProfileEx(pDrvIns, pProfile, pszName, STAMUNIT_TICKS_PER_CALL, pszDesc);
}

/**
 * Convenience wrapper that registers an advanced profiling sample which is
 * always visible.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pProfile            Pointer to the profiling variable.
 * @param   enmUnit             The unit.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegProfileAdvEx(PPDMDRVINS pDrvIns, PSTAMPROFILEADV pProfile, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    pDrvIns->pHlpR3->pfnSTAMRegisterF(pDrvIns, pProfile, STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, enmUnit, pszDesc,
                                      "/Drivers/%s-%u/%s", pDrvIns->pReg->szName, pDrvIns->iInstance, pszName);
}

/**
 * Convenience wrapper that registers an advanced profiling sample which is
 * always visible.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pProfile            Pointer to the profiling variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegProfileAdv(PPDMDRVINS pDrvIns, PSTAMPROFILEADV pProfile, const char *pszName, const char *pszDesc)
{
    PDMDrvHlpSTAMRegProfileAdvEx(pDrvIns, pProfile, pszName, STAMUNIT_TICKS_PER_CALL, pszDesc);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSTAMDeregister
 */
DECLINLINE(int) PDMDrvHlpSTAMDeregister(PPDMDRVINS pDrvIns, void *pvSample)
{
    return pDrvIns->pHlpR3->pfnSTAMDeregister(pDrvIns, pvSample);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSUPCallVMMR0Ex
 */
DECLINLINE(int) PDMDrvHlpSUPCallVMMR0Ex(PPDMDRVINS pDrvIns, unsigned uOperation, void *pvArg, unsigned cbArg)
{
    return pDrvIns->pHlpR3->pfnSUPCallVMMR0Ex(pDrvIns, uOperation, pvArg, cbArg);
}

/**
 * @copydoc PDMDRVHLPR3::pfnUSBRegisterHub
 */
DECLINLINE(int) PDMDrvHlpUSBRegisterHub(PPDMDRVINS pDrvIns, uint32_t fVersions, uint32_t cPorts, PCPDMUSBHUBREG pUsbHubReg, PPCPDMUSBHUBHLP ppUsbHubHlp)
{
    return pDrvIns->pHlpR3->pfnUSBRegisterHub(pDrvIns, fVersions, cPorts, pUsbHubReg, ppUsbHubHlp);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSetAsyncNotification
 */
DECLINLINE(int) PDMDrvHlpSetAsyncNotification(PPDMDRVINS pDrvIns, PFNPDMDRVASYNCNOTIFY pfnAsyncNotify)
{
    return pDrvIns->pHlpR3->pfnSetAsyncNotification(pDrvIns, pfnAsyncNotify);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncNotificationCompleted
 */
DECLINLINE(void) PDMDrvHlpAsyncNotificationCompleted(PPDMDRVINS pDrvIns)
{
    pDrvIns->pHlpR3->pfnAsyncNotificationCompleted(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnThreadCreate
 */
DECLINLINE(int) PDMDrvHlpThreadCreate(PPDMDRVINS pDrvIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADDRV pfnThread,
                                      PFNPDMTHREADWAKEUPDRV pfnWakeup, size_t cbStack, RTTHREADTYPE enmType, const char *pszName)
{
    return pDrvIns->pHlpR3->pfnThreadCreate(pDrvIns, ppThread, pvUser, pfnThread, pfnWakeup, cbStack, enmType, pszName);
}

# ifdef VBOX_WITH_PDM_ASYNC_COMPLETION
/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionTemplateCreate
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionTemplateCreate(PPDMDRVINS pDrvIns, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate,
                                                       PFNPDMASYNCCOMPLETEDRV pfnCompleted, void *pvTemplateUser, const char *pszDesc)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionTemplateCreate(pDrvIns, ppTemplate, pfnCompleted, pvTemplateUser, pszDesc);
}
# endif

# ifdef VBOX_WITH_NETSHAPER
/**
 * @copydoc PDMDRVHLPR3::pfnNetShaperAttach
 */
DECLINLINE(int) PDMDrvHlpNetShaperAttach(PPDMDRVINS pDrvIns, const char *pcszBwGroup, PPDMNSFILTER pFilter)
{
    return pDrvIns->pHlpR3->pfnNetShaperAttach(pDrvIns, pcszBwGroup, pFilter);
}

/**
 * @copydoc PDMDRVHLPR3::pfnNetShaperDetach
 */
DECLINLINE(int) PDMDrvHlpNetShaperDetach(PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter)
{
    return pDrvIns->pHlpR3->pfnNetShaperDetach(pDrvIns, pFilter);
}
# endif

/**
 * @copydoc PDMDRVHLPR3::pfnCritSectInit
 */
DECLINLINE(int) PDMDrvHlpCritSectInit(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL, const char *pszName)
{
    return pDrvIns->pHlpR3->pfnCritSectInit(pDrvIns, pCritSect, RT_SRC_POS_ARGS, pszName);
}

/**
 * @copydoc PDMDRVHLPR3::pfnCallR0
 */
DECLINLINE(int) PDMDrvHlpCallR0(PPDMDRVINS pDrvIns, uint32_t uOperation, uint64_t u64Arg)
{
    return pDrvIns->pHlpR3->pfnCallR0(pDrvIns, uOperation, u64Arg);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheRetain
 */
DECLINLINE(int) PDMDrvHlpBlkCacheRetain(PPDMDRVINS pDrvIns, PPPDMBLKCACHE ppBlkCache,
                                        PFNPDMBLKCACHEXFERCOMPLETEDRV pfnXferComplete,
                                        PFNPDMBLKCACHEXFERENQUEUEDRV pfnXferEnqueue,
                                        PFNPDMBLKCACHEXFERENQUEUEDISCARDDRV pfnXferEnqueueDiscard,
                                        const char *pcszId)
{
    return pDrvIns->pHlpR3->pfnBlkCacheRetain(pDrvIns, ppBlkCache, pfnXferComplete, pfnXferEnqueue, pfnXferEnqueueDiscard, pcszId);
}

/**
 * @copydoc PDMDRVHLPR3::pfnVMGetSuspendReason
 */
DECLINLINE(VMSUSPENDREASON) PDMDrvHlpVMGetSuspendReason(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnVMGetSuspendReason(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnVMGetResumeReason
 */
DECLINLINE(VMRESUMEREASON) PDMDrvHlpVMGetResumeReason(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnVMGetResumeReason(pDrvIns);
}


/** Pointer to callbacks provided to the VBoxDriverRegister() call. */
typedef struct PDMDRVREGCB *PPDMDRVREGCB;
/** Pointer to const callbacks provided to the VBoxDriverRegister() call. */
typedef const struct PDMDRVREGCB *PCPDMDRVREGCB;

/**
 * Callbacks for VBoxDriverRegister().
 */
typedef struct PDMDRVREGCB
{
    /** Interface version.
     * This is set to PDM_DRVREG_CB_VERSION. */
    uint32_t                    u32Version;

    /**
     * Registers a driver with the current VM instance.
     *
     * @returns VBox status code.
     * @param   pCallbacks      Pointer to the callback table.
     * @param   pReg            Pointer to the driver registration record.
     *                          This data must be permanent and readonly.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegister,(PCPDMDRVREGCB pCallbacks, PCPDMDRVREG pReg));
} PDMDRVREGCB;

/** Current version of the PDMDRVREGCB structure.  */
#define PDM_DRVREG_CB_VERSION                   PDM_VERSION_MAKE(0xf0fa, 1, 0)


/**
 * The VBoxDriverRegister callback function.
 *
 * PDM will invoke this function after loading a driver module and letting
 * the module decide which drivers to register and how to handle conflicts.
 *
 * @returns VBox status code.
 * @param   pCallbacks      Pointer to the callback table.
 * @param   u32Version      VBox version number.
 */
typedef DECLCALLBACK(int) FNPDMVBOXDRIVERSREGISTER(PCPDMDRVREGCB pCallbacks, uint32_t u32Version);

VMMR3DECL(int) PDMR3DrvStaticRegistration(PVM pVM, FNPDMVBOXDRIVERSREGISTER pfnCallback);

#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif
