// $Id: vbox-standard.dsl $
/// @file
//
// VirtualBox ACPI
//
// Copyright (C) 2006-2015 Oracle Corporation
//
// This file is part of VirtualBox Open Source Edition (OSE), as
// available from http://www.virtualbox.org. This file is free software;
// you can redistribute it and/or modify it under the terms of the GNU
// General Public License (GPL) as published by the Free Software
// Foundation, in version 2 as it comes in the "COPYING" file of the
// VirtualBox OSE distribution. VirtualBox OSE is distributed in the
// hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

DefinitionBlock ("SSDT.aml", "SSDT", 1, "VBOX  ", "VBOXCPUT", 2)
{
    // Processor object
    // #1463: Showing the CPU can make the guest do bad things on it like SpeedStep.
    // In this case, XP SP2 contains this buggy Intelppm.sys driver which wants to mess
    // with SpeedStep if it finds a CPU object and when it finds out that it can't, it
    // tries to unload and crashes (MS probably never tested this code path).
    // So we enable this ACPI object only for certain guests, which do need it,
    // if by accident Windows guest seen enabled CPU object, just boot from latest
    // known good configuration, as it remembers state, even if ACPI object gets disabled.
    Scope (\_PR)
    {
        Processor (CPU0, /* Name */
                    0x00, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }

        Processor (CPU1, /* Name */
                    0x01, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPU2, /* Name */
                    0x02, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPU3, /* Name */
                    0x03, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPU4, /* Name */
                    0x04, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPU5, /* Name */
                    0x05, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPU6, /* Name */
                    0x06, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPU7, /* Name */
                    0x07, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPU8, /* Name */
                    0x08, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPU9, /* Name */
                    0x09, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUA, /* Name */
                    0x0a, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUB, /* Name */
                    0x0b, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUC, /* Name */
                    0x0c, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUD, /* Name */
                    0x0d, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUE, /* Name */
                    0x0e, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUF, /* Name */
                    0x0f, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUG, /* Name */
                    0x10, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUH, /* Name */
                    0x11, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUI, /* Name */
                    0x12, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUJ, /* Name */
                    0x13, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUK, /* Name */
                    0x14, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUL, /* Name */
                    0x15, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUM, /* Name */
                    0x16, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUN, /* Name */
                    0x17, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUO, /* Name */
                    0x18, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUP, /* Name */
                    0x19, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUQ, /* Name */
                    0x1a, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUR, /* Name */
                    0x1b, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUS, /* Name */
                    0x1c, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUT, /* Name */
                    0x1d, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUU, /* Name */
                    0x1e, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
        Processor (CPUV, /* Name */
                    0x1f, /* Id */
                    0x0,  /* Processor IO ports range start */
                    0x0   /* Processor IO ports range length */
                    )
        {
        }
    }
}

/*
 * Local Variables:
 * comment-start: "//"
 * End:
 */

