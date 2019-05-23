/* $Id: kAvlUndef.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kAvlTmpl - Undefines All Macros (both config and temp).
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

/*
 * The configuration.
 */
#undef KAVL_EQUAL_ALLOWED
#undef KAVL_CHECK_FOR_EQUAL_INSERT
#undef KAVL_MAX_STACK
#undef KAVL_RANGE
#undef KAVL_OFFSET
#undef KAVL_STD_KEY_COMP
#undef KAVL_LOOKTHRU
#undef KAVL_LOOKTHRU_HASH
#undef KAVL_LOCKED
#undef KAVL_WRITE_LOCK
#undef KAVL_WRITE_UNLOCK
#undef KAVL_READ_LOCK
#undef KAVL_READ_UNLOCK
#undef KAVLKEY
#undef KAVLNODE
#undef KAVLTREEPTR
#undef KAVLROOT
#undef KAVL_FN
#undef KAVL_TYPE
#undef KAVL_INT
#undef KAVL_DECL
#undef mKey
#undef mKeyLast
#undef mHeight
#undef mpLeft
#undef mpRight
#undef mpList
#undef mpRoot
#undef maLookthru

/*
 * The internal macros.
 */
#undef KAVL_HEIGHTOF
#undef KAVL_GET_POINTER
#undef KAVL_GET_POINTER_NULL
#undef KAVL_SET_POINTER
#undef KAVL_SET_POINTER_NULL
#undef KAVL_NULL
#undef KAVL_G
#undef KAVL_E
#undef KAVL_NE
#undef KAVL_R_IS_IDENTICAL
#undef KAVL_R_IS_INTERSECTING
#undef KAVL_R_IS_IN_RANGE

