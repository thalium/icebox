/** @file
 * tstDevice: Plugin API.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___tstDevicePlugin_h
#define ___tstDevicePlugin_h

#include <VBox/types.h>

/**
 * Config item type.
 */
typedef enum TSTDEVCFGITEMTYPE
{
    /** Invalid type. */
    TSTDEVCFGITEMTYPE_INVALID = 0,
    /** String type. */
    TSTDEVCFGITEMTYPE_STRING,
    /** Integer value encoded in the string. */
    TSTDEVCFGITEMTYPE_INTEGER,
    /** Raw bytes. */
    TSTDEVCFGITEMTYPE_BYTES,
    /** 32bit hack. */
    TSTDEVCFGITEMTYPE_32BIT_HACK = 0x7fffffff
} TSTDEVCFGITEMTYPE;
/** Pointer to a config item type. */
typedef TSTDEVCFGITEMTYPE *PTSTDEVCFGITEMTYPE;


/**
 * Testcase config item.
 */
typedef struct TSTDEVCFGITEM
{
    /** The key of the item. */
    const char          *pszKey;
    /** Type of the config item. */
    TSTDEVCFGITEMTYPE   enmType;
    /** The value of the item (as a string/number of bytes to make static
     * instantiation easier). */
    const char          *pszVal;
} TSTDEVCFGITEM;
/** Pointer to a testcase config item. */
typedef TSTDEVCFGITEM *PTSTDEVCFGITEM;
/** Pointer to a constant testcase config item. */
typedef const TSTDEVCFGITEM *PCTSTDEVCFGITEM;


/** Device under test handle. */
typedef struct TSTDEVDUTINT *TSTDEVDUT;

/**
 * Testcase registration structure.
 */
typedef struct TSTDEVTESTCASEREG
{
    /** Testcase name. */
    char                szName[16];
    /** Testcase description. */
    const char          *pszDesc;
    /** The device name the testcase handles. */
    char                szDevName[16];
    /** Flags for this testcase. */
    uint32_t            fFlags;
    /** CFGM configuration for the device to be instantiated. */
    PCTSTDEVCFGITEM     paDevCfg;

    /**
     * Testcase entry point.
     *
     * @returns VBox status code.
     * @param   hDut      Handle of the device under test.
     */
    DECLR3CALLBACKMEMBER(int, pfnTestEntry, (TSTDEVDUT hDut));
} TSTDEVTESTCASEREG;
/** Pointer to a testcase registration structure. */
typedef TSTDEVTESTCASEREG *PTSTDEVTESTCASEREG;
/** Pointer to a constant testcase registration structure. */
typedef const TSTDEVTESTCASEREG *PCTSTDEVTESTCASEREG;


/**
 * Testcase register callbacks structure.
 */
typedef struct TSTDEVPLUGINREGISTER
{
    /**
     * Registers a new testcase.
     *
     * @returns VBox status code.
     * @param   pvUser       Opaque user data given in the plugin load callback.
     * @param   pTestcaseReg The testcase descriptor to register.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegisterTestcase, (void *pvUser, PCTSTDEVTESTCASEREG pTestcaseReg));

} TSTDEVPLUGINREGISTER;
/** Pointer to a backend register callbacks structure. */
typedef TSTDEVPLUGINREGISTER *PTSTDEVPLUGINREGISTER;


/**
 * Initialization entry point called by the device test framework when
 * a plugin is loaded.
 *
 * @returns VBox status code.
 * @param   pvUser             Opaque user data passed in the register callbacks.
 * @param   pRegisterCallbacks Pointer to the register callbacks structure.
 */
typedef DECLCALLBACK(int) FNTSTDEVPLUGINLOAD(void *pvUser, PTSTDEVPLUGINREGISTER pRegisterCallbacks);
typedef FNTSTDEVPLUGINLOAD *PFNTSTDEVPLUGINLOAD;
#define TSTDEV_PLUGIN_LOAD_NAME "TSTDevPluginLoad"

#endif
