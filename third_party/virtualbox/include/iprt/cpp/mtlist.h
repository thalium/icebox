/** @file
 * IPRT - Generic thread-safe list Class.
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

#ifndef ___iprt_cpp_mtlist_h
#define ___iprt_cpp_mtlist_h

#include <iprt/cpp/list.h>
#include <iprt/semaphore.h>

/** @addtogroup grp_rt_cpp_list
 * @{
 */

/**
 * A guard class for thread-safe read/write access.
 */
template <>
class RTCListGuard<true>
{
public:
    RTCListGuard() : m_hRWSem(NIL_RTSEMRW)
    {
#if defined(RT_LOCK_STRICT_ORDER) && defined(IN_RING3)
        RTLOCKVALCLASS hClass;
        int rc = RTLockValidatorClassCreate(&hClass, true /*fAutodidact*/, RT_SRC_POS, "RTCListGuard");
        AssertStmt(RT_SUCCESS(rc), hClass = NIL_RTLOCKVALCLASS);
        rc = RTSemRWCreateEx(&m_hRWSem, 0 /*fFlags*/, hClass, RTLOCKVAL_SUB_CLASS_NONE, NULL /*pszNameFmt*/);
        AssertRC(rc);
#else
        int rc = RTSemRWCreateEx(&m_hRWSem, 0 /*fFlags*/, NIL_RTLOCKVALCLASS, 0, NULL);
        AssertRC(rc);
#endif
    }

    ~RTCListGuard()
    {
        RTSemRWDestroy(m_hRWSem);
        m_hRWSem = NIL_RTSEMRW;
    }

    inline void enterRead() const { int rc = RTSemRWRequestRead(m_hRWSem, RT_INDEFINITE_WAIT); AssertRC(rc); }
    inline void leaveRead() const { int rc = RTSemRWReleaseRead(m_hRWSem); AssertRC(rc); }
    inline void enterWrite()      { int rc = RTSemRWRequestWrite(m_hRWSem, RT_INDEFINITE_WAIT); AssertRC(rc); }
    inline void leaveWrite()      { int rc = RTSemRWReleaseWrite(m_hRWSem); AssertRC(rc); }

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();

private:
    mutable RTSEMRW m_hRWSem;
};

/**
 * @brief Generic thread-safe list class.
 *
 * RTCMTList is a thread-safe implementation of the list class. It uses a
 * read/write semaphore to serialize the access to the items. Several readers
 * can simultaneous access different or the same item. If one thread is writing
 * to an item, the other accessors are blocked until the write has finished.
 *
 * Although the access is guarded, the user has to make sure the list content
 * is consistent when iterating over the list or doing any other kind of access
 * which makes assumptions about the list content. For a finer control of access
 * restrictions, use your own locking mechanism and the standard list
 * implementation.
 *
 * @see RTCListBase
 */
template <class T, typename ITYPE = typename RTCIf<(sizeof(T) > sizeof(void*)), T*, T>::result>
class RTCMTList : public RTCListBase<T, ITYPE, true>
{
    /* Traits */
    typedef RTCListBase<T, ITYPE, true> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCMTList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/**
 * Specialized thread-safe list class for using the native type list for
 * unsigned 64-bit values even on a 32-bit host.
 *
 * @see RTCListBase
 */
template <>
class RTCMTList<uint64_t>: public RTCListBase<uint64_t, uint64_t, true>
{
    /* Traits */
    typedef RTCListBase<uint64_t, uint64_t, true> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCMTList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/**
 * Specialized thread-safe list class for using the native type list for
 * signed 64-bit values even on a 32-bit host.
 *
 * @see RTCListBase
 */
template <>
class RTCMTList<int64_t>: public RTCListBase<int64_t, int64_t, true>
{
    /* Traits */
    typedef RTCListBase<int64_t, int64_t, true> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCMTList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/** @} */

#endif /* !___iprt_cpp_mtlist_h */

