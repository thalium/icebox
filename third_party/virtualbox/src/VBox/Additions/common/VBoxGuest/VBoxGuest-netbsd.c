/* $Id: VBoxGuest-netbsd.c $ */
/** @file
 * VirtualBox Guest Additions Driver for NetBSD.
 */

/*
 * Copyright (C) 2007-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/** @todo r=bird: This must merge with SUPDrv-netbsd.c before long. The two
 * source files should only differ on prefixes and the extra bits wrt to the
 * pci device. I.e. it should be diffable so that fixes to one can easily be
 * applied to the other. */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/select.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/selinfo.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/vfs_syscalls.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#ifdef PVM
#  undef PVM
#endif
#include "VBoxGuestInternal.h"
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>
#include <iprt/process.h>
#include <iprt/mem.h>
#include <iprt/asm.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The module name. */
#define DEVICE_NAME  "vboxguest"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct VBoxGuestDeviceState
{
    device_t sc_dev;
    pci_chipset_tag_t pc;
    bus_space_tag_t io_tag;
    bus_space_handle_t io_handle;
    bus_addr_t uIOPortBase;
    bus_size_t io_regsize;
    bus_space_tag_t iVMMDevMemResId;
    bus_space_handle_t VMMDevMemHandle;

    /** Size of the memory area. */
    bus_size_t         VMMDevMemSize;
    /** Mapping of the register space */
    bus_addr_t         pMMIOBase;

    /** IRQ resource handle. */
    pci_intr_handle_t  ih;
    /** Pointer to the IRQ handler. */
    void              *pfnIrqHandler;

    /** Controller features, limits and status. */
    u_int              vboxguest_state;
} vboxguest_softc;


struct vboxguest_session
{
    vboxguest_softc *sc;
    PVBOXGUESTSESSION session;
};

#define VBOXGUEST_STATE_INITOK 1 << 0


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
/*
 * Character device file handlers.
 */
static int VBoxGuestNetBSDOpen(dev_t device, int flags, int fmt, struct lwp *process);
static int VBoxGuestNetBSDClose(struct file *fp);
static int VBoxGuestNetBSDIOCtl(struct file *fp, u_long cmd, void *addr);
static int VBoxGuestNetBSDPoll(struct file *fp, int events);
static void VBoxGuestNetBSDAttach(device_t, device_t, void*);
static int VBoxGuestNetBSDDetach(device_t pDevice, int flags);

/*
 * IRQ related functions.
 */
static void VBoxGuestNetBSDRemoveIRQ(vboxguest_softc *sc);
static int  VBoxGuestNetBSDAddIRQ(vboxguest_softc *pvState, struct pci_attach_args *pa);
static int  VBoxGuestNetBSDISR(void *pvState);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/*
 * The /dev/vboxguest character device entry points.
 */
static struct cdevsw g_VBoxGuestNetBSDChrDevSW =
{
    VBoxGuestNetBSDOpen,
    noclose,
    noread,
    nowrite,
    noioctl,
    nostop,
    notty,
    nopoll,
    nommap,
    nokqfilter,
};

static const struct fileops vboxguest_fileops = {
    .fo_read = fbadop_read,
    .fo_write = fbadop_write,
    .fo_ioctl = VBoxGuestNetBSDIOCtl,
    .fo_fcntl = fnullop_fcntl,
    .fo_poll = VBoxGuestNetBSDPoll,
    .fo_stat = fbadop_stat,
    .fo_close = VBoxGuestNetBSDClose,
    .fo_kqfilter = fnullop_kqfilter,
    .fo_restart = fnullop_restart
};

/** Device extention & session data association structure. */
static VBOXGUESTDEVEXT      g_DevExt;
/** Reference counter */
static volatile uint32_t    cUsers;
/** selinfo structure used for polling. */
static struct selinfo       g_SelInfo;

CFDRIVER_DECL(vboxguest, DV_DULL, NULL);
extern struct cfdriver vboxguest_cd;

/**
 * File open handler
 *
 */
