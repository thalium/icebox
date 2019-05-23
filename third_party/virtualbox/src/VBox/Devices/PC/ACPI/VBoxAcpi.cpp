/* $Id: VBoxAcpi.cpp $ */
/** @file
 * VBoxAcpi - VirtualBox ACPI manipulation functionality.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/cdefs.h>
#if !defined(IN_RING3)
# error Pure R3 code
#endif

#define LOG_GROUP LOG_GROUP_DEV_ACPI
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pgm.h>
#include <VBox/log.h>
#include <VBox/param.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/mm.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/string.h>
#include <iprt/file.h>

#ifdef VBOX_WITH_DYNAMIC_DSDT
/* vbox.dsl - input to generate proper DSDT on the fly */
# include <vboxdsl.hex>
#else
/* Statically compiled AML */
# include <vboxaml.hex>
# include <vboxssdt_standard.hex>
# include <vboxssdt_cpuhotplug.hex>
#endif

#include "VBoxDD.h"


#ifdef VBOX_WITH_DYNAMIC_DSDT

static int prepareDynamicDsdt(PPDMDEVINS pDevIns, void **ppvPtr, size_t *pcbDsdt)
{
  *ppvPtr = NULL;
  *pcbDsdt = 0;
  return 0;
}

static int cleanupDynamicDsdt(PPDMDEVINS pDevIns, void *pvPtr)
{
    return 0;
}

#else  /* VBOX_WITH_DYNAMIC_DSDT */

static int patchAml(PPDMDEVINS pDevIns, uint8_t *pabAml, size_t cbAml)
{
    uint16_t cNumCpus;
    int rc;

    rc = CFGMR3QueryU16Def(pDevIns->pCfg, "NumCPUs", &cNumCpus, 1);

    if (RT_FAILURE(rc))
        return rc;

    /* Clear CPU objects at all, if needed */
    bool fShowCpu;
    rc = CFGMR3QueryBoolDef(pDevIns->pCfg, "ShowCpu", &fShowCpu, false);
    if (RT_FAILURE(rc))
        return rc;

    if (!fShowCpu)
        cNumCpus = 0;

    /**
     * Now search AML for:
     *  AML_PROCESSOR_OP            (UINT16) 0x5b83
     * and replace whole block with
     *  AML_NOOP_OP                 (UINT16) 0xa3
     * for VCPU not configured
     */
    for (uint32_t i = 0; i < cbAml - 7; i++)
    {
        /*
         * AML_PROCESSOR_OP
         *
         * DefProcessor := ProcessorOp PkgLength NameString ProcID
                             PblkAddr PblkLen ObjectList
         * ProcessorOp  := ExtOpPrefix 0x83
         * ProcID       := ByteData
         * PblkAddr     := DwordData
         * PblkLen      := ByteData
         */
        if (pabAml[i] == 0x5b && pabAml[i+1] == 0x83)
        {
            if (pabAml[i+3] != 'C' || pabAml[i+4] != 'P')
                /* false alarm, not named starting CP */
                continue;

            /* Processor ID */
            if (pabAml[i+7] < cNumCpus)
              continue;

            /* Will fill unwanted CPU block with NOOPs */
            /*
             * See 18.2.4 Package Length Encoding in ACPI spec
             * for full format
             */
            uint32_t cBytes = pabAml[i + 2];
            AssertReleaseMsg((cBytes >> 6) == 0,
                             ("So far, we only understand simple package length"));

            /* including AML_PROCESSOR_OP itself */
            for (uint32_t j = 0; j < cBytes + 2; j++)
                pabAml[i+j] = 0xa3;

            /* Can increase i by cBytes + 1, but not really worth it */
        }
    }

    /* now recompute checksum, whole file byte sum must be 0 */
    pabAml[9] = 0;
    uint8_t         bSum = 0;
    for (uint32_t i = 0; i < cbAml; i++)
      bSum = bSum + pabAml[i];
    pabAml[9] = (uint8_t)(0 - bSum);

    return 0;
}

/**
 * Patch the CPU hot-plug SSDT version to
 * only contain the ACPI containers which may have a CPU
 */
