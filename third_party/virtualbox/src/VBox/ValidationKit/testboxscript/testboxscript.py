#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: testboxscript.py $

"""
TestBox Script Wrapper.

This script aimes at respawning the Test Box Script when it terminates
abnormally or due to an UPGRADE request.
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

import platform;
import subprocess;
import sys;
import os;
import time;


## @name Test Box script exit statuses (see also RTEXITCODE)
# @remarks These will _never_ change
# @{
TBS_EXITCODE_FAILURE        = 1;        # RTEXITCODE_FAILURE
TBS_EXITCODE_SYNTAX         = 2;        # RTEXITCODE_SYNTAX
TBS_EXITCODE_NEED_UPGRADE   = 9;
## @}


class TestBoxScriptWrapper(object): # pylint: disable=R0903
    """
    Wrapper class
    """

    TESTBOX_SCRIPT_FILENAME = 'testboxscript_real.py'

    def __init__(self):
        """
        Init
        """
        self.oTask = None

    def __del__(self):
        """
        Cleanup
        """
        if self.oTask is not None:
            print 'Wait for child task...'
            self.oTask.terminate()
            self.oTask.wait()
            print 'done. Exiting'
            self.oTask = None;

    def run(self):
        """
        Start spawning the real TestBox script.
        """

        # Figure out where we live first.
        try:
            __file__
        except:
            __file__ = sys.argv[0];
        sTestBoxScriptDir = os.path.dirname(os.path.abspath(__file__));

        # Construct the argument list for the real script (same dir).
        sRealScript = os.path.join(sTestBoxScriptDir, TestBoxScriptWrapper.TESTBOX_SCRIPT_FILENAME);
        asArgs = sys.argv[1:];
        asArgs.insert(0, sRealScript);
        if sys.executable:
            asArgs.insert(0, sys.executable);

        # Look for --pidfile <name> and write a pid file.
        sPidFile = None;
        for i, _ in enumerate(asArgs):
            if asArgs[i] == '--pidfile' and i + 1 < len(asArgs):
                sPidFile = asArgs[i + 1];
                break;
            if asArgs[i] == '--':
                break;
        if sPidFile:
            oPidFile = open(sPidFile, 'w');
            oPidFile.write(str(os.getpid()));
            oPidFile.close();

        # Execute the testbox script almost forever in a relaxed loop.
        rcExit = TBS_EXITCODE_FAILURE;
        while True:
            self.oTask = subprocess.Popen(asArgs,
                                          shell = False,
                                          creationflags = (0 if platform.system() != 'Windows'
                                                           else subprocess.CREATE_NEW_PROCESS_GROUP)); # for Ctrl-C isolation
            rcExit = self.oTask.wait();
            self.oTask = None;
            if rcExit == TBS_EXITCODE_SYNTAX:
                break;

            # Relax.
            time.sleep(1);
        return rcExit;

if __name__ == '__main__':
    sys.exit(TestBoxScriptWrapper().run());

