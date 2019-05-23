/* $Id: HGCMObjects.h $ */
/** @file
 * HGCMObjects - Host-Guest Communication Manager objects header.
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

#ifndef ___HGCMOBJECTS__H
#define ___HGCMOBJECTS__H

#include <iprt/assert.h>
#include <iprt/avl.h>
#include <iprt/critsect.h>
#include <iprt/asm.h>

class HGCMObject;

typedef struct _ObjectAVLCore
{
    AVLULNODECORE AvlCore;
    HGCMObject *pSelf;
} ObjectAVLCore;

typedef enum
{
    HGCMOBJ_CLIENT,
    HGCMOBJ_THREAD,
    HGCMOBJ_MSG,
    HGCMOBJ_SizeHack   = 0x7fffffff
} HGCMOBJ_TYPE;

class HGCMObject
{
    private:
        friend uint32_t hgcmObjMake(HGCMObject *pObject, uint32_t u32HandleIn);

        int32_t volatile m_cRefs;
        HGCMOBJ_TYPE     m_enmObjType;

        ObjectAVLCore   m_core;

    protected:
        virtual ~HGCMObject()
        {};

    public:
        HGCMObject(HGCMOBJ_TYPE enmObjType)
            : m_cRefs(0)
        {
            this->m_enmObjType  = enmObjType;
        };

        void Reference()
        {
            int32_t refCnt = ASMAtomicIncS32(&m_cRefs);
            NOREF(refCnt);
            Log(("Reference: refCnt = %d\n", refCnt));
        }

        void Dereference()
        {
            int32_t refCnt = ASMAtomicDecS32(&m_cRefs);

            Log(("Dereference: refCnt = %d\n", refCnt));

            AssertRelease(refCnt >= 0);

            if (refCnt)
            {
                return;
            }

            delete this;
        }

        uint32_t Handle()
        {
            return (uint32_t)m_core.AvlCore.Key;
        };

        HGCMOBJ_TYPE Type()
        {
            return m_enmObjType;
        };
};

int hgcmObjInit();

void hgcmObjUninit();

uint32_t hgcmObjGenerateHandle(HGCMObject *pObject);
uint32_t hgcmObjAssignHandle(HGCMObject *pObject, uint32_t u32Handle);

void hgcmObjDeleteHandle(uint32_t handle);

HGCMObject *hgcmObjReference(uint32_t handle, HGCMOBJ_TYPE enmObjType);

void hgcmObjDereference(HGCMObject *pObject);

uint32_t hgcmObjQueryHandleCount();
void     hgcmObjSetHandleCount(uint32_t u32HandleCount);

#endif /* !___HGCMOBJECTS__H */
