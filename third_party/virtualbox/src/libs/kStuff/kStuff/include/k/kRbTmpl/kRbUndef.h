/* $Id: kRbUndef.h 35 2009-11-08 19:39:03Z bird $ */
/** @file
 * kRbTmpl - Undefines All Macros (both config and temp).
 */

/*
 * Copyright (c) 2006-2009 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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

/*
 * The configuration.
 */
#undef KRB_EQUAL_ALLOWED
#undef KRB_CHECK_FOR_EQUAL_INSERT
#undef KRB_MAX_STACK
#undef KRB_RANGE
#undef KRB_OFFSET
#undef KRB_STD_KEY_COMP
#undef KRB_CACHE_SIZE
#undef KRB_CACHE_HASH
#undef KRB_LOCKED
#undef KRB_WRITE_LOCK
#undef KRB_WRITE_UNLOCK
#undef KRB_READ_LOCK
#undef KRB_READ_UNLOCK
#undef KRBKEY
#undef KRBNODE
#undef KRBTREEPTR
#undef KRBROOT
#undef KRB_FN
#undef KRB_TYPE
#undef KRB_INT
#undef KRB_DECL
#undef mKey
#undef mKeyLast
#undef mfIsRed
#undef mpLeft
#undef mpRight
#undef mpList
#undef mpRoot
#undef maLookthru
#undef KRB_CMP_G
#undef KRB_CMP_E
#undef KRB_CMP_NE
#undef KRB_R_IS_IDENTICAL
#undef KRB_R_IS_INTERSECTING
#undef KRB_R_IS_IN_RANGE

/*
 * Internal ones.
 */
#undef KRB_IS_RED
#undef KRB_NULL
#undef KRB_GET_POINTER
#undef KRB_GET_POINTER_NULL
#undef KRB_SET_POINTER
#undef KRB_SET_POINTER_NULL