static int VBoxGuestNetBSDOpen(dev_t device, int flags, int fmt, struct lwp *process)
{
    int rc;
    vboxguest_softc *vboxguest;
    struct vboxguest_session *session;
    file_t *fp;
    int fd, error;

    LogFlow((DEVICE_NAME ": %s\n", __func__));

    if ((vboxguest = device_lookup_private(&vboxguest_cd, minor(device))) == NULL)
    {
        printf("device_lookup_private failed\n");
        return (ENXIO);
    }

    if ((vboxguest->vboxguest_state & VBOXGUEST_STATE_INITOK) == 0)
    {
        aprint_error_dev(vboxguest->sc_dev, "device not configured\n");
        return (ENXIO);
    }

    session = kmem_alloc(sizeof(*session), KM_SLEEP);
    if (session == NULL)
    {
        return (ENOMEM);
    }

    session->sc = vboxguest;

    if ((error = fd_allocfile(&fp, &fd)) != 0)
    {
        kmem_free(session, sizeof(*session));
        return error;
    }

    /*
     * Create a new session.
     */
    rc = VGDrvCommonCreateUserSession(&g_DevExt, &session->session);
    if (! RT_SUCCESS(rc))
    {
        aprint_error_dev(vboxguest->sc_dev, "VBox session creation failed\n");
        closef(fp); /* ??? */
        kmem_free(session, sizeof(*session));
        return RTErrConvertToErrno(rc);
    }
    ASMAtomicIncU32(&cUsers);
    return fd_clone(fp, fd, flags, &vboxguest_fileops, session);

}

/**
 * File close handler
 *
 */
static int VBoxGuestNetBSDClose(struct file *fp)
{
    struct vboxguest_session *session = fp->f_data;
    vboxguest_softc *vboxguest = session->sc;

    LogFlow((DEVICE_NAME ": %s\n", __func__));

    VGDrvCommonCloseSession(&g_DevExt, session->session);
    ASMAtomicDecU32(&cUsers);

    kmem_free(session, sizeof(*session));

    return 0;
}

/**
 * IOCTL handler
 *
 */
static int VBoxGuestNetBSDIOCtl(struct file *fp, u_long command, void *data)
{
    struct vboxguest_session *session = fp->f_data;
    vboxguest_softc *vboxguest = session->sc;

    int rc = 0;

    LogFlow((DEVICE_NAME ": %s: command=%#lx data=%p\n",
             __func__, command, data));

    /*
     * Validate the request wrapper.
     */
    if (IOCPARM_LEN(command) != sizeof(VBGLBIGREQ))
    {
        Log((DEVICE_NAME ": %s: bad request %#lx size=%lu expected=%zu\n",
             __func__, command, IOCPARM_LEN(command), sizeof(VBGLBIGREQ)));
        return ENOTTY;
    }

    PVBGLBIGREQ ReqWrap = (PVBGLBIGREQ)data;
    if (ReqWrap->u32Magic != VBGLBIGREQ_MAGIC)
    {
        Log((DEVICE_NAME ": %s: bad magic %#" PRIx32 "; pArg=%p Cmd=%#lx\n",
             __func__, ReqWrap->u32Magic, data, command));
        return EINVAL;
    }
    if (RT_UNLIKELY(   ReqWrap->cbData == 0
                    || ReqWrap->cbData > _1M*16))
    {
        Log((DEVICE_NAME ": %s: bad size %" PRIu32 "; pArg=%p Cmd=%#lx\n",
             __func__, ReqWrap->cbData, data, command));
        return EINVAL;
    }

    /*
     * Read the request.
     */
    void *pvBuf = RTMemTmpAlloc(ReqWrap->cbData);
    if (RT_UNLIKELY(!pvBuf))
    {
        Log((DEVICE_NAME ": %s: RTMemTmpAlloc failed to alloc %" PRIu32 " bytes.\n",
             __func__, ReqWrap->cbData));
        return ENOMEM;
    }

    rc = copyin((void *)(uintptr_t)ReqWrap->pvDataR3, pvBuf, ReqWrap->cbData);
    if (RT_UNLIKELY(rc))
    {
        RTMemTmpFree(pvBuf);
        Log((DEVICE_NAME ": %s: copyin failed; pvBuf=%p pArg=%p Cmd=%#lx. rc=%d\n",
             __func__, pvBuf, data, command, rc));
        aprint_error_dev(vboxguest->sc_dev, "copyin failed\n");
        return EFAULT;
    }
    if (RT_UNLIKELY(   ReqWrap->cbData != 0
                    && !VALID_PTR(pvBuf)))
    {
        RTMemTmpFree(pvBuf);
        Log((DEVICE_NAME ": %s: invalid pvBuf=%p\n",
             __func__, pvBuf));
        return EINVAL;
    }

    /*
     * Process the IOCtl.
     */
    size_t cbDataReturned;
    rc = VGDrvCommonIoCtl(command, &g_DevExt, session->session, pvBuf, ReqWrap->cbData, &cbDataReturned);
    if (RT_SUCCESS(rc))
    {
        rc = 0;
        if (RT_UNLIKELY(cbDataReturned > ReqWrap->cbData))
        {
            Log((DEVICE_NAME ": %s: too much output data %zu expected %" PRIu32 "\n",
                 __func__, cbDataReturned, ReqWrap->cbData));
            cbDataReturned = ReqWrap->cbData;
        }
        if (cbDataReturned > 0)
        {
            rc = copyout(pvBuf, (void *)(uintptr_t)ReqWrap->pvDataR3, cbDataReturned);
            if (RT_UNLIKELY(rc))
            {
                Log((DEVICE_NAME ": %s: copyout failed; pvBuf=%p pArg=%p. rc=%d\n",
                     __func__, pvBuf, data, rc));
            }
        }
    } else {
        Log((DEVICE_NAME ": %s: VGDrvCommonIoCtl failed. rc=%d\n",
             __func__, rc));
        rc = -rc;
    }
    RTMemTmpFree(pvBuf);
    return rc;
}

