/** @file
 * VD: Plugin support API.
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

#ifndef ___VBox_vd_plugin_h
#define ___VBox_vd_plugin_h

#include <VBox/vd.h>
#include <VBox/vd-common.h>
#include <VBox/vd-image-backend.h>
#include <VBox/vd-cache-backend.h>
#include <VBox/vd-filter-backend.h>

/**
 * Backend register callbacks structure.
 */
typedef struct VDBACKENDREGISTER
{
    /** Interface version.
     * This is set to VD_BACKENDREG_CB_VERSION. */
    uint32_t                    u32Version;

    /**
     * Registers a new image backend.
     *
     * @returns VBox status code.
     * @param   pvUser    Opaque user data given in the plugin load callback.
     * @param   pBackend  The image backend to register.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegisterImage, (void *pvUser, PCVDIMAGEBACKEND pBackend));

    /**
     * Registers a new cache backend.
     *
     * @returns VBox status code.
     * @param   pvUser    Opaque user data given in the plugin load callback.
     * @param   pBackend  The cache backend to register.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegisterCache, (void *pvUser, PCVDCACHEBACKEND pBackend));

    /**
     * Registers a new filter plugin.
     * @param   pvUser    Opaque user data given in the plugin load callback.
     * @param   pBackend  The filter backend to register.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegisterFilter, (void *pvUser, PCVDFILTERBACKEND pBackend));

} VDBACKENDREGISTER;
/** Pointer to a backend register callbacks structure. */
typedef VDBACKENDREGISTER *PVDBACKENDREGISTER;

/** Current version of the VDBACKENDREGISTER structure.  */
#define VD_BACKENDREG_CB_VERSION                VD_VERSION_MAKE(0xff00, 1, 0)

/**
 * Initialization entry point called by the generic VD layer when
 * a plugin is loaded.
 *
 * @returns VBox status code.
 * @param   pvUser             Opaque user data passed in the register callbacks.
 * @param   pRegisterCallbacks Pointer to the register callbacks structure.
 */
typedef DECLCALLBACK(int) FNVDPLUGINLOAD(void *pvUser, PVDBACKENDREGISTER pRegisterCallbacks);
typedef FNVDPLUGINLOAD *PFNVDPLUGINLOAD;
#define VD_PLUGIN_LOAD_NAME "VDPluginLoad"

/** The prefix to identify Storage Plugins. */
#define VD_PLUGIN_PREFIX "VDPlugin"
/** The size of the prefix excluding the '\\0' terminator. */
#define VD_PLUGIN_PREFIX_LENGTH (sizeof(VD_PLUGIN_PREFIX)-1)

#endif
