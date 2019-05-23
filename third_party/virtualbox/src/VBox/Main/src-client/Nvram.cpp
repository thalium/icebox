/* $Id: Nvram.cpp $ */
/** @file
 * VBox NVRAM COM Class implementation.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV_EFI
#include "LoggingNew.h"

#include "Nvram.h"
#include "ConsoleImpl.h"
#include "Global.h"

#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/pdmnvram.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/uuid.h>
#include <iprt/base64.h>
#include <VBox/version.h>
#include <iprt/file.h>
#include <iprt/semaphore.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct NVRAM NVRAM;
typedef struct NVRAM *PNVRAM;

/**
 * Intstance data associated with PDMDRVINS.
 */
struct NVRAM
{
    /** Pointer to the associated class instance. */
    Nvram              *pNvram;
    /** The NVRAM connector interface we provide to DevEFI. */
    PDMINVRAMCONNECTOR  INvramConnector;
    /** The root of the 'Vars' child of the driver config (i.e.
     * VBoxInternal/Devices/efi/0/LUN#0/Config/Vars/).
     * This node has one child node per NVRAM variable. */
    PCFGMNODE           pCfgVarRoot;
    /** The variable node used in the privous drvNvram_VarQueryByIndex call. */
    PCFGMNODE           pLastVarNode;
    /** The index pLastVarNode corresponds to. */
    uint32_t            idxLastVar;
    /** Whether to permanently save the variables or not. */
    bool                fPermanentSave;
};


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The default NVRAM attribute value (non-volatile, boot servier access,
  runtime access). */
#define NVRAM_DEFAULT_ATTRIB        UINT32_C(0x7)
/** The CFGM overlay path of the NVRAM variables. */
#define NVRAM_CFGM_OVERLAY_PATH     "VBoxInternal/Devices/efi/0/LUN#0/Config/Vars"

/**
 * Constructor/destructor
 */
Nvram::Nvram(Console *pConsole)
    : mParent(pConsole),
      mpDrv(NULL)
{
}

Nvram::~Nvram()
{
    if (mpDrv)
    {
        mpDrv->pNvram = NULL;
        mpDrv = NULL;
    }
}


/**
 * @interface_method_impl{PDMINVRAMCONNECTOR,pfnVarStoreSeqEnd}
 */
DECLCALLBACK(int) drvNvram_VarStoreSeqEnd(PPDMINVRAMCONNECTOR pInterface, int rc)
{
    NOREF(pInterface);
    return rc;
}

/**
 * Converts the binary to a CFGM overlay binary string.
 *
 * @returns Pointer to a heap buffer (hand it to RTMemFree when done).
 * @param   pvBuf               The binary data to convert.
 * @param   cbBuf               The number of bytes to convert.
 */
static char *drvNvram_binaryToCfgmString(void const *pvBuf, size_t cbBuf)
{
    static char s_szPrefix[] = "bytes:";
    size_t cbStr   = RTBase64EncodedLength(cbBuf) + sizeof(s_szPrefix);
    char   *pszStr = (char *)RTMemAlloc(cbStr);
    if (pszStr)
    {
        memcpy(pszStr, s_szPrefix, sizeof(s_szPrefix) - 1);
        int rc = RTBase64Encode(pvBuf, cbBuf, &pszStr[sizeof(s_szPrefix) - 1], cbStr - sizeof(s_szPrefix) + 1, NULL);
        if (RT_FAILURE(rc))
        {
            RTMemFree(pszStr);
            pszStr = NULL;
        }
    }
    return pszStr;
}

/**
 * @interface_method_impl{PDMINVRAMCONNECTOR,pfnVarStoreSeqPut}
 */
