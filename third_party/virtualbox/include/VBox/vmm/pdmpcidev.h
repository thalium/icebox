/** @file
 * PCI - The PCI Controller And Devices. (DEV)
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

#ifndef ___VBox_vmm_pdmpcidev_h
#define ___VBox_vmm_pdmpcidev_h

#include <VBox/pci.h>
#include <iprt/assert.h>


/** @defgroup grp_pdm_pcidev       PDM PCI Device
 * @ingroup grp_pdm_device
 * @{
 */

/**
 * Callback function for reading from the PCI configuration space.
 *
 * @returns The register value.
 * @param   pDevIns         Pointer to the device instance the PCI device
 *                          belongs to.
 * @param   pPciDev         Pointer to PCI device. Use pPciDev->pDevIns to get the device instance.
 * @param   uAddress        The configuration space register address. [0..4096]
 * @param   cb              The register size. [1,2,4]
 *
 * @remarks Called with the PDM lock held.  The device lock is NOT take because
 *          that is very likely be a lock order violation.
 */
typedef DECLCALLBACK(uint32_t) FNPCICONFIGREAD(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t uAddress, unsigned cb);
/** Pointer to a FNPCICONFIGREAD() function. */
typedef FNPCICONFIGREAD *PFNPCICONFIGREAD;
/** Pointer to a PFNPCICONFIGREAD. */
typedef PFNPCICONFIGREAD *PPFNPCICONFIGREAD;

/**
 * Callback function for writing to the PCI configuration space.
 *
 * @param   pDevIns         Pointer to the device instance the PCI device
 *                          belongs to.
 * @param   pPciDev         Pointer to PCI device. Use pPciDev->pDevIns to get the device instance.
 * @param   uAddress        The configuration space register address. [0..4096]
 * @param   u32Value        The value that's being written. The number of bits actually used from
 *                          this value is determined by the cb parameter.
 * @param   cb              The register size. [1,2,4]
 *
 * @remarks Called with the PDM lock held.  The device lock is NOT take because
 *          that is very likely be a lock order violation.
 */
typedef DECLCALLBACK(void) FNPCICONFIGWRITE(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t uAddress, uint32_t u32Value, unsigned cb);
/** Pointer to a FNPCICONFIGWRITE() function. */
typedef FNPCICONFIGWRITE *PFNPCICONFIGWRITE;
/** Pointer to a PFNPCICONFIGWRITE. */
typedef PFNPCICONFIGWRITE *PPFNPCICONFIGWRITE;

/**
 * Callback function for mapping an PCI I/O region.
 *
 * @returns VBox status code.
 * @param   pDevIns         Pointer to the device instance the PCI device
 *                          belongs to.
 * @param   pPciDev         Pointer to the PCI device.
 * @param   iRegion         The region number.
 * @param   GCPhysAddress   Physical address of the region. If enmType is PCI_ADDRESS_SPACE_IO, this
 *                          is an I/O port, otherwise it's a physical address.
 *
 *                          NIL_RTGCPHYS indicates that a MMIO2 mapping is about to be unmapped and
 *                          that the device deregister access handlers for it and update its internal
 *                          state to reflect this.
 *
 * @param   cb              Size of the region in bytes.
 * @param   enmType         One of the PCI_ADDRESS_SPACE_* values.
 *
 * @remarks Called with the PDM lock held.  The device lock is NOT take because
 *          that is very likely be a lock order violation.
 */
typedef DECLCALLBACK(int) FNPCIIOREGIONMAP(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                           RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType);
/** Pointer to a FNPCIIOREGIONMAP() function. */
typedef FNPCIIOREGIONMAP *PFNPCIIOREGIONMAP;


/**
 * Sets the size and type for old saved states from within a
 * PDMPCIDEV::pfnRegionLoadChangeHookR3 callback.
 *
 * @returns VBox status code.
 * @param   pPciDev         Pointer to the PCI device.
 * @param   iRegion         The region number.
 * @param   cbRegion        The region size.
 * @param   enmType         Combination of the PCI_ADDRESS_SPACE_* values.
 */
typedef DECLCALLBACK(int) FNPCIIOREGIONOLDSETTER(PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS cbRegion, PCIADDRESSSPACE enmType);
/** Pointer to a FNPCIIOREGIONOLDSETTER() function. */
typedef FNPCIIOREGIONOLDSETTER *PFNPCIIOREGIONOLDSETTER;



/*
 * Hack to include the PDMPCIDEVINT structure at the right place
 * to avoid duplications of FNPCIIOREGIONMAP and such.
 */