static int patchAmlCpuHotPlug(PPDMDEVINS pDevIns, uint8_t *pabAml, size_t cbAml)
{
    uint16_t cNumCpus;
    int rc;
    uint32_t idxAml = 0;

    rc = CFGMR3QueryU16Def(pDevIns->pCfg, "NumCPUs", &cNumCpus, 1);

    if (RT_FAILURE(rc))
        return rc;

    /**
     * Now search AML for:
     *  AML_DEVICE_OP               (UINT16) 0x5b82
     * and replace whole block with
     *  AML_NOOP_OP                 (UINT16) 0xa3
     * for VCPU not configured
     */
    while (idxAml < cbAml - 7)
    {
        /*
         * AML_DEVICE_OP
         *
         * DefDevice    := DeviceOp PkgLength NameString ObjectList
         * DeviceOp     := ExtOpPrefix 0x82
         */
        if (pabAml[idxAml] == 0x5b && pabAml[idxAml+1] == 0x82)
        {
            /* Check if the enclosed CPU device is configured. */
            uint8_t *pabAmlPkgLength = &pabAml[idxAml+2];
            uint32_t cBytes = 0;
            uint32_t cLengthBytesFollow = pabAmlPkgLength[0] >> 6;

            if (cLengthBytesFollow == 0)
            {
                /* Simple package length */
                cBytes = pabAmlPkgLength[0];
            }
            else
            {
                unsigned idxLengthByte = 1;

                cBytes = pabAmlPkgLength[0] & 0xF;

                while (idxLengthByte <= cLengthBytesFollow)
                {
                    cBytes |= pabAmlPkgLength[idxLengthByte] << (4*idxLengthByte);
                    idxLengthByte++;
                }
            }

            uint8_t *pabAmlDevName = &pabAmlPkgLength[cLengthBytesFollow+1];
            uint8_t *pabAmlCpu     = &pabAmlDevName[4];
            bool fCpuConfigured = false;
            bool fCpuFound      = false;

            if ((pabAmlDevName[0] != 'S') || (pabAmlDevName[1] != 'C') || (pabAmlDevName[2] != 'K'))
            {
                /* false alarm, not named starting SCK */
                idxAml++;
                continue;
            }

            for (uint32_t idxAmlCpu = 0; idxAmlCpu < cBytes - 7; idxAmlCpu++)
            {
                /*
                 * AML_PROCESSOR_OP
                 *
                 * DefProcessor := ProcessorOp PkgLength NameString ProcID
                                     PblkAddr PblkLen ObjectList
                 * ProcessorOp  := ExtOpPrefix 0x83
                 * ProcID       := ByteData
                 * PblkAddr     := DwordData
                 * PblkLen      := ByteData
                 */
                if ((pabAmlCpu[idxAmlCpu] == 0x5b) && (pabAmlCpu[idxAmlCpu+1] == 0x83))
                {
                    if ((pabAmlCpu[idxAmlCpu+4] != 'C') || (pabAmlCpu[idxAmlCpu+5] != 'P'))
                        /* false alarm, not named starting CP */
                        continue;

                    fCpuFound = true;

                    /* Processor ID */
                    uint8_t const idAmlCpu = pabAmlCpu[idxAmlCpu + 8];
                    if (idAmlCpu < cNumCpus)
                    {
                        LogFlow(("CPU %u is configured\n", idAmlCpu));
                        fCpuConfigured = true;
                    }
                    else
                    {
                        LogFlow(("CPU %u is not configured\n", idAmlCpu));
                        fCpuConfigured = false;
                    }
                    break;
                }
            }

            Assert(fCpuFound);

            if (!fCpuConfigured)
            {
                /* Will fill unwanted CPU block with NOOPs */
                /*
                 * See 18.2.4 Package Length Encoding in ACPI spec
                 * for full format
                 */

                /* including AML_DEVICE_OP itself */
                for (uint32_t j = 0; j < cBytes + 2; j++)
                    pabAml[idxAml+j] = 0xa3;
            }

            idxAml++;
        }
        else
            idxAml++;
    }

    /* now recompute checksum, whole file byte sum must be 0 */
    pabAml[9] = 0;
    uint8_t         bSum = 0;
    for (uint32_t i = 0; i < cbAml; i++)
      bSum = bSum + pabAml[i];
    pabAml[9] = (uint8_t)(0 - bSum);

    return 0;
}

#endif /* VBOX_WITH_DYNAMIC_DSDT */

/**
 * Loads an AML file if present in CFGM
 *
 * @returns VBox status code
 * @param   pDevIns        The device instance
 * @param   pcszCfgName    The configuration key holding the file path
 * @param   pcszSignature  The signature to check for
 * @param   ppabAmlCode     Where to store the pointer to the AML code on success.
 * @param   pcbAmlCode     Where to store the number of bytes of the AML code on success.
 */
static int acpiAmlLoadExternal(PPDMDEVINS pDevIns, const char *pcszCfgName, const char *pcszSignature, uint8_t **ppabAmlCode, size_t *pcbAmlCode)
{
    uint8_t *pabAmlCode = NULL;
    size_t cbAmlCode = 0;
    char *pszAmlFilePath = NULL;
    int rc = CFGMR3QueryStringAlloc(pDevIns->pCfg, pcszCfgName, &pszAmlFilePath);

    if (RT_SUCCESS(rc))
    {
        /* Load from file. */
        RTFILE FileAml = NIL_RTFILE;

        rc = RTFileOpen(&FileAml, pszAmlFilePath, RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_NONE);
        if (RT_SUCCESS(rc))
        {
            /*
             * An AML file contains the raw DSDT thus the size of the file
             * is equal to the size of the DSDT.
             */
            uint64_t cbAmlFile = 0;
            rc = RTFileGetSize(FileAml, &cbAmlFile);

            cbAmlCode = (size_t)cbAmlFile;

            /* Don't use AML files over 4GB ;) */
            if (   RT_SUCCESS(rc)
                && ((uint64_t)cbAmlCode == cbAmlFile))
            {
                pabAmlCode = (uint8_t *)RTMemAllocZ(cbAmlCode);
                if (pabAmlCode)
                {
                    rc = RTFileReadAt(FileAml, 0, pabAmlCode, cbAmlCode, NULL);

                    /*
                     * We fail if reading failed or the identifier at the
                     * beginning is wrong.
                     */
                    if (   RT_FAILURE(rc)
                        || strncmp((const char *)pabAmlCode, pcszSignature, 4))
                    {
                        RTMemFree(pabAmlCode);
                        pabAmlCode = NULL;

                        /* Return error if file header check failed */
                        if (RT_SUCCESS(rc))
                            rc = VERR_PARSE_ERROR;
                    }
                    else
                    {
                        *ppabAmlCode = pabAmlCode;
                        *pcbAmlCode = cbAmlCode;
                        rc = VINF_SUCCESS;
                    }
                }
                else
                    rc = VERR_NO_MEMORY;
            }

            RTFileClose(FileAml);
        }
        MMR3HeapFree(pszAmlFilePath);
    }

    return rc;
}


