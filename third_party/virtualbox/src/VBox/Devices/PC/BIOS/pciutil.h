/* $Id: pciutil.h $ */
/** @file
 * Utility routines for calling the PCI BIOS.
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

extern  uint16_t    pci_find_device(uint16_t v_id, uint16_t d_id);
/* Warning: pci_find_classcode destroys the high bits of ECX. */
extern  uint16_t    pci_find_classcode(uint32_t dev_class);
extern  uint32_t    pci_read_config_byte(uint8_t bus, uint8_t dev_fn, uint8_t reg);
extern  uint32_t    pci_read_config_word(uint8_t bus, uint8_t dev_fn, uint8_t reg);
/* Warning: pci_read_config_dword destroys the high bits of ECX. */
extern  uint32_t    pci_read_config_dword(uint8_t bus, uint8_t dev_fn, uint8_t reg);
extern  void        pci_write_config_byte(uint8_t bus, uint8_t dev_fn, uint8_t reg, uint8_t val);
extern  void        pci_write_config_word(uint8_t bus, uint8_t dev_fn, uint8_t reg, uint16_t val);
/* Warning: pci_write_config_dword destroys the high bits of ECX. */
extern  void        pci_write_config_dword(uint8_t bus, uint8_t dev_fn, uint8_t reg, uint32_t val);

