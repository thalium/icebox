/* $Id: misc.c $ */
/** @file
 * NAT - helpers.
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

/*
 * This code is based on:
 *
 * Copyright (c) 1995 Danny Gasparovski.
 *
 * Please read the file COPYRIGHT for the
 * terms and conditions of the copyright.
 */

#ifndef VBOX_NAT_TST_QUEUE
#include <slirp.h>
#include "zone.h"

# ifndef HAVE_INET_ATON
int
inet_aton(const char *cp, struct in_addr *ia)
{
    u_int32_t addr = inet_addr(cp);
    if (addr == 0xffffffff)
        return 0;
    ia->s_addr = addr;
    return 1;
}
# endif

/*
 * Get our IP address and put it in our_addr
 */
void
getouraddr(PNATState pData)
{
    our_addr.s_addr = loopback_addr.s_addr;
}
#else /* VBOX_NAT_TST_QUEUE */
# include <iprt/cdefs.h>
# include <iprt/types.h>
# include "misc.h"
#endif
struct quehead
{
    struct quehead *qh_link;
    struct quehead *qh_rlink;
};

void
insque(PNATState pData, void *a, void *b)
{
    register struct quehead *element = (struct quehead *) a;
    register struct quehead *head = (struct quehead *) b;
    NOREF(pData);
    element->qh_link = head->qh_link;
    head->qh_link = (struct quehead *)element;
    element->qh_rlink = (struct quehead *)head;
    ((struct quehead *)(element->qh_link))->qh_rlink = (struct quehead *)element;
}

void
remque(PNATState pData, void *a)
{
    register struct quehead *element = (struct quehead *) a;
    NOREF(pData);
    ((struct quehead *)(element->qh_link))->qh_rlink = element->qh_rlink;
    ((struct quehead *)(element->qh_rlink))->qh_link = element->qh_link;
    element->qh_rlink = NULL;
    /*  element->qh_link = NULL;  TCP FIN1 crashes if you do this.  Why ? */
}

#ifndef VBOX_NAT_TST_QUEUE
/*
 * Set fd blocking and non-blocking
 */
void
fd_nonblock(int fd)
{
# ifdef FIONBIO
#  ifdef RT_OS_WINDOWS
    u_long opt = 1;
#  else
    int opt = 1;
#  endif
    ioctlsocket(fd, FIONBIO, &opt);
# else /* !FIONBIO */
    int opt;

    opt = fcntl(fd, F_GETFL, 0);
    opt |= O_NONBLOCK;
    fcntl(fd, F_SETFL, opt);
# endif
}

# if !defined(VBOX_NAT_MEM_DEBUG)
#  if defined (LOG_ENABLED)
#   undef LogFlowFunc
#   define LogFlowFunc(x)

#   undef LogFlowFuncEnter
#   define LogFlowFuncEnter()

#   undef LogFlowFuncLeave
#   define LogFlowFuncLeave()

#   undef Log2
#   define Log2(x)
#  endif /* !LOG_ENABLED */
# else /* VBOX_NAT_MEM_DEBUG */
# define NAT_MEM_LOG_ENABLED
# endif


/**
 * Called when memory becomes available, works pfnXmitPending.
 *
 * @note    This will LEAVE the critical section of the zone and RE-ENTER it
 *          again.  Changes to the zone data should be expected across calls to
 *          this function!
 *
 * @param   zone        The zone.
 */
DECLINLINE(void) slirp_zone_check_and_send_pending(uma_zone_t zone)
{
    LogFlowFunc(("ENTER: zone:%R[mzone]\n", zone));
    if (   zone->fDoXmitPending
        && zone->master_zone == NULL)
    {
        int rc2;
        zone->fDoXmitPending = false;
        rc2 = RTCritSectLeave(&zone->csZone); AssertRC(rc2);

        slirp_output_pending(zone->pData->pvUser);

        rc2 = RTCritSectEnter(&zone->csZone); AssertRC(rc2);
    }
    LogFlowFuncLeave();
}

