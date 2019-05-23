/* $Id: Pcap.h $ */
/** @file
 * Helpers for writing libpcap files.
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
 */

#ifndef ___VBox_Pcap_h
#define ___VBox_Pcap_h

#include <iprt/stream.h>
#include <VBox/types.h>

RT_C_DECLS_BEGIN

int PcapStreamHdr(PRTSTREAM pStream, uint64_t StartNanoTS);
int PcapStreamFrame(PRTSTREAM pStream, uint64_t StartNanoTS, const void *pvFrame, size_t cbFrame, size_t cbMax);
int PcapStreamGsoFrame(PRTSTREAM pStream, uint64_t StartNanoTS, PCPDMNETWORKGSO pGso,
                       const void *pvFrame, size_t cbFrame, size_t cbMax);

int PcapFileHdr(RTFILE File, uint64_t StartNanoTS);
int PcapFileFrame(RTFILE File, uint64_t StartNanoTS, const void *pvFrame, size_t cbFrame, size_t cbMax);
int PcapFileGsoFrame(RTFILE File, uint64_t StartNanoTS, PCPDMNETWORKGSO pGso,
                     const void *pvFrame, size_t cbFrame, size_t cbSegMax);

RT_C_DECLS_END

#endif

