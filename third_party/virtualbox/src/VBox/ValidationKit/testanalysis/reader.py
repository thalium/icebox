# -*- coding: utf-8 -*-
# $Id: reader.py $

"""
XML reader module.

This produces a test result tree that can be processed and passed to
reporting.
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
__all__     = ['ParseTestResult', ]

# Standard python imports.
import os
import traceback

# pylint: disable=C0111

class Value(object):
    """
    Represents a value.  Usually this is benchmark result or parameter.
    """
    def __init__(self, oTest, hsAttrs):
        self.oTest      = oTest;
        self.sName      = hsAttrs['name'];
        self.sUnit      = hsAttrs['unit'];
        self.sTimestamp = hsAttrs['timestamp'];
        self.sValue     = '';
        self.sDiff      = None;

    # parsing

    def addData(self, sData):
        self.sValue    += sData;

    # debug

    def printValue(self, cIndent):
        print '%sValue: name=%s timestamp=%s unit=%s value="%s"' \
                % (''.ljust(cIndent*2), self.sName, self.sTimestamp, self.sUnit, self.sValue);


class Test(object):
    """
    Nested test result.
    """
    def __init__(self, oParent, hsAttrs):
        self.aoChildren     = [];
        self.aoValues       = [];
        self.oParent        = oParent;
        self.sName          = hsAttrs['name'];
        self.sStartTS       = hsAttrs['timestamp'];
        self.sEndTS         = None;
        self.sStatus        = None;
        self.cErrors        = -1;
        self.sStatusDiff    = None;
        self.cErrorsDiff    = None;

    # parsing

    def addChild(self, oChild):
        self.aoChildren.append(oChild);
        return oChild;

    def addValue(self, hsAttrs):
        oValue = hsAttrs['value'];
        self.aoValues.append(oValue);
        return oValue;

    def markPassed(self, hsAttrs):
        try:    self.sEndTS = hsAttrs['timestamp'];
        except: pass;
        self.sStatus = 'passed';
        self.cErrors = 0;

    def markSkipped(self, hsAttrs):
        try:    self.sEndTS = hsAttrs['timestamp'];
        except: pass;
        self.sStatus = 'skipped';
        self.cErrors = 0;

    def markFailed(self, hsAttrs):
        try:    self.sEndTS = hsAttrs['timestamp'];
        except: pass;
        self.sStatus = 'failed';
        self.cErrors = int(hsAttrs['errors']);

    def markEnd(self, hsAttrs):
        try:    self.sEndTS = hsAttrs['timestamp'];
        except: pass;
        if self.sStatus is None:
            self.sStatus = 'end';
            self.cErrors = 0;

    def mergeInIncludedTest(self, oTest):
        """ oTest will be robbed. """
        if oTest is not None:
            for oChild in oTest.aoChildren:
                oChild.oParent = self;
                self.aoChildren.append(oChild);
            for oValue in oTest.aoValues:
                oValue.oTest = self;
                self.aoValues.append(oValue);
            oTest.aoChildren = [];
            oTest.aoValues   = [];

    # debug

    def printTree(self, iLevel = 0):
        print '%sTest: name=%s start=%s end=%s' % (''.ljust(iLevel*2), self.sName, self.sStartTS, self.sEndTS);
        for oChild in self.aoChildren:
            oChild.printTree(iLevel + 1);
        for oValue in self.aoValues:
            oValue.printValue(iLevel + 1);

    # getters / queries

    def getFullNameWorker(self, cSkipUpper):
        if self.oParent is None:
            return (self.sName, 0);
        sName, iLevel = self.oParent.getFullNameWorker(cSkipUpper);
        if iLevel < cSkipUpper:
            sName = self.sName;
        else:
            sName += ', ' + self.sName;
        return (sName, iLevel + 1);

    def getFullName(self, cSkipUpper = 2):
        return self.getFullNameWorker(cSkipUpper)[0];

    def matchFilters(self, asFilters):
        """
        Checks if the all of the specified filter strings are substrings
        of the full test name.  Returns True / False.
        """
        sName = self.getFullName();
        for sFilter in asFilters:
            if sName.find(sFilter) < 0:
                return False;
        return True;

    # manipulation

    def filterTestsWorker(self, asFilters):
        # depth first
        i = 0;
        while i < len(self.aoChildren):
            if self.aoChildren[i].filterTestsWorker(asFilters):
                i += 1;
            else:
                self.aoChildren[i].oParent = None;
                del self.aoChildren[i];

        # If we have children, they must've matched up.
        if len(self.aoChildren) != 0:
            return True;
        return self.matchFilters(asFilters);

    def filterTests(self, asFilters):
        if len(asFilters) > 0:
            self.filterTestsWorker(asFilters)
        return self;


class XmlLogReader(object):
    """
    XML log reader class.
    """

    def __init__(self, sXmlFile):
        self.sXmlFile = sXmlFile;
        self.oRoot    = Test(None, {'name': 'root', 'timestamp': ''});
        self.oTest    = self.oRoot;
        self.iLevel   = 0;
        self.oValue   = None;

    def parse(self):
        try:
            oFile = open(self.sXmlFile, 'r');
        except:
            traceback.print_exc();
            return False;

        from xml.parsers.expat import ParserCreate
        oParser = ParserCreate();
        oParser.StartElementHandler  = self.handleElementStart;
        oParser.CharacterDataHandler = self.handleElementData;
        oParser.EndElementHandler    = self.handleElementEnd;
        try:
            oParser.ParseFile(oFile);
        except:
            traceback.print_exc();
            oFile.close();
            return False;
        oFile.close();
        return True;

    def handleElementStart(self, sName, hsAttrs):
        #print '%s%s: %s' % (''.ljust(self.iLevel * 2), sName, str(hsAttrs));
        if sName == 'Test' or sName == 'SubTest':
            self.iLevel += 1;
            self.oTest = self.oTest.addChild(Test(self.oTest, hsAttrs));
        elif sName == 'Value':
            self.oValue = self.oTest.addValue(hsAttrs);
        elif sName == 'End':
            self.oTest.markEnd(hsAttrs);
        elif sName == 'Passed':
            self.oTest.markPassed(hsAttrs);
        elif sName == 'Skipped':
            self.oTest.markSkipped(hsAttrs);
        elif sName == 'Failed':
            self.oTest.markFailed(hsAttrs);
        elif sName == 'Include':
            self.handleInclude(hsAttrs);
        else:
            print 'Unknonwn element "%s"' % (sName);

    def handleElementData(self, sData):
        if self.oValue is not None:
            self.oValue.addData(sData);
        elif sData.strip() != '':
            print 'Unexpected data "%s"' % (sData);
        return True;

    def handleElementEnd(self, sName):
        if sName == 'Test' or sName == 'Subtest':
            self.iLevel -= 1;
            self.oTest = self.oTest.oParent;
        elif sName == 'Value':
            self.oValue = None;
        return True;

    def handleInclude(self, hsAttrs):
        # relative or absolute path.
        sXmlFile = hsAttrs['filename'];
        if not os.path.isabs(sXmlFile):
            sXmlFile = os.path.join(os.path.dirname(self.sXmlFile), sXmlFile);

        # Try parse it.
        oSub = parseTestResult(sXmlFile);
        if oSub is None:
            print 'error: failed to parse include "%s"' % (sXmlFile);
        else:
            # Skip the root and the next level before merging it the subtest and
            # values in to the current test.  The reason for this is that the
            # include is the output of some sub-program we've run and we don't need
            # the extra test level it automatically adds.
            #
            # More benchmark heuristics: Walk down until we find more than one
            # test or values.
            oSub2 = oSub;
            while len(oSub2.aoChildren) == 1 and len(oSub2.aoValues) == 0:
                oSub2 = oSub2.aoChildren[0];
            if len(oSub2.aoValues) == 0:
                oSub2 = oSub;
            self.oTest.mergeInIncludedTest(oSub2);
        return True;

def parseTestResult(sXmlFile):
    """
    Parses the test results in the XML.
    Returns result tree.
    Returns None on failure.
    """
    oXlr = XmlLogReader(sXmlFile);
    if oXlr.parse():
        return oXlr.oRoot;
    return None;

