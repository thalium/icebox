/* $Id: DevEEPROM.cpp $ */
/** @file
 * DevEEPROM - Microware-compatible 64x16-bit 93C46 EEPROM Emulation.
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
 */

#define LOG_GROUP LOG_GROUP_DEV_E1000   /// @todo Add a EEPROM logging group.
#include <VBox/log.h>
#include <VBox/vmm/pdmdev.h>
#include <iprt/string.h>
#include "DevEEPROM.h"

#define E1kLog(a)               Log(a)

/**
 * Initialize EEPROM device.
 *
 * @param   pu16Initial Initial EEPROM content (optional). The size of initial
 *                      content must be sizeof(uint16_t)*EEPROM93C46::SIZE
 *                      bytes.
 */
void EEPROM93C46::init(const uint16_t *pu16Initial)
{
    if ( pu16Initial ) {
        memcpy(this->m_au16Data, pu16Initial, sizeof(this->m_au16Data));
    } else {
        memset(this->m_au16Data, 0, sizeof(this->m_au16Data));
    }
    m_fWriteEnabled = false;
    m_u32InternalWires = 0;
    m_eState = STANDBY;
}

/**
 * Writes one word to specified location if write is enabled.
 *
 * @param   u32Addr     Address to write at
 * @param   u16Value    Value to store
 */
void EEPROM93C46::storeWord(uint32_t u32Addr, uint16_t u16Value)
{
    if (m_fWriteEnabled) {
        E1kLog(("EEPROM: Stored word %04x at %08x\n", u16Value, u32Addr));
        m_au16Data[u32Addr] = u16Value;
    }
    m_u16Mask = DATA_MSB;
}

/**
 * Reads one word at specified location.
 *
 * @returns True if read was successful.
 *
 * @param   u32Addr     Address to read from
 * @param   pu16Value   Placeholder to store the value
 */
bool EEPROM93C46::readWord(uint32_t u32Addr, uint16_t *pu16Value)
{
    if (u32Addr < SIZE)
    {
        *pu16Value = m_au16Data[u32Addr];
        return true;
    }

    return false;
}

/**
 * Fetch next word pointer by m_u16Addr.
 *
 * m_u16Addr is advanced and mask is reset to support sequential reads.
 *
 * @returns New state
 */
EEPROM93C46::State EEPROM93C46::opRead()
{
    m_u16Word = m_au16Data[m_u16Addr];
    E1kLog(("EEPROM: Reading word %04x at %08x\n", m_u16Word, m_u16Addr));
    m_u16Addr = (m_u16Addr + 1) & ADDR_MASK;
    m_u16Mask = DATA_MSB;
    return WRITING_DO;
}

/**
 * Write the value of m_u16Word to the location specified by m_u16Addr.
 *
 * @returns New state
 *
 * @remarks Need to wait for CS lower/raise to show busy/ready indication.
 */
EEPROM93C46::State EEPROM93C46::opWrite()
{
    storeWord(m_u16Addr, m_u16Word);
    return WAITING_CS_FALL;
}

/**
 * Overwrite the entire contents of EEPROM with the value of m_u16Word.
 *
 * @returns New state
 *
 * @remarks Need to wait for CS lower/raise to show busy/ready indication.
 */
EEPROM93C46::State EEPROM93C46::opWriteAll()
{
    for (int i = 0; i < SIZE; i++)
        storeWord(i, m_u16Word);
    return WAITING_CS_FALL;
}

/**
 * Decode opcode and address from 'opAddr' member.
 *
 * Decodes operation and executes it immediately if possible; otherwise, stores
 * the decoded operation and address.
 *
 * @returns New state
 */
EEPROM93C46::State EEPROM93C46::opDecode()
{
    switch (m_u16Word>>6) {
    case 3: /* ERASE */
        storeWord(m_u16Word & ADDR_MASK, 0xFFFF);
        return WAITING_CS_FALL;
    case 2: /* READ */
        m_eOp     = OP_READ;
        m_u16Addr = m_u16Word & ADDR_MASK;
        return opRead(); /* Load first word */
    case 1: /* WRITE */
        m_eOp     = OP_WRITE;
        m_u16Addr = m_u16Word & ADDR_MASK;
        m_u16Word = 0;
        m_u16Mask = DATA_MSB;
        return READING_DI;
    case 0:
        switch (m_u16Word>>4) {
        case 0: /* ERASE/WRITE DISABLE */
            m_fWriteEnabled = false;
            return STANDBY;
        case 1: /* WRITE ALL */
            m_eOp     = OP_WRITE_ALL;
            m_u16Word = 0;
            m_u16Mask = DATA_MSB;
            return READING_DI;
        case 2: /* ERASE ALL */
            /* Re-use opWriteAll */
            m_u16Word = 0xFFFF;
            return opWriteAll();
        case 3: /* ERASE/WRITE ENABLE */
            m_fWriteEnabled = true;
            return STANDBY;
        }
    }
    return m_eState;
}

