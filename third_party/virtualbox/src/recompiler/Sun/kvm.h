/* $Id: kvm.h $ */
/** @file
 * VBox Recompiler - kvm stub header.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___kvm_stub_h___
#define ___kvm_stub_h___

#define kvm_enabled()                   false
#define kvm_update_guest_debug(a, b)    AssertFailed()
#define kvm_set_phys_mem(a, b, c)       AssertFailed()
#define kvm_arch_get_registers(a)       AssertFailed()
#define cpu_synchronize_state(a)        do { } while (0)

#endif

