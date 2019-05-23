/** @file
 * IPRT - C++ Meta programming.
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

#ifndef ___iprt_cpp_meta_h
#define ___iprt_cpp_meta_h

#include <iprt/types.h>

/** @defgroup grp_rt_cpp_meta   C++ Meta programming utilities
 * @ingroup grp_rt_cpp
 * @{
 */

/**
 * Check for a condition on compile time and dependent of the result TrueResult
 * or FalseResult will be defined.
 *
 * @param   Condition     Condition to check.
 * @param   TrueResult    Result when condition is true.
 * @param   FalseResult   Result when condition is false
 */
template <bool Condition, typename TrueResult, typename FalseResult>
struct RTCIf;

/**
 * Check for a condition on compile time and dependent of the result TrueResult
 * or FalseResult will be defined.
 *
 * True specialization of RTCIf.
 *
 * @param   TrueResult    Result when condition is true.
 * @param   FalseResult   Result when condition is false
 */
template <typename TrueResult, typename FalseResult>
struct RTCIf<true, TrueResult, FalseResult>
{
    typedef TrueResult result;
};

/**
 * Check for a condition on compile time and dependent of the result TrueResult
 * or FalseResult will be defined.
 *
 * False specialization of RTCIf.
 *
 * @param   TrueResult    Result when condition is true.
 * @param   FalseResult   Result when condition is false
 */
template <typename TrueResult, typename FalseResult>
struct RTCIf<false, TrueResult, FalseResult>
{
    typedef FalseResult result;
};

/**
 * Check if @a T is a pointer or not at compile time and dependent of the
 * result TrueResult or FalseResult will be defined.
 *
 * False version of RTCIfPtr.
 *
 * @param   Condition     Condition to check.
 * @param   TrueResult    Result when condition is true.
 * @param   FalseResult   Result when condition is false
 */
template <class T, typename TrueResult, typename FalseResult>
struct RTCIfPtr
{
    typedef FalseResult result;
};

/**
 * Check if @a T is a pointer or not at compile time and dependent of the
 * result TrueResult or FalseResult will be defined.
 *
 * True specialization of RTCIfPtr.
 *
 * @param   Condition     Condition to check.
 * @param   TrueResult    Result when condition is true.
 * @param   FalseResult   Result when condition is false
 */
template <class T, typename TrueResult, typename FalseResult>
struct RTCIfPtr<T*, TrueResult, FalseResult>
{
    typedef TrueResult result;
};

/** @} */

#endif /* !___iprt_cpp_meta_h */

