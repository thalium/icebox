/* $Id: gvm.h $ */
/** @file
 * GVM - The Global VM Data.
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


#ifndef ___VBox_vmm_gvm_h
#define ___VBox_vmm_gvm_h

#include <VBox/types.h>
#include <iprt/thread.h>


/** @defgroup grp_gvmcpu    GVMCPU - The Global VMCPU Data
 * @ingroup grp_vmm
 * @{
 */

typedef struct GVMCPU
{
    /** VCPU id (0 - (pVM->cCpus - 1). */
    VMCPUID         idCpu;

    /** Handle to the EMT thread. */
    RTNATIVETHREAD  hEMT;

    /** The GVMM per vcpu data. */
    union
    {
#ifdef ___GVMMR0Internal_h
        struct GVMMPERVCPU  s;
#endif
        uint8_t             padding[64];
    } gvmm;
} GVMCPU;
/** Pointer to the GVMCPU data. */
typedef GVMCPU *PGVMCPU;

/** @} */

/** @defgroup grp_gvm   GVM - The Global VM Data
 * @ingroup grp_vmm
 * @{
 */

/**
 * The Global VM Data.
 *
 * This is a ring-0 only structure where we put items we don't need to
 * share with ring-3 or GC, like for instance various RTR0MEMOBJ handles.
 *
 * Unlike VM, there are no special alignment restrictions here. The
 * paddings are checked by compile time assertions.
 */
typedef struct GVM
{
    /** Magic / eye-catcher (GVM_MAGIC). */
    uint32_t        u32Magic;
    /** The global VM handle for this VM. */
    uint32_t        hSelf;
    /** The ring-0 mapping of the VM structure. */
    PVM             pVM;
    /** The support driver session the VM is associated with. */
    PSUPDRVSESSION  pSession;
    /** Number of Virtual CPUs, i.e. how many entries there are in aCpus.
     * Same same as VM::cCpus. */
    uint32_t        cCpus;
    uint32_t        padding;

    /** The GVMM per vm data. */
    union
    {
#ifdef ___GVMMR0Internal_h
        struct GVMMPERVM    s;
#endif
        uint8_t             padding[256];
    } gvmm;

    /** The GMM per vm data. */
    union
    {
#ifdef ___GMMR0Internal_h
        struct GMMPERVM     s;
#endif
        uint8_t             padding[512];
    } gmm;

    /** The RAWPCIVM per vm data. */
    union
    {
#ifdef ___VBox_rawpci_h
        struct RAWPCIPERVM s;
#endif
        uint8_t     padding[64];
    } rawpci;


    /** GVMCPU array for the configured number of virtual CPUs. */
    GVMCPU          aCpus[1];
} GVM;

/** The GVM::u32Magic value (Wayne Shorter). */
#define GVM_MAGIC       0x19330825

/** @} */

#endif