DECLCALLBACK(int) drvNvram_VarStoreSeqPut(PPDMINVRAMCONNECTOR pInterface, int idxVariable,
                                          PCRTUUID pVendorUuid, const char *pszName, size_t cchName,
                                          uint32_t fAttributes, uint8_t const *pbValue, size_t cbValue)
{
    PNVRAM pThis = RT_FROM_MEMBER(pInterface, NVRAM, INvramConnector);
    int    rc    = VINF_SUCCESS;

    if (pThis->fPermanentSave && pThis->pNvram)
    {
        char    szExtraName[256];
        size_t  offValueNm = RTStrPrintf(szExtraName, sizeof(szExtraName) - 16,
                                         NVRAM_CFGM_OVERLAY_PATH "/%04u/", idxVariable);

        char    szUuid[RTUUID_STR_LENGTH];
        int rc2 = RTUuidToStr(pVendorUuid, szUuid, sizeof(szUuid)); AssertRC(rc2);

        char    szAttribs[32];
        if (fAttributes != NVRAM_DEFAULT_ATTRIB)
            RTStrPrintf(szAttribs, sizeof(szAttribs), "%#x", fAttributes);
        else
            szAttribs[0] = '\0';

        char   *pszValue = drvNvram_binaryToCfgmString(pbValue, cbValue);
        if (pszValue)
        {
            const char *apszTodo[] =
            {
                "Name",     pszName,
                "Uuid",     szUuid,
                "Value",    pszValue,
                "Attribs",  szAttribs,
            };
            for (unsigned i = 0; i < RT_ELEMENTS(apszTodo); i += 2)
            {
                if (!apszTodo[i + 1][0])
                    continue;

                Assert(strlen(apszTodo[i]) < 16);
                strcpy(szExtraName + offValueNm, apszTodo[i]);
                try
                {
                    HRESULT hrc = pThis->pNvram->getParent()->i_machine()->SetExtraData(Bstr(szExtraName).raw(),
                                                                                      Bstr(apszTodo[i + 1]).raw());
                    if (FAILED(hrc))
                    {
                        LogRel(("drvNvram_deleteVar: SetExtraData(%s,%s) returned %Rhrc\n", szExtraName, apszTodo[i + 1], hrc));
                        rc = Global::vboxStatusCodeFromCOM(hrc);
                    }
                }
                catch (...)
                {
                    LogRel(("drvNvram_deleteVar: SetExtraData(%s,%s) threw exception\n", szExtraName, apszTodo[i + 1]));
                    rc = VERR_UNEXPECTED_EXCEPTION;
                }
            }
        }
        else
            rc = VERR_NO_MEMORY;
        RTMemFree(pszValue);
    }

    NOREF(cchName);
    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Deletes a variable.
 *
 * @param   pThis           The NVRAM driver instance data.
 * @param   pszVarNodeNm    The variable node name.
 */
static void drvNvram_deleteVar(PNVRAM pThis, const char *pszVarNodeNm)
{
    char   szExtraName[256];
    size_t offValue = RTStrPrintf(szExtraName, sizeof(szExtraName) - 16, NVRAM_CFGM_OVERLAY_PATH "/%s/", pszVarNodeNm);
    static const char *s_apszValueNames[] = { "Name", "Uuid", "Value", "Attribs" };
    for (unsigned i = 0; i < RT_ELEMENTS(s_apszValueNames); i++)
    {
        Assert(strlen(s_apszValueNames[i]) < 16);
        strcpy(szExtraName + offValue, s_apszValueNames[i]);
        try
        {
            HRESULT hrc = pThis->pNvram->getParent()->i_machine()->SetExtraData(Bstr(szExtraName).raw(), Bstr().raw());
            if (FAILED(hrc))
                LogRel(("drvNvram_deleteVar: SetExtraData(%s,) returned %Rhrc\n", szExtraName, hrc));
        }
        catch (...)
        {
            LogRel(("drvNvram_deleteVar: SetExtraData(%s,) threw exception\n", szExtraName));
        }
    }
}

/**
 * @interface_method_impl{PDMINVRAMCONNECTOR,pfnVarStoreSeqBegin}
 */
DECLCALLBACK(int) drvNvram_VarStoreSeqBegin(PPDMINVRAMCONNECTOR pInterface, uint32_t cVariables)
{
    PNVRAM pThis = RT_FROM_MEMBER(pInterface, NVRAM, INvramConnector);
    int    rc    = VINF_SUCCESS;
    if (pThis->fPermanentSave && pThis->pNvram)
    {
        /*
         * Remove all existing variables.
         */
        for (PCFGMNODE pVarNode = CFGMR3GetFirstChild(pThis->pCfgVarRoot); pVarNode; pVarNode = CFGMR3GetNextChild(pVarNode))
        {
            char szName[128];
            rc = CFGMR3GetName(pVarNode, szName, sizeof(szName));
            if (RT_SUCCESS(rc))
                drvNvram_deleteVar(pThis, szName);
            else
                LogRel(("drvNvram_VarStoreSeqBegin: CFGMR3GetName -> %Rrc\n", rc));
        }
    }

    NOREF(cVariables);
    return rc;
}

/**
 * @interface_method_impl{PDMINVRAMCONNECTOR,pfnVarQueryByIndex}
 */
DECLCALLBACK(int) drvNvram_VarQueryByIndex(PPDMINVRAMCONNECTOR pInterface, uint32_t idxVariable,
                                           PRTUUID pVendorUuid, char *pszName, uint32_t *pcchName,
                                           uint32_t *pfAttributes, uint8_t *pbValue, uint32_t *pcbValue)
{
    PNVRAM pThis = RT_FROM_MEMBER(pInterface, NVRAM, INvramConnector);

    /*
     * Find the requested variable node.
     */
    PCFGMNODE pVarNode;
    if (pThis->idxLastVar + 1 == idxVariable && pThis->pLastVarNode)
        pVarNode = CFGMR3GetNextChild(pThis->pLastVarNode);
    else
    {
        pVarNode = CFGMR3GetFirstChild(pThis->pCfgVarRoot);
        for (uint32_t i = 0; i < idxVariable && pVarNode; i++)
            pVarNode = CFGMR3GetNextChild(pVarNode);
    }
    if (!pVarNode)
        return VERR_NOT_FOUND;

    /* cache it */
    pThis->pLastVarNode = pVarNode;
    pThis->idxLastVar   = idxVariable;

    /*
     * Read the variable node.
     */
    int rc = CFGMR3QueryString(pVarNode, "Name", pszName, *pcchName);
    AssertRCReturn(rc, rc);
    *pcchName = (uint32_t)strlen(pszName);

    char    szUuid[RTUUID_STR_LENGTH];
    rc = CFGMR3QueryString(pVarNode, "Uuid", szUuid, sizeof(szUuid));
    AssertRCReturn(rc, rc);
    rc = RTUuidFromStr(pVendorUuid, szUuid);
    AssertRCReturn(rc, rc);

    rc = CFGMR3QueryU32Def(pVarNode, "Attribs", pfAttributes, NVRAM_DEFAULT_ATTRIB);
    AssertRCReturn(rc, rc);

    size_t cbValue;
    rc = CFGMR3QuerySize(pVarNode, "Value", &cbValue);
    AssertRCReturn(rc, rc);
    AssertReturn(cbValue <= *pcbValue, VERR_BUFFER_OVERFLOW);
    rc = CFGMR3QueryBytes(pVarNode, "Value", pbValue, cbValue);
    AssertRCReturn(rc, rc);
    *pcbValue = (uint32_t)cbValue;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
DECLCALLBACK(void *) Nvram::drvNvram_QueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    LogFlowFunc(("pInterface=%p pszIID=%s\n", pInterface, pszIID));
    PPDMDRVINS pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PNVRAM pThis = PDMINS_2_DATA(pDrvIns, PNVRAM);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMINVRAMCONNECTOR, &pThis->INvramConnector);
    return NULL;
}


