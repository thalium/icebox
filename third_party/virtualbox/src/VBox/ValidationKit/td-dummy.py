#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Copyright (C) 2010-2017 Oracle Corporation

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

__version__ = "$Id: td-dummy.py $"

import sys
sys.path.insert(0, "../");
import testdriver.reporter as reporter
import testdriver.vbox


class DummyTestDriver(testdriver.vbox.TestDriver):
    """
    Dummy Test driver for testing the package layout.
    """

    def __init__(self):
        testdriver.vbox.TestDriver.__init__(self);
        self.dummy = 1;

    def main(self):
        print "works";
        reporter.log("the reporter works")
        self.dump();
        return 0;


td = DummyTestDriver();
sys.exit(td.main());

