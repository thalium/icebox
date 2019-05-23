/* $Id: the-netbsd-kernel.h $ */
/** @file
 * IPRT - Ring-0 Driver, The NetBSD Kernel Headers.
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

#ifndef ___the_netbsd_kernel_h
#define ___the_netbsd_kernel_h

#include <iprt/types.h>

/* Deal with conflicts first. */
#include <sys/param.h>
#undef PVM
#include <sys/bus.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/syslimits.h>
#include <sys/sleepq.h>
#include <sys/unistd.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/callout.h>
#include <sys/rwlock.h>
#include <sys/kmem.h>
#include <sys/cpu.h>
#include <sys/vmmeter.h>        /* cnt */
#include <sys/resourcevar.h>
#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>
#include <machine/cpu.h>

/**
 * Check whether we can use kmem_alloc_prot.
 */
#if 0 /** @todo Not available yet. */
# define USE_KMEM_ALLOC_PROT
#endif

#endif /* ___the_netbsd_kernel_h */
