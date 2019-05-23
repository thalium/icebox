/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 *  ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
 *
 *  Copyright (C) 2002  MandrakeSoft S.A.
 *
 *    MandrakeSoft S.A.
 *    43, rue d'Aboukir
 *    75002 Paris - France
 *    http://www.linux-mandrake.com/
 *    http://www.mandrakesoft.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */


#include <stdint.h>
#include "biosint.h"
#include "inlines.h"

#if DEBUG_INT15
#  define BX_DEBUG_INT15(...) BX_DEBUG(__VA_ARGS__)
#else
#  define BX_DEBUG_INT15(...)
#endif


#define UNSUPPORTED_FUNCTION    0x86    /* Specific to INT 15h. */

#define BIOS_CONFIG_TABLE       0xe6f5  /** @todo configurable? put elsewhere? */

#define ACPI_DATA_SIZE    0x00010000L   /** @todo configurable? put elsewhere? */

#define BX_CPU                  3

extern  int pmode_IDT;
extern  int rmode_IDT;

uint16_t read_ss(void);
#pragma aux read_ss = "mov ax, ss" modify exact [ax] nomemory;

#if VBOX_BIOS_CPU >= 80386

/* The 386+ code uses CR0 to switch to/from protected mode.
 * Quite straightforward.
 */

void pm_stack_save(uint16_t cx, uint16_t es, uint16_t si, uint16_t frame);
#pragma aux pm_stack_save =     \
    ".386"                      \
    "push   ds"                 \
    "push   eax"                \
    "xor    eax, eax"           \
    "mov    ds, ax"             \
    "mov    ds:[467h], sp"      \
    "mov    ds:[469h], ss"      \
    parm [cx] [es] [si] [ax] modify nomemory;

/* Uses position independent code... because it was too hard to figure
 * out how to code the far call in inline assembler.
 */
void pm_enter(void);
#pragma aux pm_enter =              \
    ".386p"                         \
    "call   pentry"                 \
    "pentry:"                       \
    "pop    di"                     \
    "add    di, 1Bh"                \
    "push   20h"                    \
    "push   di"                     \
    "lgdt   fword ptr es:[si+8]"    \
    "lidt   fword ptr cs:pmode_IDT" \
    "mov    eax, cr0"               \
    "or     al, 1"                  \
    "mov    cr0, eax"               \
    "retf"                          \
    "pm_pm:"                        \
    "mov    ax, 28h"                \
    "mov    ss, ax"                 \
    "mov    ax, 10h"                \
    "mov    ds, ax"                 \
    "mov    ax, 18h"                \
    "mov    es, ax"                 \
    modify nomemory;

/* Restore segment limits to real mode compatible values and
 * return to real mode.
 */
void pm_exit(void);
#pragma aux pm_exit =               \
    ".386p"                         \
    "call   pexit"                  \
    "pexit:"                        \
    "pop    ax"                     \
    "push   0F000h"                 \
    "add    ax, 18h"                \
    "push   ax"                     \
    "mov    ax, 28h"                \
    "mov    ds, ax"                 \
    "mov    es, ax"                 \
    "mov    eax, cr0"               \
    "and    al, 0FEh"               \
    "mov    cr0, eax"               \
    "retf"                          \
    "real_mode:"                    \
    "lidt   fword ptr cs:rmode_IDT" \
    modify nomemory;

/* Restore stack and reload segment registers in real mode to ensure
 * real mode compatible selector+base.
 */
void pm_stack_restore(void);
#pragma aux pm_stack_restore =  \
    ".386"                      \
    "xor    ax, ax"             \
    "mov    ds, ax"             \
    "mov    es, ax"             \
    "lss    sp, ds:[467h]"      \
    "pop    eax"                \
    "pop    ds"                 \
    modify nomemory;

#elif VBOX_BIOS_CPU >= 80286

/* The 286 code uses LMSW to switch to protected mode but it has to reset
 * the CPU to get back to real mode. Ugly! See return_blkmove in orgs.asm
 * for the other matching half.
 */
void pm_stack_save(uint16_t cx, uint16_t es, uint16_t si, uint16_t frame);
#pragma aux pm_stack_save =     \
    "xor    ax, ax"             \
    "mov    ds, ax"             \
    "mov    ds:[467h], bx"      \
    "mov    ds:[469h], ss"      \
    parm [cx] [es] [si] [bx] modify nomemory;

