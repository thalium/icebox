/* $Id: Guid.h $ */
/** @file
 * MS COM / XPCOM Abstraction Layer - Guid class declaration.
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

#ifndef ___VBox_com_Guid_h
#define ___VBox_com_Guid_h

/* Make sure all the stdint.h macros are included - must come first! */
#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
# define __STDC_CONSTANT_MACROS
#endif

#include "VBox/com/string.h"

#include <iprt/uuid.h>
#include <iprt/err.h>


/** @defgroup grp_com_guid  GUID Class
 * @ingroup grp_com
 * @{
 */

namespace com
{

typedef enum GuidState_t
{
    GUID_ZERO,
    GUID_NORMAL,
    GUID_INVALID
} GuidState_t;

/**
 *  Helper class that represents the UUID type and hides platform-specific
 *  implementation details.
 */
class Guid
{
public:

    Guid()
    {
        ::RTUuidClear(&mUuid);
        mGuidState = GUID_ZERO;
        dbg_refresh();
    }

    Guid(const Guid &that)
    {
        mUuid = that.mUuid;
        mGuidState = that.mGuidState;
        dbg_refresh();
    }

    Guid(const RTUUID &that)
    {
        mGuidState = GUID_NORMAL;
        mUuid = that;
        if (isZero())
            mGuidState = GUID_ZERO;
        dbg_refresh();
    }

    Guid(const GUID &that)
    {
        AssertCompileSize(GUID, sizeof(RTUUID));
        ::memcpy(&mUuid, &that, sizeof(GUID));
        mGuidState = GUID_NORMAL;
        if (isZero())
            mGuidState = GUID_ZERO;
        dbg_refresh();
    }

    /**
     * Construct a GUID from a string.
     *
     * @param   that        The UUID string. Can be with or without the curly
     *                      brackets. Empty strings are translated to a zero
     *                      GUID, and strings which are not confirming to
     *                      valid GUID string representations are marked as
     *                      invalid.
     */
    Guid(const char *that)
    {
        initString(that);
    }

    /**
     * Construct a GUID from a BSTR.
     *
     * @param   that        The UUID BSTR. Can be with or without the curly
     *                      brackets. Empty strings are translated to a zero
     *                      GUID, and strings which are not confirming to
     *                      valid GUID string representations are marked as
     *                      invalid.
     */
    Guid(CBSTR that)
    {
        initBSTR(that);
    }

    /**
     * Construct a GUID from a Utf8Str.
     *
     * @param   that        The UUID Utf8Str. Can be with or without the curly
     *                      brackets. Empty strings are translated to a zero
     *                      GUID, and strings which are not confirming to
     *                      valid GUID string representations are marked as
     */
    Guid(const Utf8Str &that)
    {
        initString(that.c_str());
    }

    /**
     * Construct a GUID from a RTCString.
     *
     * @param   that        The UUID RTCString. Can be with or without the curly
     *                      brackets. Empty strings are translated to a zero
     *                      GUID, and strings which are not confirming to
     *                      valid GUID string representations are marked as
     */
    Guid(const RTCString &that)
    {
        initString(that.c_str());
    }

    /**
     * Construct a GUID from a Bstr.
     *
     * @param   that        The UUID Bstr. Can be with or without the curly
     *                      brackets. Empty strings are translated to a zero
     *                      GUID, and strings which are not confirming to
     *                      valid GUID string representations are marked as
     */
    Guid(const Bstr &that)
    {
        initBSTR(that.raw());
    }

    Guid& operator=(const Guid &that)
    {
        mUuid = that.mUuid;
        mGuidState = that.mGuidState;
        dbg_refresh();
        return *this;
    }

    Guid& operator=(const RTUUID &guid)
    {
        mUuid = guid;
        mGuidState = GUID_NORMAL;
        if (isZero())
            mGuidState = GUID_ZERO;
        dbg_refresh();
        return *this;
    }

    Guid& operator=(const GUID &guid)
    {
        AssertCompileSize(GUID, sizeof(RTUUID));
        ::memcpy(&mUuid, &guid, sizeof(GUID));
        mGuidState = GUID_NORMAL;
        if (isZero())
            mGuidState = GUID_ZERO;
        dbg_refresh();
        return *this;
    }

