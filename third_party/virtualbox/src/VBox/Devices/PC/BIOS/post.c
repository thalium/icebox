/** @file
 * BIOS POST routines. Used only during initialization.
 */

/*
 * Copyright (C) 2004-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <stdint.h>
#include <string.h>
#include "biosint.h"
#include "inlines.h"

#if DEBUG_POST
#  define DPRINT(...) BX_DEBUG(__VA_ARGS__)
#else
#  define DPRINT(...)
#endif

/* In general, checksumming ROMs in a VM just wastes time. */
//#define CHECKSUM_ROMS

/* The format of a ROM is as follows:
 *
 *     ------------------------------
 *   0 | AA55h signature (word)     |
 *     ------------------------------
 *   2 | Size in 512B blocks (byte) |
 *     ------------------------------
 *   3 | Start of executable code   |
 *     |          .......           |
 * end |                            |
 *     ------------------------------
 */

typedef struct rom_hdr_tag {
    uint16_t    signature;
    uint8_t     num_blks;
    uint8_t     code;
} rom_hdr;


/* Calculate the checksum of a ROM. Note that the ROM might be
 * larger than 64K.
 */
static inline uint8_t rom_checksum(uint8_t __far *rom, uint8_t blocks)
{
    uint8_t     sum = 0;

#ifdef CHECKSUM_ROMS
    while (blocks--) {
        int     i;

        for (i = 0; i < 512; ++i)
            sum += rom[i];
        /* Add 512 bytes (32 paragraphs) to segment. */
        rom = MK_FP(FP_SEG(rom) + (512 >> 4), 0);
    }
#endif
    return sum;
}

/* Scan for ROMs in the given range and execute their POST code. */
void rom_scan(uint16_t start_seg, uint16_t end_seg)
{
    rom_hdr __far   *rom;
    uint8_t         rom_blks;

    DPRINT("Scanning for ROMs in %04X-%04X range\n", start_seg, end_seg);

    while (start_seg < end_seg) {
        rom = MK_FP(start_seg, 0);
        /* Check for the ROM signature. */
        if (rom->signature == 0xAA55) {
            DPRINT("Found ROM at segment %04X\n", start_seg);
            if (!rom_checksum((void __far *)rom, rom->num_blks)) {
                void (__far * rom_init)(void);

                /* Checksum good, initialize ROM. */
                rom_init = (void __far *)&rom->code;
                rom_init();
                int_disable();
                DPRINT("ROM initialized\n");

                /* Continue scanning past the end of this ROM. */
                rom_blks   = (rom->num_blks + 3) & ~3;  /* 4 blocks = 2K */
                start_seg += rom_blks / 4;
            }
        } else {
            /* Scanning is done in 2K steps. */
            start_seg += 2048 >> 4;
        }
    }
}

#if VBOX_BIOS_CPU >= 80386

/* NB: The CPUID detection is generic but currently not used elsewhere. */

/* Check CPUID availability. */
int is_cpuid_supported( void )
{
    uint32_t    old_flags, new_flags;

    old_flags = eflags_read();
    new_flags = old_flags ^ (1L << 21); /* Toggle CPUID bit. */
    eflags_write( new_flags );
    new_flags = eflags_read();
    return( old_flags != new_flags );   /* Supported if bit changed. */
}

#define APICMODE_DISABLED   0
#define APICMODE_APIC       1
#define APICMODE_X2APIC     2

#define APIC_BASE_MSR       0x1B
#define APICBASE_X2APIC     0x400   /* bit 10 */
#define APICBASE_DISABLE    0x800   /* bit 11 */

/*
 * Set up APIC/x2APIC. See also DevPcBios.cpp.
 *
 * NB: Virtual wire compatibility is set up earlier in 32-bit protected
 * mode assembler (because it needs to access MMIO just under 4GB).
 * Switching to x2APIC mode or disabling the APIC is done through an MSR
 * and needs no 32-bit addressing. Going to x2APIC mode does not lose the
 * existing virtual wire setup.
 *
 * NB: This code does not assume that there is a local APIC. It is necessary
 * to check CPUID whether APIC is present; the CPUID instruction might not be
 * available either.
 *
 * NB: Destroys high bits of 32-bit registers.
 */
void BIOSCALL apic_setup(void)
{
    uint64_t    base_msr;
    uint16_t    mask;
    uint8_t     apic_mode;
    uint32_t    cpu_id[4];

    /* If there's no CPUID, there's certainly no APIC. */
    if (!is_cpuid_supported()) {
        return;
    }

    /* Check EDX bit 9 */
    cpuid(&cpu_id, 1);
    BX_DEBUG("CPUID EDX: 0x%lx\n", cpu_id[3]);
    if ((cpu_id[3] & (1 << 9)) == 0) {
        return; /* No local APIC, nothing to do. */
    }

    /* APIC mode at offset 78h in CMOS NVRAM. */
    apic_mode = inb_cmos(0x78);

    if (apic_mode == APICMODE_X2APIC)
        mask = APICBASE_X2APIC;
    else if (apic_mode == APICMODE_DISABLED)
        mask = APICBASE_DISABLE;
    else
        mask = 0;   /* Any other setting leaves things alone. */

    if (mask) {
        base_msr = msr_read(APIC_BASE_MSR);
        base_msr |= mask;
        msr_write(base_msr, APIC_BASE_MSR);
    }
}

#endif
