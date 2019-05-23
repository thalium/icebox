/* $Id: kHlpDefs.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpDefs - Helper Definitions.
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

#ifndef ___k_kHlpDefs_h___
#define ___k_kHlpDefs_h___

#include <k/kDefs.h>

/** @defgroup grp_kHlpDefs - Definitions
 * @addtogroup grp_kHlp
 * @{ */

/** @def KHLP_DECL
 * Declares a kHlp function according to build context.
 * @param type          The return type.
 */
#if defined(KHLP_BUILDING_DYNAMIC)
# define KHLP_DECL(type)    K_DECL_EXPORT(type)
#elif defined(KHLP_BUILT_DYNAMIC)
# define KHLP_DECL(type)    K_DECL_IMPORT(type)
#else
# define KHLP_DECL(type)    type
#endif

/** @} */

#endif

