# -*- coding: utf-8 -*-
# $Id: reporting.py $

"""
Test Result Report Writer.

This takes a processed test result tree and creates a HTML, re-structured text,
or normal text report from it.
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
__all__     = ['HtmlReport', 'RstReport', 'TextReport'];


def tryAddThousandSeparators(sPotentialInterger):
    """ Apparently, python 3.0(/3.1) has(/will have) support for this..."""
    # Try convert the string/value to a long.
    try:
        lVal = long(sPotentialInterger);
        lVal = long(sPotentialInterger);
    except:
        return sPotentialInterger;

    # Convert it back to a string (paranoia) and build up the new string.
    sOld      = str(lVal);
    chSign    = '';
    if sOld[0] == '-':
        chSign = '-';
        sOld = sOld[1:];
    elif sPotentialInterger[0] == '+':
        chSign = '+';
    cchDigits = len(sOld);
    iDigit    = 0
    sNewVal   = '';
    while iDigit < cchDigits:
        if (iDigit % 3) == 0 and iDigit > 0:
            sNewVal = ' ' + sNewVal;
        sNewVal = sOld[cchDigits - iDigit - 1] + sNewVal;
        iDigit += 1;
    return chSign + sNewVal;


class Table(object):
    """
    A table as a header as well as data rows, thus this class.
    """
    def __init__(self, oTest, fSplitDiff):
        self.aasRows  = [];
        self.asHeader = ['Test',];
        self.asUnits  = ['',];
        for oValue in oTest.aoValues:
            self.asHeader.append(oValue.sName);
            self.asUnits.append(oValue.sUnit);
        self.addRow(oTest, fSplitDiff);

    def addRow(self, oTest, fSplitDiff):
        """Adds a row."""
        asRow = [oTest.getFullName(),];
        for oValue in oTest.aoValues:
            asRow.append(oValue.sValue);
        if not fSplitDiff:
            self.aasRows.append(asRow);
        else:
            # Split cells into multiple rows on '|'. Omit the first column.
            iRow = 0;
            asThisRow = [asRow[0], ];
            fMoreTodo = True;
            while fMoreTodo:
                for i in range(1, len(asRow)):
                    asSplit = asRow[i].split('|');
                    asThisRow.append(asSplit[0]);
                    asRow[i] = '|'.join(asSplit[1:])
                self.aasRows.append(asThisRow);

                # Done?
                fMoreTodo = False;
                for i in range(1, len(asRow)):
                    if len(asRow[i]):
                        fMoreTodo = True;
                asThisRow = ['', ];
                iRow += 1;

            # Readability hack: Add an extra row if there are diffs.
            if iRow > 1:
                asRow[0] = '';
                self.aasRows.append(asRow);

        return True;

    def hasTheSameHeadingAsTest(self, oTest):
        """ Checks if the test values has the same heading."""
        i = 1;
        for oValue in oTest.aoValues:
            if self.asHeader[i] != oValue.sName:
                return False;
            if self.asUnits[i]  != oValue.sUnit:
                return False;
            i += 1;
        return True;

    def hasTheSameHeadingAsTable(self, oTable):
        """ Checks if the other table has the same heading."""
        if len(oTable.asHeader) != len(self.asHeader):
            return False;
        for i in range(len(self.asHeader)):
            if self.asHeader[i] != oTable.asHeader[i]:
                return False;
            if self.asUnits[i]  != oTable.asUnits[i]:
                return False;
        return True;

    def appendTable(self, oTable):
        """ Append the rows in oTable.  oTable has the same heading as us. """
        self.aasRows.extend(oTable.aasRows);
        return True;

    # manipulation and stuff

    def optimizeUnit(self):
        """ Turns bytes into KB, MB or GB. """
        ## @todo
        pass;

    def addThousandSeparators(self):
        """ Adds thousand separators to make numbers more readable. """
        for iRow in range(len(self.aasRows)):
            for iColumn in range(1, len(self.aasRows[iRow])):
                asValues = self.aasRows[iRow][iColumn].split('|');
                for i in range(len(asValues)):
                    asValues[i] = tryAddThousandSeparators(asValues[i]);
                self.aasRows[iRow][iColumn] = '|'.join(asValues);
        return True;

    def getRowWidths(self):
        """Figure out the column withs."""
        # Header is first.
        acchColumns = [];
        for i in range(len(self.asHeader)):
            cch = 1;
            asWords = self.asHeader[i].split();
            for s in asWords:
                if len(s) > cch:
                    cch = len(s);
            if i > 0 and len(self.asUnits[i]) > cch:
                cch = len(self.asUnits[i]);
            acchColumns.append(cch);

        # Check out all cells.
        for asColumns in self.aasRows:
            for i in range(len(asColumns)):
                if len(asColumns[i]) > acchColumns[i]:
                    acchColumns[i] = len(asColumns[i]);
        return acchColumns;


def tabelizeTestResults(oTest, fSplitDiff):
    """
    Break the test results down into a list of tables containing the values.

    TODO: Handle passed / failed stuff too. Not important for benchmarks.
    """
    # Pass 1
    aoTables = [];
    aoStack  = [];
    aoStack.append((oTest, 0));
    while len(aoStack) > 0:
        oCurTest, iChild =  aoStack.pop();

        # depth first
        if iChild < len(oCurTest.aoChildren):
            aoStack.append((oCurTest, iChild + 1));
            aoStack.append((oCurTest.aoChildren[iChild], 0));
            continue;

        # values -> row
        if len(oCurTest.aoValues) > 0:
            if len(aoTables) > 0 and aoTables[len(aoTables) - 1].hasTheSameHeadingAsTest(oCurTest):
                aoTables[len(aoTables) - 1].addRow(oCurTest, fSplitDiff);
            else:
                aoTables.append(Table(oCurTest, fSplitDiff));

    # Pass 2 - Combine tables with the same heading.
    aoTables2 = [];
    for oTable in aoTables:
        for i in range(len(aoTables2)):
            if aoTables2[i].hasTheSameHeadingAsTable(oTable):
                aoTables2[i].appendTable(oTable);
                oTable = None;
                break;
        if oTable is not None:
            aoTables2.append(oTable);

    return aoTables2;

def produceHtmlReport(oTest):
    """
    Produce an HTML report on stdout (via print).
    """
    print 'not implemented: %s' % (oTest);
    return False;

def produceReStructuredTextReport(oTest):
    """
    Produce a ReStructured text report on stdout (via print).
    """
    print 'not implemented: %s' % (oTest);
    return False;

def produceTextReport(oTest):
    """
    Produce a text report on stdout (via print).
    """

    #
    # Report header.
    #
    ## @todo later

    #
    # Tabelize the results and display the tables.
    #
    aoTables = tabelizeTestResults(oTest, True)
    for oTable in aoTables:
        ## @todo do max/min on the columns where we can do [GMK]B(/s).
        oTable.addThousandSeparators();
        acchColumns = oTable.getRowWidths();

        # The header.
        # This is a bit tedious and the solution isn't entirely elegant due
        # to the pick-it-up-as-you-go-along python skills.
        aasHeader = [];
        aasHeader.append([]);
        for i in range(len(oTable.asHeader)):
            aasHeader[0].append('');

        for iColumn in range(len(oTable.asHeader)):
            asWords = oTable.asHeader[iColumn].split();
            iLine   = 0;
            for s in asWords:
                if len(aasHeader[iLine][iColumn]) <= 0:
                    aasHeader[iLine][iColumn] = s;
                elif len(s) + 1 + len(aasHeader[iLine][iColumn]) <= acchColumns[iColumn]:
                    aasHeader[iLine][iColumn] += ' ' + s;
                else:
                    iLine += 1;
                    if iLine >= len(aasHeader):  # There must be a better way to do this...
                        aasHeader.append([]);
                        for i in range(len(oTable.asHeader)):
                            aasHeader[iLine].append('');
                    aasHeader[iLine][iColumn] = s;

        for asLine in aasHeader:
            sLine = '';
            for i in range(len(asLine)):
                if i > 0: sLine += '  ';
                sLine += asLine[i].center(acchColumns[i]);
            print sLine;

        # Units.
        sLine = '';
        for i in range(len(oTable.asUnits)):
            if i > 0: sLine += '  ';
            sLine += oTable.asUnits[i].center(acchColumns[i]);
        print sLine;

        # Separator line.
        sLine = '';
        for i in range(len(oTable.asHeader)):
            if i > 0: sLine += '  '
            sLine += '=' * acchColumns[i];
        print sLine;

        # The rows.
        for asColumns in oTable.aasRows:
            sText = asColumns[0].ljust(acchColumns[0]);
            for i in range(1, len(asColumns)):
                sText += '  ' + asColumns[i].rjust(acchColumns[i]);
            print sText;

    return None;