#ifdef PDMPCIDEV_INCLUDE_PRIVATE
# include "pdmpcidevint.h"
#endif

/**
 * PDM PCI Device structure.
 *
 * A PCI device belongs to a PDM device.  A PDM device may have zero or more PCI
 * devices associated with it.  The first PCI device that it registers
 * automatically becomes the default PCI device and can be used implicitly
 * with the device helper APIs.  Subsequent PCI devices must be specified
 * explicitly to the device helper APIs when used.
 */
typedef struct PDMPCIDEV
{
    /** PCI config space. */
    uint8_t                 abConfig[256];

    /** Internal data. */
    union
    {
#ifdef PDMPCIDEVINT_DECLARED
        PDMPCIDEVINT        s;
#endif
        uint8_t             padding[HC_ARCH_BITS == 32 ? 288 : 384];
    } Int;

    /** @name Read only data.
     * @{
     */
    /** PCI device number [11:3] and function [2:0] on the pci bus.
     * @sa VBOX_PCI_DEVFN_MAKE, VBOX_PCI_DEVFN_FUN_MASK, VBOX_PCI_DEVFN_DEV_SHIFT */
    uint32_t                uDevFn;
    uint32_t                Alignment0; /**< Alignment. */
    /** Device name. */
    R3PTRTYPE(const char *) pszNameR3;
    /** @} */

    /**
     * Callback for dealing with size changes.
     *
     * This is set by the PCI device when needed.  It is only needed if any changes
     * in the PCI resources have been made that may be incompatible with saved state
     * (i.e. does not reflect configuration, but configuration defaults changed).
     *
     * The implementation can use PDMDevHlpMMIOExReduce to adjust the resource
     * allocation down in size.  There is currently no way of growing resources.
     * Dropping a resource is automatic.
     *
     * @returns VBox status code.
     * @param   pDevIns         Pointer to the device instance the PCI device
     *                          belongs to.
     * @param   pPciDev         Pointer to the PCI device.
     * @param   iRegion         The region number or UINT32_MAX if old saved state call.
     * @param   cbRegion        The size being loaded, RTGCPHYS_MAX if old saved state
     *                          call, or 0 for dummy 64-bit top half region.
     * @param   enmType         The type being loaded, -1 if old saved state call, or
     *                          0xff if dummy 64-bit top half region.
     * @param   pfnOldSetter    Callback for setting size and type for call
     *                          regarding old saved states.  NULL otherwise.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegionLoadChangeHookR3,(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                                         uint64_t cbRegion, PCIADDRESSSPACE enmType,
                                                         PFNPCIIOREGIONOLDSETTER pfnOldSetter));
} PDMPCIDEV;
#ifdef PDMPCIDEVINT_DECLARED
AssertCompile(RT_SIZEOFMEMB(PDMPCIDEV, Int.s) <= RT_SIZEOFMEMB(PDMPCIDEV, Int.padding));
#endif



/** @name PDM PCI config space accessor function.
 * @{
 */

/** @todo handle extended space access. */

DECLINLINE(void)     PDMPciDevSetByte(PPDMPCIDEV pPciDev, uint32_t offReg, uint8_t u8Value)
{
    Assert(offReg < sizeof(pPciDev->abConfig));
    pPciDev->abConfig[offReg] = u8Value;
}

DECLINLINE(uint8_t)  PDMPciDevGetByte(PPDMPCIDEV pPciDev, uint32_t offReg)
{
    Assert(offReg < sizeof(pPciDev->abConfig));
    return pPciDev->abConfig[offReg];
}

DECLINLINE(void)     PDMPciDevSetWord(PPDMPCIDEV pPciDev, uint32_t offReg, uint16_t u16Value)
{
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint16_t));
    *(uint16_t*)&pPciDev->abConfig[offReg] = RT_H2LE_U16(u16Value);
}

DECLINLINE(uint16_t) PDMPciDevGetWord(PPDMPCIDEV pPciDev, uint32_t offReg)
{
    uint16_t u16Value;
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint16_t));
    u16Value = *(uint16_t*)&pPciDev->abConfig[offReg];
    return RT_H2LE_U16(u16Value);
}

DECLINLINE(void)     PDMPciDevSetDWord(PPDMPCIDEV pPciDev, uint32_t offReg, uint32_t u32Value)
{
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint32_t));
    *(uint32_t*)&pPciDev->abConfig[offReg] = RT_H2LE_U32(u32Value);
}

