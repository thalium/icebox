// $Id: vbox-cpuhotplug.dsl $
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

DefinitionBlock ("SSDT-cpuhotplug.aml", "SSDT", 1, "VBOX  ", "VBOXCPUT", 2)
{
    External(CPUC)
    External(CPUL)
    External(CPEV)
    External(CPET)

    // Method to check for the CPU status
    Method(CPCK, 1)
    {
        Store (Arg0, CPUC)
        Return(LEqual(CPUL, 0x01))
    }

    // Method to notify the VMM that a CPU is not
    // in use anymore and can be safely removed.
    // Using the extra method here because the CPUL
    // register identifer clashes with the CPUL object defined
    // below making iasl starting with version 20150930 fail.
    //
    // Think of CPLO as "CPU Lock Open"
    Method(CPLO, 1)
    {
        Store(Arg0, CPUL)
    }

    Scope (\_SB)
    {

#define GENERATE_CPU_OBJECT(id, sck, sckuid, cpu, cpuuid)<NL>              \
    Device (sck)                                                           \
    {                                                                      \
        Name (_HID, "ACPI0004")                                            \
        Name (_UID, sckuid)                                                \
                                                                           \
        <NL>                                                               \
        Processor (cpu, /* Name */                                         \
                   id,  /* Id */                                           \
                   0x0, /* Processor IO ports range start */               \
                   0x0  /* Processor IO ports range length */              \
                   )                                                       \
        {                                                                  \
            Name (_HID, "ACPI0007")                                        \
            Name (_UID, cpuuid)                                            \
            Name (_PXM, 0x00)                                              \
            <NL>                                                           \
            Method(_MAT, 0, Serialized)                                    \
            {                                                              \
                Name (APIC, Buffer (8) {0x00, 0x08, id, id})               \
                IF (CPCK(id))                                              \
                {                                                          \
                    Store (One, Index (APIC, 4))                           \
                }                                                          \
                Else                                                       \
                {                                                          \
                    Store (Zero, Index (APIC, 4))                          \
                }                                                          \
                Return (APIC)                                              \
            }                                                              \
            <NL>                                                           \
            Method(_STA) /* Used for device presence detection */          \
            {                                                              \
                IF (CPCK(id))                                              \
                {                                                          \
                    Return (0xF)                                           \
                }                                                          \
                Else                                                       \
                {                                                          \
                    Return (0x0)                                           \
                }                                                          \
            }                                                              \
            <NL>                                                           \
            Method(_EJ0, 1)                                                \
            {                                                              \
                CPLO(id) /* Unlock the CPU */                              \
                Return                                                     \
            }                                                              \
        }                                                                  \
    }                                                                      \

        GENERATE_CPU_OBJECT(0x00, SCK0, "SCKCPU0", CPU0, "SCK0-CPU0")
        GENERATE_CPU_OBJECT(0x01, SCK1, "SCKCPU1", CPU1, "SCK1-CPU0")
        GENERATE_CPU_OBJECT(0x02, SCK2, "SCKCPU2", CPU2, "SCK2-CPU0")
        GENERATE_CPU_OBJECT(0x03, SCK3, "SCKCPU3", CPU3, "SCK3-CPU0")
        GENERATE_CPU_OBJECT(0x04, SCK4, "SCKCPU4", CPU4, "SCK4-CPU0")
        GENERATE_CPU_OBJECT(0x05, SCK5, "SCKCPU5", CPU5, "SCK5-CPU0")
        GENERATE_CPU_OBJECT(0x06, SCK6, "SCKCPU6", CPU6, "SCK6-CPU0")
        GENERATE_CPU_OBJECT(0x07, SCK7, "SCKCPU7", CPU7, "SCK7-CPU0")
        GENERATE_CPU_OBJECT(0x08, SCK8, "SCKCPU8", CPU8, "SCK8-CPU0")
        GENERATE_CPU_OBJECT(0x09, SCK9, "SCKCPU9", CPU9, "SCK9-CPU0")
        GENERATE_CPU_OBJECT(0x0a, SCKA, "SCKCPUA", CPUA, "SCKA-CPU0")
        GENERATE_CPU_OBJECT(0x0b, SCKB, "SCKCPUB", CPUB, "SCKB-CPU0")
        GENERATE_CPU_OBJECT(0x0c, SCKC, "SCKCPUC", CPUC, "SCKC-CPU0")
        GENERATE_CPU_OBJECT(0x0d, SCKD, "SCKCPUD", CPUD, "SCKD-CPU0")
        GENERATE_CPU_OBJECT(0x0e, SCKE, "SCKCPUE", CPUE, "SCKE-CPU0")
        GENERATE_CPU_OBJECT(0x0f, SCKF, "SCKCPUF", CPUF, "SCKF-CPU0")
        GENERATE_CPU_OBJECT(0x10, SCKG, "SCKCPUG", CPUG, "SCKG-CPU0")
        GENERATE_CPU_OBJECT(0x11, SCKH, "SCKCPUH", CPUH, "SCKH-CPU0")
        GENERATE_CPU_OBJECT(0x12, SCKI, "SCKCPUI", CPUI, "SCKI-CPU0")
        GENERATE_CPU_OBJECT(0x13, SCKJ, "SCKCPUJ", CPUJ, "SCKJ-CPU0")
        GENERATE_CPU_OBJECT(0x14, SCKK, "SCKCPUK", CPUK, "SCKK-CPU0")
        GENERATE_CPU_OBJECT(0x15, SCKL, "SCKCPUL", CPUL, "SCKL-CPU0")
        GENERATE_CPU_OBJECT(0x16, SCKM, "SCKCPUM", CPUM, "SCKM-CPU0")
        GENERATE_CPU_OBJECT(0x17, SCKN, "SCKCPUN", CPUN, "SCKN-CPU0")
        GENERATE_CPU_OBJECT(0x18, SCKO, "SCKCPUO", CPUO, "SCKO-CPU0")
        GENERATE_CPU_OBJECT(0x19, SCKP, "SCKCPUP", CPUP, "SCKP-CPU0")
        GENERATE_CPU_OBJECT(0x1a, SCKQ, "SCKCPUQ", CPUQ, "SCKQ-CPU0")
        GENERATE_CPU_OBJECT(0x1b, SCKR, "SCKCPUR", CPUR, "SCKR-CPU0")
        GENERATE_CPU_OBJECT(0x1c, SCKS, "SCKCPUS", CPUS, "SCKS-CPU0")
        GENERATE_CPU_OBJECT(0x1d, SCKT, "SCKCPUT", CPUT, "SCKT-CPU0")
        GENERATE_CPU_OBJECT(0x1e, SCKU, "SCKCPUU", CPUU, "SCKU-CPU0")
        GENERATE_CPU_OBJECT(0x1f, SCKV, "SCKCPUV", CPUV, "SCKV-CPU0")

#undef GENERATE_CPU_OBJECT
    }

    Scope (\_GPE)
    {

#define CHECK_CPU(cpu, sck, cpuname)      \
    IF (LEqual(Local0, cpu))              \
    {                                     \
        Notify (\_SB.sck.cpuname, Local1) \
    }                                     \

        // GPE bit 1 handler
        // GPE.1 must be set and SCI raised when
        // processor info changed and CPU must be
        // re-evaluated
        Method (_L01, 0, NotSerialized)
        {
            Store(CPEV, Local0)
            Store(CPET, Local1)

            CHECK_CPU(0x01, SCK1, CPU1)
            CHECK_CPU(0x02, SCK2, CPU2)
            CHECK_CPU(0x03, SCK3, CPU3)
            CHECK_CPU(0x04, SCK4, CPU4)
            CHECK_CPU(0x05, SCK5, CPU5)
            CHECK_CPU(0x06, SCK6, CPU6)
            CHECK_CPU(0x07, SCK7, CPU7)
            CHECK_CPU(0x08, SCK8, CPU8)
            CHECK_CPU(0x09, SCK9, CPU9)
            CHECK_CPU(0x0a, SCKA, CPUA)
            CHECK_CPU(0x0b, SCKB, CPUB)
            CHECK_CPU(0x0c, SCKC, CPUC)
            CHECK_CPU(0x0d, SCKD, CPUD)
            CHECK_CPU(0x0e, SCKE, CPUE)
            CHECK_CPU(0x0f, SCKF, CPUF)
            CHECK_CPU(0x10, SCKG, CPUG)
            CHECK_CPU(0x11, SCKH, CPUH)
            CHECK_CPU(0x12, SCKI, CPUI)
            CHECK_CPU(0x13, SCKJ, CPUJ)
            CHECK_CPU(0x14, SCKK, CPUK)
            CHECK_CPU(0x15, SCKL, CPUL)
            CHECK_CPU(0x16, SCKM, CPUM)
            CHECK_CPU(0x17, SCKN, CPUN)
            CHECK_CPU(0x18, SCKO, CPUO)
            CHECK_CPU(0x19, SCKP, CPUP)
            CHECK_CPU(0x1a, SCKQ, CPUQ)
            CHECK_CPU(0x1b, SCKR, CPUR)
            CHECK_CPU(0x1c, SCKS, CPUS)
            CHECK_CPU(0x1d, SCKT, CPUT)
            CHECK_CPU(0x1e, SCKU, CPUU)
            CHECK_CPU(0x1f, SCKV, CPUV)
        }

#undef CHECK_CPU
    }

}

/*
 * Local Variables:
 * comment-start: "//"
 * End:
 */
