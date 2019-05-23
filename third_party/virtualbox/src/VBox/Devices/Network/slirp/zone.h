/* $Id: zone.h $ */
/** @file
 * NAT - this file is for sharing zone declaration with emu emulation and logging routines.
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

#ifndef __ZONE_H__
# define __ZONE_H__

# define ITEM_MAGIC 0xdead0001
struct item
{
    uint32_t magic;
    uma_zone_t zone;
    uint32_t ref_count;
    LIST_ENTRY(item) list;
};

# define ZONE_MAGIC 0xdead0002
struct uma_zone
{
    uint32_t magic;
    PNATState pData; /* to minimize changes in the rest of UMA emulation code */
    RTCRITSECT csZone;
    const char *name;
    size_t size; /* item size */
    ctor_t pfCtor;
    dtor_t pfDtor;
    zinit_t pfInit;
    zfini_t pfFini;
    uma_alloc_t pfAlloc;
    uma_free_t pfFree;
    int max_items;
    int cur_items;
    LIST_HEAD(RT_NOTHING, item) used_items;
    LIST_HEAD(RT_NOTHING, item) free_items;
    uma_zone_t master_zone;
    void *area;
    /** Needs call pfnXmitPending when memory becomes available if @c true.
     * @remarks Only applies to the master zone (master_zone == NULL) */
    bool fDoXmitPending;
};
#endif
