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
#include "inlines.h"
#include "biosint.h"
#ifndef VBOX_VERSION_STRING
#include <VBox/version.h>
#endif

static  const char  bios_cvs_version_string[] = "VirtualBox " VBOX_VERSION_STRING;

uint8_t read_byte(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint8_t *)offset) );
}

void write_byte(uint16_t seg, uint16_t offset, uint8_t data)
{
    *(seg:>(uint8_t *)offset) = data;
}

uint16_t read_word(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint16_t *)offset) );
}

void write_word(uint16_t seg, uint16_t offset, uint16_t data)
{
    *(seg:>(uint16_t *)offset) = data;
}

uint32_t read_dword(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint32_t *)offset) );
}

void write_dword(uint16_t seg, uint16_t offset, uint32_t data)
{
    *(seg:>(uint32_t *)offset) = data;
}

uint8_t inb_cmos(uint8_t cmos_reg)
{
    uint8_t     cmos_port = 0x70;

    if (cmos_reg >= 0x80)
        cmos_port += 2;
    outb(cmos_port, cmos_reg);
    return inb(cmos_port + 1);
}

void outb_cmos(uint8_t cmos_reg, uint8_t val)
{
    uint8_t     cmos_port = 0x70;

    if (cmos_reg >= 0x80)
        cmos_port += 2;
    outb(cmos_port, cmos_reg);
    outb(cmos_port + 1, val);
}

void BIOSCALL dummy_isr_function(pusha_regs_t regs, uint16_t es,
                                 uint16_t ds, iret_addr_t iret_addr)
{
    // Interrupt handler for unexpected hardware interrupts. We have to clear
    // the PIC because if we don't, the next EOI will clear the wrong interrupt
    // and all hell will break loose! This routine also masks the unexpected
    // interrupt so it will generally be called only once for each unexpected
    // interrupt level.
    uint8_t     isrA, isrB, imr, last_int = 0xFF;

    outb(PIC_MASTER, PIC_CMD_RD_ISR);           // Read master ISR
    isrA = inb(PIC_MASTER);
    if (isrA) {
        outb(PIC_SLAVE, PIC_CMD_RD_ISR);        // Read slave ISR
        isrB = inb(PIC_SLAVE);
        if (isrB) {
            imr = inb(PIC_SLAVE_MASK);
            outb(PIC_SLAVE_MASK, imr | isrB );  // Mask this interrupt
            outb(PIC_SLAVE, PIC_CMD_EOI);       // Send EOI on slave PIC
        } else {
            imr = inb(PIC_MASTER_MASK);
            isrA &= 0xFB;                       // Never mask the cascade interrupt
            outb(PIC_MASTER_MASK, imr | isrA);  // Mask this interrupt
        }
        outb(PIC_MASTER, PIC_CMD_EOI);          // Send EOI on master PIC
        last_int = isrA;
    }
    write_byte(0x40, 0x6B, last_int);           // Write INTR_FLAG
}


void BIOSCALL nmi_handler_msg(void)
{
    BX_PANIC("NMI Handler called\n");
}

void BIOSCALL int18_panic_msg(void)
{
    BX_PANIC("INT18: BOOT FAILURE\n");
}

void BIOSCALL log_bios_start(void)
{
#if BX_DEBUG_SERIAL
    outb(BX_DEBUG_PORT+UART_LCR, 0x03); /* setup for serial logging: 8N1 */
#endif
    BX_INFO("%s\n", bios_cvs_version_string);
}

/* Set video mode. */
void set_mode(uint8_t mode);
#pragma aux set_mode =      \
    "mov    ah, 0"          \
    "int    10h"            \
    parm [al] modify [ax];

/// @todo restore
//#undef VBOX

#define BX_PCIBIOS  1
#define BX_APPNAME          "VirtualBox"
#define BIOS_BUILD_DATE     __DATE__
//--------------------------------------------------------------------------
// print_bios_banner
//   displays a the bios version
//--------------------------------------------------------------------------
void BIOSCALL print_bios_banner(void)
{
#ifdef VBOX
    // Skip the logo if a warm boot is requested.
    uint16_t    warm_boot = read_word(0x0040,0x0072);
    write_word(0x0040,0x0072, 0);
    if (warm_boot == 0x1234)
    {
        /* Only set text mode. */
        set_mode(3);
        return;
    }
    /* show graphical logo */
    show_logo();
#else /* !VBOX */
    char        *bios_conf;

    /* Avoid using preprocessing directives within macro arguments. */
    bios_conf =
#ifdef __WATCOMC__
    "watcom "
#endif
#if BX_APM
    "apmbios "
#endif
#if BX_PCIBIOS
    "pcibios "
#endif
#if BX_ELTORITO_BOOT
    "eltorito "
#endif
#if BX_ROMBIOS32
    "rombios32 "
#endif
    "\n\n";

    printf(BX_APPNAME" BIOS - build: %s\n%s\nOptions: ",
           BIOS_BUILD_DATE, bios_cvs_version_string);
    printf(bios_conf, 0);
#endif /* VBOX */
}