static void *slirp_uma_alloc(uma_zone_t zone,
                             int size, uint8_t *pflags, int fWait)
{
    struct item *it;
    uint8_t *sub_area;
    void *ret = NULL;
    int rc;

    LogFlowFunc(("ENTER: %R[mzone], size:%d, pflags:%p, %RTbool\n", zone, size, pflags, fWait));
# ifndef NAT_MEM_LOG_ENABLED
    NOREF(size);
    NOREF(pflags);
    NOREF(fWait);
# endif
    RTCritSectEnter(&zone->csZone);
    for (;;)
    {
        if (!LIST_EMPTY(&zone->free_items))
        {
            it = LIST_FIRST(&zone->free_items);
            Assert(it->magic == ITEM_MAGIC);
            rc = 0;
            if (zone->pfInit)
                rc = zone->pfInit(zone->pData, (void *)&it[1], (int /*sigh*/)zone->size, M_DONTWAIT);
            if (rc == 0)
            {
                zone->cur_items++;
                LIST_REMOVE(it, list);
                LIST_INSERT_HEAD(&zone->used_items, it, list);
                slirp_zone_check_and_send_pending(zone); /* may exit+enter the cs! */
                ret = (void *)&it[1];
            }
            else
            {
                AssertMsgFailed(("NAT: item initialization failed for zone %s\n", zone->name));
                ret = NULL;
            }
            break;
        }

        if (!zone->master_zone)
        {
            /* We're on the master zone and we can't allocate more. */
            Log2(("NAT: no room on %s zone\n", zone->name));
            /* AssertMsgFailed(("NAT: OOM!")); */
            zone->fDoXmitPending = true;
            break;
        }

        /* we're on a sub-zone, we need get a chunk from the master zone and split
         * it into sub-zone conforming chunks.
         */
        sub_area = slirp_uma_alloc(zone->master_zone, (int /*sigh*/)zone->master_zone->size, NULL, 0);
        if (!sub_area)
        {
            /* No room on master */
            Log2(("NAT: no room on %s zone for %s zone\n", zone->master_zone->name, zone->name));
            break;
        }
        zone->max_items++;
        it = &((struct item *)sub_area)[-1];
        /* It's the chunk descriptor of the master zone, we should remove it
         * from the master list first.
         */
        Assert((it->zone && it->zone->magic == ZONE_MAGIC));
        RTCritSectEnter(&it->zone->csZone);
        /** @todo should we alter count of master counters? */
        LIST_REMOVE(it, list);
        RTCritSectLeave(&it->zone->csZone);

        /** @todo '+ zone->size' should be depend on flag */
        memset(it, 0, sizeof(struct item));
        it->zone = zone;
        it->magic = ITEM_MAGIC;
        LIST_INSERT_HEAD(&zone->free_items, it, list);
        if (zone->cur_items >= zone->max_items)
            LogRel(("NAT: Zone(%s) has reached it maximum\n", zone->name));
    }
    RTCritSectLeave(&zone->csZone);
    LogFlowFunc(("LEAVE: %p\n", ret));
    return ret;
}

static void slirp_uma_free(void *item, int size, uint8_t flags)
{
    struct item *it;
    uma_zone_t zone;
# ifndef NAT_MEM_LOG_ENABLED
    NOREF(size);
    NOREF(flags);
# endif

    Assert(item);
    it = &((struct item *)item)[-1];
    LogFlowFunc(("ENTER: item:%p(%R[mzoneitem]), size:%d, flags:%RX8\n", item, it, size, flags));
    Assert(it->magic == ITEM_MAGIC);
    zone = it->zone;
    /* check border magic */
    Assert((*(uint32_t *)(((uint8_t *)&it[1]) + zone->size) == 0xabadbabe));

    RTCritSectEnter(&zone->csZone);
    Assert(zone->magic == ZONE_MAGIC);
    LIST_REMOVE(it, list);
    if (zone->pfFini)
    {
        zone->pfFini(zone->pData, item, (int /*sigh*/)zone->size);
    }
    if (zone->pfDtor)
    {
        zone->pfDtor(zone->pData, item, (int /*sigh*/)zone->size, NULL);
    }
    LIST_INSERT_HEAD(&zone->free_items, it, list);
    zone->cur_items--;
    slirp_zone_check_and_send_pending(zone); /* may exit+enter the cs! */
    RTCritSectLeave(&zone->csZone);
    LogFlowFuncLeave();
}

