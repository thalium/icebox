/* $Id: the-freebsd-kernel.h $ */
/** @file
 * IPRT - Ring-0 Driver, The FreeBSD Kernel Headers.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
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

#ifndef ___the_freebsd_kernel_h
#define ___the_freebsd_kernel_h

#include <iprt/types.h>

/* Deal with conflicts first. */
#include <sys/param.h>
#undef PVM
#include <sys/bus.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/libkern.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/limits.h>
#include <sys/unistd.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#if __FreeBSD_version >= 1000030
#include <sys/rwlock.h>
#endif
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/callout.h>
#include <sys/cpu.h>
#include <sys/smp.h>
#include <sys/sleepqueue.h>
#include <sys/sx.h>
#include <vm/vm.h>
#include <vm/pmap.h>            /* for vtophys */
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_param.h>        /* KERN_SUCCESS ++ */
#include <vm/vm_page.h>
#include <vm/vm_phys.h>         /* vm_phys_alloc_* */
#include <vm/vm_extern.h>       /* kmem_alloc_attr */
#include <vm/vm_pageout.h>      /* vm_contig_grow_cache */
#include <sys/vmmeter.h>        /* cnt */
#include <sys/resourcevar.h>
#include <machine/cpu.h>

/**
 * Wrappers around the sleepq_ KPI.
 */
#if __FreeBSD_version >= 800026
# define SLEEPQ_TIMEDWAIT(EventInt) sleepq_timedwait(EventInt, 0)
# define SLEEPQ_TIMEDWAIT_SIG(EventInt) sleepq_timedwait_sig(EventInt, 0)
# define SLEEPQ_WAIT(EventInt) sleepq_wait(EventInt, 0)
# define SLEEPQ_WAIT_SIG(EventInt) sleepq_wait_sig(EventInt, 0)
#else
# define SLEEPQ_TIMEDWAIT(EventInt) sleepq_timedwait(EventInt)
# define SLEEPQ_TIMEDWAIT_SIG(EventInt) sleepq_timedwait_sig(EventInt)
# define SLEEPQ_WAIT(EventInt) sleepq_wait(EventInt)
# define SLEEPQ_WAIT_SIG(EventInt) sleepq_wait_sig(EventInt)
#endif

/**
 * Our pmap_enter version
 */
#if __FreeBSD_version >= 701105
# define MY_PMAP_ENTER(pPhysMap, AddrR3, pPage, fProt, fWired) \
    pmap_enter(pPhysMap, AddrR3, VM_PROT_NONE, pPage, fProt, fWired)
#else
# define MY_PMAP_ENTER(pPhysMap, AddrR3, pPage, fProt, fWired) \
    pmap_enter(pPhysMap, AddrR3, pPage, fProt, fWired)
#endif

/**
 * Check whether we can use kmem_alloc_attr for low allocs.
 */
#if    (__FreeBSD_version >= 900011) \
    || (__FreeBSD_version < 900000 && __FreeBSD_version >= 800505) \
    || (__FreeBSD_version < 800000 && __FreeBSD_version >= 703101)
# define USE_KMEM_ALLOC_ATTR
#endif

/**
 * Check whether we can use kmem_alloc_prot.
 */
#if 0 /** @todo Not available yet. */
# define USE_KMEM_ALLOC_PROT
#endif

#endif