DECLINLINE(uint32_t) PDMPciDevGetDWord(PPDMPCIDEV pPciDev, uint32_t offReg)
{
    uint32_t u32Value;
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint32_t));
    u32Value = *(uint32_t*)&pPciDev->abConfig[offReg];
    return RT_H2LE_U32(u32Value);
}

DECLINLINE(void)     PDMPciDevSetQWord(PPDMPCIDEV pPciDev, uint32_t offReg, uint64_t u64Value)
{
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint64_t));
    *(uint64_t*)&pPciDev->abConfig[offReg] = RT_H2LE_U64(u64Value);
}

DECLINLINE(uint64_t) PDMPciDevGetQWord(PPDMPCIDEV pPciDev, uint32_t offReg)
{
    uint64_t u64Value;
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint64_t));
    u64Value = *(uint64_t*)&pPciDev->abConfig[offReg];
    return RT_H2LE_U64(u64Value);
}

/**
 * Sets the vendor id config register.
 * @param   pPciDev         The PCI device.
 * @param   u16VendorId     The vendor id.
 */
DECLINLINE(void) PDMPciDevSetVendorId(PPDMPCIDEV pPciDev, uint16_t u16VendorId)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_VENDOR_ID, u16VendorId);
}

/**
 * Gets the vendor id config register.
 * @returns the vendor id.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetVendorId(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_VENDOR_ID);
}


/**
 * Sets the device id config register.
 * @param   pPciDev         The PCI device.
 * @param   u16DeviceId     The device id.
 */
DECLINLINE(void) PDMPciDevSetDeviceId(PPDMPCIDEV pPciDev, uint16_t u16DeviceId)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_DEVICE_ID, u16DeviceId);
}

/**
 * Gets the device id config register.
 * @returns the device id.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetDeviceId(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_DEVICE_ID);
}

/**
 * Sets the command config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u16Command      The command register value.
 */
DECLINLINE(void) PDMPciDevSetCommand(PPDMPCIDEV pPciDev, uint16_t u16Command)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_COMMAND, u16Command);
}


/**
 * Gets the command config register.
 * @returns The command register value.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetCommand(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_COMMAND);
}

/**
 * Checks if the given PCI device is a bus master.
 * @returns true if the device is a bus master, false if not.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(bool) PDMPciDevIsBusmaster(PPDMPCIDEV pPciDev)
{
    return (PDMPciDevGetCommand(pPciDev) & VBOX_PCI_COMMAND_MASTER) != 0;
}

/**
 * Checks if INTx interrupts disabled in the command config register.
 * @returns true if disabled.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(bool) PDMPciDevIsIntxDisabled(PPDMPCIDEV pPciDev)
{
    return (PDMPciDevGetCommand(pPciDev) & VBOX_PCI_COMMAND_INTX_DISABLE) != 0;
}

/**
 * Gets the status config register.
 *
 * @returns status config register.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetStatus(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_STATUS);
}

/**
 * Sets the status config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u16Status       The status register value.
 */
DECLINLINE(void) PDMPciDevSetStatus(PPDMPCIDEV pPciDev, uint16_t u16Status)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_STATUS, u16Status);
}


/**
 * Sets the revision id config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8RevisionId    The revision id.
 */
DECLINLINE(void) PDMPciDevSetRevisionId(PPDMPCIDEV pPciDev, uint8_t u8RevisionId)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_REVISION_ID, u8RevisionId);
}


/**
 * Sets the register level programming class config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8ClassProg     The new value.
 */
DECLINLINE(void) PDMPciDevSetClassProg(PPDMPCIDEV pPciDev, uint8_t u8ClassProg)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_CLASS_PROG, u8ClassProg);
}


/**
 * Sets the sub-class (aka device class) config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8SubClass      The sub-class.
 */
DECLINLINE(void) PDMPciDevSetClassSub(PPDMPCIDEV pPciDev, uint8_t u8SubClass)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_CLASS_SUB, u8SubClass);
}


/**
 * Sets the base class config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8BaseClass     The base class.
 */
DECLINLINE(void) PDMPciDevSetClassBase(PPDMPCIDEV pPciDev, uint8_t u8BaseClass)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_CLASS_BASE, u8BaseClass);
}

/**
 * Sets the header type config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8HdrType       The header type.
 */
DECLINLINE(void) PDMPciDevSetHeaderType(PPDMPCIDEV pPciDev, uint8_t u8HdrType)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_HEADER_TYPE, u8HdrType);
}