uma_zone_t uma_zcreate(PNATState pData, char *name, size_t size,
                       ctor_t ctor, dtor_t dtor, zinit_t init, zfini_t fini, int flags1, int flags2)
{
    uma_zone_t zone = NULL;
# ifndef NAT_MEM_LOG_ENABLED
    NOREF(flags1);
    NOREF(flags2);
# endif
    LogFlowFunc(("ENTER: name:%s size:%d, ctor:%p, dtor:%p, init:%p, fini:%p, flags1:%RX32, flags2:%RX32\n",
                name, ctor, dtor, init, fini, flags1, flags2));
    zone = RTMemAllocZ(sizeof(struct uma_zone));
    Assert((pData));
    zone->magic = ZONE_MAGIC;
    zone->pData = pData;
    zone->name = name;
    zone->size = size;
    zone->pfCtor = ctor;
    zone->pfDtor = dtor;
    zone->pfInit = init;
    zone->pfFini = fini;
    zone->pfAlloc = slirp_uma_alloc;
    zone->pfFree = slirp_uma_free;
    RTCritSectInit(&zone->csZone);
    LogFlowFunc(("LEAVE: %R[mzone]\n", zone));
    return zone;

}
uma_zone_t uma_zsecond_create(char *name, ctor_t ctor,
    dtor_t dtor, zinit_t init, zfini_t fini, uma_zone_t master)
{
    uma_zone_t zone;
    Assert(master);
    LogFlowFunc(("ENTER: name:%s ctor:%p, dtor:%p, init:%p, fini:%p, master:%R[mzone]\n",
                name, ctor, dtor, init, fini, master));
    zone = RTMemAllocZ(sizeof(struct uma_zone));
    if (zone == NULL)
    {
        LogFlowFunc(("LEAVE: %R[mzone]\n", NULL));
        return NULL;
    }

    Assert((master && master->pData));
    zone->magic = ZONE_MAGIC;
    zone->pData = master->pData;
    zone->name = name;
    zone->pfCtor = ctor;
    zone->pfDtor = dtor;
    zone->pfInit = init;
    zone->pfFini = fini;
    zone->pfAlloc = slirp_uma_alloc;
    zone->pfFree = slirp_uma_free;
    zone->size = master->size;
    zone->master_zone = master;
    RTCritSectInit(&zone->csZone);
    LogFlowFunc(("LEAVE: %R[mzone]\n", zone));
    return zone;
}

void uma_zone_set_max(uma_zone_t zone, int max)
{
    int i = 0;
    struct item *it;
    LogFlowFunc(("ENTER: zone:%R[mzone], max:%d\n", zone, max));
    zone->max_items = max;
    zone->area = RTMemAllocZ(max * (sizeof(struct item) + zone->size + sizeof(uint32_t)));
    for (; i < max; ++i)
    {
        it = (struct item *)(((uint8_t *)zone->area) + i*(sizeof(struct item) + zone->size + sizeof(uint32_t)));
        it->magic = ITEM_MAGIC;
        it->zone = zone;
        *(uint32_t *)(((uint8_t *)&it[1]) + zone->size) = 0xabadbabe;
        LIST_INSERT_HEAD(&zone->free_items, it, list);
    }
    LogFlowFuncLeave();
}

void uma_zone_set_allocf(uma_zone_t zone, uma_alloc_t pfAlloc)
{
    LogFlowFunc(("ENTER: zone:%R[mzone], pfAlloc:%Rfn\n", zone, pfAlloc));
    zone->pfAlloc = pfAlloc;
    LogFlowFuncLeave();
}

void uma_zone_set_freef(uma_zone_t zone, uma_free_t pfFree)
{
    LogFlowFunc(("ENTER: zone:%R[mzone], pfAlloc:%Rfn\n", zone, pfFree));
    zone->pfFree = pfFree;
    LogFlowFuncLeave();
}