/* Uses position independent code... because it was too hard to figure
 * out how to code the far call in inline assembler.
 * NB: Trashes MSW bits but the CPU will be reset anyway.
 */
void pm_enter(void);
#pragma aux pm_enter =              \
    ".286p"                         \
    "call   pentry"                 \
    "pentry:"                       \
    "pop    di"                     \
    "add    di, 18h"                \
    "push   20h"                    \
    "push   di"                     \
    "lgdt   fword ptr es:[si+8]"    \
    "lidt   fword ptr cs:pmode_IDT" \
    "or     al, 1"                  \
    "lmsw   ax"                     \
    "retf"                          \
    "pm_pm:"                        \
    "mov    ax, 28h"                \
    "mov    ss, ax"                 \
    "mov    ax, 10h"                \
    "mov    ds, ax"                 \
    "mov    ax, 18h"                \
    "mov    es, ax"                 \
    modify nomemory;

/* Set up shutdown status and reset the CPU. The POST code
 * will regain control. Port 80h is written with status.
 * Code 9 is written to CMOS shutdown status byte (0Fh).
 * CPU is triple faulted.                                                    .
 */
void pm_exit(void);
#pragma aux pm_exit =               \
    "xor    ax, ax"                 \
    "out    80h, al"                \
    "mov    al, 0Fh"                \
    "out    70h, al"                \
    "mov    al, 09h"                \
    "out    71h, al"                \
    ".286p"                         \
    "lidt   fword ptr cs:pmode_IDT" \
    "int    3"                      \
    modify nomemory;

/* Dummy. Actually done in return_blkmove. */
void pm_stack_restore(void);
#pragma aux pm_stack_restore =  \
    "rm_return:"                \
    modify nomemory;

#endif

void pm_copy(void);
#pragma aux pm_copy =               \
    "xor    si, si"                 \
    "xor    di, di"                 \
    "cld"                           \
    "rep    movsw"                  \
    modify nomemory;

/* The pm_switch has a few crucial differences from pm_enter, hence
 * it is replicated here. Uses LMSW to avoid trashing high word of eax.
 */
void pm_switch(uint16_t reg_si);
#pragma aux pm_switch =             \
    ".286p"                         \
    "call   pentry"                 \
    "pentry:"                       \
    "pop    di"                     \
    "add    di, 18h"                \
    "push   38h"                    \
    "push   di"                     \
    "lgdt   fword ptr es:[si+08h]"  \
    "lidt   fword ptr es:[si+10h]"  \
    "mov    ax, 1"                  \
    "lmsw   ax"                     \
    "retf"                          \
    "pm_pm:"                        \
    "mov    ax, 28h"                \
    "mov    ss, ax"                 \
    "mov    ax, 18h"                \
    "mov    ds, ax"                 \
    "mov    ax, 20h"                \
    "mov    es, ax"                 \
    parm [si] modify nomemory;

/* Return to caller - we do not use IRET because we should not enable
 * interrupts. Note that AH must be zero on exit.
 * WARNING: Needs to be adapted if calling sequence is modified!
 */
void pm_unwind(uint16_t args);
#pragma aux pm_unwind =     \
    ".286"                  \
    "mov    sp, ax"         \
    "popa"                  \
    "add    sp, 6"          \
    "pop    cx"             \
    "pop    ax"             \
    "pop    ax"             \
    "mov    ax, 30h"        \
    "push   ax"             \
    "push   cx"             \
    "retf"                  \
    parm [ax] modify nomemory aborts;

/// @todo This method is silly. The RTC should be programmed to fire an interrupt
// instead of hogging the CPU with inaccurate code.
void timer_wait(uint32_t usec_wait)
{
    uint32_t    cycles;
    uint8_t     old_val;
    uint8_t     cur_val;

    /* We wait in 15 usec increments. */
    cycles = usec_wait / 15;

    old_val = inp(0x61) & 0x10;
    while (cycles--) {
        /* Wait 15us. */
        do {
            cur_val = inp(0x61) & 0x10;
        } while (cur_val != old_val);
        old_val = cur_val;
    }
}

bx_bool set_enable_a20(bx_bool val)
{
    uint8_t     oldval;

    // Use PS/2 System Control port A to set A20 enable

    // get current setting first
    oldval = inb(0x92);

    // change A20 status
    if (val)
        outb(0x92, oldval | 0x02);
    else
        outb(0x92, oldval & 0xfd);

    return((oldval & 0x02) != 0);
}

