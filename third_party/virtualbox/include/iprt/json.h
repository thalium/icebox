/** @file
 * IPRT - JavaScript Object Notation (JSON) Parser.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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

#ifndef ___iprt_json_h
#define ___iprt_json_h

#include <iprt/types.h>
#include <iprt/err.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_json      RTJson - JavaScript Object Notation (JSON) Parser
 * @ingroup grp_rt
 * @{
 */

/**
 * JSON value types.
 */
typedef enum RTJSONVALTYPE
{
    /** Invalid first value. */
    RTJSONVALTYPE_INVALID = 0,
    /** Value containing an object. */
    RTJSONVALTYPE_OBJECT,
    /** Value containing an array. */
    RTJSONVALTYPE_ARRAY,
    /** Value containing a string. */
    RTJSONVALTYPE_STRING,
    /** Value containg a number. */
    RTJSONVALTYPE_NUMBER,
    /** Value containg the special null value. */
    RTJSONVALTYPE_NULL,
    /** Value containing true. */
    RTJSONVALTYPE_TRUE,
    /** Value containing false. */
    RTJSONVALTYPE_FALSE,
    /** 32-bit hack. */
    RTJSONVALTYPE_32BIT_HACK = 0x7fffffff
} RTJSONVALTYPE;
/** Pointer to a JSON value type. */
typedef RTJSONVALTYPE *PRTJSONVALTYPE;

/** JSON value handle. */
typedef struct RTJSONVALINT *RTJSONVAL;
/** Pointer to a JSON value handle. */
typedef RTJSONVAL *PRTJSONVAL;
/** NIL JSON value handle. */
#define NIL_RTJSONVAL ((RTJSONVAL)~(uintptr_t)0)

/** JSON iterator handle. */
typedef struct RTJSONITINT  *RTJSONIT;
/** Pointer to a JSON iterator handle. */
typedef RTJSONIT *PRTJSONIT;
/** NIL JSON iterator handle. */
#define NIL_RTJSONIT ((RTJSONIT)~(uintptr_t)0)

/**
 * Parses a JSON document in the provided buffer returning the root JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_MALFORMED if the document does not conform to the spec.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   pbBuf           The byte buffer containing the JSON document.
 * @param   cbBuf           Size of the buffer.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
RTDECL(int) RTJsonParseFromBuf(PRTJSONVAL phJsonVal, const uint8_t *pbBuf, size_t cbBuf, PRTERRINFO pErrInfo);

/**
 * Parses a JSON document from the provided string returning the root JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_MALFORMED if the document does not conform to the spec.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   pszStr          The string containing the JSON document.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
RTDECL(int) RTJsonParseFromString(PRTJSONVAL phJsonVal, const char *pszStr, PRTERRINFO pErrInfo);

/**
 * Parses a JSON document from the file pointed to by the given filename
 * returning the root JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_MALFORMED if the document does not conform to the spec.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   pszFilename     The name of the file containing the JSON document.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
RTDECL(int) RTJsonParseFromFile(PRTJSONVAL phJsonVal, const char *pszFilename, PRTERRINFO pErrInfo);

/**
 * Retain a given JSON value.
 *
 * @returns New reference count.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(uint32_t) RTJsonValueRetain(RTJSONVAL hJsonVal);

/**
 * Release a given JSON value.
 *
 * @returns New reference count, if this drops to 0 the value is freed.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(uint32_t) RTJsonValueRelease(RTJSONVAL hJsonVal);

/**
 * Return the type of a given JSON value.
 *
 * @returns Type of the given JSON value.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(RTJSONVALTYPE) RTJsonValueGetType(RTJSONVAL hJsonVal);

/**
 * Returns the string from a given JSON string value.
 *
 * @returns Pointer to the string of the JSON value, NULL if the value type
 *          doesn't indicate a string.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(const char *) RTJsonValueGetString(RTJSONVAL hJsonVal);

/**
 * Returns the string from a given JSON string value, extended.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not a string.
 * @param   hJsonVal        The JSON value handle.
 * @param   ppszStr         Where to store the pointer to the string on success.
 */