/**
 * @interface_method_impl{PDMDRVREG,pfnDestruct}
 */
DECLCALLBACK(void) Nvram::drvNvram_Destruct(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    LogFlowFunc(("iInstance/#%d\n", pDrvIns->iInstance));
    PNVRAM pThis = PDMINS_2_DATA(pDrvIns, PNVRAM);
    if (pThis->pNvram != NULL)
        pThis->pNvram->mpDrv = NULL;
}


/**
 * @interface_method_impl{PDMDRVREG,pfnConstruct}
 */
DECLCALLBACK(int) Nvram::drvNvram_Construct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    LogFlowFunc(("iInstance/#%d pCfg=%p fFlags=%x\n", pDrvIns->iInstance, pCfg, fFlags));
    PNVRAM pThis = PDMINS_2_DATA(pDrvIns, PNVRAM);

    /*
     * Initalize instance data variables first.
     */
    //pThis->pNvram                               = NULL;
    //pThis->cLoadedVariables                     = 0;
    //pThis->fPermanentSave                       = false;
    pThis->pCfgVarRoot                          = CFGMR3GetChild(pCfg, "Vars");
    //pThis->pLastVarNode                         = NULL;
    pThis->idxLastVar                           = UINT32_MAX / 2;

    pDrvIns->IBase.pfnQueryInterface            = Nvram::drvNvram_QueryInterface;
    pThis->INvramConnector.pfnVarQueryByIndex   = drvNvram_VarQueryByIndex;
    pThis->INvramConnector.pfnVarStoreSeqBegin  = drvNvram_VarStoreSeqBegin;
    pThis->INvramConnector.pfnVarStoreSeqPut    = drvNvram_VarStoreSeqPut;
    pThis->INvramConnector.pfnVarStoreSeqEnd    = drvNvram_VarStoreSeqEnd;

    /*
     * Validate and read configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "Object\0"
                                    "PermanentSave\0"))
        return VERR_PDM_DRVINS_UNKNOWN_CFG_VALUES;
    AssertMsgReturn(PDMDrvHlpNoAttach(pDrvIns) == VERR_PDM_NO_ATTACHED_DRIVER,
                    ("Configuration error: Not possible to attach anything to this driver!\n"),
                    VERR_PDM_DRVINS_NO_ATTACH);

    int rc = CFGMR3QueryPtr(pCfg, "Object", (void **)&pThis->pNvram);
    AssertMsgRCReturn(rc, ("Configuration error: No/bad \"Object\" value! rc=%Rrc\n", rc), rc);

    rc = CFGMR3QueryBoolDef(pCfg, "PermanentSave", &pThis->fPermanentSave, false);
    AssertRCReturn(rc, rc);

    /*
     * Let the associated class instance know about us.
     */
    pThis->pNvram->mpDrv = pThis;

    return VINF_SUCCESS;
}


const PDMDRVREG Nvram::DrvReg =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName[32] */
    "NvramStorage",
    /* szRCMod[32] */
    "",
    /* szR0Mod[32] */
    "",
    /* pszDescription */
    "NVRAM Main Driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass */
    PDM_DRVREG_CLASS_VMMDEV,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(NVRAM),
    /* pfnConstruct */
    Nvram::drvNvram_Construct,
    /* pfnDestruct */
    Nvram::drvNvram_Destruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DRVREG_VERSION
};
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
