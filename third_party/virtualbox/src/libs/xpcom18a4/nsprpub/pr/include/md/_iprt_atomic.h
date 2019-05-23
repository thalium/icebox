/* $Id: _iprt_atomic.h $ */
/** @file
 * IPRT Atomic Operation, for including into a system config header.
 */

/*
 * Copyright (C) 2009-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___iprt_atomic_h___
#define ___iprt_atomic_h___

/* Note! Do not copy this around, put it in a header of its own if reused anywhere! */
#include <iprt/asm.h>
#define _PR_HAVE_ATOMIC_OPS
#define _MD_INIT_ATOMIC()               do {} while (0)

DECLINLINE(PRInt32) _PR_IPRT_AtomicIncrement(PRInt32 *pVal)
{
    return ASMAtomicIncS32(pVal);
}
#define _MD_ATOMIC_INCREMENT(pVal)      _PR_IPRT_AtomicIncrement(pVal)

DECLINLINE(PRInt32) _PR_IPRT_AtomicDecrement(PRInt32 *pVal)
{
    return ASMAtomicDecS32(pVal);
}
#define _MD_ATOMIC_DECREMENT(pVal)      _PR_IPRT_AtomicDecrement(pVal)

DECLINLINE(PRInt32) _PR_IPRT_AtomicSet(PRInt32 *pVal, PRInt32 NewVal)
{
    return ASMAtomicXchgS32(pVal, NewVal);
}
#define _MD_ATOMIC_SET(pVal, NewVal)    _PR_IPRT_AtomicSet(pVal, NewVal)

DECLINLINE(PRInt32) _PR_IPRT_AtomicAdd(PRInt32 *pVal, PRInt32 ToAdd)
{
    return ASMAtomicAddS32(pVal, ToAdd) + ToAdd;
}
#define _MD_ATOMIC_ADD(pVal, ToAdd)     _PR_IPRT_AtomicAdd(pVal, ToAdd)

#endif

