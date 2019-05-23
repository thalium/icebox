/* $Id: NetworkServiceRunner.h $ */
/** @file
 * VirtualBox Main - interface for VBox DHCP server.
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
 */

#include <iprt/err.h>
#include <iprt/types.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <VBox/com/string.h>

#include <string>

#define TRUNKTYPE_WHATEVER "whatever"
#define TRUNKTYPE_NETFLT   "netflt"
#define TRUNKTYPE_NETADP   "netadp"
#define TRUNKTYPE_SRVNAT   "srvnat"

class NetworkServiceRunner
{
public:
    NetworkServiceRunner(const char *aProcName);
    virtual ~NetworkServiceRunner();

    int setOption(const std::string& key, const std::string& val);

    int  start(bool aKillProcOnStop);
    int  stop();
    bool isRunning();
    void detachFromServer();

    static const std::string kNsrKeyName;
    static const std::string kNsrKeyNetwork;
    static const std::string kNsrKeyTrunkType;
    static const std::string kNsrTrunkName;
    static const std::string kNsrMacAddress;
    static const std::string kNsrIpAddress;
    static const std::string kNsrIpNetmask;
    static const std::string kNsrKeyNeedMain;

private:
    struct Data;
    Data *m;
};