/** No docs, lazy coder. */
int acpiPrepareDsdt(PPDMDEVINS pDevIns,  void **ppvPtr, size_t *pcbDsdt)
{
#ifdef VBOX_WITH_DYNAMIC_DSDT
    return prepareDynamicDsdt(pDevIns, ppvPtr, pcbDsdt);
#else
    uint8_t *pabAmlCodeDsdt = NULL;
    size_t cbAmlCodeDsdt = 0;
    int rc = acpiAmlLoadExternal(pDevIns, "DsdtFilePath", "DSDT", &pabAmlCodeDsdt, &cbAmlCodeDsdt);

    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        rc = VINF_SUCCESS;

        /* Use the compiled in AML code */
        cbAmlCodeDsdt = sizeof(AmlCode);
        pabAmlCodeDsdt = (uint8_t *)RTMemAllocZ(cbAmlCodeDsdt);
        if (pabAmlCodeDsdt)
            memcpy(pabAmlCodeDsdt, AmlCode, cbAmlCodeDsdt);
        else
            rc = VERR_NO_MEMORY;
    }
    else if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to read \"DsdtFilePath\""));

    if (RT_SUCCESS(rc))
    {
        patchAml(pDevIns, pabAmlCodeDsdt, cbAmlCodeDsdt);
        *ppvPtr = pabAmlCodeDsdt;
        *pcbDsdt = cbAmlCodeDsdt;
    }
    return rc;
#endif
}

/** No docs, lazy coder. */
int acpiCleanupDsdt(PPDMDEVINS pDevIns, void *pvPtr)
{
#ifdef VBOX_WITH_DYNAMIC_DSDT
    return cleanupDynamicDsdt(pDevIns, pvPtr);
#else
    RT_NOREF1(pDevIns);
    if (pvPtr)
        RTMemFree(pvPtr);
    return VINF_SUCCESS;
#endif
}

/** No docs, lazy coder. */
int acpiPrepareSsdt(PPDMDEVINS pDevIns, void **ppvPtr, size_t *pcbSsdt)
{
    uint8_t *pabAmlCodeSsdt = NULL;
    size_t   cbAmlCodeSsdt = 0;
    int rc = acpiAmlLoadExternal(pDevIns, "SsdtFilePath", "SSDT", &pabAmlCodeSsdt, &cbAmlCodeSsdt);

    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        bool fCpuHotPlug = false;
        uint8_t *pabAmlCode = NULL;
        rc = CFGMR3QueryBoolDef(pDevIns->pCfg, "CpuHotPlug", &fCpuHotPlug, false);

        if (RT_FAILURE(rc))
            return rc;

        if (fCpuHotPlug)
        {
            pabAmlCode     = AmlCodeSsdtCpuHotPlug;
            cbAmlCodeSsdt = sizeof(AmlCodeSsdtCpuHotPlug);
        }
        else
        {
            pabAmlCode     = AmlCodeSsdtStandard;
            cbAmlCodeSsdt = sizeof(AmlCodeSsdtStandard);
        }

        pabAmlCodeSsdt = (uint8_t *)RTMemAllocZ(cbAmlCodeSsdt);
        if (pabAmlCodeSsdt)
        {
            memcpy(pabAmlCodeSsdt, pabAmlCode, cbAmlCodeSsdt);

            if (fCpuHotPlug)
                patchAmlCpuHotPlug(pDevIns, pabAmlCodeSsdt, cbAmlCodeSsdt);
            else
                patchAml(pDevIns, pabAmlCodeSsdt, cbAmlCodeSsdt);
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to read \"SsdtFilePath\""));

    if (RT_SUCCESS(rc))
    {
        *ppvPtr = pabAmlCodeSsdt;
        *pcbSsdt = cbAmlCodeSsdt;
    }

    return VINF_SUCCESS;
}

/** No docs, lazy coder. */
int acpiCleanupSsdt(PPDMDEVINS pDevIns, void *pvPtr)
{
    RT_NOREF1(pDevIns);
    if (pvPtr)
        RTMemFree(pvPtr);
    return VINF_SUCCESS;
}