typedef struct {
    uint32_t    start;
    uint32_t    xstart;
    uint32_t    len;
    uint32_t    xlen;
    uint32_t    type;
} mem_range_t;

void set_e820_range(uint16_t ES, uint16_t DI, uint32_t start, uint32_t end,
                    uint8_t extra_start, uint8_t extra_end, uint16_t type)
{
    mem_range_t __far   *range;

    range = ES :> (mem_range_t *)DI;
    range->start  = start;
    range->xstart = extra_start;
    end -= start;
    extra_end -= extra_start;
    range->len    = end;
    range->xlen   = extra_end;
    range->type   = type;
}

/// @todo move elsewhere?
#define AX      r.gr.u.r16.ax
#define BX      r.gr.u.r16.bx
#define CX      r.gr.u.r16.cx
#define DX      r.gr.u.r16.dx
#define SI      r.gr.u.r16.si
#define DI      r.gr.u.r16.di
#define BP      r.gr.u.r16.bp
#define SP      r.gr.u.r16.sp
#define FLAGS   r.fl.u.r16.flags
#define EAX     r.gr.u.r32.eax
#define EBX     r.gr.u.r32.ebx
#define ECX     r.gr.u.r32.ecx
#define EDX     r.gr.u.r32.edx
#define ESI     r.gr.u.r32.esi
#define EDI     r.gr.u.r32.edi
#define ES      r.es


