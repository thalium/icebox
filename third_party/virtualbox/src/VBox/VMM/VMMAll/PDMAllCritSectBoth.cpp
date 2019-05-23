/* $Id: PDMAllCritSectBoth.cpp $ */
/** @file
 * PDM - Code Common to Both Critical Section Types, All Contexts.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_PDM//_CRITSECT
#include "PDMInternal.h"
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmcritsectrw.h>
#include <VBox/vmm/vm.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>


#if defined(IN_RING3) || defined(IN_RING0)
/**
 * Process the critical sections (both types) queued for ring-3 'leave'.
 *
 * @param   pVCpu         The cross context virtual CPU structure.
 */
VMM_INT_DECL(void) PDMCritSectBothFF(PVMCPU pVCpu)
{
    uint32_t i;
    Assert(   pVCpu->pdm.s.cQueuedCritSectLeaves       > 0
           || pVCpu->pdm.s.cQueuedCritSectRwShrdLeaves > 0
           || pVCpu->pdm.s.cQueuedCritSectRwExclLeaves > 0);

    /* Shared leaves. */
    i = pVCpu->pdm.s.cQueuedCritSectRwShrdLeaves;
    pVCpu->pdm.s.cQueuedCritSectRwShrdLeaves = 0;
    while (i-- > 0)
    {
# ifdef IN_RING3
        PPDMCRITSECTRW pCritSectRw = pVCpu->pdm.s.apQueuedCritSectRwShrdLeaves[i];
# else
        PPDMCRITSECTRW pCritSectRw = (PPDMCRITSECTRW)MMHyperR3ToCC(pVCpu->CTX_SUFF(pVM),
                                                                   pVCpu->pdm.s.apQueuedCritSectRwShrdLeaves[i]);
# endif

        pdmCritSectRwLeaveSharedQueued(pCritSectRw);
        LogFlow(("PDMR3CritSectFF: %p (R/W)\n", pCritSectRw));
    }

    /* Last, exclusive leaves. */
    i = pVCpu->pdm.s.cQueuedCritSectRwExclLeaves;
    pVCpu->pdm.s.cQueuedCritSectRwExclLeaves = 0;
    while (i-- > 0)
    {
# ifdef IN_RING3
        PPDMCRITSECTRW pCritSectRw = pVCpu->pdm.s.apQueuedCritSectRwExclLeaves[i];
# else
        PPDMCRITSECTRW pCritSectRw = (PPDMCRITSECTRW)MMHyperR3ToCC(pVCpu->CTX_SUFF(pVM),
                                                                   pVCpu->pdm.s.apQueuedCritSectRwExclLeaves[i]);
# endif

        pdmCritSectRwLeaveExclQueued(pCritSectRw);
        LogFlow(("PDMR3CritSectFF: %p (R/W)\n", pCritSectRw));
    }

    /* Normal leaves. */
    i = pVCpu->pdm.s.cQueuedCritSectLeaves;
    pVCpu->pdm.s.cQueuedCritSectLeaves = 0;
    while (i-- > 0)
    {
# ifdef IN_RING3
        PPDMCRITSECT pCritSect = pVCpu->pdm.s.apQueuedCritSectLeaves[i];
# else
        PPDMCRITSECT pCritSect = (PPDMCRITSECT)MMHyperR3ToCC(pVCpu->CTX_SUFF(pVM), pVCpu->pdm.s.apQueuedCritSectLeaves[i]);
# endif

        PDMCritSectLeave(pCritSect);
        LogFlow(("PDMR3CritSectFF: %p\n", pCritSect));
    }

    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_PDM_CRITSECT);
}
#endif /* IN_RING3 || IN_RING0 */