    Guid& operator=(const char *str)
    {
        if (!str || !*str)
        {
            ::RTUuidClear(&mUuid);
            mGuidState = GUID_ZERO;
        }
        else
        {
            mGuidState = GUID_NORMAL;
            int rc = ::RTUuidFromStr(&mUuid, str);
            if (RT_FAILURE(rc))
            {
                ::RTUuidClear(&mUuid);
                mGuidState = GUID_INVALID;
            }
            else if (isZero())
                mGuidState = GUID_ZERO;
        }
        dbg_refresh();
        return *this;
    }

    Guid& operator=(CBSTR str)
    {
        if (!str || !*str)
        {
            ::RTUuidClear(&mUuid);
            mGuidState = GUID_ZERO;
        }
        else
        {
            mGuidState = GUID_NORMAL;
            int rc = ::RTUuidFromUtf16(&mUuid, str);
            if (RT_FAILURE(rc))
            {
                ::RTUuidClear(&mUuid);
                mGuidState = GUID_INVALID;
            }
            else if (isZero())
                mGuidState = GUID_ZERO;
        }
        dbg_refresh();
        return *this;
    }

    Guid& operator=(const Utf8Str &str)
    {
        return operator=(str.c_str());
    }

    Guid& operator=(const RTCString &str)
    {
        return operator=(str.c_str());
    }

    Guid& operator=(const Bstr &str)
    {
        return operator=(str.raw());
    }

    void create()
    {
        ::RTUuidCreate(&mUuid);
        mGuidState = GUID_NORMAL;
        dbg_refresh();
    }

    void clear()
    {
        ::RTUuidClear(&mUuid);
        mGuidState = GUID_ZERO;
        dbg_refresh();
    }

    /**
     * Convert the GUID to a string.
     *
     * @returns String object containing the formatted GUID.
     * @throws  std::bad_alloc
     */
    Utf8Str toString() const
    {
        if (mGuidState == GUID_INVALID)
        {
            /* What to return in case of wrong Guid */
            return Utf8Str("00000000-0000-0000-0000-00000000000");
        }

        char buf[RTUUID_STR_LENGTH];
        ::memset(buf, '\0', sizeof(buf));
        ::RTUuidToStr(&mUuid, buf, sizeof(buf));

        return Utf8Str(buf);
    }

    /**
     * Like toString, but encloses the returned string in curly brackets.
     *
     * @returns String object containing the formatted GUID in curly brackets.
     * @throws  std::bad_alloc
     */
    Utf8Str toStringCurly() const
    {
        if (mGuidState == GUID_INVALID)
        {
            /* What to return in case of wrong Guid */
            return Utf8Str("{00000000-0000-0000-0000-00000000000}");
        }

        char buf[RTUUID_STR_LENGTH + 2];
        ::memset(buf, '\0', sizeof(buf));
        ::RTUuidToStr(&mUuid, buf + 1, sizeof(buf) - 2);
        buf[0] = '{';
        buf[sizeof(buf) - 2] = '}';

        return Utf8Str(buf);
    }

    /**
     * Convert the GUID to a string.
     *
     * @returns Bstr object containing the formatted GUID.
     * @throws  std::bad_alloc
     */
    Bstr toUtf16() const
    {
        if (mGuidState == GUID_INVALID)
        {
            /* What to return in case of wrong Guid */
            return Bstr("00000000-0000-0000-0000-00000000000");
        }

        RTUTF16 buf[RTUUID_STR_LENGTH];
        ::memset(buf, '\0', sizeof(buf));
        ::RTUuidToUtf16(&mUuid, buf, RT_ELEMENTS(buf));

        return Bstr(buf);
    }

    bool isValid() const
    {
        return mGuidState != GUID_INVALID;
    }

    bool isZero() const
    {
        return mGuidState == GUID_ZERO;
    }

    bool operator==(const Guid &that) const { return ::RTUuidCompare(&mUuid, &that.mUuid)    == 0; }
    bool operator==(const RTUUID &guid) const { return ::RTUuidCompare(&mUuid, &guid) == 0; }
    bool operator==(const GUID &guid) const { return ::RTUuidCompare(&mUuid, (PRTUUID)&guid) == 0; }
    bool operator!=(const Guid &that) const { return !operator==(that); }
    bool operator!=(const GUID &guid) const { return !operator==(guid); }
    bool operator!=(const RTUUID &guid) const { return !operator==(guid); }
    bool operator<(const Guid &that) const { return ::RTUuidCompare(&mUuid, &that.mUuid)    < 0; }
    bool operator<(const GUID &guid) const { return ::RTUuidCompare(&mUuid, (PRTUUID)&guid) < 0; }
    bool operator<(const RTUUID &guid) const { return ::RTUuidCompare(&mUuid, &guid) < 0; }

