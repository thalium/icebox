/** @file
 *
 */

/*
 * Copyright (C) 2006-2011 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __VRDPUSB__H
#define __VRDPUSB__H

#define VRDP_USB_STATUS_SUCCESS         0
#define VRDP_USB_STATUS_ACCESS_DENIED   1
#define VRDP_USB_STATUS_DEVICE_REMOVED  2

#define VRDP_USB_REAP_FLAG_CONTINUED (0)
#define VRDP_USB_REAP_FLAG_LAST      (1)

#define VRDP_USB_CAPS_FLAG_ASYNC    (0)
#define VRDP_USB_CAPS_FLAG_POLL     (1)

#endif /* __VRDPUSB__H  */
