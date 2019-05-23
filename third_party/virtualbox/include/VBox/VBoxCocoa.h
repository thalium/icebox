/** @file
 * VBoxCocoa Helper
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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

#ifndef ___VBox_VBoxCocoa_h
#define ___VBox_VBoxCocoa_h

/** Macro which add a typedef of the given Cocoa class in an appropriate form
 * for the current context. This means void* in the C/CPP context and
 * NSWhatever* in the ObjC/ObjCPP context. Use
 * NativeNSWhateverRef/ConstNativeNSWhateverRef when you reference the Cocoa
 * type somewhere. The use of this prevents extensive casting of void* to the
 * right type in the Cocoa context. */
#ifdef __OBJC__
# define ADD_COCOA_NATIVE_REF(CocoaClass) \
    @class CocoaClass; \
    typedef CocoaClass *Native##CocoaClass##Ref; \
    typedef const CocoaClass *ConstNative##CocoaClass##Ref
#else  /* !__OBJC__ */
# define ADD_COCOA_NATIVE_REF(CocoaClass) \
    typedef void *Native##CocoaClass##Ref; \
    typedef const void *ConstNative##CocoaClass##Ref
#endif /* !__OBJC__ */


/*
 * Objective-C++ Helpers.
 */
#if defined(__OBJC__) && defined (__cplusplus)

/* Global includes */
# import <Foundation/NSAutoreleasePool.h>

/** Helper class for automatic creation & destroying of a cocoa auto release
 *  pool. */
class CocoaAutoreleasePool
{
public:
    inline CocoaAutoreleasePool()
    {
         mPool = [[NSAutoreleasePool alloc] init];
    }
    inline ~CocoaAutoreleasePool()
    {
        [mPool release];
    }

private:
    NSAutoreleasePool *mPool;
};

#endif /* __OBJC__ && __cplusplus */

#endif /* !___VBox_VBoxCocoa_h */

