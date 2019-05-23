/* $Id: VBoxUhgsmi.h $ */
/** @file
 * Document me, pretty please.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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

#ifndef ___VBox_VBoxUhgsmi_h
#define ___VBox_VBoxUhgsmi_h

#include <iprt/cdefs.h>
#include <iprt/types.h>

typedef struct VBOXUHGSMI *PVBOXUHGSMI;

typedef struct VBOXUHGSMI_BUFFER *PVBOXUHGSMI_BUFFER;

typedef struct VBOXUHGSMI_BUFFER_TYPE_FLAGS
{
    union
    {
        struct
        {
            uint32_t fCommand    : 1;
            uint32_t Reserved    : 31;
        } RT_STRUCT_NM(s);
        uint32_t Value;
    } RT_UNION_NM(u);
} VBOXUHGSMI_BUFFER_TYPE_FLAGS;

typedef struct VBOXUHGSMI_BUFFER_LOCK_FLAGS
{
    union
    {
        struct
        {
            uint32_t bReadOnly   : 1;
            uint32_t bWriteOnly  : 1;
            uint32_t bDonotWait  : 1;
            uint32_t bDiscard    : 1;
            uint32_t bLockEntire : 1;
            uint32_t Reserved    : 27;
        } RT_STRUCT_NM(s);
        uint32_t Value;
    } RT_UNION_NM(u);
} VBOXUHGSMI_BUFFER_LOCK_FLAGS;

typedef struct VBOXUHGSMI_BUFFER_SUBMIT_FLAGS
{
    union
    {
        struct
        {
            uint32_t bHostReadOnly          : 1;
            uint32_t bHostWriteOnly         : 1;
            uint32_t bDoNotRetire           : 1; /**< the buffer will be used in a subsequent command */
            uint32_t bEntireBuffer          : 1;
            uint32_t Reserved               : 28;
        } RT_STRUCT_NM(s);
        uint32_t Value;
    } RT_UNION_NM(u);
} VBOXUHGSMI_BUFFER_SUBMIT_FLAGS, *PVBOXUHGSMI_BUFFER_SUBMIT_FLAGS;

/* the caller can specify NULL as a hSynch and specify a valid enmSynchType to make UHGSMI create a proper object itself,
 *  */
typedef DECLCALLBACK(int) FNVBOXUHGSMI_BUFFER_CREATE(PVBOXUHGSMI pHgsmi, uint32_t cbBuf, VBOXUHGSMI_BUFFER_TYPE_FLAGS fType, PVBOXUHGSMI_BUFFER* ppBuf);
typedef FNVBOXUHGSMI_BUFFER_CREATE *PFNVBOXUHGSMI_BUFFER_CREATE;

typedef struct VBOXUHGSMI_BUFFER_SUBMIT
{
    PVBOXUHGSMI_BUFFER pBuf;
    uint32_t offData;
    uint32_t cbData;
    VBOXUHGSMI_BUFFER_SUBMIT_FLAGS fFlags;
} VBOXUHGSMI_BUFFER_SUBMIT, *PVBOXUHGSMI_BUFFER_SUBMIT;

typedef DECLCALLBACK(int) FNVBOXUHGSMI_BUFFER_SUBMIT(PVBOXUHGSMI pHgsmi, PVBOXUHGSMI_BUFFER_SUBMIT aBuffers, uint32_t cBuffers);
typedef FNVBOXUHGSMI_BUFFER_SUBMIT *PFNVBOXUHGSMI_BUFFER_SUBMIT;

typedef DECLCALLBACK(int) FNVBOXUHGSMI_BUFFER_DESTROY(PVBOXUHGSMI_BUFFER pBuf);
typedef FNVBOXUHGSMI_BUFFER_DESTROY *PFNVBOXUHGSMI_BUFFER_DESTROY;

typedef DECLCALLBACK(int) FNVBOXUHGSMI_BUFFER_LOCK(PVBOXUHGSMI_BUFFER pBuf, uint32_t offLock, uint32_t cbLock, VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags, void**pvLock);
typedef FNVBOXUHGSMI_BUFFER_LOCK *PFNVBOXUHGSMI_BUFFER_LOCK;

typedef DECLCALLBACK(int) FNVBOXUHGSMI_BUFFER_UNLOCK(PVBOXUHGSMI_BUFFER pBuf);
typedef FNVBOXUHGSMI_BUFFER_UNLOCK *PFNVBOXUHGSMI_BUFFER_UNLOCK;

typedef struct VBOXUHGSMI
{
    PFNVBOXUHGSMI_BUFFER_CREATE pfnBufferCreate;
    PFNVBOXUHGSMI_BUFFER_SUBMIT pfnBufferSubmit;
    /** User custom data. */
    void *pvUserData;
} VBOXUHGSMI;

typedef struct VBOXUHGSMI_BUFFER
{
    PFNVBOXUHGSMI_BUFFER_LOCK pfnLock;
    PFNVBOXUHGSMI_BUFFER_UNLOCK pfnUnlock;
    PFNVBOXUHGSMI_BUFFER_DESTROY pfnDestroy;

    /* r/o data added for ease of access and simplicity
     * modifying it leads to unpredictable behavior */
    VBOXUHGSMI_BUFFER_TYPE_FLAGS fType;
    uint32_t cbBuffer;
    /** User custom data. */
    void *pvUserData;
} VBOXUHGSMI_BUFFER;

#define VBoxUhgsmiBufferCreate(_pUhgsmi, _cbBuf, _fType, _ppBuf) ((_pUhgsmi)->pfnBufferCreate(_pUhgsmi, _cbBuf, _fType, _ppBuf))
#define VBoxUhgsmiBufferSubmit(_pUhgsmi, _aBuffers, _cBuffers) ((_pUhgsmi)->pfnBufferSubmit(_pUhgsmi, _aBuffers, _cBuffers))

#define VBoxUhgsmiBufferLock(_pBuf, _offLock, _cbLock, _fFlags, _pvLock) ((_pBuf)->pfnLock(_pBuf, _offLock, _cbLock, _fFlags, _pvLock))
#define VBoxUhgsmiBufferUnlock(_pBuf) ((_pBuf)->pfnUnlock(_pBuf))
#define VBoxUhgsmiBufferDestroy(_pBuf) ((_pBuf)->pfnDestroy(_pBuf))

#endif