static int VBoxGuestNetBSDPoll(struct file *fp, int events)
{
    struct vboxguest_session *session = fp->f_data;
    vboxguest_softc *vboxguest = session->sc;

    int rc = 0;
    int events_processed;

    uint32_t u32CurSeq;

    LogFlow((DEVICE_NAME ": %s\n", __func__));

    u32CurSeq = ASMAtomicUoReadU32(&g_DevExt.u32MousePosChangedSeq);
    if (session->session->u32MousePosChangedSeq != u32CurSeq)
    {
        events_processed = events & (POLLIN | POLLRDNORM);
        session->session->u32MousePosChangedSeq = u32CurSeq;
    }
    else
    {
        events_processed = 0;

        selrecord(curlwp, &g_SelInfo);
    }

    return events_processed;
}

static int VBoxGuestNetBSDDetach(device_t self, int flags)
{
    vboxguest_softc *vboxguest;
    vboxguest = device_private(self);

    LogFlow((DEVICE_NAME ": %s\n", __func__));

    if (cUsers > 0)
        return EBUSY;

    if ((vboxguest->vboxguest_state & VBOXGUEST_STATE_INITOK) == 0)
        return 0;

    /*
     * Reverse what we did in VBoxGuestNetBSDAttach.
     */

    VBoxGuestNetBSDRemoveIRQ(vboxguest);

    VGDrvCommonDeleteDevExt(&g_DevExt);

    bus_space_unmap(vboxguest->iVMMDevMemResId, vboxguest->VMMDevMemHandle, vboxguest->VMMDevMemSize);
    bus_space_unmap(vboxguest->io_tag, vboxguest->io_handle, vboxguest->io_regsize);

    RTR0Term();

    return 0;
}

/**
 * Interrupt service routine.
 *
 * @returns Whether the interrupt was from VMMDev.
 * @param   pvState Opaque pointer to the device state.
 */
static int VBoxGuestNetBSDISR(void *pvState)
{
    LogFlow((DEVICE_NAME ": %s: pvState=%p\n", __func__, pvState));

    bool fOurIRQ = VGDrvCommonISR(&g_DevExt);

    return fOurIRQ ? 0 : 1;
}

void VGDrvNativeISRMousePollEvent(PVBOXGUESTDEVEXT pDevExt)
{
    LogFlow((DEVICE_NAME ": %s\n", __func__));

    /*
     * Wake up poll waiters.
     */
    selnotify(&g_SelInfo, 0, 0);
}

/**
 * Sets IRQ for VMMDev.
 *
 * @returns NetBSD error code.
 * @param   vboxguest  Pointer to the state info structure.
 * @param   pa  Pointer to the PCI attach arguments.
 */
