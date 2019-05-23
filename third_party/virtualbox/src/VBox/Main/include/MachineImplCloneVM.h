/* $Id: MachineImplCloneVM.h $ */
/** @file
 * Definition of MachineCloneVM
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

#ifndef ____H_MACHINEIMPLCLONEVM
#define ____H_MACHINEIMPLCLONEVM

#include "MachineImpl.h"
#include "ProgressImpl.h"

/* Forward declaration of the d-pointer. */
struct MachineCloneVMPrivate;

class MachineCloneVM
{
public:
    MachineCloneVM(ComObjPtr<Machine> pSrcMachine, ComObjPtr<Machine> pTrgMachine, CloneMode_T mode, const RTCList<CloneOptions_T> &opts);
    ~MachineCloneVM();

    HRESULT start(IProgress **pProgress);

protected:
    HRESULT run();
    void destroy();

    /* d-pointer */
    MachineCloneVM(MachineCloneVMPrivate &d);
    MachineCloneVMPrivate *d_ptr;

    friend struct MachineCloneVMPrivate;
};

#endif // ____H_MACHINEIMPLCLONEVM
/* vi: set tabstop=4 shiftwidth=4 expandtab: */

