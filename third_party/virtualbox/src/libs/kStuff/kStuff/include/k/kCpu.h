/* $Id: kCpu.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kCpu - The CPU and Architecture API.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ___k_kCpu_h___
#define ___k_kCpu_h___

#include <k/kDefs.h>
#include <k/kTypes.h>
#include <k/kCpus.h>


/** @defgroup grp_kCpu      kCpu  - The CPU And Architecture API
 * @{
 */

/** @def KCPU_DECL
 * Declares a kCpu function according to build context.
 * @param type          The return type.
 */
#if defined(KCPU_BUILDING_DYNAMIC)
# define KCPU_DECL(type)        K_DECL_EXPORT(type)
#elif defined(KCPU_BUILT_DYNAMIC)
# define KCPU_DECL(type)        K_DECL_IMPORT(type)
#else
# define KCPU_DECL(type)        type
#endif

#ifdef __cplusplus
extern "C" {
#endif

KCPU_DECL(void) kCpuGetArchAndCpu(PKCPUARCH penmArch, PKCPU penmCpu);
KCPU_DECL(int)  kCpuCompare(KCPUARCH enmCodeArch, KCPU enmCodeCpu, KCPUARCH enmArch, KCPU enmCpu);

#ifdef __cplusplus
}
#endif

/** @} */

#endif