uint32_t *uma_find_refcnt(uma_zone_t zone, void *mem)
{
    /** @todo (vvl) this function supposed to work with special zone storing
    reference counters */
    struct item *it = NULL;
# ifndef NAT_MEM_LOG_ENABLED
    NOREF(zone);
# endif
    LogFlowFunc(("ENTER: zone:%R[mzone], mem:%p\n", zone, mem));
    it = (struct item *)mem; /* 1st element */
    Assert(mem != NULL);
    Assert(zone->magic == ZONE_MAGIC);
    /* for returning pointer to counter we need get 0 elemnt */
    Assert(it[-1].magic == ITEM_MAGIC);
    LogFlowFunc(("LEAVE: %p\n", &it[-1].ref_count));
    return &it[-1].ref_count;
}

void *uma_zalloc_arg(uma_zone_t zone, void *args, int how)
{
    void *mem;
# ifndef NAT_MEM_LOG_ENABLED
    NOREF(how);
# endif
    Assert(zone->magic == ZONE_MAGIC);
    LogFlowFunc(("ENTER: zone:%R[mzone], args:%p, how:%RX32\n", zone, args, how));
    if (zone->pfAlloc == NULL)
    {
        LogFlowFunc(("LEAVE: NULL\n"));
        return NULL;
    }
    RTCritSectEnter(&zone->csZone);
    mem = zone->pfAlloc(zone, (int /*sigh*/)zone->size, NULL, 0);
    if (mem != NULL)
    {
        if (zone->pfCtor)
            zone->pfCtor(zone->pData, mem, (int /*sigh*/)zone->size, args, M_DONTWAIT);
    }
    RTCritSectLeave(&zone->csZone);
    LogFlowFunc(("LEAVE: %p\n", mem));
    return mem;
}

void uma_zfree(uma_zone_t zone, void *item)
{
    LogFlowFunc(("ENTER: zone:%R[mzone], item:%p\n", zone, item));
    uma_zfree_arg(zone, item, NULL);
    LogFlowFuncLeave();
}

void uma_zfree_arg(uma_zone_t zone, void *mem, void *flags)
{
    struct item *it;
    Assert(zone->magic == ZONE_MAGIC);
    Assert((zone->pfFree));
    Assert((mem));
    LogFlowFunc(("ENTER: zone:%R[mzone], mem:%p, flags:%p\n", zone, mem, flags));
# ifndef NAT_MEM_LOG_ENABLED
    NOREF(flags);
# endif

    RTCritSectEnter(&zone->csZone);
    it = &((struct item *)mem)[-1];
    Assert((it->magic == ITEM_MAGIC));
    Assert((zone->magic == ZONE_MAGIC && zone == it->zone));

    zone->pfFree(mem,  0, 0);
    RTCritSectLeave(&zone->csZone);
    LogFlowFuncLeave();
}

int uma_zone_exhausted_nolock(uma_zone_t zone)
{
    int fExhausted;
    LogFlowFunc(("ENTER: zone:%R[mzone]\n", zone));
    RTCritSectEnter(&zone->csZone);
    fExhausted = (zone->cur_items == zone->max_items);
    RTCritSectLeave(&zone->csZone);
    LogFlowFunc(("LEAVE: %RTbool\n", fExhausted));
    return fExhausted;
}

void zone_drain(uma_zone_t zone)
{
    struct item *it;
    uma_zone_t master_zone;

    /* vvl: Huh? What to do with zone which hasn't got backstore ? */
    Assert((zone->master_zone));
    LogFlowFunc(("ENTER: zone:%R[mzone]\n", zone));
    master_zone = zone->master_zone;
    while (!LIST_EMPTY(&zone->free_items))
    {
        it = LIST_FIRST(&zone->free_items);
        Assert((it->magic == ITEM_MAGIC));

        RTCritSectEnter(&zone->csZone);
        LIST_REMOVE(it, list);
        zone->max_items--;
        RTCritSectLeave(&zone->csZone);

        it->zone = master_zone;

        RTCritSectEnter(&master_zone->csZone);
        LIST_INSERT_HEAD(&master_zone->free_items, it, list);
        master_zone->cur_items--;
        slirp_zone_check_and_send_pending(master_zone); /* may exit+enter the cs! */
        RTCritSectLeave(&master_zone->csZone);
    }
    LogFlowFuncLeave();
}

