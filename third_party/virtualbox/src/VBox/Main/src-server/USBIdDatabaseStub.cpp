/* $Id: USBIdDatabaseStub.cpp $ */
/** @file
 * USB device vendor and product ID database - stub.
 */

/*
 * Copyright (C) 2015-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "USBIdDatabase.h"

const RTBLDPROGSTRTAB   USBIdDatabase::s_StrTab          =  { "", 0, 0 NULL };

const size_t            USBIdDatabase::s_cVendors        = 0;
const USBIDDBVENDOR     USBIdDatabase::s_aVendors[]      = { 0 };
const RTBLDPROGSTRREF   USBIdDatabase::s_aVendorNames[]  = { {0,0} };

const size_t            USBIdDatabase::s_cProducts       = 0;
const USBIDDBPROD       USBIdDatabase::s_aProducts[]     = { 0 };
const RTBLDPROGSTRREF   USBIdDatabase::s_aProductNames[] = { {0,0} };