/**
 * Gets the header type config register.
 *
 * @param   pPciDev         The PCI device.
 * @returns u8HdrType       The header type.
 */
DECLINLINE(uint8_t) PDMPciDevGetHeaderType(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_HEADER_TYPE);
}

/**
 * Sets the BIST (built-in self-test) config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8Bist          The BIST value.
 */
DECLINLINE(void) PDMPciDevSetBIST(PPDMPCIDEV pPciDev, uint8_t u8Bist)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_BIST, u8Bist);
}

/**
 * Gets the BIST (built-in self-test) config register.
 *
 * @param   pPciDev         The PCI device.
 * @returns u8Bist          The BIST.
 */
DECLINLINE(uint8_t) PDMPciDevGetBIST(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_BIST);
}


/**
 * Sets a base address config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   iReg            Base address register number (0..5).
 * @param   fIOSpace        Whether it's I/O (true) or memory (false) space.
 * @param   fPrefetchable   Whether the memory is prefetachable. Must be false if fIOSpace == true.
 * @param   f64Bit          Whether the memory can be mapped anywhere in the 64-bit address space. Otherwise restrict to 32-bit.
 * @param   u32Addr         The address value.
 */
DECLINLINE(void) PDMPciDevSetBaseAddress(PPDMPCIDEV pPciDev, uint8_t iReg, bool fIOSpace, bool fPrefetchable, bool f64Bit,
                                         uint32_t u32Addr)
{
    if (fIOSpace)
    {
        Assert(!(u32Addr & 0x3)); Assert(!fPrefetchable); Assert(!f64Bit);
        u32Addr |= RT_BIT_32(0);
    }
    else
    {
        Assert(!(u32Addr & 0xf));
        if (fPrefetchable)
            u32Addr |= RT_BIT_32(3);
        if (f64Bit)
            u32Addr |= 0x2 << 1;
    }
    switch (iReg)
    {
        case 0: iReg = VBOX_PCI_BASE_ADDRESS_0; break;
        case 1: iReg = VBOX_PCI_BASE_ADDRESS_1; break;
        case 2: iReg = VBOX_PCI_BASE_ADDRESS_2; break;
        case 3: iReg = VBOX_PCI_BASE_ADDRESS_3; break;
        case 4: iReg = VBOX_PCI_BASE_ADDRESS_4; break;
        case 5: iReg = VBOX_PCI_BASE_ADDRESS_5; break;
        default: AssertFailedReturnVoid();
    }

    PDMPciDevSetDWord(pPciDev, iReg, u32Addr);
}

/**
 * Please document me. I don't seem to be getting as much as calculating
 * the address of some PCI region.
 */
DECLINLINE(uint32_t) PDMPciDevGetRegionReg(uint32_t iRegion)
{
    return iRegion == VBOX_PCI_ROM_SLOT
         ? VBOX_PCI_ROM_ADDRESS : (VBOX_PCI_BASE_ADDRESS_0 + iRegion * 4);
}

/**
 * Sets the sub-system vendor id config register.
 *
 * @param   pPciDev             The PCI device.
 * @param   u16SubSysVendorId   The sub-system vendor id.
 */
DECLINLINE(void) PDMPciDevSetSubSystemVendorId(PPDMPCIDEV pPciDev, uint16_t u16SubSysVendorId)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_SUBSYSTEM_VENDOR_ID, u16SubSysVendorId);
}

/**
 * Gets the sub-system vendor id config register.
 * @returns the sub-system vendor id.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetSubSystemVendorId(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_SUBSYSTEM_VENDOR_ID);
}


/**
 * Sets the sub-system id config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u16SubSystemId  The sub-system id.
 */
DECLINLINE(void) PDMPciDevSetSubSystemId(PPDMPCIDEV pPciDev, uint16_t u16SubSystemId)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_SUBSYSTEM_ID, u16SubSystemId);
}

/**
 * Gets the sub-system id config register.
 * @returns the sub-system id.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetSubSystemId(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_SUBSYSTEM_ID);
}

/**
 * Sets offset to capability list.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8Offset        The offset to capability list.
 */
DECLINLINE(void) PDMPciDevSetCapabilityList(PPDMPCIDEV pPciDev, uint8_t u8Offset)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_CAPABILITY_LIST, u8Offset);
}

/**
 * Returns offset to capability list.
 *
 * @returns offset to capability list.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint8_t) PDMPciDevGetCapabilityList(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_CAPABILITY_LIST);
}

/**
 * Sets the interrupt line config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8Line          The interrupt line.
 */