void BIOSCALL int15_function(sys_regs_t r)
{
    uint16_t    bRegister;
    uint8_t     irqDisable;

    BX_DEBUG_INT15("int15 AX=%04x\n",AX);

    switch (GET_AH()) {
    case 0x00: /* assorted functions */
        if (GET_AL() != 0xc0)
            goto undecoded;
        /* GRUB calls int15 with ax=0x00c0 to get the ROM configuration table,
        * which we don't support, but logging that event is annoying. In fact
        * it is likely that they just misread some specs, because there is a
        * int15 BIOS function AH=0xc0 which sounds quite similar to what GRUB
        * wants to achieve. */
        SET_CF();
        SET_AH(UNSUPPORTED_FUNCTION);
        break;
    case 0x24: /* A20 Control */
        switch (GET_AL()) {
        case 0x00:
            set_enable_a20(0);
            CLEAR_CF();
            SET_AH(0);
            break;
        case 0x01:
            set_enable_a20(1);
            CLEAR_CF();
            SET_AH(0);
            break;
        case 0x02:
            SET_AL( (inb(0x92) >> 1) & 0x01 );
            CLEAR_CF();
            SET_AH(0);
            break;
        case 0x03:
            CLEAR_CF();
            SET_AH(0);
            BX = 3;
            break;
        default:
            BX_INFO("int15: Func 24h, subfunc %02xh, A20 gate control not supported\n", (unsigned) GET_AL());
            SET_CF();
            SET_AH(UNSUPPORTED_FUNCTION);
        }
        break;

    case 0x41:
        SET_CF();
        SET_AH(UNSUPPORTED_FUNCTION);
        break;

    /// @todo Why does this need special handling? All we need is to set CF
    //       but not handle this as an unknown function (regardless of CPU type).
    case 0x4f:
        /* keyboard intercept */
#if BX_CPU < 2
        SET_AH(UNSUPPORTED_FUNCTION);
#else
        // nop
#endif
        SET_CF();
        break;

    case 0x52:    // removable media eject
        CLEAR_CF();
        SET_AH(0);  // "ok ejection may proceed"
        break;

    case 0x83: {
        if( GET_AL() == 0 ) {
            // Set Interval requested.
            if( ( read_byte( 0x40, 0xA0 ) & 1 ) == 0 ) {
                // Interval not already set.
                write_byte( 0x40, 0xA0, 1 );  // Set status byte.
                write_word( 0x40, 0x98, ES ); // Byte location, segment
                write_word( 0x40, 0x9A, BX ); // Byte location, offset
                write_word( 0x40, 0x9C, DX ); // Low word, delay
                write_word( 0x40, 0x9E, CX ); // High word, delay.
                CLEAR_CF( );
                irqDisable = inb( 0xA1 );
                outb( 0xA1, irqDisable & 0xFE );
                bRegister = inb_cmos( 0xB );  // Unmask IRQ8 so INT70 will get through.
                outb_cmos( 0xB, bRegister | 0x40 ); // Turn on the Periodic Interrupt timer
            } else {
                // Interval already set.
                BX_DEBUG_INT15("int15: Func 83h, failed, already waiting.\n" );
                SET_CF();
                SET_AH(UNSUPPORTED_FUNCTION);
            }
        } else if( GET_AL() == 1 ) {
            // Clear Interval requested
            write_byte( 0x40, 0xA0, 0 );  // Clear status byte
            CLEAR_CF( );
            bRegister = inb_cmos( 0xB );
            outb_cmos( 0xB, bRegister & ~0x40 );  // Turn off the Periodic Interrupt timer
        } else {
            BX_DEBUG_INT15("int15: Func 83h, failed.\n" );
            SET_CF();
            SET_AH(UNSUPPORTED_FUNCTION);
            SET_AL(GET_AL() - 1);
        }

        break;
        }

    case 0x88:
        // Get the amount of extended memory (above 1M)
#if BX_CPU < 2
        SET_AH(UNSUPPORTED_FUNCTION);
        SET_CF();
#else
        AX = (inb_cmos(0x31) << 8) | inb_cmos(0x30);

        // According to Ralf Brown's interrupt the limit should be 15M,
        // but real machines mostly return max. 63M.
        if(AX > 0xffc0)
            AX = 0xffc0;

        CLEAR_CF();
#endif
        break;

    case 0x89:
        // Switch to Protected Mode.
        // ES:DI points to user-supplied GDT
        // BH/BL contains starting interrupt numbers for PIC0/PIC1
        // This subfunction does not return!

        // turn off interrupts
        int_disable();  /// @todo aren't they off already?

        set_enable_a20(1); // enable A20 line; we're supposed to fail if that fails

        // Initialize CS descriptor for BIOS
        write_word(ES, SI+0x38+0, 0xffff);// limit 15:00 = normal 64K limit
        write_word(ES, SI+0x38+2, 0x0000);// base 15:00
        write_byte(ES, SI+0x38+4, 0x000f);// base 23:16 (hardcoded to f000:0000)
        write_byte(ES, SI+0x38+5, 0x9b);  // access
        write_word(ES, SI+0x38+6, 0x0000);// base 31:24/reserved/limit 19:16

        /* Reprogram the PICs. */
        outb(PIC_MASTER, PIC_CMD_INIT);
        outb(PIC_SLAVE,  PIC_CMD_INIT);
        outb(PIC_MASTER + 1, GET_BH());
        outb(PIC_SLAVE + 1,  GET_BL());
        outb(PIC_MASTER + 1, 4);
        outb(PIC_SLAVE + 1,  2);
        outb(PIC_MASTER + 1, 1);
        outb(PIC_SLAVE + 1,  1);
        /* Mask all IRQs, user must re-enable. */
        outb(PIC_MASTER_MASK, 0xff);
        outb(PIC_SLAVE_MASK, 0xff);

        pm_switch(SI);
        pm_unwind((uint16_t)&r);

        break;

    case 0x90:
        /* Device busy interrupt.  Called by Int 16h when no key available */
        break;

    case 0x91:
        /* Interrupt complete.  Called by Int 16h when key becomes available */
        break;

    case 0xbf:
        BX_INFO("*** int 15h function AH=bf not yet supported!\n");
        SET_CF();
        SET_AH(UNSUPPORTED_FUNCTION);
        break;

    case 0xC0:
        CLEAR_CF();
        SET_AH(0);
        BX = BIOS_CONFIG_TABLE;
        ES = 0xF000;
        break;

    case 0xc1:
        ES = read_word(0x0040, 0x000E);
        CLEAR_CF();
        break;

    case 0xd8:
        bios_printf(BIOS_PRINTF_DEBUG, "EISA BIOS not present\n");
        SET_CF();
        SET_AH(UNSUPPORTED_FUNCTION);
        break;

    /* Make the BIOS warning for pretty much every Linux kernel start
    * disappear - it calls with ax=0xe980 to figure out SMI info. */
    case 0xe9: /* SMI functions (SpeedStep and similar things) */
        SET_CF();
        SET_AH(UNSUPPORTED_FUNCTION);
        break;
    case 0xec: /* AMD64 target operating mode callback */
        if (GET_AL() != 0)
            goto undecoded;
        SET_AH(0);
        if (GET_BL() >= 1 && GET_BL() <= 3)
            CLEAR_CF();   /* Accepted value. */
        else
            SET_CF();     /* Reserved, error. */
        break;
undecoded:
    default:
        BX_INFO("*** int 15h function AX=%04x, BX=%04x not yet supported!\n",
                (unsigned) AX, (unsigned) BX);
        SET_CF();
        SET_AH(UNSUPPORTED_FUNCTION);
        break;
    }
}

