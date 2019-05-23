/* $Id: kernel_cpp.h $ */
/** @file
 * Kernel C++, Haiku private.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*
 * This code is based on:
 *
 * VirtualBox Guest Additions for Haiku. C++ in the kernel.
 *
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#ifndef KERNEL_CPP_H
#define KERNEL_CPP_H

#ifdef __cplusplus

#include <new>
#include <stdlib.h>

#if _KERNEL_MODE || _LOADER_MODE

using namespace std;
extern const nothrow_t std::nothrow;

// We need new() versions we can use when also linking against libgcc.
// std::nothrow can't be used since it's defined in both libgcc and
// kernel_cpp.cpp.
typedef struct {} mynothrow_t;
extern const mynothrow_t mynothrow;


inline void *
operator new(size_t size) throw (std::bad_alloc)
{
        // we don't actually throw any exceptions, but we have to
        // keep the prototype as specified in <new>, or else GCC 3
        // won't like us
        return malloc(size);
}


inline void *
operator new[](size_t size) throw (std::bad_alloc)
{
        return malloc(size);
}


inline void *
operator new(size_t size, const std::nothrow_t &) throw ()
{
        return malloc(size);
}


inline void *
operator new[](size_t size, const std::nothrow_t &) throw ()
{
        return malloc(size);
}


inline void *
operator new(size_t size, const mynothrow_t &) throw ()
{
        return malloc(size);
}


inline void *
operator new[](size_t size, const mynothrow_t &) throw ()
{
        return malloc(size);
}


inline void
operator delete(void *ptr) throw ()
{
        free(ptr);
}


inline void
operator delete[](void *ptr) throw ()
{
        free(ptr);
}

#endif  // #if _KERNEL_MODE

#endif  // __cplusplus

#endif  /* KERNEL_CPP_H */
