/* $Id: CFGMInternal.h $ */
/** @file
 * CFGM - Internal header file.
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
 */

#ifndef ___CFGMInternal_h
#define ___CFGMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>


/** @defgroup grp_cfgm_int Internals.
 * @ingroup grp_cfgm
 * @{
 */


/**
 * Configuration manager propertype value.
 */
typedef union CFGMVALUE
{
    /** Integer value. */
    struct CFGMVALUE_INTEGER
    {
        /** The integer represented as 64-bit unsigned. */
        uint64_t        u64;
    } Integer;

    /** String value. (UTF-8 of course) */
    struct CFGMVALUE_STRING
    {
        /** Length of string. (In bytes, including the terminator.) */
        size_t          cb;
        /** Pointer to the string. */
        char           *psz;
    } String;

    /** Byte string value. */
    struct CFGMVALUE_BYTES
    {
        /** Length of byte string. (in bytes) */
        size_t          cb;
        /** Pointer to the byte string. */
        uint8_t        *pau8;
    } Bytes;
} CFGMVALUE;
/** Pointer to configuration manager property value. */
typedef CFGMVALUE *PCFGMVALUE;


/**
 * Configuration manager tree node.
 */
typedef struct CFGMLEAF
{
    /** Pointer to the next leaf. */
    PCFGMLEAF       pNext;
    /** Pointer to the previous leaf. */
    PCFGMLEAF       pPrev;

    /** Property type. */
    CFGMVALUETYPE   enmType;
    /** Property value. */
    CFGMVALUE       Value;

    /** Name length. (exclusive) */
    size_t          cchName;
    /** Name. */
    char            szName[1];
} CFGMLEAF;


/**
 * Configuration manager tree node.
 */
typedef struct CFGMNODE
{
    /** Pointer to the next node (on this level). */
    PCFGMNODE       pNext;
    /** Pointer to the previous node (on this level). */
    PCFGMNODE       pPrev;
    /** Pointer Parent node. */
    PCFGMNODE       pParent;
    /** Pointer to first child node. */
    PCFGMNODE       pFirstChild;
    /** Pointer to first property leaf. */
    PCFGMLEAF       pFirstLeaf;

    /** Pointer to the VM owning this node. */
    PVM             pVM;

    /** The root of a 'restricted' subtree, i.e. the parent is
     * invisible to non-trusted users.
     */
    bool            fRestrictedRoot;

    /** Name length. (exclusive) */
    size_t          cchName;
    /** Name. */
    char            szName[1];
} CFGMNODE;



/**
 * CFGM VM Instance data.
 * Changes to this must checked against the padding of the cfgm union in VM!
 */
typedef struct CFGM
{
    /** Pointer to root node. */
    R3PTRTYPE(PCFGMNODE)    pRoot;
} CFGM;

/** @} */

#endif
