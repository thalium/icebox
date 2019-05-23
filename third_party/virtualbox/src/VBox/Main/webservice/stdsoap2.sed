# $Id: stdsoap2.sed $
## @file
# WebService - SED script for inserting a iprt/win/windows.h include
#              before stdsoap2.h in soapStub.h.  This prevents hacking
#              client and server code to do the same when using -Wall.
#

#
# Copyright (C) 2016-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

s/\(#include "stdsoap2\.h"\)/#ifdef RT_OS_WINDOWS\n# include <iprt\/win\/windows.h>\n#endif\n\1/

