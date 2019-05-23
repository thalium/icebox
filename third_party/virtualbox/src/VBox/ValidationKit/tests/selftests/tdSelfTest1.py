#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: tdSelfTest1.py $

"""
Test Manager Self Test - Dummy Test Driver.
"""

__copyright__ = \
"""
Copyright (C) 2012-2017 Oracle Corporation

This file is part of VirtualBox Open Source Edition (OSE), as
available from http://www.virtualbox.org. This file is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software
Foundation, in version 2 as it comes in the "COPYING" file of the
VirtualBox OSE distribution. VirtualBox OSE is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

The contents of this file may alternatively be used under the terms
of the Common Development and Distribution License Version 1.0
(CDDL) only, as it comes in the "COPYING.CDDL" file of the
VirtualBox OSE distribution, in which case the provisions of the
CDDL are applicable instead of those of the GPL.

You may elect to license modified versions of this file under the
terms and conditions of either the GPL or the CDDL or both.
"""
__version__ = "$Revision: 118412 $"


import sys;

print('dummydriver.py: hello world!');
print('dummydriver.py: args: %s' % (sys.argv,));
if sys.argv[-1] in [ 'all', 'execute' ]:

    import time;
    for i in range(10, 1, -1):
        print('dummydriver.py: %u...', i);
        sys.stdout.flush();
        time.sleep(1);
    print('dummydriver.py: ...0! done');

sys.exit(0);

