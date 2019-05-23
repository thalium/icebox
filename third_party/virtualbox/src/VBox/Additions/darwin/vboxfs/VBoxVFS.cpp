/* $Id: VBoxVFS.cpp $ */
/** @file
 * VBoxVFS - Guest Additions Shared Folders driver. KEXT entry point.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <IOKit/IOLib.h> /* Assert as function */
#include <IOKit/IOService.h>
#include <mach/mach_port.h>


#include <mach/kmod.h>
#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <sys/mount.h>

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <sys/param.h>
#include <VBox/version.h>
#include <iprt/asm.h>

#include <VBox/log.h>

#include "vboxvfs.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * The service class for dealing with Share Folder filesystem.
 */
class org_virtualbox_VBoxVFS : public IOService
{
    OSDeclareDefaultStructors(org_virtualbox_VBoxVFS);

private:
    IOService * waitForCoreService(void);

    IOService * coreService;

public:
    virtual bool start(IOService *pProvider);
    virtual void stop(IOService *pProvider);
};

OSDefineMetaClassAndStructors(org_virtualbox_VBoxVFS, IOService);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

/**
 * Declare the module stuff.
 */
RT_C_DECLS_BEGIN
static kern_return_t VBoxVFSModuleLoad(struct kmod_info *pKModInfo, void *pvData);
static kern_return_t VBoxVFSModuleUnLoad(struct kmod_info *pKModInfo, void *pvData);
extern kern_return_t _start(struct kmod_info *pKModInfo, void *pvData);
extern kern_return_t _stop(struct kmod_info *pKModInfo, void *pvData);
KMOD_EXPLICIT_DECL(VBoxVFS, VBOX_VERSION_STRING, _start, _stop)
DECLHIDDEN(kmod_start_func_t *) _realmain      = VBoxVFSModuleLoad;
DECLHIDDEN(kmod_stop_func_t *)  _antimain      = VBoxVFSModuleUnLoad;
DECLHIDDEN(int)                 _kext_apple_cc = __APPLE_CC__;
RT_C_DECLS_END

/** The number of IOService class instances. */
static bool volatile        g_fInstantiated = 0;
/* Global connection to the host service */
VBGLSFCLIENT                  g_vboxSFClient;
/* VBoxVFS filesystem handle. Needed for FS unregistering. */
static vfstable_t           g_oVBoxVFSHandle;


/**
 * KEXT Module BSD entry point
 */
static kern_return_t VBoxVFSModuleLoad(struct kmod_info *pKModInfo, void *pvData)
{
    int rc;

    /* Initialize the R0 guest library. */
#if 0
    rc = VbglR0SfInit();
    if (RT_FAILURE(rc))
        return KERN_FAILURE;
#endif

    PINFO("VirtualBox " VBOX_VERSION_STRING " shared folders "
          "driver is loaded");

    return KERN_SUCCESS;
}


/**
 * KEXT Module BSD exit point
 */
static kern_return_t VBoxVFSModuleUnLoad(struct kmod_info *pKModInfo, void *pvData)
{
    int rc;

#if 0
   VbglR0SfTerm();
#endif

    PINFO("VirtualBox " VBOX_VERSION_STRING " shared folders driver is unloaded");

    return KERN_SUCCESS;
}


/**
 * Register VBoxFS filesystem.
 *
 * @returns IPRT status code.
 */
int VBoxVFSRegisterFilesystem(void)
{
    struct vfs_fsentry oVFsEntry;
    int rc;

    memset(&oVFsEntry, 0, sizeof(oVFsEntry));
    /* Attach filesystem operations set */
    oVFsEntry.vfe_vfsops = &g_oVBoxVFSOpts;
    /* Attach vnode operations */
    oVFsEntry.vfe_vopcnt = g_cVBoxVFSVnodeOpvDescListSize;
    oVFsEntry.vfe_opvdescs = g_VBoxVFSVnodeOpvDescList;
    /* Set flags */
    oVFsEntry.vfe_flags =
#if ARCH_BITS == 64
            VFS_TBL64BITREADY |
#endif
            VFS_TBLTHREADSAFE |
            VFS_TBLFSNODELOCK |
            VFS_TBLNOTYPENUM;

    memcpy(oVFsEntry.vfe_fsname, VBOXVBFS_NAME, MFSNAMELEN);

    rc = vfs_fsadd(&oVFsEntry, &g_oVBoxVFSHandle);
    if (rc)
    {
        PINFO("Unable to register VBoxVFS filesystem (%d)", rc);
        return VERR_GENERAL_FAILURE;
    }

    PINFO("VBoxVFS filesystem successfully registered");
    return VINF_SUCCESS;
}