RTDECL(int) RTJsonValueQueryString(RTJSONVAL hJsonVal, const char **ppszStr);

/**
 * Returns the number from a given JSON number value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not a number.
 * @param   hJsonVal        The JSON value handle.
 * @param   pi64Num         WHere to store the number on success.
 *
 * @note    This JSON implementation does not implement support for floating point
 *          numbers currently.  When it does, it will be in the form of a
 *          RTJsonValueQueryFloat method.
 */
RTDECL(int) RTJsonValueQueryInteger(RTJSONVAL hJsonVal, int64_t *pi64Num);

/**
 * Returns the value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 */
RTDECL(int) RTJsonValueQueryByName(RTJSONVAL hJsonVal, const char *pszName, PRTJSONVAL phJsonVal);

/**
 * Returns the number of a number value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object or
 *          the name does not point to a number value.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   pi64Num         Where to store the number on success.
 */
RTDECL(int) RTJsonValueQueryIntegerByName(RTJSONVAL hJsonVal, const char *pszName, int64_t *pi64Num);

/**
 * Returns the string of a string value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object or
 *          the name does not point to a string value.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   ppszStr         Where to store the pointer to the string on success.
 *                          Must be freed with RTStrFree().
 */
RTDECL(int) RTJsonValueQueryStringByName(RTJSONVAL hJsonVal, const char *pszName, char **ppszStr);

/**
 * Returns the boolean of a true/false value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object or
 *          the name does not point to a true/false value.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   pfBoolean       Where to store the boolean value on success.
 */
RTDECL(int) RTJsonValueQueryBooleanByName(RTJSONVAL hJsonVal, const char *pszName, bool *pfBoolean);

/**
 * Returns the size of a given JSON array value.
 *
 * @returns Size of the JSON array value.
 * @retval  0 if the array is empty or the JSON value is not an array.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(unsigned) RTJsonValueGetArraySize(RTJSONVAL hJsonVal);

/**
 * Returns the size of a given JSON array value - extended version.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an array.
 * @param   hJsonVal        The JSON value handle.
 * @param   pcItems         Where to store the size of the JSON array value on success.
 */
RTDECL(int) RTJsonValueQueryArraySize(RTJSONVAL hJsonVal, unsigned *pcItems);

/**
 * Returns the value for the given index of a given JSON array value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an array.
 * @retval  VERR_OUT_OF_RANGE if @a idx is out of bounds.
 *
 * @param   hJsonVal        The JSON value handle.
 * @param   idx             The index to get the value from.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 */
RTDECL(int) RTJsonValueQueryByIndex(RTJSONVAL hJsonVal, unsigned idx, PRTJSONVAL phJsonVal);

/**
 * Creates an iterator for a given JSON array or object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an array or
 *          object.
 * @param   hJsonVal        The JSON value handle.
 * @param   phJsonIt        Where to store the JSON iterator handle on success.
 */
RTDECL(int) RTJsonIteratorBegin(RTJSONVAL hJsonVal, PRTJSONIT phJsonIt);

/**
 * Gets the value and optional name for the current iterator position.
 *
 * @returns IPRT status code.
 * @param   hJsonIt         The JSON iterator handle.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   ppszName        Where to store the object member name for an object.
 *                          NULL is returned for arrays.
 */
RTDECL(int) RTJsonIteratorQueryValue(RTJSONIT hJsonIt, PRTJSONVAL phJsonVal, const char **ppszName);

/**
 * Advances to the next element in the referenced JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_ITERATOR_END if the end for this iterator was reached.
 * @param   hJsonIt         The JSON iterator handle.
 */
RTDECL(int) RTJsonIteratorNext(RTJSONIT hJsonIt);

/**
 * Frees a given JSON iterator.
 *
 * @param   hJsonIt         The JSON iterator to free.
 */
RTDECL(void) RTJsonIteratorFree(RTJSONIT hJsonIt);

RT_C_DECLS_END

/** @} */

#endif

