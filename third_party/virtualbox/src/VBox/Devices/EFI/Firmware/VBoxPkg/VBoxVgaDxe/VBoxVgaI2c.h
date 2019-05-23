/* $Id: VBoxVgaI2c.h $ */
/** @file
 * VBoxVgaI2c.h
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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

/*
  This code is based on:

  I2c Bus byte read/write functions.

  Copyright (c) 2008 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef _CIRRUS_LOGIC_I2C_H_
#define _CIRRUS_LOGIC_I2C_H_

#include <Protocol/PciIo.h>

/**
  Read one byte data on I2C Bus.

  Read one byte data from the slave device connected to I2C Bus.
  If Data is NULL, then ASSERT().

  @param  PciIo              The pointer to PCI_IO_PROTOCOL.
  @param  DeviceAddress      Slave device's address.
  @param  RegisterAddress    The register address on slave device.
  @param  Data               The pointer to returned data if EFI_SUCCESS returned.

  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
I2cReadByte (
  EFI_PCI_IO_PROTOCOL    *PciIo,
  UINT8                  DeviceAddress,
  UINT8                  RegisterAddress,
  UINT8                  *Data
  );

/**
  Write one byte data onto I2C Bus.

  Write one byte data to the slave device connected to I2C Bus.
  If Data is NULL, then ASSERT().

  @param  PciIo              The pointer to PCI_IO_PROTOCOL.
  @param  DeviceAddress      Slave device's address.
  @param  RegisterAddress    The register address on slave device.
  @param  Data               The pointer to write data.

  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
I2cWriteByte (
  EFI_PCI_IO_PROTOCOL    *PciIo,
  UINT8                  DeviceAddress,
  UINT8                  RegisterAddress,
  UINT8                  *Data
  );

#endif