void BIOSCALL int15_function32(sys32_regs_t r)
{
    uint32_t    extended_memory_size=0; // 64bits long
    uint32_t    extra_lowbits_memory_size=0;
    uint8_t     extra_highbits_memory_size=0;
    uint32_t    mcfgStart, mcfgSize;

    BX_DEBUG_INT15("int15 AX=%04x\n",AX);

    switch (GET_AH()) {
    case 0x86:
        // Wait for CX:DX microseconds. currently using the
        // refresh request port 0x61 bit4, toggling every 15usec
        int_enable();
        timer_wait(((uint32_t)CX << 16) | DX);
        break;

    case 0xd0:
        if (GET_AL() != 0x4f)
            goto int15_unimplemented;
        if (EBX == 0x50524f43 && ECX == 0x4d4f4445 && ESI == 0 && EDI == 0)
        {
            CLEAR_CF();
            ESI = EBX;
            EDI = ECX;
            EAX = 0x49413332;
        }
        else
            goto int15_unimplemented;
        break;

    case 0xe8:
        switch(GET_AL()) {
        case 0x20: // coded by osmaker aka K.J.
            if(EDX == 0x534D4150) {
                extended_memory_size = inb_cmos(0x35);
                extended_memory_size <<= 8;
                extended_memory_size |= inb_cmos(0x34);
                extended_memory_size *= 64;
#ifndef VBOX /* The following excludes 0xf0000000 thru 0xffffffff. Trust DevPcBios.cpp to get this right. */
                // greater than EFF00000???
                if(extended_memory_size > 0x3bc000) {
                    extended_memory_size = 0x3bc000; // everything after this is reserved memory until we get to 0x100000000
                }
#endif /* !VBOX */
                extended_memory_size *= 1024;
                extended_memory_size += (16L * 1024 * 1024);

                if(extended_memory_size <= (16L * 1024 * 1024)) {
                    extended_memory_size = inb_cmos(0x31);
                    extended_memory_size <<= 8;
                    extended_memory_size |= inb_cmos(0x30);
                    extended_memory_size *= 1024;
                    extended_memory_size += (1L * 1024 * 1024);
                }

#ifdef VBOX     /* We've already used the CMOS entries for SATA.
                   BTW. This is the amount of memory above 4GB measured in 64KB units. */
                extra_lowbits_memory_size = inb_cmos(0x62);
                extra_lowbits_memory_size <<= 8;
                extra_lowbits_memory_size |= inb_cmos(0x61);
                extra_lowbits_memory_size <<= 16;
                extra_highbits_memory_size = inb_cmos(0x63);
                /* 0x64 and 0x65 can be used if we need to dig 1 TB or more at a later point. */
#else
                extra_lowbits_memory_size = inb_cmos(0x5c);
                extra_lowbits_memory_size <<= 8;
                extra_lowbits_memory_size |= inb_cmos(0x5b);
                extra_lowbits_memory_size *= 64;
                extra_lowbits_memory_size *= 1024;
                extra_highbits_memory_size = inb_cmos(0x5d);
#endif /* !VBOX */

                mcfgStart = 0;
                mcfgSize  = 0;

                switch(BX)
                {
                    case 0:
                        set_e820_range(ES, DI,
#ifndef VBOX /** @todo Upstream suggests the following, needs checking. (see next as well) */
                                       0x0000000L, 0x0009f000L, 0, 0, 1);
#else
                                       0x0000000L, 0x0009fc00L, 0, 0, 1);
#endif
                        EBX = 1;
                        break;
                    case 1:
                        set_e820_range(ES, DI,
#ifndef VBOX /** @todo Upstream suggests the following, needs checking. (see next as well) */
                                       0x0009f000L, 0x000a0000L, 0, 0, 2);
#else
                                       0x0009fc00L, 0x000a0000L, 0, 0, 2);
#endif
                        EBX = 2;
                        break;
                    case 2:
#ifdef VBOX
                        /* Mark the BIOS as reserved. VBox doesn't currently
                         * use the 0xe0000-0xeffff area. It does use the
                         * 0xd0000-0xdffff area for the BIOS logo, but it's
                         * not worth marking it as reserved. (this is not
                         * true anymore because the VGA adapter handles the logo stuff)
                         * The whole 0xe0000-0xfffff can be used for the BIOS.
                         * Note that various
                         * Windows versions don't accept (read: in debug builds
                         * they trigger the "Too many similar traps" assertion)
                         * a single reserved range from 0xd0000 to 0xffffff.
                         * A 128K area starting from 0xd0000 works. */
                        set_e820_range(ES, DI,
                                       0x000f0000L, 0x00100000L, 0, 0, 2);
#else /* !VBOX */
                        set_e820_range(ES, DI,
                                       0x000e8000L, 0x00100000L, 0, 0, 2);
#endif /* !VBOX */
                        EBX = 3;
                        break;
                    case 3:
#if BX_ROMBIOS32 || defined(VBOX)
                        set_e820_range(ES, DI,
                                       0x00100000L,
                                       extended_memory_size - ACPI_DATA_SIZE, 0, 0, 1);
                        EBX = 4;
#else
                        set_e820_range(ES, DI,
                                       0x00100000L,
                                       extended_memory_size, 1);
                        EBX = 5;
#endif
                        break;
                    case 4:
                        set_e820_range(ES, DI,
                                       extended_memory_size - ACPI_DATA_SIZE,
                                       extended_memory_size, 0, 0, 3); // ACPI RAM
                        EBX = 5;
                        break;
                    case 5:
                        set_e820_range(ES, DI,
                                       0xfec00000,
                                       0xfec00000 + 0x1000, 0, 0, 2); // I/O APIC
                        EBX = 6;
                        break;
                    case 6:
                        set_e820_range(ES, DI,
                                       0xfee00000,
                                       0xfee00000 + 0x1000, 0, 0, 2); // Local APIC
                        EBX = 7;
                        break;
                    case 7:
                        /* 256KB BIOS area at the end of 4 GB */
#ifdef VBOX
                        /* We don't set the end to 1GB here and rely on the 32-bit
                           unsigned wrap around effect (0-0xfffc0000L). */
#endif
                        set_e820_range(ES, DI,
                                       0xfffc0000L, 0x00000000L, 0, 0, 2);
                        if (mcfgStart != 0)
                            EBX = 8;
                        else
                        {
                            if (extra_highbits_memory_size || extra_lowbits_memory_size)
                                EBX = 9;
                            else
                                EBX = 0;
                        }
                        break;
                     case 8:
                        /* PCI MMIO config space (MCFG) */
                        set_e820_range(ES, DI,
                                       mcfgStart, mcfgStart + mcfgSize, 0, 0, 2);

                        if (extra_highbits_memory_size || extra_lowbits_memory_size)
                            EBX = 9;
                        else
                            EBX = 0;
                        break;
                    case 9:
#ifdef VBOX /* Don't succeeded if no memory above 4 GB.  */
                        /* Mapping of memory above 4 GB if present.
                           Note1: set_e820_range needs do no borrowing in the
                                  subtraction because of the nice numbers.
                           Note2* works only up to 1TB because of uint8_t for
                                  the upper bits!*/
                        if (extra_highbits_memory_size || extra_lowbits_memory_size)
                        {
                            set_e820_range(ES, DI,
                                           0x00000000L, extra_lowbits_memory_size,
                                           1 /*x4GB*/, extra_highbits_memory_size + 1 /*x4GB*/, 1);
                            EBX = 0;
                        }
                        break;
                        /* fall thru */
#else  /* !VBOX */
                        /* Mapping of memory above 4 GB */
                        set_e820_range(ES, DI, 0x00000000L,
                        extra_lowbits_memory_size, 1, extra_highbits_memory_size
                                       + 1, 1);
                        EBX = 0;
                        break;
#endif /* !VBOX */
                    default:  /* AX=E820, DX=534D4150, BX unrecognized */
                        goto int15_unimplemented;
                        break;
                }
                EAX = 0x534D4150;
                ECX = 0x14;
                CLEAR_CF();
            } else {
                // if DX != 0x534D4150)
                goto int15_unimplemented;
            }
            break;

        case 0x01:
            // do we have any reason to fail here ?
            CLEAR_CF();

            // my real system sets ax and bx to 0
            // this is confirmed by Ralph Brown list
            // but syslinux v1.48 is known to behave
            // strangely if ax is set to 0
            // regs.u.r16.ax = 0;
            // regs.u.r16.bx = 0;

            // Get the amount of extended memory (above 1M)
            CX = (inb_cmos(0x31) << 8) | inb_cmos(0x30);

            // limit to 15M
            if(CX > 0x3c00)
                CX = 0x3c00;

            // Get the amount of extended memory above 16M in 64k blocks
            DX = (inb_cmos(0x35) << 8) | inb_cmos(0x34);

            // Set configured memory equal to extended memory
            AX = CX;
            BX = DX;
            break;
        default:  /* AH=0xE8?? but not implemented */
            goto int15_unimplemented;
        }
        break;
    int15_unimplemented:
       // fall into the default case
    default:
        BX_INFO("*** int 15h function AX=%04x, BX=%04x not yet supported!\n",
                (unsigned) AX, (unsigned) BX);
        SET_CF();
        SET_AL(UNSUPPORTED_FUNCTION);
        break;
    }
}

