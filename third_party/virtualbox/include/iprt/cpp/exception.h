/** @file
 * IPRT - C++ Base Exceptions.
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

#ifndef ___iprt_cpp_exception_h
#define ___iprt_cpp_exception_h

#include <iprt/cpp/ministring.h>
#include <exception>

/** @defgroup grp_rt_cpp_exceptions     C++ Exceptions
 * @ingroup grp_rt_cpp
 * @{
 */

/**
 * Base exception class for IPRT, derived from std::exception.
 * The XML exceptions are based on this.
 */
class RT_DECL_CLASS RTCError
    : public std::exception
{
public:

    RTCError(const char *pszMessage)
        : m_strMsg(pszMessage)
    {
    }

    RTCError(const RTCString &a_rstrMessage)
        : m_strMsg(a_rstrMessage)
    {
    }

    RTCError(const RTCError &a_rSrc)
        : std::exception(a_rSrc),
          m_strMsg(a_rSrc.what())
    {
    }

    virtual ~RTCError() throw()
    {
    }

    void operator=(const RTCError &a_rSrc)
    {
        m_strMsg = a_rSrc.what();
    }

    void setWhat(const char *a_pszMessage)
    {
        m_strMsg = a_pszMessage;
    }

    virtual const char *what() const throw()
    {
        return m_strMsg.c_str();
    }

private:
    /**
     * Hidden default constructor making sure that the extended one above is
     * always used.
     */
    RTCError();

    /** The exception message. */
    RTCString m_strMsg;
};

/** @} */

#endif

