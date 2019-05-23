/* $Id: spc-core.cpp $ */
/** @file
 * IPRT - Crypto - Microsoft SPC / Authenticode, Core APIs.
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
#include "internal/iprt.h"
#include <iprt/crypto/spc.h>

#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "spc-internal.h"


RTDECL(int) RTCrSpcSerializedPageHashes_UpdateDerivedData(PRTCRSPCSERIALIZEDPAGEHASHES pThis)
{
    pThis->pData = (PCRTCRSPCPEIMAGEPAGEHASHES)pThis->RawData.Asn1Core.uData.pv;
    return VINF_SUCCESS;
}


/*
 * SPC Indirect Data Content.
 */

RTDECL(PCRTCRSPCSERIALIZEDOBJECTATTRIBUTE)
RTCrSpcIndirectDataContent_GetPeImageObjAttrib(PCRTCRSPCINDIRECTDATACONTENT pThis,
                                               RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE enmType)
{
    if (pThis->Data.enmType == RTCRSPCAAOVTYPE_PE_IMAGE_DATA)
    {
        Assert(RTAsn1ObjId_CompareWithString(&pThis->Data.Type, RTCRSPCPEIMAGEDATA_OID) == 0);

        if (   pThis->Data.uValue.pPeImage
            && pThis->Data.uValue.pPeImage->T0.File.enmChoice == RTCRSPCLINKCHOICE_MONIKER
            && RTCrSpcSerializedObject_IsPresent(pThis->Data.uValue.pPeImage->T0.File.u.pMoniker) )
        {
            if (pThis->Data.uValue.pPeImage->T0.File.u.pMoniker->enmType == RTCRSPCSERIALIZEDOBJECTTYPE_ATTRIBUTES)
            {
                Assert(RTUuidCompareStr(pThis->Data.uValue.pPeImage->T0.File.u.pMoniker->Uuid.Asn1Core.uData.pUuid,
                                        RTCRSPCSERIALIZEDOBJECT_UUID_STR) == 0);
                PCRTCRSPCSERIALIZEDOBJECTATTRIBUTES pData = pThis->Data.uValue.pPeImage->T0.File.u.pMoniker->u.pData;
                if (pData)
                    for (uint32_t i = 0; i < pData->cItems; i++)
                        if (pData->papItems[i]->enmType == enmType)
                            return pData->papItems[i];
            }
        }
    }
    return NULL;
}


/*
 * Generate the standard core code.
 */
#include <iprt/asn1-generator-core.h>