static int VBoxGuestNetBSDAddIRQ(vboxguest_softc *vboxguest, struct pci_attach_args *pa)
{
    int iResId = 0;
    int rc = 0;
    const char *intrstr;
#if __NetBSD_Prereq__(6, 99, 39)
    char intstrbuf[100];
#endif

    LogFlow((DEVICE_NAME ": %s\n", __func__));

    if (pci_intr_map(pa, &vboxguest->ih))
    {
        aprint_error_dev(vboxguest->sc_dev, "couldn't map interrupt.\n");
        return VERR_DEV_IO_ERROR;
    }

    intrstr = pci_intr_string(vboxguest->pc, vboxguest->ih
#if __NetBSD_Prereq__(6, 99, 39)
                              , intstrbuf, sizeof(intstrbuf)
#endif
                              );
    aprint_normal_dev(vboxguest->sc_dev, "interrupting at %s\n", intrstr);

    vboxguest->pfnIrqHandler = pci_intr_establish(vboxguest->pc, vboxguest->ih, IPL_BIO, VBoxGuestNetBSDISR, vboxguest);
    if (vboxguest->pfnIrqHandler == NULL)
    {
        aprint_error_dev(vboxguest->sc_dev, "couldn't establish interrupt\n");
        return VERR_DEV_IO_ERROR;
    }

    return VINF_SUCCESS;
}

/**
 * Removes IRQ for VMMDev.
 *
 * @param   vboxguest  Opaque pointer to the state info structure.
 */
static void VBoxGuestNetBSDRemoveIRQ(vboxguest_softc *vboxguest)
{
    LogFlow((DEVICE_NAME ": %s\n", __func__));

    if (vboxguest->pfnIrqHandler)
    {
        pci_intr_disestablish(vboxguest->pc, vboxguest->pfnIrqHandler);
    }
}

static void VBoxGuestNetBSDAttach(device_t parent, device_t self, void *aux)
{
    int rc = VINF_SUCCESS;
    int iResId = 0;
    vboxguest_softc *vboxguest;
    struct pci_attach_args *pa = aux;
    bus_space_tag_t iot, memt;
    bus_space_handle_t ioh, memh;
    bus_dma_segment_t seg;
    int ioh_valid, memh_valid;

    cUsers = 0;

    aprint_normal(": VirtualBox Guest\n");

    vboxguest = device_private(self);
    vboxguest->sc_dev = self;

    /*
     * Initialize IPRT R0 driver, which internally calls OS-specific r0 init.
     */
    rc = RTR0Init(0);
    if (RT_FAILURE(rc))
    {
        LogFunc(("RTR0Init failed.\n"));
        aprint_error_dev(vboxguest->sc_dev, "RTR0Init failed\n");
        return;
    }

    vboxguest->pc = pa->pa_pc;

    /*
     * Allocate I/O port resource.
     */
    ioh_valid = (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_IO, 0, &vboxguest->io_tag, &vboxguest->io_handle, &vboxguest->uIOPortBase, &vboxguest->io_regsize) == 0);

    if (ioh_valid)
    {

        /*
         * Map the MMIO region.
         */
        memh_valid = (pci_mapreg_map(pa, PCI_MAPREG_START+4, PCI_MAPREG_TYPE_MEM, BUS_SPACE_MAP_LINEAR, &vboxguest->iVMMDevMemResId, &vboxguest->VMMDevMemHandle, &vboxguest->pMMIOBase, &vboxguest->VMMDevMemSize) == 0);
        if (memh_valid)
        {
            /*
             * Call the common device extension initializer.
             */
            rc = VGDrvCommonInitDevExt(&g_DevExt, vboxguest->uIOPortBase,
                    bus_space_vaddr(vboxguest->iVMMDevMemResId,
                            vboxguest->VMMDevMemHandle),
                                       vboxguest->VMMDevMemSize,
#if ARCH_BITS == 64
                                       VBOXOSTYPE_NetBSD_x64,
#else
                                       VBOXOSTYPE_NetBSD,
#endif
                                       VMMDEV_EVENT_MOUSE_POSITION_CHANGED);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Add IRQ of VMMDev.
                 */
                rc = VBoxGuestNetBSDAddIRQ(vboxguest, pa);
                if (RT_SUCCESS(rc))
                {
                    vboxguest->vboxguest_state |= VBOXGUEST_STATE_INITOK;
                    return;
                }
                VGDrvCommonDeleteDevExt(&g_DevExt);
            }
            else
            {
                aprint_error_dev(vboxguest->sc_dev, "init failed\n");
            }
            bus_space_unmap(vboxguest->iVMMDevMemResId, vboxguest->VMMDevMemHandle, vboxguest->VMMDevMemSize);
        }
        else
        {
            aprint_error_dev(vboxguest->sc_dev, "MMIO mapping failed\n");
        }
        bus_space_unmap(vboxguest->io_tag, vboxguest->io_handle, vboxguest->io_regsize);
    }
    else
    {
        aprint_error_dev(vboxguest->sc_dev, "IO mapping failed\n");
    }

    RTR0Term();
    return;
}