#if VBOX_BIOS_CPU >= 80286

#undef  FLAGS
#define FLAGS   r.ra.flags.u.r16.flags

/* Function 0x87 handled separately due to specific stack layout requirements. */
void BIOSCALL int15_blkmove(disk_regs_t r)
{
    bx_bool     prev_a20_enable;
    uint16_t    base15_00;
    uint8_t     base23_16;
    uint16_t    ss;

    // +++ should probably have descriptor checks
    // +++ should have exception handlers

    // turn off interrupts
    int_disable();    /// @todo aren't they disabled already?

    prev_a20_enable = set_enable_a20(1); // enable A20 line

    // 128K max of transfer on 386+ ???
    // source == destination ???

    // ES:SI points to descriptor table
    // offset   use     initially  comments
    // ==============================================
    // 00..07   Unused  zeros      Null descriptor
    // 08..0f   GDT     zeros      filled in by BIOS
    // 10..17   source  ssssssss   source of data
    // 18..1f   dest    dddddddd   destination of data
    // 20..27   CS      zeros      filled in by BIOS
    // 28..2f   SS      zeros      filled in by BIOS

    //es:si
    //eeee0
    //0ssss
    //-----

    // check for access rights of source & dest here

    // Initialize GDT descriptor
    base15_00 = (ES << 4) + SI;
    base23_16 = ES >> 12;
    if (base15_00 < (ES<<4))
        base23_16++;
    write_word(ES, SI+0x08+0, 47);       // limit 15:00 = 6 * 8bytes/descriptor
    write_word(ES, SI+0x08+2, base15_00);// base 15:00
    write_byte(ES, SI+0x08+4, base23_16);// base 23:16
    write_byte(ES, SI+0x08+5, 0x93);     // access
    write_word(ES, SI+0x08+6, 0x0000);   // base 31:24/reserved/limit 19:16

    // Initialize CS descriptor
    write_word(ES, SI+0x20+0, 0xffff);// limit 15:00 = normal 64K limit
    write_word(ES, SI+0x20+2, 0x0000);// base 15:00
    write_byte(ES, SI+0x20+4, 0x000f);// base 23:16
    write_byte(ES, SI+0x20+5, 0x9b);  // access
    write_word(ES, SI+0x20+6, 0x0000);// base 31:24/reserved/limit 19:16

    // Initialize SS descriptor
    ss = read_ss();
    base15_00 = ss << 4;
    base23_16 = ss >> 12;
    write_word(ES, SI+0x28+0, 0xffff);   // limit 15:00 = normal 64K limit
    write_word(ES, SI+0x28+2, base15_00);// base 15:00
    write_byte(ES, SI+0x28+4, base23_16);// base 23:16
    write_byte(ES, SI+0x28+5, 0x93);     // access
    write_word(ES, SI+0x28+6, 0x0000);   // base 31:24/reserved/limit 19:16

    pm_stack_save(CX, ES, SI, FP_OFF(&r));
    pm_enter();
    pm_copy();
    pm_exit();
    pm_stack_restore();

    set_enable_a20(prev_a20_enable);

    // turn interrupts back on
    int_enable();

    SET_AH(0);
    CLEAR_CF();
}
#endif