DECLINLINE(void) PDMPciDevSetInterruptLine(PPDMPCIDEV pPciDev, uint8_t u8Line)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_INTERRUPT_LINE, u8Line);
}

/**
 * Gets the interrupt line config register.
 *
 * @returns The interrupt line.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint8_t) PDMPciDevGetInterruptLine(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_INTERRUPT_LINE);
}

/**
 * Sets the interrupt pin config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8Pin           The interrupt pin.
 */
DECLINLINE(void) PDMPciDevSetInterruptPin(PPDMPCIDEV pPciDev, uint8_t u8Pin)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_INTERRUPT_PIN, u8Pin);
}

/**
 * Gets the interrupt pin config register.
 *
 * @returns The interrupt pin.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint8_t) PDMPciDevGetInterruptPin(PPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_INTERRUPT_PIN);
}

/** @} */

/** @name Aliases for old function names.
 * @{
 */
#if !defined(PDMPCIDEVICE_NO_DEPRECATED) || defined(DOXYGEN_RUNNING)
# define PCIDevSetByte               PDMPciDevSetByte
# define PCIDevGetByte               PDMPciDevGetByte
# define PCIDevSetWord               PDMPciDevSetWord
# define PCIDevGetWord               PDMPciDevGetWord
# define PCIDevSetDWord              PDMPciDevSetDWord
# define PCIDevGetDWord              PDMPciDevGetDWord
# define PCIDevSetQWord              PDMPciDevSetQWord
# define PCIDevGetQWord              PDMPciDevGetQWord
# define PCIDevSetVendorId           PDMPciDevSetVendorId
# define PCIDevGetVendorId           PDMPciDevGetVendorId
# define PCIDevSetDeviceId           PDMPciDevSetDeviceId
# define PCIDevGetDeviceId           PDMPciDevGetDeviceId
# define PCIDevSetCommand            PDMPciDevSetCommand
# define PCIDevGetCommand            PDMPciDevGetCommand
# define PCIDevIsBusmaster           PDMPciDevIsBusmaster
# define PCIDevIsIntxDisabled        PDMPciDevIsIntxDisabled
# define PCIDevGetStatus             PDMPciDevGetStatus
# define PCIDevSetStatus             PDMPciDevSetStatus
# define PCIDevSetRevisionId         PDMPciDevSetRevisionId
# define PCIDevSetClassProg          PDMPciDevSetClassProg
# define PCIDevSetClassSub           PDMPciDevSetClassSub
# define PCIDevSetClassBase          PDMPciDevSetClassBase
# define PCIDevSetHeaderType         PDMPciDevSetHeaderType
# define PCIDevGetHeaderType         PDMPciDevGetHeaderType
# define PCIDevSetBIST               PDMPciDevSetBIST
# define PCIDevGetBIST               PDMPciDevGetBIST
# define PCIDevSetBaseAddress        PDMPciDevSetBaseAddress
# define PCIDevGetRegionReg          PDMPciDevGetRegionReg
# define PCIDevSetSubSystemVendorId  PDMPciDevSetSubSystemVendorId
# define PCIDevGetSubSystemVendorId  PDMPciDevGetSubSystemVendorId
# define PCIDevSetSubSystemId        PDMPciDevSetSubSystemId
# define PCIDevGetSubSystemId        PDMPciDevGetSubSystemId
# define PCIDevSetCapabilityList     PDMPciDevSetCapabilityList
# define PCIDevGetCapabilityList     PDMPciDevGetCapabilityList
# define PCIDevSetInterruptLine      PDMPciDevSetInterruptLine
# define PCIDevGetInterruptLine      PDMPciDevGetInterruptLine
# define PCIDevSetInterruptPin       PDMPciDevSetInterruptPin
# define PCIDevGetInterruptPin       PDMPciDevGetInterruptPin
#endif
/** @} */


/* Special purpose "interface" for getting access to the PDMPCIDEV structure
 * of a ich9pcibridge instance. This is useful for unusual raw or pass-through
 * implementation which need to provide different PCI configuration space
 * content for bridges (as long as we don't allow pass-through of bridges or
 * custom bridge device implementations). */
typedef PPDMPCIDEV PPDMIICH9BRIDGEPDMPCIDEV;
typedef PDMPCIDEV PDMIICH9BRIDGEPDMPCIDEV;

#define PDMIICH9BRIDGEPDMPCIDEV_IID "785c74b1-8510-4458-9422-56750bf221db"


/** @} */

#endif