void slirp_null_arg_free(void *mem, void *arg)
{
    /** @todo (vvl) make it wiser  */
    LogFlowFunc(("ENTER: mem:%p, arg:%p\n", mem, arg));
    Assert(mem);
# ifndef NAT_MEM_LOG_ENABLED
    NOREF(arg);
# endif
    RTMemFree(mem);
    LogFlowFuncLeave();
}

void *uma_zalloc(uma_zone_t zone, int len)
{
# ifndef NAT_MEM_LOG_ENABLED
    NOREF(zone);
    NOREF(len);
# endif
    LogFlowFunc(("ENTER: zone:%R[mzone], len:%d\n", zone, len));
    LogFlowFunc(("LEAVE: NULL"));
    return NULL;
}

struct mbuf *slirp_ext_m_get(PNATState pData, size_t cbMin, void **ppvBuf, size_t *pcbBuf)
{
    struct mbuf *m;
    int size = MCLBYTES;
    LogFlowFunc(("ENTER: cbMin:%d, ppvBuf:%p, pcbBuf:%p\n", cbMin, ppvBuf, pcbBuf));

    if (cbMin < MCLBYTES)
        size = MCLBYTES;
    else if (cbMin < MJUM9BYTES)
        size = MJUM9BYTES;
    else if (cbMin < MJUM16BYTES)
        size = MJUM16BYTES;
    else
        AssertMsgFailed(("Unsupported size"));

    m = m_getjcl(pData, M_NOWAIT, MT_HEADER, M_PKTHDR, size);
    if (m == NULL)
    {
        *ppvBuf = NULL;
        *pcbBuf = 0;
        LogFlowFunc(("LEAVE: NULL\n"));
        return NULL;
    }
    m->m_len = size;
    *ppvBuf = mtod(m, void *);
    *pcbBuf = size;
    LogFlowFunc(("LEAVE: %p\n", m));
    return m;
}

void slirp_ext_m_free(PNATState pData, struct mbuf *m, uint8_t *pu8Buf)
{

    LogFlowFunc(("ENTER: m:%p, pu8Buf:%p\n", m, pu8Buf));
    if (   !pu8Buf
        && pu8Buf != mtod(m, uint8_t *))
        RTMemFree(pu8Buf); /* This buffer was allocated on heap */
    m_freem(pData, m);
    LogFlowFuncLeave();
}

static void zone_destroy(uma_zone_t zone)
{
    RTCritSectEnter(&zone->csZone);
    LogFlowFunc(("ENTER: zone:%R[mzone]\n", zone));
    LogRel(("NAT: Zone(nm:%s, used:%d)\n", zone->name, zone->cur_items));
    RTMemFree(zone->area);
    RTCritSectLeave(&zone->csZone);
    RTCritSectDelete(&zone->csZone);
    RTMemFree(zone);
    LogFlowFuncLeave();
}

void m_fini(PNATState pData)
{
    LogFlowFuncEnter();
# define ZONE_DESTROY(zone) do { zone_destroy((zone)); (zone) = NULL;} while (0)
    ZONE_DESTROY(pData->zone_clust);
    ZONE_DESTROY(pData->zone_pack);
    ZONE_DESTROY(pData->zone_mbuf);
    ZONE_DESTROY(pData->zone_jumbop);
    ZONE_DESTROY(pData->zone_jumbo9);
    ZONE_DESTROY(pData->zone_jumbo16);
    ZONE_DESTROY(pData->zone_ext_refcnt);
# undef ZONE_DESTROY
    /** @todo do finalize here.*/
    LogFlowFuncLeave();
}

void
if_init(PNATState pData)
{
    /* 14 for ethernet */
    if_maxlinkhdr = 14;
    if_comp = IF_AUTOCOMP;
    if_mtu = 1500;
    if_mru = 1500;
}
#endif /* VBOX_NAT_TST_QUEUE */
