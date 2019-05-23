/* $Id: kErr.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kErr - Status Code API.
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

#ifndef ___k_kErr_h___
#define ___k_kErr_h___

/** @defgroup   grp_kErr        kErr - Status Code API
 * @{
 */

/** @def KERR_DECL
 * Declares a kRdr function according to build context.
 * @param type          The return type.
 */
#if defined(KERR_BUILDING_DYNAMIC)
# define KERR_DECL(type)    K_DECL_EXPORT(type)
#elif defined(KRDR_BUILT_DYNAMIC)
# define KERR_DECL(type)    K_DECL_IMPORT(type)
#else
# define KERR_DECL(type)    type
#endif

#ifdef __cplusplus
extern "C" {
#endif

KERR_DECL(const char *) kErrName(int rc);
KERR_DECL(int)  kErrFromErrno(int);
KERR_DECL(int)  kErrFromOS2(unsigned long rcOs2);
KERR_DECL(int)  kErrFromNtStatus(long rcNtStatus);
KERR_DECL(int)  kErrFromMach(int rcMach);
KERR_DECL(int)  kErrFromDarwin(int rcDarwin);

#ifdef __cplusplus
}
#endif

/** @} */

#endif

