/* $Id: GMMR0Internal.h $ */
/** @file
 * GMM - The Global Memory Manager, Internal Header.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___GMMR0Internal_h
#define ___GMMR0Internal_h

#include <VBox/vmm/gmm.h>
#include <iprt/avl.h>


/**
 * Shared module registration info (per VM)
 */
typedef struct GMMSHAREDMODULEPERVM
{
    /** Tree node. */
    AVLGCPTRNODECORE            Core;
    /** Pointer to global shared module info. */
    PGMMSHAREDMODULE            pGlobalModule;
    /** Pointer to the region addresses.
     *
     * They can differe between VMs because of address space scrambling or
     * simply different loading order. */
    RTGCPTR64                   aRegionsGCPtrs[1];
} GMMSHAREDMODULEPERVM;
/** Pointer to a GMMSHAREDMODULEPERVM. */
typedef GMMSHAREDMODULEPERVM *PGMMSHAREDMODULEPERVM;


/** Pointer to a GMM allocation chunk. */
typedef struct GMMCHUNK *PGMMCHUNK;


/** The GMMCHUNK::cFree shift count employed by gmmR0SelectFreeSetList. */
#define GMM_CHUNK_FREE_SET_SHIFT    4
/** Index of the list containing completely unused chunks.
 * The code ASSUMES this is the last list. */
#define GMM_CHUNK_FREE_SET_UNUSED_LIST  (GMM_CHUNK_NUM_PAGES >> GMM_CHUNK_FREE_SET_SHIFT)

/**
 * A set of free chunks.
 */
typedef struct GMMCHUNKFREESET
{
    /** The number of free pages in the set. */
    uint64_t            cFreePages;
    /** The generation ID for the set.  This is incremented whenever
     *  something is linked or unlinked from this set. */
    uint64_t            idGeneration;
    /** Chunks ordered by increasing number of free pages.
     *  In the final list the chunks are completely unused. */
    PGMMCHUNK           apLists[GMM_CHUNK_FREE_SET_UNUSED_LIST + 1];
} GMMCHUNKFREESET;



/**
 * The per-VM GMM data.
 */
typedef struct GMMPERVM
{
    /** Free set for use in bound mode. */
    GMMCHUNKFREESET     Private;
    /** The VM statistics. */
    GMMVMSTATS          Stats;
    /** Shared module tree (per-vm). */
    PAVLGCPTRNODECORE   pSharedModuleTree;
    /** Hints at the last chunk we allocated some memory from. */
    uint32_t            idLastChunkHint;
} GMMPERVM;
/** Pointer to the per-VM GMM data. */
typedef GMMPERVM *PGMMPERVM;

#endif