/**
 * Set bits in EEPROM 4-wire interface.
 *
 * @param   u32Wires    Values of DI, CS, SK.
 * @remarks The value of DO bit in 'u32Wires' is ignored.
 */
void EEPROM93C46::write(uint32_t u32Wires)
{
    if (u32Wires & WIRES_CS) {
        if (!(m_u32InternalWires & WIRES_SK) && (u32Wires & WIRES_SK)) {
            /* Positive edge of clock */
            if (m_eState == STANDBY) {
                if (u32Wires & WIRES_DI) {
                    m_eState   = READING_DI;
                    m_eOp      = OP_DECODE;
                    m_u16Mask  = OPADDR_MSB;
                    m_u16Word  = 0;
                }
            }
            else {
                if (m_eState == READING_DI) {
                    if (u32Wires & WIRES_DI) {
                        m_u16Word |= m_u16Mask;
                    }
                }
                else if (m_eState == WRITING_DO) {
                    m_u32InternalWires &= ~WIRES_DO;
                    if (m_u16Word & m_u16Mask) {
                        m_u32InternalWires |= WIRES_DO;
                    }
                }
                else return;
                /* Next bit */
                m_u16Mask >>= 1;
                if (m_u16Mask == 0)
                {
                    switch (this->m_eOp)
                    {
                        case OP_READ:
                            m_eState = opRead();
                            break;
                        case OP_WRITE:
                            m_eState = opWrite();
                            break;
                        case OP_WRITE_ALL:
                            m_eState = opWriteAll();
                            break;
                        case OP_DECODE:
                            m_eState = opDecode();
                            break;
                        default:
                            ;
                    }
                }
            }
        }
        else if (m_eState == WAITING_CS_RISE) {
            m_u32InternalWires |= WIRES_DO; /* ready */
            m_eState = STANDBY;
        }
    }
    else {
        switch(m_eState) {
            case WAITING_CS_FALL:
                m_eState = WAITING_CS_RISE;
                m_u32InternalWires &= ~WIRES_DO; /* busy */
                break;
            case WAITING_CS_RISE:
                break;
            case READING_DI:
                m_u32InternalWires &= ~WIRES_DO; /* Clear ready/busy status from DO. */
                RT_FALL_THRU();
            default:
                m_eState = STANDBY;
                break;
        }
    }
    m_u32InternalWires &= WIRES_DO;
    m_u32InternalWires |= u32Wires & ~WIRES_DO; /* Do not overwrite DO */
}

/**
 * Read bits in EEPROM 4-wire interface.
 *
 * @returns Current values of DO, DI, CS, SK.
 *
 * @remarks Only DO is controlled by EEPROM, other bits are returned as they
 * were written by 'write'.
 */
uint32_t EEPROM93C46::read()
{
    return m_u32InternalWires;
}

void EEPROM93C46::save(PSSMHANDLE pSSM)
{
    SSMR3PutU8(  pSSM, EEPROM93C46_SAVEDSTATE_VERSION);
    SSMR3PutU8(  pSSM, m_eState);
    SSMR3PutU8(  pSSM, m_eOp);
    SSMR3PutBool(pSSM, m_fWriteEnabled);
    SSMR3PutU32( pSSM, m_u32InternalWires);
    SSMR3PutU16( pSSM, m_u16Word);
    SSMR3PutU16( pSSM, m_u16Mask);
    SSMR3PutU16( pSSM, m_u16Addr);
    SSMR3PutMem( pSSM, m_au16Data, sizeof(m_au16Data));
}

int EEPROM93C46::load(PSSMHANDLE pSSM)
{
    int rc;
    uint8_t uVersion;

    rc = SSMR3GetU8(pSSM, &uVersion);
    AssertRCReturn(rc, rc);
    if (uVersion != EEPROM93C46_SAVEDSTATE_VERSION)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;

    SSMR3GetU8(  pSSM, (uint8_t*)&m_eState);
    SSMR3GetU8(  pSSM, (uint8_t*)&m_eOp);
    SSMR3GetBool(pSSM, &m_fWriteEnabled);
    SSMR3GetU32( pSSM, &m_u32InternalWires);
    SSMR3GetU16( pSSM, &m_u16Word);
    SSMR3GetU16( pSSM, &m_u16Mask);
    SSMR3GetU16( pSSM, &m_u16Addr);
    return SSMR3GetMem( pSSM, m_au16Data, sizeof(m_au16Data));
}
