/* $Id: VBoxVideo3D.h $ */
/** @file
 * VirtualBox 3D common tooling
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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

#ifndef ___VBox_Graphics_VBoxVideo3D_h
#define ___VBox_Graphics_VBoxVideo3D_h

#include <iprt/cdefs.h>
#include <iprt/asm.h>
#ifndef VBoxTlsRefGetImpl
# ifdef VBoxTlsRefSetImpl
#  error "VBoxTlsRefSetImpl is defined, unexpected!"
# endif
# include <iprt/thread.h>
# define VBoxTlsRefGetImpl(_tls) (RTTlsGet((RTTLS)(_tls)))
# define VBoxTlsRefSetImpl(_tls, _val) (RTTlsSet((RTTLS)(_tls), (_val)))
#else
# ifndef VBoxTlsRefSetImpl
#  error "VBoxTlsRefSetImpl is NOT defined, unexpected!"
# endif
#endif

#ifndef VBoxTlsRefAssertImpl
# define VBoxTlsRefAssertImpl(_a) do {} while (0)
#endif

typedef DECLCALLBACK(void) FNVBOXTLSREFDTOR(void*);
typedef FNVBOXTLSREFDTOR *PFNVBOXTLSREFDTOR;

typedef enum {
    VBOXTLSREFDATA_STATE_UNDEFINED = 0,
    VBOXTLSREFDATA_STATE_INITIALIZED,
    VBOXTLSREFDATA_STATE_TOBE_DESTROYED,
    VBOXTLSREFDATA_STATE_DESTROYING,
    VBOXTLSREFDATA_STATE_32BIT_HACK = 0x7fffffff
} VBOXTLSREFDATA_STATE;

#define VBOXTLSREFDATA \
    volatile int32_t cTlsRefs; \
    VBOXTLSREFDATA_STATE enmTlsRefState; \
    PFNVBOXTLSREFDTOR pfnTlsRefDtor; \

struct VBOXTLSREFDATA_DUMMY
{
    VBOXTLSREFDATA
};

#define VBOXTLSREFDATA_OFFSET(_t) RT_OFFSETOF(_t, cTlsRefs)
#define VBOXTLSREFDATA_ASSERT_OFFSET(_t) RTASSERT_OFFSET_OF(_t, cTlsRefs)
#define VBOXTLSREFDATA_SIZE() (sizeof (struct VBOXTLSREFDATA_DUMMY))
#define VBOXTLSREFDATA_COPY(_pDst, _pSrc) do { \
        (_pDst)->cTlsRefs = (_pSrc)->cTlsRefs; \
        (_pDst)->enmTlsRefState = (_pSrc)->enmTlsRefState; \
        (_pDst)->pfnTlsRefDtor = (_pSrc)->pfnTlsRefDtor; \
    } while (0)

#define VBOXTLSREFDATA_EQUAL(_pDst, _pSrc) ( \
           (_pDst)->cTlsRefs == (_pSrc)->cTlsRefs \
        && (_pDst)->enmTlsRefState == (_pSrc)->enmTlsRefState \
        && (_pDst)->pfnTlsRefDtor == (_pSrc)->pfnTlsRefDtor \
    )


#define VBoxTlsRefInit(_p, _pfnDtor) do { \
        (_p)->cTlsRefs = 1; \
        (_p)->enmTlsRefState = VBOXTLSREFDATA_STATE_INITIALIZED; \
        (_p)->pfnTlsRefDtor = (_pfnDtor); \
    } while (0)

#define VBoxTlsRefIsFunctional(_p) (!!((_p)->enmTlsRefState == VBOXTLSREFDATA_STATE_INITIALIZED))

#define VBoxTlsRefAddRef(_p) do { \
        int cRefs = ASMAtomicIncS32(&(_p)->cTlsRefs); \
        VBoxTlsRefAssertImpl(cRefs > 1 || (_p)->enmTlsRefState == VBOXTLSREFDATA_STATE_DESTROYING); \
        RT_NOREF(cRefs); \
    } while (0)

#define VBoxTlsRefCountGet(_p) (ASMAtomicReadS32(&(_p)->cTlsRefs))

#define VBoxTlsRefRelease(_p) do { \
        int cRefs = ASMAtomicDecS32(&(_p)->cTlsRefs); \
        VBoxTlsRefAssertImpl(cRefs >= 0); \
        if (!cRefs && (_p)->enmTlsRefState != VBOXTLSREFDATA_STATE_DESTROYING /* <- avoid recursion if VBoxTlsRefAddRef/Release is called from dtor */) { \
            (_p)->enmTlsRefState = VBOXTLSREFDATA_STATE_DESTROYING; \
            (_p)->pfnTlsRefDtor((_p)); \
        } \
    } while (0)

#define VBoxTlsRefMarkDestroy(_p) do { \
        (_p)->enmTlsRefState = VBOXTLSREFDATA_STATE_TOBE_DESTROYED; \
    } while (0)

#define VBoxTlsRefGetCurrent(_t, _Tsd) ((_t*) VBoxTlsRefGetImpl((_Tsd)))

#define VBoxTlsRefGetCurrentFunctional(_val, _t, _Tsd) do { \
       _t * cur = VBoxTlsRefGetCurrent(_t, _Tsd); \
       if (!cur || VBoxTlsRefIsFunctional(cur)) { \
           (_val) = cur; \
       } else { \
           VBoxTlsRefSetCurrent(_t, _Tsd, NULL); \
           (_val) = NULL; \
       } \
   } while (0)

#define VBoxTlsRefSetCurrent(_t, _Tsd, _p) do { \
        _t * oldCur = VBoxTlsRefGetCurrent(_t, _Tsd); \
        if (oldCur != (_p)) { \
            VBoxTlsRefSetImpl((_Tsd), (_p)); \
            if (oldCur) { \
                VBoxTlsRefRelease(oldCur); \
            } \
            if ((_p)) { \
                VBoxTlsRefAddRef((_t*)(_p)); \
            } \
        } \
    } while (0)


/* host 3D->Fe[/Qt] notification mechanism defines */
#define VBOX3D_NOTIFY_EVENT_TYPE_TEST_FUNCTIONAL 3
#define VBOX3D_NOTIFY_EVENT_TYPE_3DDATA_VISIBLE  4
#define VBOX3D_NOTIFY_EVENT_TYPE_3DDATA_HIDDEN   5


#endif /* #ifndef ___VBox_Graphics_VBoxVideo3D_h */