    /**
     * To directly copy the contents to a GUID, or for passing it as an input
     * parameter of type (const GUID *), the compiler converts. */
    const GUID &ref() const
    {
        return *(const GUID *)&mUuid;
    }

    /**
     * To pass instances to printf-like functions.
     */
    PCRTUUID raw() const
    {
        return (PCRTUUID)&mUuid;
    }

#if !defined(VBOX_WITH_XPCOM)

    /** To assign instances to OUT_GUID parameters from within the interface
     * method. */
    const Guid &cloneTo(GUID *pguid) const
    {
        if (pguid)
            ::memcpy(pguid, &mUuid, sizeof(GUID));
        return *this;
    }

    /** To pass instances as OUT_GUID parameters to interface methods. */
    GUID *asOutParam()
    {
        return (GUID *)&mUuid;
    }

#else

    /** To assign instances to OUT_GUID parameters from within the
     * interface method */
    const Guid &cloneTo(nsID **ppGuid) const
    {
        if (ppGuid)
            *ppGuid = (nsID *)nsMemory::Clone(&mUuid, sizeof(nsID));

        return *this;
    }

    /**
     * Internal helper class for asOutParam().
     *
     * This takes a GUID reference in the constructor and copies the mUuid from
     * the method to that instance in its destructor.
     */
    class GuidOutParam
    {
        GuidOutParam(Guid &guid)
            : ptr(0),
              outer(guid)
        {
            outer.clear();
        }

        nsID *ptr;
        Guid &outer;
        GuidOutParam(const GuidOutParam &that); // disabled
        GuidOutParam &operator=(const GuidOutParam &that); // disabled
    public:
        operator nsID**() { return &ptr; }
        ~GuidOutParam()
        {
            if (ptr && outer.isZero())
            {
                outer = *ptr;
                outer.dbg_refresh();
                nsMemory::Free(ptr);
            }
        }
        friend class Guid;
    };

    /** to pass instances as OUT_GUID parameters to interface methods */
    GuidOutParam asOutParam() { return GuidOutParam(*this); }

#endif

    /**
     *  Static immutable empty (zero) object. May be used for comparison purposes.
     */
    static const Guid Empty;

private:
    void initString(const char *that)
    {
        if (!that || !*that)
        {
            ::RTUuidClear(&mUuid);
            mGuidState = GUID_ZERO;
        }
        else
        {
            mGuidState = GUID_NORMAL;
            int rc = ::RTUuidFromStr(&mUuid, that);
            if (RT_FAILURE(rc))
            {
                ::RTUuidClear(&mUuid);
                mGuidState = GUID_INVALID;
            }
            else if (isZero())
                mGuidState = GUID_ZERO;
        }
        dbg_refresh();
    }

    void initBSTR(CBSTR that)
    {
        if (!that || !*that)
        {
            ::RTUuidClear(&mUuid);
            mGuidState = GUID_ZERO;
        }
        else
        {
            mGuidState = GUID_NORMAL;
            int rc = ::RTUuidFromUtf16(&mUuid, that);
            if (RT_FAILURE(rc))
            {
                ::RTUuidClear(&mUuid);
                mGuidState = GUID_INVALID;
            }
            else if (isZero())
                mGuidState = GUID_ZERO;
        }
        dbg_refresh();
    }

    /**
     * Refresh the debug-only UUID string.
     *
     * In debug code, refresh the UUID string representatino for debugging;
     * must be called every time the internal uuid changes; compiles to nothing
     * in release code.
     */
    inline void dbg_refresh()
    {
#ifdef DEBUG
        switch (mGuidState)
        {
            case GUID_ZERO:
            case GUID_NORMAL:
                ::RTUuidToStr(&mUuid, mszUuid, RTUUID_STR_LENGTH);
                break;
            default:
                ::memset(mszUuid, '\0', sizeof(mszUuid));
                ::RTStrCopy(mszUuid, sizeof(mszUuid), "INVALID");
                break;
        }
        m_pcszUUID = mszUuid;
#endif
    }

    /** The UUID. */
    RTUUID mUuid;

    GuidState_t mGuidState;

#ifdef DEBUG
    /** String representation of mUuid for printing in the debugger. */
    char mszUuid[RTUUID_STR_LENGTH];
    /** Another string variant for the debugger, points to szUUID. */
    const char *m_pcszUUID;
#endif
};

} /* namespace com */

/** @} */

#endif /* !___VBox_com_Guid_h */