/**
 * Unregister VBoxFS filesystem.
 *
 * @returns IPRT status code.
 */
int VBoxVFSUnRegisterFilesystem(void)
{
    int rc;

    if (g_oVBoxVFSHandle == 0)
        return VERR_INVALID_PARAMETER;

    rc = vfs_fsremove(g_oVBoxVFSHandle);
    if (rc)
    {
        PINFO("Unable to unregister VBoxVFS filesystem (%d)", rc);
        return VERR_GENERAL_FAILURE;
    }

    g_oVBoxVFSHandle = 0;

    PINFO("VBoxVFS filesystem successfully unregistered");
    return VINF_SUCCESS;
}


/**
 * Start this service.
 */
bool org_virtualbox_VBoxVFS::start(IOService *pProvider)
{
    int rc;

    if (!IOService::start(pProvider))
        return false;

    /* Low level initialization should be performed only once */
    if (!ASMAtomicCmpXchgBool(&g_fInstantiated, true, false))
    {
        IOService::stop(pProvider);
        return false;
    }

    /* Wait for VBoxGuest to be started */
    coreService = waitForCoreService();
    if (coreService)
    {
        rc = VbglR0SfInit();
        if (RT_SUCCESS(rc))
        {
            /* Connect to the host service. */
            rc = VbglR0SfConnect(&g_vboxSFClient);
            if (RT_SUCCESS(rc))
            {
                PINFO("VBox client connected");
                rc = VbglR0SfSetUtf8(&g_vboxSFClient);
                if (RT_SUCCESS(rc))
                {
                    rc = VBoxVFSRegisterFilesystem();
                    if (RT_SUCCESS(rc))
                    {
                        registerService();
                        PINFO("Successfully started I/O kit class instance");
                        return true;
                    }
                    PERROR("Unable to register VBoxVFS filesystem");
                }
                else
                {
                    PERROR("VbglR0SfSetUtf8 failed: rc=%d", rc);
                }
                VbglR0SfDisconnect(&g_vboxSFClient);
            }
            else
            {
                PERROR("Failed to get connection to host: rc=%d", rc);
            }
            VbglR0SfTerm();
        }
        else
        {
            PERROR("Failed to initialize low level library");
        }
        coreService->release();
    }
    else
    {
        PERROR("VBoxGuest KEXT not started");
    }

    ASMAtomicXchgBool(&g_fInstantiated, false);
    IOService::stop(pProvider);

    return false;
}


/**
 * Stop this service.
 */
void org_virtualbox_VBoxVFS::stop(IOService *pProvider)
{
    int rc;

    AssertReturnVoid(ASMAtomicReadBool(&g_fInstantiated));

    rc = VBoxVFSUnRegisterFilesystem();
    if (RT_FAILURE(rc))
    {
        PERROR("VBoxVFS filesystem is busy. Make sure all "
               "shares are unmounted (%d)", rc);
    }

    VbglR0SfDisconnect(&g_vboxSFClient);
    PINFO("VBox client disconnected");

    VbglR0SfTerm();
    PINFO("Low level uninit done");

    coreService->release();
    PINFO("VBoxGuest service released");

    IOService::stop(pProvider);

    ASMAtomicWriteBool(&g_fInstantiated, false);

    PINFO("Successfully stopped I/O kit class instance");
}


/**
 * Wait for VBoxGuest.kext to be started
 */
IOService * org_virtualbox_VBoxVFS::waitForCoreService(void)
{
    IOService *service;

    OSDictionary *serviceToMatch = serviceMatching("org_virtualbox_VBoxGuest");
    if (!serviceToMatch)
    {
        PINFO("unable to create matching dictionary");
        return false;
    }

    /* Wait 10 seconds for VBoxGuest to be started */
    service = waitForMatchingService(serviceToMatch, 10ULL * 1000000000ULL);
    serviceToMatch->release();

    return service;
}