static int
VBoxGuestNetBSDMatch(device_t parent, cfdata_t match, void *aux)
{
    const struct pci_attach_args *pa = aux;

    if (   PCI_VENDOR(pa->pa_id) == VMMDEV_VENDORID
        && PCI_PRODUCT(pa->pa_id) == VMMDEV_DEVICEID)
    {
        return 1;
    }

    return 0;
}

/* Common code that depend on g_DevExt. */
#include "VBoxGuestIDC-unix.c.h"

CFATTACH_DECL_NEW(vboxguest, sizeof(vboxguest_softc),
    VBoxGuestNetBSDMatch, VBoxGuestNetBSDAttach, VBoxGuestNetBSDDetach, NULL);

MODULE(MODULE_CLASS_DRIVER, vboxguest, "pci");

static int loc[2] = {-1, -1};

static const struct cfparent pspec = {
    "pci", "pci", DVUNIT_ANY
};

static struct cfdata vboxguest_cfdata[] = {
    {
        .cf_name = "vboxguest",
        .cf_atname = "vboxguest",
        .cf_unit = 0,           /* Only unit 0 is ever used  */
        .cf_fstate = FSTATE_STAR,
        .cf_loc = loc,
        .cf_flags = 0,
        .cf_pspec = &pspec,
    },
    { NULL, NULL, 0, 0, NULL, 0, NULL }
};

static int
vboxguest_modcmd(modcmd_t cmd, void *opaque)
{
    devmajor_t bmajor, cmajor;
    int error;
    register_t retval;

    LogFlow((DEVICE_NAME ": %s\n", __func__));

    bmajor = cmajor = NODEVMAJOR;
    switch (cmd)
    {
        case MODULE_CMD_INIT:
            error = config_cfdriver_attach(&vboxguest_cd);
            if (error)
            {
                printf("config_cfdriver_attach failed: %d", error);
                break;
            }
            error = config_cfattach_attach(vboxguest_cd.cd_name, &vboxguest_ca);
            if (error)
            {
                config_cfdriver_detach(&vboxguest_cd);
                printf("%s: unable to register cfattach\n", vboxguest_cd.cd_name);
                break;
            }
            error = config_cfdata_attach(vboxguest_cfdata, 1);
            if (error)
            {
                printf("%s: unable to attach cfdata\n", vboxguest_cd.cd_name);
                config_cfattach_detach(vboxguest_cd.cd_name, &vboxguest_ca);
                config_cfdriver_detach(&vboxguest_cd);
                break;
            }

            error = devsw_attach("vboxguest", NULL, &bmajor, &g_VBoxGuestNetBSDChrDevSW, &cmajor);

            if (error == EEXIST)
                error = 0; /* maybe built-in ... improve eventually */

            if (error)
                break;

            error = do_sys_mknod(curlwp, "/dev/vboxguest", 0666|S_IFCHR, makedev(cmajor, 0), &retval, UIO_SYSSPACE);
            if (error == EEXIST)
                error = 0;
            break;

        case MODULE_CMD_FINI:
            error = config_cfdata_detach(vboxguest_cfdata);
            if (error)
                break;
            error = config_cfattach_detach(vboxguest_cd.cd_name, &vboxguest_ca);
            if (error)
                break;
            config_cfdriver_detach(&vboxguest_cd);
            error = devsw_detach(NULL, &g_VBoxGuestNetBSDChrDevSW);
            break;

        default:
            return ENOTTY;
    }
    return error;
}
