#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: tdSelfTest2.py $

"""
Test Manager / Suite Self Test #2 - Everything should succeed.
"""

__copyright__ = \
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
__version__ = "$Revision: 118412 $"


# Standard Python imports.
import os;
import sys;

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0];
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksValidationKitDir);

# Validation Kit imports.
from common             import utils;
from testdriver         import reporter;
from testdriver.base    import TestDriverBase;


class tdSelfTest2(TestDriverBase):
    """
    Test Manager / Suite Self Test #2 - Everything should succeed.
    """

    def __init__(self):
        TestDriverBase.__init__(self);


    def actionExecute(self):
        reporter.testStart('reporter.testXXXX API');
        reporter.testValue('value-name1', 123456789, 'ms');

        reporter.testStart('subtest');
        reporter.testValue('value-name2',  11223344, 'times');
        reporter.testDone();

        reporter.testStart('subtest2');
        reporter.testValue('value-name3',  39, 'sec');
        reporter.testValue('value-name4',  42, 'ns');
        reporter.testDone();

        reporter.testStart('subtest3');
        reporter.testDone(fSkipped = True);

        # No spaces in XML.
        reporter.testStart('subtest4');
        oSubXmlFile = reporter.FileWrapperTestPipe();
        oSubXmlFile.write('<?xml version="1.0" encoding="UTF-8" ?>');
        oSubXmlFile.write('<Test timestamp="%s" name="foobar1">' % (utils.getIsoTimestamp(),));
        oSubXmlFile.write('<Test timestamp="%s" name="sub1">' % (utils.getIsoTimestamp(),));
        oSubXmlFile.write('<Passed timestamp="%s"/>' % (utils.getIsoTimestamp(),));
        oSubXmlFile.write('</Test>');
        oSubXmlFile.write('<End timestamp="%s" errors="0"/>' % (utils.getIsoTimestamp(),));
        oSubXmlFile.write('</Test>');
        oSubXmlFile.close();
        oSubXmlFile = None;
        reporter.testDone();

        # Spaces + funny line endings.
        reporter.testStart('subtest5');
        oSubXmlFile = reporter.FileWrapperTestPipe();
        oSubXmlFile.write('<?xml version="1.0" encoding="UTF-8" ?>\r\n');
        oSubXmlFile.write('<Test timestamp="%s" name="foobar2">\n\n\t\n\r\n' % (utils.getIsoTimestamp(),));
        oSubXmlFile.write('<Test timestamp="%s" name="sub2">' % (utils.getIsoTimestamp(),));
        oSubXmlFile.write('  <Passed timestamp="%s"/>\n' % (utils.getIsoTimestamp(),));
        oSubXmlFile.write('  </Test>\n');
        oSubXmlFile.write('  <End timestamp="%s" errors="0"/>\r' % (utils.getIsoTimestamp(),));
        oSubXmlFile.write('</Test>');
        oSubXmlFile.close();
        reporter.testDone();

        # A few long log times for WUI log testing.
        reporter.log('long line: asdfasdfljkasdlfkjasldkfjlaksdfjl asdlfkjasdlkfjalskdfjlaksdjfa falkjaldkjfalskdjflaksdjf ' \
                     'lajksdflkjasdlfkjalsdfj asldfkjlaskdjflaksdjflaksdjflkj asdlfkjalsdkfjalsdkjflaksdj fasdlfkj ' \
                     'asdlkfj  aljkasdflkj alkjdsf lakjsdf');
        reporter.log('long line: asdfasdfljkasdlfkjasldkfjlaksdfjl asdlfkjasdlkfjalskdfjlaksdjfa falkjaldkjfalskdjflaksdjf ' \
                     'lajksdflkjasdlfkjalsdfj asldfkjlaskdjflaksdjflaksdjflkj asdlfkjalsdkfjalsdkjflaksdj fasdlfkj ' \
                     'asdlkfj  aljkasdflkj alkjdsf lakjsdf');
        reporter.log('long line: asdfasdfljkasdlfkjasldkfjlaksdfjl asdlfkjasdlkfjalskdfjlaksdjfa falkjaldkjfalskdjflaksdjf ' \
                     'lajksdflkjasdlfkjalsdfj asldfkjlaskdjflaksdjflaksdjflkj asdlfkjalsdkfjalsdkjflaksdj fasdlfkj ' \
                     'asdlkfj  aljkasdflkj alkjdsf lakjsdf');

        # Upload a file.
        reporter.addLogFile(__file__, sKind = 'log/release/vm');

        reporter.testDone();
        return True;


if __name__ == '__main__':
    sys.exit(tdSelfTest2().main(sys.argv));

