/* $Id: mtlist.h $ */
/** @file
 * MS COM / XPCOM Abstraction Layer - Thread-safe list classes declaration.
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

#ifndef ___VBox_com_mtlist_h
#define ___VBox_com_mtlist_h

#include <VBox/com/ptr.h>
#include <VBox/com/string.h>
#include <VBox/com/array.h>
#include <iprt/cpp/mtlist.h>


/** @defgroup grp_com_mtlist    Thread-safe List Classes
 * @ingroup grp_com
 * @{
 */

/**
 * Specialized thread-safe list class for using with com::ComPtr<C>
 *
 * @note: This is necessary cause ComPtr<IFACE> has a size of 8.
 */
template <typename C>
class RTCMTList< ComPtr<C> >: public RTCListBase< ComPtr<C>, ComPtr<C>*, true>
{
    /* Traits */
    typedef ComPtr<C>                 T;
    typedef T                        *ITYPE;
    static const bool                 MT = true;
    typedef RTCListBase<T, ITYPE, MT> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity   The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCMTList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/**
 * Specialized thread-safe list class for using with com::ComObjPtr<C>
 *
 * @note: This is necessary cause ComObjPtr<IFACE> has a size of 8.
 */
template <typename C>
class RTCMTList< ComObjPtr<C> >: public RTCListBase< ComObjPtr<C>, ComObjPtr<C>*, true>
{
    /* Traits */
    typedef ComObjPtr<C>              T;
    typedef T                        *ITYPE;
    static const bool                 MT = true;
    typedef RTCListBase<T, ITYPE, MT> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity   The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCMTList(size_t cCapacity = BASE::kDefaultCapacity)
     : BASE(cCapacity) {}

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/**
 * Specialized thread-safe list class for using with com::Utf8Str.
 *
 * The class offers methods for importing com::SafeArray's of com::Bstr's.
 */
template <>
class RTCMTList<com::Utf8Str>: public RTCListBase<com::Utf8Str, com::Utf8Str *, true>
{
    /* Traits */
    typedef com::Utf8Str              T;
    typedef T                        *ITYPE;
    static const bool                 MT = true;
    typedef RTCListBase<T, ITYPE, MT> BASE;

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

    /**
     * Creates a copy of another list.
     *
     * The other list will be fully copied and the capacity will be the same as
     * the size of the other list. The com::Bstr's are silently converted to
     * com::Utf8Str's.
     *
     * @param   other          The list to copy.
     * @throws  std::bad_alloc
     */
    RTCMTList(ComSafeArrayIn(IN_BSTR, other))
    {
        com::SafeArray<IN_BSTR> sfaOther(ComSafeArrayInArg(other));
        size_t const cElementsOther = sfaOther.size();
        resizeArray(cElementsOther);
        m_cElements = cElementsOther;
        for (size_t i = 0; i < cElementsOther; ++i)
            RTCListHelper<T, ITYPE>::set(m_pArray, i, T(sfaOther[i]));
    }

    /**
     * Creates a copy of another list.
     *
     * The other list will be fully copied and the capacity will be the same as
     * the size of the other list. The com::Bstr's are silently converted to
     * com::Utf8Str's.
     *
     * @param   other          The list to copy.
     * @throws  std::bad_alloc
     */
    RTCMTList(const com::SafeArray<IN_BSTR> &other)
      : BASE(other.size())
    {
        for (size_t i = 0; i < m_cElements; ++i)
            RTCListHelper<T, ITYPE>::set(m_pArray, i, T(other[i]));
    }

    /**
     * Copy the items of the other list into this list. All previous items of
     * this list are deleted.
     *
     * @param   other   The list to copy.
     * @return  a reference to this list.
     * @throws  std::bad_alloc
     */
    RTCListBase<T, ITYPE, MT> &operator=(const com::SafeArray<IN_BSTR> &other)
    {
        m_guard.enterWrite();
         /* Values cleanup */
        RTCListHelper<T, ITYPE>::eraseRange(m_pArray, 0, m_cElements);
        /* Copy */
        if (other.size() != m_cCapacity)
            resizeArrayNoErase(other.size());
        m_cElements = other.size();
        for (size_t i = 0; i < other.size(); ++i)
            RTCListHelper<T, ITYPE>::set(m_pArray, i, T(other[i]));
        m_guard.leaveWrite();

        return *this;
    }

    /**
     * Implicit conversion to a RTCString list.
     *
     * This allows the usage of the RTCString::join method with this list type.
     *
     * @return  a converted const reference to this list.
     */
    operator const RTCMTList<RTCString, RTCString*>&()
    {
        return *reinterpret_cast<RTCMTList<RTCString, RTCString*> *>(this);
    }

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/** @} */

#endif /* !___VBox_com_mtlist_h */

